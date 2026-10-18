const assert=require('node:assert/strict'),vm=require('node:vm'),fs=require('node:fs');
const source=fs.readFileSync('preview/talk_mic_worklet.js','utf8');
function create(rate){
 const messages=[];let Processor;
 const context={sampleRate:rate,AudioWorkletProcessor:class{constructor(){this.port={postMessage:m=>messages.push(m instanceof ArrayBuffer?m.slice(0):m)};}},registerProcessor:(_,p)=>Processor=p,Int16Array,ArrayBuffer,Math};
 vm.runInNewContext(source,context);return {processor:new Processor(),messages};
}
for(const rate of [16000,44100,48000]){
 const {processor,messages}=create(rate),count=rate+137;
 for(let i=0;i<count;i+=128)processor.process([[new Float32Array(Math.min(128,count-i)).fill(.25)]]);
 processor.port.onmessage({data:'finish'});
 const frames=messages.filter(x=>x instanceof ArrayBuffer),samples=frames.flatMap(x=>Array.from(new Int16Array(x)));
 const useful=Math.ceil(count*16000/rate-1e-7);
 assert.equal(frames.length,Math.ceil(useful/320));
 assert(samples.slice(0,useful).every(x=>x===8192));assert(samples.slice(useful).every(x=>x===0));
 assert.equal(messages.at(-1).finished,true);assert.equal(processor.process([[new Float32Array(128)]]),false);
 const total=messages.length;processor.port.onmessage({data:'finish'});assert.equal(messages.length,total);
}
console.log('Capture worklet: PASS (16/44.1/48 kHz, bounded frames, partial tail, finish ordering)');
