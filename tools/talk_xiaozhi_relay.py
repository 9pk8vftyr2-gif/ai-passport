#!/usr/bin/env python3
"""Opt-in desktop XiaoZhi bridge. Credentials remain outside the served repo.
Streams button-triggered PCM to Opus/WSS; never saves speech or transcripts.
No tools, firmware updates, wake-word detection, or local conversation memory.
"""
import argparse
import asyncio
import json
import ssl
import time
from websockets.asyncio.client import connect
from websockets.asyncio.server import serve
from websockets.exceptions import ConnectionClosed
from talk_mock_server import HEADER, decode_audio
from talk_opus import Opus
from xiaozhi_setup import PRIVATE_DIR, load_identity, allowed_ws


def clipped(text, limit):
    if not isinstance(text, str):
        raise ValueError('Invalid transcript')
    return text.encode('utf-8')[:limit].decode('utf-8', errors='ignore')


async def open_cloud():
    identity = load_identity(PRIVATE_DIR / 'xiaozhi-desktop.json')
    settings = identity.get('websocket', {})
    if identity.get('binding_required', True) or not allowed_ws(settings.get('url', '')):
        raise ValueError('Complete official account binding first')
    ws = await connect(settings['url'], additional_headers={
        'Authorization': 'Bearer ' + settings['token'], 'Protocol-Version': '1',
        'Device-Id': identity['device_id'], 'Client-Id': identity['client_id']},
        ssl=ssl.create_default_context(), proxy=None, open_timeout=8, close_timeout=1,
        max_size=65536, max_queue=4, write_limit=4096, compression=None)
    try:
        await ws.send(json.dumps({'type': 'hello', 'version': 1, 'transport': 'websocket',
            'audio_params': {'format': 'opus', 'sample_rate': 16000, 'channels': 1, 'frame_duration': 60}}))
        data = json.loads(await asyncio.wait_for(ws.recv(), 8))
        audio = data.get('audio_params', {})
        if (data.get('type') != 'hello' or data.get('transport') != 'websocket'
                or audio.get('format') != 'opus' or audio.get('channels') != 1
                or audio.get('sample_rate') not in (16000, 24000, 48000)
                or not isinstance(data.get('session_id'), str)):
            raise ValueError('Unsupported official hello')
        return ws, data['session_id'], audio
    except BaseException:
        await ws.close()
        raise


