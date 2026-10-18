#!/usr/bin/env python3
"""Generate local test speech using the installed macOS voice; never upload it."""
from pathlib import Path
import json
import shutil
import subprocess
import wave
ROOT = Path(__file__).resolve().parents[1]
FIXTURES = json.loads((ROOT/'tools/talk_mock_fixtures.json').read_text())
def main():
    say, ffmpeg = shutil.which('say'), shutil.which('ffmpeg')
    if not say or not ffmpeg:
        raise SystemExit('Requires local macOS say and ffmpeg, or supply build/talk-mock/{intro,solar}.wav as 16 kHz mono PCM16.')
    folder = ROOT / 'build/talk-mock'
    folder.mkdir(parents=True, exist_ok=True)
    for name, fixture in FIXTURES.items():
        subprocess.run([say, '-v', 'Daniel', '-r', '180', '-o', str(folder/f'{name}.aiff'), fixture['spoken']], check=True)
        subprocess.run([ffmpeg, '-v', 'error', '-y', '-i', str(folder/f'{name}.aiff'), '-ar', '16000', '-ac', '1', '-c:a', 'pcm_s16le', str(folder/f'{name}.wav')], check=True)
        with wave.open(str(folder/f'{name}.wav')) as audio:
            assert (audio.getframerate(), audio.getnchannels(), audio.getsampwidth()) == (16000,1,2)
    print('Local speech fixture ready; it is a test voice, not the approved Zumi voice.')
if __name__ == '__main__':
    main()
