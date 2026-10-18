"""Prevent stale browser modules and unnoticed character-source replacement."""
import hashlib
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]

class ArtifactTests(unittest.TestCase):
    def test_browser_sources_match_compiled_module(self):
        hashes = json.loads((ROOT / 'preview/build-manifest.json').read_text())
        self.assertEqual(set(hashes), {'main/zumi/core.c', 'main/zumi/core.h',
            'main/zumi/view.c', 'main/zumi/view.h', 'preview/bridge.c', 'preview/pocket.wasm'})
        for name, sha in hashes.items():
            self.assertEqual(hashlib.sha256((ROOT / name).read_bytes()).hexdigest(), sha,
                             f'{name}: rebuild with tools/build_preview.sh')

    def test_character_provenance_and_budget(self):
        folder = ROOT / 'assets/images/zumi-pocket'
        manifest = json.loads((folder / 'manifest.json').read_text())
        self.assertEqual(hashlib.sha256((folder / 'zumi-original.svg').read_bytes()).hexdigest(),
                         manifest['source_sha256'])
        self.assertEqual(len(manifest['assets']), 3)
        self.assertEqual(sum(a['rgb565_bytes'] for a in manifest['assets']), 43056)
        for asset in manifest['assets']:
            self.assertEqual(hashlib.sha256((folder / asset['name']).read_bytes()).hexdigest(),
                             asset['sha256'])

if __name__ == '__main__':
    unittest.main()
