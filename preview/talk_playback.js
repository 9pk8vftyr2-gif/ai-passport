const modules=new WeakMap();
export async function createPlayback(context,window,live,onReport,onError){
 if(!modules.has(context))modules.set(context,context.audioWorklet.addModule(new URL('./talk_playback_worklet.js?v=20261019-envelope2',import.meta.url)).catch(e=>{modules.delete(context);throw e;}));
 await modules.get(context);
 const node=new AudioWorkletNode(context,'zumi-playback',{numberOfInputs:0,numberOfOutputs:1,outputChannelCount:[1],processorOptions:{window,prebuffer:Math.min(window,live?15:6)}});
 node.port.onmessage=({data})=>{if(data.error)onError();else onReport(data);};
 node.onprocessorerror=onError;
 node.connect(context.destination);
 return {push(pcm){node.port.postMessage({pcm},[pcm.buffer]);},end(){node.port.postMessage({end:true});},stop(){node.port.postMessage({stop:true});node.port.onmessage=null;node.onprocessorerror=null;node.disconnect();node.port.close();}};
}
