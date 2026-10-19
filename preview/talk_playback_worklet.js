// Bounded PCM queue rendered on the audio thread, independent of page timers.
// Input: 16 kHz mono, <=320 samples/frame. Output: the context's native rate.
class ZumiPlayback extends AudioWorkletProcessor {
 constructor(options) {
  super();
  const {window=12,prebuffer=6}=options.processorOptions||{};
  this.limit=window;this.target=prebuffer;this.queue=[];this.offset=0;this.phase=0;
  this.playing=false;this.ended=false;this.finished=false;
  this.consumed=0;this.energy=0;this.samples=0;this.underruns=0;this.fade=0;
  this.port.onmessage=({data})=>{
   if(data.stop){this.finished=true;this.queue=[];return;}
   if(data.end){this.ended=true;return;}
   if(!(data.pcm instanceof Float32Array)||!data.pcm.length||data.pcm.length>320||this.queue.length>=this.limit||this.ended){this.port.postMessage({error:true});return;}
   this.queue.push(data.pcm);
  };
 }
 process(_inputs,outputs) {
  const out=outputs[0][0];out.fill(0);
  if(this.finished)return false;
  const step=16000/sampleRate;
  for(let i=0;i<out.length;i++){
   if(!this.playing&&(this.queue.length>=this.target||(this.ended&&this.queue.length))){this.playing=true;this.fade=0;}
   if(this.playing&&this.queue.length){
    const first=this.queue[0],a=first[this.offset];
    const b=this.offset+1<first.length?first[this.offset+1]:(this.queue[1]?.[0]??a);
    // One short attack at stream start/rebuffer, never one fade per packet.
    this.fade=Math.min(1,this.fade+1/(sampleRate*.003));
    out[i]=(a+(b-a)*this.phase)*this.fade;
    this.phase+=step;
    while(this.phase>=1&&this.queue.length){
     this.phase-=1;this.offset++;
     if(this.offset===this.queue[0].length){this.queue.shift();this.offset=0;this.consumed++;}
    }
   }
   if(this.playing&&!this.queue.length){
    this.playing=false;this.phase=0;
    if(!this.ended)this.underruns++;
   }
   this.energy+=Math.abs(out[i]);this.samples++;
   if(this.samples>=sampleRate*.02)this.report(false);
   if(this.ended&&!this.queue.length){this.finished=true;this.report(true);break;}
  }
  return !this.finished;
 }
 report(done){
  this.port.postMessage({consumed:this.consumed,level:done?0:Math.round(this.energy/Math.max(1,this.samples)*32768),underruns:this.underruns,done});
  this.consumed=0;this.energy=0;this.samples=0;this.underruns=0;
 }
}
registerProcessor('zumi-playback',ZumiPlayback);
