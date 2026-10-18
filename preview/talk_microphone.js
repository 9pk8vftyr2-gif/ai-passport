// Requested only by an explicit recording button action; late grants are stopped.
export function captureMicrophone(onFrame,onError,onStatus=()=>{}){
 let stopped=false,finishing=false,stream=null,context=null,source=null,node=null,mute=null,highpass=null,lowpass=null,resolveFinish=null,finishTimer=null;
 let frames=0,energy=0,peak=0;
 function stop(){stopped=true;clearTimeout(finishTimer);stream?.getTracks().forEach(t=>t.stop());if(node)node.port.onmessage=null;source?.disconnect();highpass?.disconnect();lowpass?.disconnect();node?.disconnect();mute?.disconnect();context?.close().catch(()=>{});resolveFinish?.(false);resolveFinish=null;}
 function finish(){
  if(stopped)return Promise.resolve(false);
  if(!node){stop();return Promise.resolve(false);}
  if(finishing)return Promise.resolve(false);
  finishing=true;stream.getTracks().forEach(t=>t.stop());
  return new Promise(resolve=>{resolveFinish=resolve;finishTimer=setTimeout(()=>stop(),500);node.port.postMessage('finish');});
 }
 onStatus('preparing');
 (async()=>{
  try{
   stream=await navigator.mediaDevices.getUserMedia({audio:{channelCount:1,echoCancellation:true,noiseSuppression:true,autoGainControl:true},video:false});
   if(stopped){stop();return;}
   // Let the browser resample to 16 kHz where supported; retain the worklet fallback.
   try{context=new AudioContext({sampleRate:16000});}catch{context=new AudioContext();}
   await context.audioWorklet.addModule(new URL('./talk_mic_worklet.js',import.meta.url));
   if(stopped){stop();return;}
   source=context.createMediaStreamSource(stream);node=new AudioWorkletNode(context,'zumi-pcm');mute=context.createGain();mute.gain.value=0;
   highpass=context.createBiquadFilter();highpass.type='highpass';highpass.frequency.value=70;
   lowpass=context.createBiquadFilter();lowpass.type='lowpass';lowpass.frequency.value=7000;
   node.port.onmessage=e=>{
    if(stopped)return;
    if(e.data?.finished){const resolve=resolveFinish;resolveFinish=null;stop();resolve?.(true);return;}
    if(!(e.data instanceof ArrayBuffer))return;
    const samples=new Int16Array(e.data);for(const sample of samples){const x=sample/32768;energy+=x*x;peak=Math.max(peak,Math.abs(x));}
    onFrame(e.data);frames++;
    if(frames%10===0){const rms=Math.sqrt(energy/3200);onStatus(peak>.98?'loud':rms<.006?'quiet':'ready');energy=peak=0;}
   };
   source.connect(highpass);highpass.connect(lowpass);lowpass.connect(node);node.connect(mute);mute.connect(context.destination);await context.resume();
   if(!stopped)onStatus('ready');
  }catch(error){if(!stopped){stop();onError(error.name);}}
 })();
 return {stop,finish};
}
