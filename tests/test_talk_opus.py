#!/usr/bin/env python3
import math
import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from talk_opus import Opus

class OpusTests(unittest.TestCase):
    def test_roundtrip_and_bounds(self):
        codec=Opus()
        try:
            pcm=struct.pack('<960h',*[round(4000*math.sin(i*2*math.pi*440/16000)) for i in range(960)])
            packet=codec.encode(pcm);decoded=codec.decode(packet)
            self.assertLess(len(packet),4000);self.assertEqual(len(decoded),1920)
            self.assertGreater(max(struct.unpack('<960h',decoded)),1000)
            with self.assertRaises(ValueError):codec.encode(b'')
            with self.assertRaises(ValueError):codec.decode(bytes(4001))
        finally:codec.close();codec.close()

if __name__=='__main__':unittest.main()
