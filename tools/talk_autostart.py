#!/usr/bin/env python3
"""Install/remove user-level macOS Talk services. No microphone or cloud call at login."""
import argparse
import os
from pathlib import Path
import plistlib
import subprocess
import shutil
import sys

ROOT = Path(__file__).resolve().parents[1]
PREFIX = 'eco.zco.zumi-pocket.'
RUNTIME = Path.home()/'Library/Application Support/Zumi Pocket/desktop-runtime'
SERVICES = {'preview': ['-m', 'http.server', '8876', '--bind', '127.0.0.1'],
            'mock': ['tools/talk_mock_server.py'],
            'voice': ['tools/talk_xiaozhi_relay.py']}


def definition(name, python, opus):
    return {'Label': PREFIX+name, 'ProgramArguments': [python, *SERVICES[name]],
            'WorkingDirectory': str(RUNTIME), 'RunAtLoad': True, 'KeepAlive': True,
            'ThrottleInterval': 10, 'ProcessType': 'Background',
            'EnvironmentVariables': {'PYTHONUNBUFFERED': '1', 'PYTHONPATH': str(RUNTIME/'python'), 'ZUMI_OPUS_LIBRARY': opus},
            'StandardOutPath': '/dev/null', 'StandardErrorPath': '/dev/null'}


def stage_runtime():
    # A narrow runtime snapshot avoids granting a background Python process access
    # to the user's protected Documents folder. Credentials remain in its parent.
    import websockets
    RUNTIME.mkdir(parents=True, exist_ok=True)
    owner = RUNTIME.parent/'desktop-runtime-checkout.txt'
    if owner.exists() and owner.read_text().strip() != str(ROOT):
        raise ValueError('Runtime belongs to another checkout')
    owner.write_text(str(ROOT)+'\n')
    for relative in ('preview', 'assets/images/zumi-pocket', 'docs/assets', 'config'):
        shutil.copytree(ROOT/relative, RUNTIME/relative, dirs_exist_ok=True,
                        ignore=shutil.ignore_patterns('__pycache__', '*.pyc'))
    (RUNTIME/'build/talk-mock').mkdir(parents=True, exist_ok=True)
    for fixture in ('intro.wav', 'solar.wav'):
        shutil.copy2(ROOT/'build/talk-mock'/fixture, RUNTIME/'build/talk-mock'/fixture)
    (RUNTIME/'tools').mkdir(exist_ok=True)
    for name in ('talk_xiaozhi_relay.py','talk_mock_server.py','talk_opus.py','xiaozhi_setup.py','talk_mock_fixtures.json'):
        shutil.copy2(ROOT/'tools'/name, RUNTIME/'tools'/name)
    shutil.copytree(Path(websockets.__file__).parent, RUNTIME/'python/websockets', dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns('__pycache__', '*.pyc'))
    return str(Path(sys.executable).resolve())


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('action', choices=['install', 'status', 'remove'])
    p.add_argument('--opus', default=os.environ.get('ZUMI_OPUS_LIBRARY'))
    a = p.parse_args()
    if sys.platform != 'darwin':
        p.error('macOS only')
    domain = f'gui/{os.getuid()}'
    folder = Path.home()/'Library/LaunchAgents'
    if a.action == 'install':
        if not a.opus or not Path(a.opus).is_file():
            p.error('Pass --opus with the existing local libopus path')
        # Verify dependencies before changing any job. Never install system packages here.
        import websockets.asyncio.server  # noqa: F401
        os.environ['ZUMI_OPUS_LIBRARY'] = str(Path(a.opus).absolute())
        from talk_opus import Opus
        codec = Opus(); codec.close()
        if not (ROOT/'preview/pocket.wasm').is_file():
            p.error('Build the browser preview first')
        folder.mkdir(parents=True, exist_ok=True)
        python = stage_runtime()
    for name in SERVICES:
        label = PREFIX+name
        path = folder/(label+'.plist')
        if a.action == 'status':
            result = subprocess.run(['launchctl', 'print', domain+'/'+label], capture_output=True, text=True)
            fields = [line.strip() for line in result.stdout.splitlines()
                      if line.strip().startswith(('state =', 'pid =', 'last exit code ='))]
            print(label+': '+('; '.join(fields) if result.returncode == 0 else 'not loaded'))
            continue
        if a.action == 'remove' and not path.exists():
            continue
        # Refuse to replace an unrelated job using the same filename.
        if path.exists():
            prior = plistlib.loads(path.read_bytes())
            if prior.get('Label') != label or prior.get('WorkingDirectory') not in (str(ROOT), str(RUNTIME)):
                p.error('Existing job belongs to another checkout; keep it unchanged')
        subprocess.run(['launchctl', 'bootout', domain+'/'+label], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if a.action == 'remove':
            path.unlink(missing_ok=True)
            print(label+': removed')
        else:
            path.write_bytes(plistlib.dumps(definition(name, python, str(Path(a.opus).absolute()))))
            path.chmod(0o600)
            subprocess.run(['launchctl', 'bootstrap', domain, str(path)], check=True)
            print(label+': installed')


if __name__ == '__main__':
    main()