class Session:
    def __init__(self, local, cloud_factory=open_cloud, codec_factory=Opus):
        self.local, self.cloud_factory, self.codec_factory = local, cloud_factory, codec_factory
        self.cloud = self.codec = self.task = None
        self.session = None
        self.hello = False
        self.turn = None
        self.seq = self.credits = 0
        self.committed = False
        self.pcm = bytearray()
        self.credit_event = asyncio.Event()
        self.started = 0
        self.cancelled = False

    async def send(self, kind, **values):
        await self.local.send(json.dumps(dict(v=1, type=kind, **values)))

    async def connect_cloud(self):
        if self.cloud is None:
            self.cloud, self.session, _ = await self.cloud_factory()
            self.codec = self.codec_factory()

    async def close_cloud(self):
        if self.cloud:
            await self.cloud.close(); self.cloud = None
        if self.codec:
            self.codec.close(); self.codec = None
        self.pcm.clear()

    async def cancel(self):
        self.cancelled = True
        self.credit_event.set()
        # Notify the provider before cancelling the receiver closes its socket.
        if self.cloud:
            try:
                await asyncio.wait_for(self.cloud.send(json.dumps({'type': 'abort', 'session_id': self.session})), .5)
            except (ConnectionClosed, asyncio.TimeoutError, OSError):
                pass
        if self.task:
            self.task.cancel()
            try:
                await self.task
            except (asyncio.CancelledError, ConnectionClosed):
                pass
            self.task = None
        await self.close_cloud()
        self.turn = None; self.credits = 0

    async def answer(self, turn):
        seq, answer = 0, ''
        completed = False
        deadline = time.monotonic() + 60
        try:
            while True:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise asyncio.TimeoutError()
                raw = await asyncio.wait_for(self.cloud.recv(), min(20 if not seq else 5, remaining))
                # Python 3.9 wait_for can race a completed recv with cancellation.
                if self.cancelled:
                    raise asyncio.CancelledError()
                if isinstance(raw, bytes):
                    pcm = self.codec.decode(raw)
                    for start in range(0, len(pcm), 640):
                        if self.cancelled:
                            raise asyncio.CancelledError()
                        while not self.credits:
                            if self.cancelled:
                                raise asyncio.CancelledError()
                            self.credit_event.clear()
                            await asyncio.wait_for(self.credit_event.wait(), 5)
                        self.credits -= 1
                        part = pcm[start:start+640]
                        await self.local.send(HEADER.pack(1, 2, 0, turn, seq, len(part)) + part)
                        seq += 1
                    continue
                msg = json.loads(raw)
                if not isinstance(msg, dict):
                    raise ValueError('Invalid cloud message')
                if msg.get('session_id') not in (None, self.session):
                    continue
                if msg.get('type') == 'stt':
                    await self.send('asr.final', turn=turn, text=clipped(msg.get('text'), 1023))
                elif msg.get('type') == 'tts':
                    if msg.get('state') == 'sentence_start':
                        answer = clipped(answer + msg.get('text', ''), 1535)
                        await self.send('answer.snapshot', turn=turn, text=answer)
                    elif msg.get('state') == 'stop':
                        if not seq:
                            raise ValueError('No speech returned')
                        await self.send('output.end', turn=turn, last_seq=seq-1)
                        completed = True
                        return
                elif msg.get('type') in ('error', 'goodbye', 'alert'):
                    raise ValueError('Cloud service unavailable')
                # No MCP/tools/system commands are advertised or executed.
        except (ConnectionClosed, OSError, ValueError, TypeError, asyncio.TimeoutError):
            await self.send('error', turn=turn, code='xiaozhi_unavailable')
        finally:
            # Keep only this live Talk session's provider context after success.
            # Cancellation/error closes it so obsolete audio cannot cross turns.
            if not completed:
                await self.close_cloud()

    async def message(self, raw):
        if isinstance(raw, bytes):
            if self.turn is None or self.committed or self.seq >= 750 or time.monotonic()-self.started > 16:
                raise ValueError('No active recording')
            decode_audio(raw, self.turn, self.seq)
            self.seq += 1; self.pcm.extend(raw[14:])
            if len(self.pcm) >= 1920:
                await self.cloud.send(self.codec.encode(bytes(self.pcm[:1920])))
                del self.pcm[:1920]
            return
        msg = json.loads(raw)
        if not isinstance(msg, dict) or msg.get('v') != 1:
            raise ValueError('Invalid protocol')
        kind = msg.get('type')
        if kind == 'hello':
            if self.hello:
                raise ValueError('Duplicate hello')
            await self.connect_cloud(); self.hello = True
            await self.send('ready', audio='pcm_s16le', rate=16000, frame_ms=20, backend='xiaozhi')
            return
        if not self.hello:
            raise ValueError('Hello required')
        if kind == 'input.begin':
            turn = msg.get('turn')
            if type(turn) is not int or not 0 < turn < 2**32 or msg.get('capability') != 'qa' or msg.get('memory') != 'none':
                raise ValueError('Unsupported turn')
            if self.turn is not None:
                if self.task and self.task.done():
                    await self.task
                    self.task = None; self.turn = None; self.credits = 0
                    self.pcm.clear()
                else:
                    raise ValueError('Cancel previous turn first')
            await self.connect_cloud()
            self.turn, self.seq, self.committed = turn, 0, False
            self.cancelled = False
            self.started = time.monotonic()
            await self.cloud.send(json.dumps({'type': 'listen', 'state': 'start', 'mode': 'manual', 'session_id': self.session}))
        elif kind == 'input.commit':
            if self.turn is None or msg.get('turn') != self.turn or self.committed or not self.seq or msg.get('last_seq') != self.seq-1:
                raise ValueError('Invalid commit')
            if self.pcm:
                await self.cloud.send(self.codec.encode(bytes(self.pcm).ljust(1920, b'\0')))
                self.pcm.clear()
            self.committed = True
            await self.cloud.send(json.dumps({'type': 'listen', 'state': 'stop', 'session_id': self.session}))
            self.task = asyncio.create_task(self.answer(self.turn))
        elif kind == 'output.credit':
            if msg.get('turn') != self.turn:
                return
            n = msg.get('frames')
            if type(n) is not int or not 0 < n <= 12 or self.credits+n > 12:
                raise ValueError('Invalid credit')
            self.credits += n; self.credit_event.set()
        elif kind == 'cancel':
            if msg.get('turn') == self.turn:
                await self.cancel()
        else:
            raise ValueError('Unsupported message')


active = False


async def handler(ws):
    global active
    if active:
        await ws.close(code=1013, reason='One desktop test at a time'); return
    active = True
    session = Session(ws)
    try:
        while True:
            await session.message(await asyncio.wait_for(ws.recv(), 60))
    except (ValueError, TypeError, OSError, RuntimeError, asyncio.TimeoutError, ConnectionClosed):
        await ws.close(code=1011, reason='Talk unavailable; reconnect')
    finally:
        await session.cancel(); active = False


async def main(args):
    if args.probe:
        ws, _, audio = await open_cloud()
        await ws.close()
        print(json.dumps({'tls_verified': True, 'official_hello': True, 'audio_params': audio,
                          'audio_or_inference_sent': False}))
        return
    codec = Opus(); codec.close()
    async with serve(handler, '127.0.0.1', args.port, max_size=2048, max_queue=4,
                     write_limit=4096, origins=['http://127.0.0.1:8876', 'http://localhost:8876'],
                     close_timeout=1):
        print(f'XiaoZhi desktop bridge: 127.0.0.1:{args.port}; button-only microphone; no local audio logs', flush=True)
        await asyncio.Future()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=8882)
    parser.add_argument('--probe', action='store_true')
    try:
        asyncio.run(main(parser.parse_args()))
    except Exception as error:
        # Exception messages can contain issued endpoints. Print only the error class.
        raise SystemExit(f'XiaoZhi unavailable ({type(error).__name__}); keep local mode.')
