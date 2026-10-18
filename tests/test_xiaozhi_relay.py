#!/usr/bin/env python3
"""Offline protocol tests: no account credentials or cloud requests."""
import asyncio
import json
import sys
import time
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from talk_xiaozhi_relay import Session, clipped, HEADER


class Socket:
    def __init__(self):
        self.sent = []; self.queue = asyncio.Queue(); self.closed = False
    async def send(self, data): self.sent.append(data)
    async def recv(self): return await self.queue.get()
    async def close(self): self.closed = True


class Codec:
    def encode(self, pcm):
        assert len(pcm) == 1920
        return b'opus'
    def decode(self, packet): return b'\0' * 1920
    def close(self): pass


class RelayTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.local = Socket(); self.clouds = []
        async def cloud():
            ws = Socket(); self.clouds.append(ws); return ws, 'test-session', {}
        self.session = Session(self.local, cloud, Codec)
        await self.msg('hello')
    async def asyncTearDown(self): await self.session.cancel()
    async def msg(self, kind, **kw):
        await self.session.message(json.dumps(dict(v=1, type=kind, **kw)))
    async def begin(self, turn=1):
        await self.msg('input.begin', turn=turn, capability='qa', memory='none')
    async def audio(self, seq=0, turn=1):
        await self.session.message(HEADER.pack(1, 1, 0, turn, seq, 640) + b'\0'*640)
    async def test_complete_and_fresh_session(self):
        await self.begin(); await self.audio()
        self.assertEqual(len(self.session.pcm), 640)
        await self.msg('input.commit', turn=1, last_seq=0)
        await self.msg('output.credit', turn=1, frames=12)
        cloud = self.clouds[-1]
        for m in [json.dumps({'type':'stt','text':'Hello'}),json.dumps({'type':'tts','state':'sentence_start','text':'Hi'}),b'opus',json.dumps({'type':'tts','state':'stop'})]:
            await cloud.queue.put(m)
        await self.session.task
        kinds = [json.loads(x)['type'] for x in self.local.sent if isinstance(x,str)]
        self.assertEqual(kinds,['ready','asr.final','answer.snapshot','output.end'])
        self.assertEqual(len([x for x in self.local.sent if isinstance(x,bytes)]),3)
        self.assertTrue(cloud.closed)
        await self.begin(2); self.assertEqual(len(self.clouds),2)
    async def test_cancel_waiting_credit_discards_old_audio(self):
        await self.begin(); await self.audio(); await self.msg('input.commit',turn=1,last_seq=0)
        old = self.clouds[-1]; await old.queue.put(b'opus'); await asyncio.sleep(0)
        started = time.monotonic()
        await self.msg('cancel',turn=1)
        self.assertLess(time.monotonic()-started, .2)
        self.assertTrue(old.closed); self.assertIsNone(self.session.turn)
        await self.begin(2); await old.queue.put(b'opus'); await asyncio.sleep(0)
        self.assertFalse(any(isinstance(x,bytes) for x in self.local.sent))
    async def test_bounds_sequence_and_duplicate_commit(self):
        await self.begin()
        with self.assertRaises(ValueError): await self.audio(1)
        await self.audio(); await self.msg('input.commit',turn=1,last_seq=0)
        with self.assertRaises(ValueError): await self.msg('input.commit',turn=1,last_seq=0)
        with self.assertRaises(ValueError): await self.msg('output.credit',turn=1,frames=13)
    async def test_cloud_error_propagates(self):
        await self.begin();await self.audio();await self.msg('input.commit',turn=1,last_seq=0)
        await self.clouds[-1].queue.put(json.dumps({'type':'error'}));await self.session.task
        self.assertEqual(json.loads(self.local.sent[-1])['type'],'error')
        self.assertTrue(self.clouds[-1].closed)
    def test_utf8_bounds(self):
        self.assertEqual(clipped('你好',4),'你')
        with self.assertRaises(ValueError): clipped(None,3)


if __name__ == '__main__': unittest.main()
