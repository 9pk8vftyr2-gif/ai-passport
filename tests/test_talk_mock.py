#!/usr/bin/env python3
"""Exercise the real local WebSocket server without external services."""
import asyncio
import json
import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from talk_mock_server import handler, decode_audio, HEADER
from websockets.asyncio.server import serve
from websockets.asyncio.client import connect
from websockets.exceptions import ConnectionClosed

class ProtocolTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.server = await serve(handler,'127.0.0.1',0,max_size=2048,max_queue=4)
        self.ws = await connect('ws://127.0.0.1:'+str(self.server.sockets[0].getsockname()[1]),proxy=None)
    async def asyncTearDown(self):
        await self.ws.close();self.server.close();await self.server.wait_closed()
    async def send(self,type,**values):
        await self.ws.send(json.dumps(dict(v=1,type=type,**values)))
    async def start(self):
        await self.send('hello');self.assertEqual(json.loads(await self.ws.recv())['type'],'ready')
        await self.send('input.begin',turn=1,capability='qa',memory='none')
        await self.ws.send(HEADER.pack(1,1,0,1,0,640)+bytes(640))
    async def test_stream_and_credit(self):
        await self.start();await self.send('input.commit',turn=1,last_seq=0)
        self.assertEqual(json.loads(await self.ws.recv())['type'],'asr.final')
        self.assertEqual(json.loads(await self.ws.recv())['type'],'answer.snapshot')
        with self.assertRaises(asyncio.TimeoutError):await asyncio.wait_for(self.ws.recv(),.08)
        await self.send('output.credit',turn=1,frames=1)
        first=await self.ws.recv();self.assertEqual(HEADER.unpack_from(first)[:5],(1,2,0,1,0))
        with self.assertRaises(asyncio.TimeoutError):await asyncio.wait_for(self.ws.recv(),.08)
        await self.send('output.credit',turn=1,frames=12);seq=1
        while True:
            data=await asyncio.wait_for(self.ws.recv(),2)
            if isinstance(data,str):
                self.assertEqual(json.loads(data)['last_seq'],seq-1);break
            self.assertEqual(HEADER.unpack_from(data)[4],seq);seq+=1
            await self.send('output.credit',turn=1,frames=1)
        self.assertGreater(seq,100)
        await self.send('input.begin',turn=2,capability='qa')
        await self.send('cancel',turn=2)
    async def test_duplicate_commit_rejected(self):
        await self.start();await self.send('input.commit',turn=1,last_seq=0);await self.send('input.commit',turn=1,last_seq=0)
        with self.assertRaises(ConnectionClosed):await self.ws.recv()
    async def test_sequence_rejected(self):
        await self.start();await self.ws.send(HEADER.pack(1,1,0,1,4,640)+bytes(640))
        with self.assertRaises(ConnectionClosed):await self.ws.recv()
    async def test_cancel_stops_answer(self):
        await self.start();await self.send('input.commit',turn=1,last_seq=0);await self.send('cancel',turn=1)
        with self.assertRaises(asyncio.TimeoutError):await asyncio.wait_for(self.ws.recv(),.25)
    async def test_credit_overflow_rejected(self):
        await self.start();await self.send('output.credit',turn=1,frames=13)
        with self.assertRaises(ConnectionClosed):await self.ws.recv()
    async def test_no_hello_rejected(self):
        await self.send('input.begin',turn=1,capability='qa')
        with self.assertRaises(ConnectionClosed):await self.ws.recv()
    def test_frame_bounds(self):
        for n in (0,1,642):
            with self.assertRaises(ValueError):decode_audio(HEADER.pack(1,1,0,1,0,n)+bytes(n),1,0)

if __name__=='__main__':unittest.main()
