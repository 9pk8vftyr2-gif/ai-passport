// Bounded streaming downsampler: one 20 ms PCM16 frame, no utterance storage.
class TalkPCM extends AudioWorkletProcessor {
 constructor(){super();this.ratio=sampleRate/16000;this.phase=0;this.sum=0;this.frame=new Int16Array(320);this.index=0;this.finished=false;
  this.port.onmessage=e=>{if(e.data==='finish'&&!this.finished){this.finished=true;
   if(this.phase>0)this.frame[this.index++]=Math.round(Math.max(-1,Math.min(1,this.sum/this.phase))*32767);
   if(this.index)this.port.postMessage(this.frame.buffer,[this.frame.buffer]);
   this.port.postMessage({finished:true});
  }};
 }
 process(inputs){
  if(this.finished)return false;
  const channel=inputs[0]?.[0];if(!channel)return true;
  for(const sample of channel){
   let left=1;
   while(left>1e-8){
    const weight=Math.min(left,this.ratio-this.phase);this.sum+=sample*weight;this.phase+=weight;left-=weight;
    if(this.phase>=this.ratio-1e-8){
     this.frame[this.index++]=Math.round(Math.max(-1,Math.min(1,this.sum/this.ratio))*32767);this.sum=0;this.phase=0;
     if(this.index===320){this.port.postMessage(this.frame.buffer,[this.frame.buffer]);this.frame=new Int16Array(320);this.index=0;}
    }
   }
  }
  return true;
 }
}
registerProcessor('zumi-pcm',TalkPCM);
