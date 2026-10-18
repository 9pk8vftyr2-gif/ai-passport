const fs=require('node:fs'),cp=require('node:child_process'),assert=require('node:assert/strict'),crypto=require('node:crypto');
for(const [path,hash] of Object.entries(JSON.parse(fs.readFileSync('preview/talk/manifest.json'))))
 assert.equal(crypto.createHash('sha256').update(fs.readFileSync(path)).digest('hex'),hash,path);
const audit=JSON.parse(fs.readFileSync('docs/assets/talk-v1/audit.json'));
assert.equal(audit.runtime_free_internal_heap_bytes,null);assert.equal(audit.runtime_largest_internal_block_bytes,null);
for(const [path,hash] of Object.entries(audit.source_sha256))
 assert.equal(crypto.createHash('sha256').update(fs.readFileSync(path)).digest('hex'),hash,`Audited runtime changed: ${path}`);
cp.execFileSync('cc',['-std=c11','-Wall','-Wextra','-Werror','-Imain/zumi','tests/talk_trace.c','main/zumi/talk_model.c','-o','build/talk_trace']);
(async()=>{
 const {instance}=await WebAssembly.instantiate(fs.readFileSync('preview/talk/talk.wasm'),{wasi_snapshot_preview1:{}}),a=instance.exports;
 a._initialize?.();a.talk_init();let seed=103,rows=[],actual=[];
 for(let i=0;i<10000;i++){
  seed=(Math.imul(seed,1664525)+1013904223)>>>0;
  const ev=seed%13-3,turn=(seed&128)?a.talk_turn():(a.talk_turn()-1)>>>0,ms=seed%61001;
  rows.push(`${ev} ${turn} ${ms}`);if(ev<0)a.talk_tick(ms);else a.talk_event(ev,turn);
  actual.push([a.talk_state(),a.talk_effect(),a.talk_error(),a.talk_turn(),a.talk_elapsed(),a.talk_mic(),a.talk_speaker(),a.talk_connecting()].join(' '));
 }
 const expected=cp.execFileSync('build/talk_trace',{input:rows.join('\n')+'\n',encoding:'utf8'}).trim().split('\n');
 assert.deepEqual(actual,expected);console.log('Talk native/WASM parity: PASS (10000 transitions); source/asset provenance PASS');
})().catch(e=>{console.error(e);process.exit(1)});
