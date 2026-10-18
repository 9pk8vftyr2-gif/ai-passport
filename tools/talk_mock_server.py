#!/usr/bin/env python3
"""Local-only ASR/answer fixtures and streamed, locally synthesized speech.
Never stores incoming PCM/transcripts; never calls an AI provider.
"""
import argparse
import asyncio
import json
import struct
import wave
from pathlib import Path
from websockets.asyncio.server import serve
from websockets.exceptions import ConnectionClosed

ROOT = Path(__file__).resolve().parents[1]
HEADER = struct.Struct('<BBHIIH')
FIXTURES = json.loads((ROOT/'tools/talk_mock_fixtures.json').read_text())

def decode_audio(data, turn, seq):
    if not isinstance(data, bytes) or not 16 <= len(data) <= 654:
        raise ValueError('frame size')
    version, kind, flags, actual_turn, actual_seq, length = HEADER.unpack_from(data)
    if (version, kind, flags, actual_turn, actual_seq) != (1, 1, 0, turn, seq):
        raise ValueError('frame identity or sequence')
    if not 0 < length <= 640 or length % 2 or len(data) != 14 + length:
        raise ValueError('PCM length')
    return length

class Session:
    def __init__(self, ws, fault='none'):
        self.ws, self.fault = ws, fault
        self.fixture = "intro"
        self.hello = False
        self.turn = None
        self.seq = 0
        self.committed = False
        self.credits = 0
        self.credit_event = asyncio.Event()
        self.task = None

    async def send(self, kind, **values):
        await self.ws.send(json.dumps(dict(v=1, type=kind, **values)))

    async def cancel(self):
        if self.task:
            self.task.cancel()
            try:
                await self.task
            except (asyncio.CancelledError, ConnectionClosed):
                pass
        self.task = None
        self.turn = None
        self.credits = 0

    async def answer(self, turn):
        try:
            if self.fault == 'timeout':
                await asyncio.sleep(25)
                return
            if self.fault == 'disconnect':
                await self.ws.close()
                return
            if self.fault == 'error':
                await self.send('error', turn=turn, code='mock_service_error')
                return
            await asyncio.sleep(.15)
            fixture = FIXTURES[self.fixture]
            await self.send('asr.final', turn=turn, text=fixture['question'])
            await self.send('answer.snapshot', turn=turn, seq=0,
                            text=fixture['answer'])
            seq = 0
            with wave.open(str(ROOT/'build/talk-mock'/f'{self.fixture}.wav'), 'rb') as audio:
                assert (audio.getframerate(), audio.getnchannels(), audio.getsampwidth()) == (16000, 1, 2)
                while pcm := audio.readframes(320):
                    while self.credits <= 0:
                        self.credit_event.clear()
                        await asyncio.wait_for(self.credit_event.wait(), 5)
                    self.credits -= 1
                    packet = HEADER.pack(1, 2, 0, turn, seq, len(pcm)) + pcm
                    if self.fault == 'bad-frame':
                        packet = packet[:12] + b'\xff\xff' + packet[14:]
                    await self.ws.send(packet)
                    seq += 1
                    # Playback credits provide pacing; fixed sleeps accumulate gaps.
                    await asyncio.sleep(0)
            await self.send('output.end', turn=turn, last_seq=seq-1)
        except (ConnectionClosed, asyncio.TimeoutError, OSError, wave.Error):
            await self.ws.close()

    async def message(self, raw):
        if isinstance(raw, bytes):
            if self.turn is None or self.committed or self.seq >= 750:
                raise ValueError('no active recording')
            decode_audio(raw, self.turn, self.seq)
            self.seq += 1
            return
        msg = json.loads(raw)
        if not isinstance(msg, dict) or msg.get('v') != 1:
            raise ValueError('version')
        kind = msg.get('type')
        if kind == 'hello':
            self.hello = True
            await self.send('ready', audio='pcm_s16le', rate=16000, frame_ms=20)
            return
        if not self.hello:
            raise ValueError('hello required')
        if kind == 'input.begin':
            turn = msg.get('turn')
            if type(turn) is not int or not 0 < turn < 2**32 or msg.get('capability') != 'qa':
                raise ValueError('unsupported turn or capability')
            fixture = msg.get('fixture', 'intro')
            if fixture not in FIXTURES:
                raise ValueError('unknown mock fixture')
            await self.cancel()
            self.fixture = fixture
            self.turn, self.seq, self.committed = turn, 0, False
        elif kind == 'cancel':
            if msg.get('turn') == self.turn:
                await self.cancel()
        elif kind == 'input.commit':
            if self.turn is None or msg.get('turn') != self.turn or self.committed or not self.seq or msg.get('last_seq') != self.seq-1:
                raise ValueError('incomplete or duplicate commit')
            self.committed = True
            self.task = asyncio.create_task(self.answer(self.turn))
        elif kind == 'output.credit':
            n = msg.get('frames')
            if msg.get('turn') != self.turn:
                return
            if type(n) is not int or not 0 < n <= 12 or self.credits+n > 12:
                raise ValueError('credit overflow')
            self.credits += n
            self.credit_event.set()
        else:
            raise ValueError('unsupported message')

async def handler(ws, fault='none'):
    session = Session(ws, fault)
    try:
        while True:
            data = await asyncio.wait_for(ws.recv(), 60)
            await session.message(data)
    except (ValueError, TypeError, json.JSONDecodeError, struct.error):
        await ws.close(code=1008, reason='invalid local test protocol')
    except asyncio.TimeoutError:
        await ws.close(code=1000, reason="idle local session")
    except ConnectionClosed:
        pass
    finally:
        await session.cancel()

async def main(args):
    async with serve(lambda ws:handler(ws,args.fault), args.host, args.port,
                     max_size=2048, max_queue=4, write_limit=4096,
                     origins=[None,'http://127.0.0.1:8876','http://localhost:8876'],
                     ping_interval=10, ping_timeout=10, close_timeout=1):
        print(f'Local Talk mock listening on {args.host}:{args.port}; no AI provider, no recording storage', flush=True)
        await asyncio.Future()

if __name__ == '__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--host', default='127.0.0.1')
    parser.add_argument('--port',type=int,default=8881)
    parser.add_argument('--fault',choices=['none','timeout','error','disconnect','bad-frame'],default='none')
    asyncio.run(main(parser.parse_args()))
