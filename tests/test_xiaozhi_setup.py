#!/usr/bin/env python3
import json
import stat
import sys
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from xiaozhi_setup import load_identity,apply_response,public_status,allowed_ws

class SetupTests(unittest.TestCase):
    def test_private_stable_software_identity(self):
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'private'/'config.json';a=load_identity(path);b=load_identity(path)
            self.assertEqual(a,b);self.assertTrue(a['device_id'].startswith('02:'))
            self.assertEqual(stat.S_IMODE(path.stat().st_mode),0o600)
            self.assertEqual(stat.S_IMODE(path.parent.stat().st_mode),0o700)
    def test_no_endpoint_escape_or_secret_output(self):
        for url in ['ws://api.tenclass.net/','wss://tenclass.net.attacker.example/','wss://u:p@xiaozhi.me/','https://xiaozhi.me/']:
            self.assertFalse(allowed_ws(url))
        secret='fictional-unit-credential'
        d=apply_response({'kind':'test'},{'websocket':{'url':'wss://api.tenclass.net/test','token':secret},'firmware':{'url':'https://example.invalid/fw'},'activation':{'code':'123456'}})
        self.assertTrue(d['binding_required']);self.assertNotIn('firmware',d)
        self.assertNotIn(secret,json.dumps(public_status(d)))
        self.assertFalse(d['account_service_verified'])
    def test_no_activation_is_not_account_proof(self):
        d=apply_response({}, {'websocket':{'url':'wss://xiaozhi.me/test','token':''}})
        self.assertFalse(d['binding_required']);self.assertFalse(d['account_service_verified'])
        self.assertIsNone(apply_response({}, {'activation':{'code':'<script>'}})['binding_code'])

if __name__=='__main__':unittest.main()
