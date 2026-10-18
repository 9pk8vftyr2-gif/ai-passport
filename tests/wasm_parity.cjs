const fs=require('node:fs'),cp=require('node:child_process'),assert=require('node:assert/strict');
cp.execFileSync(process.env.CC||'cc',['-std=c11','-Wall','-Wextra','-Werror','-Imain/zumi','tests/zumi_replay.c','preview/bridge.c','main/zumi/core.c','main/zumi/view.c','-o','build/zumi_replay']);
const native=cp.execFileSync('build/zumi_replay',{encoding:'utf8'}).trim().split('\n');
(async()=>{
 const {instance}=await WebAssembly.instantiate(fs.readFileSync('preview/pocket.wasm'),{wasi_snapshot_preview1:{clock_time_get:()=>52,fd_close:()=>0,fd_seek:()=>0,fd_write:()=>0}});
 const a=instance.exports;a._initialize?.();a.pocket_init(42);let random=123;
 for(let i=0;i<5000;i++) {
  if(i%250===0){a.pocket_init(42+i);a.pocket_button(1,0);a.pocket_button(2,0);a.pocket_button(2,4);if(i%500)a.pocket_button(1,0);a.pocket_button(2,0);a.pocket_button(2,4);}
  random=(Math.imul(random,1664525)+1013904223)>>>0;
  const key=random%3;a.pocket_button(key,0);a.pocket_button(key,4);a.pocket_tick(20);
  assert.equal([a.pocket_page(),a.pocket_score(),a.pocket_lives(),a.pocket_lane(),a.pocket_target()].join(','),native[i],`transition ${i}`);
 }
 console.log('PASS: native C / browser WASM parity, 5000 transitions');
})().catch(e=>{console.error(e);process.exit(1)});
