const assert=require('node:assert/strict'),vm=require('node:vm'),fs=require('node:fs');
const source=fs.readFileSync('preview/talk_playback_worklet.js','utf8');
function create(rate,window=36,prebuffer=15){
 const messages=[];let Processor;
 vm.runInNewContext(source,{sampleRate:rate,Float32Array,Math,AudioWorkletProcessor:class{constructor(){this.port={postMessage:m=>messages.push(m)};}},registerProcessor:(_,p)=>Processor=p});
 const p=new Processor({processorOptions:{window,prebuffer}});
 return {p,messages,push:(value,n=320)=>p.port.onmessage({data:{pcm:new Float32Array(n).fill(value)}}),end:()=>p.port.onmessage({data:{end:true}}),step:()=>{const out=new Float32Array(128);p.process([],[[out]]);return out;}};
}
for(const rate of [16000,44100,48000]){
 const a=create(rate),out=[];
 for(let i=0;i<10;i++)a.push(.25);
 for(let i=0;i<30;i++)assert(a.step().every(v=>v===0)); // prebuffer is silence
 assert(a.messages.every(m=>m.level===0&&m.consumed===0));
 for(let i=0;i<10;i++)a.push(0);
 for(let i=0;i<10;i++)a.push(.25);
 a.end();a.messages.length=0;
 for(let i=0;i<Math.ceil(rate*.65/128);i++)out.push(...a.step());
 assert.equal(a.messages.reduce((n,m)=>n+m.consumed,0),30);
 assert.equal(a.messages.reduce((n,m)=>n+m.underruns,0),0);
 assert.equal(a.messages.filter(m=>m.done).length,1);
 assert(out.slice(Math.ceil(rate*.03),Math.floor(rate*.18)).every(v=>Math.abs(v-.25)<.0001));
 assert(out.slice(Math.ceil(rate*.22),Math.floor(rate*.38)).every(v=>v===0));
 assert(out.slice(Math.ceil(rate*.43),Math.floor(rate*.58)).every(v=>Math.abs(v-.25)<.0001));
 assert.equal(a.messages.at(-1).level,0);
 const short=create(rate);short.push(.3,83);short.end();for(let i=0;i<20;i++)short.step();
 assert.equal(short.messages.reduce((n,m)=>n+m.consumed,0),1);assert.equal(short.messages.filter(m=>m.done).length,1);
 const gap=create(rate);for(let i=0;i<15;i++)gap.push(.25);
 for(let i=0;i<Math.ceil(rate*.4/128);i++)gap.step();
 assert.equal(gap.messages.reduce((n,m)=>n+m.underruns,0),1);assert.equal(gap.messages.at(-1).level,0);
 gap.push(.3);for(let i=0;i<10;i++)assert(gap.step().every(v=>v===0));
 gap.end();for(let i=0;i<Math.ceil(rate*.05/128);i++)gap.step();
 assert.equal(gap.messages.reduce((n,m)=>n+m.consumed,0),16);assert.equal(gap.messages.filter(m=>m.done).length,1);
 const stopped=create(rate);stopped.push(.25);stopped.p.port.onmessage({data:{stop:true}});assert.equal(stopped.p.process([],[[new Float32Array(128)]]),false);assert.equal(stopped.p.queue.length,0);
 const bounded=create(rate);for(let i=0;i<37;i++)bounded.push(.1);assert(bounded.messages.at(-1).error);assert.equal(bounded.p.queue.length,36);
}
console.log('Playback worklet: PASS (16/44.1/48 kHz, silence envelope, short tail, exact credits, bounded queue, rebuffer and drain)');
