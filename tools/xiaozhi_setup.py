#!/usr/bin/env python3
"""Request official XiaoZhi binding metadata for a distinct desktop test client.
No audio, inference, firmware downloads, activation polling or account login.
Private identity and issued credentials live outside the repository/HTTP root.
"""
import argparse
import json
import os
from pathlib import Path
import secrets
import tempfile
import urllib.error
import urllib.parse
import urllib.request
import uuid

OTA_URL = 'https://api.tenclass.net/xiaozhi/ota/'
PRIVATE_DIR = Path.home()/'Library/Application Support/Zumi Pocket'


def save_private(path, data):
    path.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    os.chmod(path.parent, 0o700)
    fd, name = tempfile.mkstemp(prefix='.config-', dir=path.parent)
    try:
        with os.fdopen(fd, 'w') as handle:
            json.dump(data, handle, indent=2)
            handle.write('\n')
        os.chmod(name, 0o600)
        os.replace(name, path)
    finally:
        if os.path.exists(name):
            os.unlink(name)


def load_identity(path):
    if path.exists():
        return json.loads(path.read_text())
    mac = bytes([0x02])+secrets.token_bytes(5)  # Local, unicast, software-only identity.
    identity = {'kind':'zumi-pocket-desktop', 'client_id':str(uuid.uuid4()),
                'device_id':':'.join(f'{n:02x}' for n in mac)}
    save_private(path, identity)
    return identity


def allowed_ws(url):
    if not isinstance(url, str):
        return False
    p = urllib.parse.urlsplit(url)
    host = p.hostname or ''
    return p.scheme=='wss' and not p.username and not p.password and any(
        host==domain or host.endswith('.'+domain) for domain in ('xiaozhi.me','tenclass.net'))


def apply_response(identity, data):
    if not isinstance(data, dict):
        raise ValueError('invalid official response')
    result = dict(identity)
    # Firmware metadata is deliberately ignored: this client never downloads firmware.
    result.pop('websocket', None)
    ws = data.get('websocket')
    if isinstance(ws, dict) and allowed_ws(ws.get('url','')):
        token = ws.get('token')
        if isinstance(token,str) and len(token)<=8192:
            result['websocket'] = {'url':ws['url'], 'token':token, 'version':ws.get('version',1)}
    activation = data.get('activation')
    result['binding_required'] = isinstance(activation,dict)
    code = activation.get('code') if isinstance(activation,dict) else None
    result['binding_code'] = str(code) if code is not None and str(code).isascii() and str(code).isdigit() and len(str(code))<=12 else None
    result['binding_challenge_present'] = bool(isinstance(activation,dict) and activation.get('challenge'))
    # A credential response is not proof of account binding or available free service.
    result['account_service_verified'] = False
    return result


def request_binding(path):
    identity = load_identity(path)
    payload = {'application':{'version':'1.0.0'},
               'board':{'type':'zumi-pocket-desktop','name':'Zumi Pocket desktop test',
                        'mac':identity['device_id']}}
    req = urllib.request.Request(OTA_URL, data=json.dumps(payload).encode(), headers={
        'Content-Type':'application/json','Activation-Version':'1',
        'Device-Id':identity['device_id'],'Client-Id':identity['client_id'],
        'User-Agent':'zumi-pocket-desktop/1.0.0','Accept-Language':'zh-CN'}, method='POST')
    # Default TLS certificate and hostname checks remain enabled.
    with urllib.request.urlopen(req, timeout=15) as response:
        raw = response.read(65537)
        if len(raw)>65536:
            raise ValueError('oversize official response')
    result = apply_response(identity,json.loads(raw))
    save_private(path,result)
    return result


def public_status(data):
    return {'client':'Zumi Pocket desktop test','binding_required':data.get('binding_required'),
            'binding_code':data.get('binding_code'),
            'challenge_present':data.get('binding_challenge_present',False),
            'secure_endpoint_received':bool(data.get('websocket')),
            'account_service_verified':False,'audio_or_inference_sent':False}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action',choices=['status','request-binding'])
    args=parser.parse_args();path=PRIVATE_DIR/'xiaozhi-desktop.json'
    try:
        data = request_binding(path) if args.action=='request-binding' else load_identity(path)
        print(json.dumps(public_status(data),indent=2))
    except urllib.error.HTTPError as error:
        raise SystemExit(f'Official binding endpoint returned HTTP {error.code}; no audio or inference was sent.')
    except (OSError,ValueError,urllib.error.URLError):
        raise SystemExit('Official binding metadata unavailable; keep local mode. No audio or inference was sent.')

if __name__=='__main__':
    main()
