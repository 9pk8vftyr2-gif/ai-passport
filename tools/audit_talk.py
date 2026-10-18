#!/usr/bin/env python3
"""Read-only artifact audit; never queries or opens a device."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--archive', required=True, type=Path)
parser.add_argument('--size-report', required=True, type=Path)
args = parser.parse_args()
subprocess.run(['python3', str(ROOT/'tools/archive_firmware.py'), 'verify', str(args.archive)],
               check=True, stdout=subprocess.DEVNULL)
manifest = json.loads((args.archive/'manifest.json').read_text())
partitions = []
for line in (ROOT/'partitions.csv').read_text().splitlines():
    if not line.strip() or line.startswith('#'):
        continue
    fields = [s.strip() for s in line.split(',')]
    partitions.append(dict(name=fields[0], type=fields[1], subtype=fields[2],
                           offset=int(fields[3], 0), size=int(fields[4], 0)))
app = manifest['files']['FoloToy-AI-Passport.bin']['size']
factory = next(p for p in partitions if p['name']=='factory')
assert app <= factory['size']
display_source = (ROOT/'components/bsp/src/bsp_display_lvgl.c').read_text()
lines = int(re.search(r'#define BSP_LVGL_DRAW_BUFFER_LINES (\d+)', display_source)[1])
size = json.loads(args.size_report.read_text())
dram = next(x for x in size['layout'] if x['name']=='DRAM')
files = ['partitions.csv','sdkconfig.defaults','main/CMakeLists.txt','main/zumi_main.c',
         'main/zumi/audio.c','components/bsp/src/bsp_audio.c','components/bsp/src/bsp_display_lvgl.c']
report = {
    'date':'2026-10-18',
    'source_revision':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
    'scope':'Zumi runtime baseline; Talk model is not linked into firmware',
    'partitions':partitions,'flash_bytes':manifest['flash_size_bytes'],
    'app_bytes':app,'app_remaining_bytes':factory['size']-app,
    'merged_bytes':manifest['files']['FoloToy-AI-Passport-full.bin']['size'],
    'full_sha256':manifest['full_bin_sha256'],'elf_sha256':manifest['app_elf_sha256'],
    'image_version':manifest['app_descriptor']['version'],
    'idf_version':manifest['app_descriptor']['idf_version'],
    'linker_dram_used_bytes':dram['used'],'linker_dram_remaining_bytes':dram['free'],
    'linker_remainder_is_runtime_heap':False,
    'lvgl_static_pool_bytes':24576,'lcd_dma_buffer_bytes':240*lines*2,
    'runtime_free_internal_heap_bytes':None,'runtime_largest_internal_block_bytes':None,
    'runtime_min_free_heap_bytes':None,'device_tests':'NOT RUN',
    'source_sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in files}
}
print(json.dumps(report,indent=2))
