"""Prevent stale browser modules and unnoticed character-source replacement."""
import hashlib
import json
from pathlib import Path
import unittest
import subprocess
import sys
import struct
import re

ROOT = Path(__file__).resolve().parents[1]

class ArtifactTests(unittest.TestCase):
    def test_eco_pop_icons_match_original_sources_and_firmware(self):
        folder = ROOT / 'assets/images/zumi-pocket/energy-icons'
        manifest = json.loads((folder / 'manifest.json').read_text())
        self.assertEqual(manifest['flash_bytes'], 12288)
        self.assertEqual(len(manifest['assets']), 6)
        c = (ROOT / 'main/zumi/energy_icons.c').read_text()
        arrays = re.findall(r'static const uint8_t pixels_\d+\[\]=\{(.*?)\};', c, re.S)
        self.assertEqual(len(arrays), 6)
        for asset, array in zip(manifest['assets'], arrays):
            png = (folder / asset['file']).read_bytes()
            self.assertEqual(struct.unpack('>II', png[16:24]), (32, 32))
            self.assertEqual(hashlib.sha256(png).hexdigest(), asset['sha256'])
            self.assertEqual(hashlib.sha256((folder / 'source' / asset['source']).read_bytes()).hexdigest(), asset['source_sha256'])
            raw = bytes(int(n) for n in re.findall(r'\d+', array))
            self.assertEqual(len(raw), 2048)
            self.assertEqual(hashlib.sha256(raw).hexdigest(), asset['rgb565_sha256'])

    def test_browser_sources_match_compiled_module(self):
        hashes = json.loads((ROOT / 'preview/build-manifest.json').read_text())
        self.assertEqual(set(hashes), {'main/zumi/core.c', 'main/zumi/core.h', 'main/zumi/talk_model.c', 'main/zumi/talk_model.h',
            'main/zumi/view.c', 'main/zumi/view.h', 'main/zumi/sound.h', 'main/zumi/sound.c', 'main/zumi/sounds.c', 'preview/bridge.c', 'preview/pocket.wasm'})
        for name, sha in hashes.items():
            self.assertEqual(hashlib.sha256((ROOT / name).read_bytes()).hexdigest(), sha,
                             f'{name}: rebuild with tools/build_preview.sh')

    def test_sound_pcm_matches_source(self):
        subprocess.run([sys.executable, str(ROOT / 'tools/build_zumi_sounds.py'), '--check'], check=True)
        spec = json.loads((ROOT / 'assets/music/zumi-link-tones.json').read_text())
        self.assertEqual(spec['win_delay_ms'], 620)
        self.assertEqual(spec['sample_rate'], 16000)

    def test_character_provenance_and_budget(self):
        folder = ROOT / 'assets/images/zumi-pocket'
        manifest = json.loads((folder / 'manifest.json').read_text())
        self.assertEqual(hashlib.sha256((folder / 'zumi-original.svg').read_bytes()).hexdigest(),
                         manifest['source_sha256'])
        self.assertEqual(len(manifest['assets']), 4)
        self.assertEqual(sum(a['rgb565_bytes'] for a in manifest['assets']), 57408)
        for asset in manifest['assets']:
            self.assertEqual(hashlib.sha256((folder / asset['name']).read_bytes()).hexdigest(),
                             asset['sha256'])

    def test_wake_cards_identity_and_flash_budget(self):
        folder = ROOT / 'assets/images/zumi-pocket/wake-cards'
        manifest = json.loads((folder / 'manifest.json').read_text())
        raw = (folder / 'wake_cards.rgb565').read_bytes()
        self.assertEqual(len(raw), 8 * 240 * 320 * 2)
        self.assertEqual(manifest['flash_bytes'], len(raw))
        self.assertEqual(hashlib.sha256(raw).hexdigest(), manifest['sha256'])
        self.assertEqual(len(manifest['cards']), 8)
        self.assertEqual(len({c['original_sticker_sha256'] for c in manifest['cards']}), 8)
        for i, card in enumerate(manifest['cards']):
            self.assertEqual(card['id'], i)
            self.assertEqual(card['offset'], i * 153600)
            png = (folder / card['file']).read_bytes()
            self.assertEqual(struct.unpack('>II', png[16:24]), (240, 320))
            self.assertEqual(hashlib.sha256(png).hexdigest(), card['sha256'])
            self.assertEqual(hashlib.sha256(raw[i*153600:(i+1)*153600]).hexdigest(), card['rgb565_sha256'])
            self.assertEqual(hashlib.sha256((folder / 'source' / card['source']).read_bytes()).hexdigest(), card['source_sha256'])

if __name__ == '__main__':
    unittest.main()
