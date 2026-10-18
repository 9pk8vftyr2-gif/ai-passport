// Requested only by an explicit recording button action; late grants are stopped.
export function captureMicrophone(onFrame,onError){
 let stopped=false,stream=null,context=null,source=null,node=null,mute=null;
 function stop(){stopped=true;stream?.getTracks().forEach(t=>t.stop());if(node)node.port.onmessage=null;source?.disconnect();node?.disconnect();mute?.disconnect();context?.close().catch(()=>{});}
 (async()=>{
  try{
   stream=await navigator.mediaDevices.getUserMedia({audio:{channelCount:1,echoCancellation:true,noiseSuppression:true,autoGainControl:true},video:false});
   if(stopped){stop();return;}
   context=new AudioContext();
   await context.audioWorklet.addModule(new URL('./talk_mic_worklet.js',import.meta.url));
   if(stopped){stop();return;}
   source=context.createMediaStreamSource(stream);node=new AudioWorkletNode(context,'zumi-pcm');mute=context.createGain();mute.gain.value=0;
   node.port.onmessage=e=>{if(!stopped)onFrame(e.data);};
   source.connect(node);node.connect(mute);mute.connect(context.destination);await context.resume();
  }catch(error){if(!stopped){stop();onError(error.name);}}
 })();
 return {stop};
}
