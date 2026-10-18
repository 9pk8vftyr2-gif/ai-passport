import {captureMicrophone} from './talk_microphone.js';
// Default remains a synthetic local fixture. Official mode is explicitly selected.
export function createTalkClient(api,getAudio,stopGame){
 const live=new URLSearchParams(location.search).get('talk')==='xiaozhi';
 const E={OK:0,CANCEL:1,CONNECTED:2,DISCONNECTED:3,ASR:4,REPLY:5,AUDIO:6,DONE:7,FAILURE:8,OVERFLOW:9};
 let socket=null,generation=0,capture=null,microphone=null,turn=0,seq=0,rxSeq=0,nodes=new Set(),ended=false,nextTime=0;
 const evidence={sentFrames:0,receivedFrames:0,playedFrames:0,maxQueued:0,underruns:0,commits:0,cancels:0,done:0};
 const enc=new TextEncoder();
 function event(e,t=api.pocket_talk_turn(),text=''){
  if(live&&(e===E.ASR||e===E.REPLY)){
   const output=document.querySelector(e===E.ASR?'#live-question':'#live-answer');if(output)output.textContent=text;
   // The firmware font/wrapping is ASCII-only. Desktop Chinese stays in the transcript.
   if(/[^\x20-\x7e]/.test(text))text=e===E.ASR?'Question received.':'Answer shown below.';
  }
  const b=enc.encode(text);if(b.length>=1536){fail();return;}
  const dest=new Uint8Array(api.memory.buffer,api.pocket_talk_buffer(),1536);dest.fill(0);dest.set(b);api.pocket_talk_event(e,t);
 }
 function send(obj){if(socket?.readyState!==WebSocket.OPEN)throw Error('local relay offline');socket.send(JSON.stringify({v:1,...obj}));}
 function stopCapture(){clearInterval(capture);capture=null;microphone?.stop();microphone=null;}
 function stopPlayback(){for(const n of nodes){n.onended=null;try{n.stop();}catch{}n.disconnect();}nodes.clear();nextTime=0;ended=false;}
 function close(){generation++;stopCapture();stopPlayback();const old=socket;socket=null;if(old){old.onclose=null;old.onmessage=null;old.close();}}
 function fail(){event(E.FAILURE);close();event(E.DISCONNECTED);}
 function finish(){if(ended&&!nodes.size){ended=false;evidence.done++;event(E.DONE,turn);}}
 function incoming(data,epoch){
  if(epoch!==generation)return;
  try{
   if(typeof data==='string'){
    if(enc.encode(data).length>2048)throw Error('oversize JSON');const m=JSON.parse(data);if(m.v!==1)throw Error('version');
    if(m.type==='ready'){if(m.audio!=='pcm_s16le'||m.rate!==16000||m.frame_ms!==20||(live&&m.backend!=='xiaozhi'))throw Error('format');event(E.CONNECTED);return;}
    if(m.turn!==turn||turn!==api.pocket_talk_turn())return;
    if(m.type==='asr.final'||m.type==='answer.snapshot'){
     if(typeof m.text!=='string'||enc.encode(m.text).length>(m.type==='asr.final'?1023:1535))throw Error('text bounds');
     event(m.type==='asr.final'?E.ASR:E.REPLY,turn,m.text);
    }else if(m.type==='output.end'){if(m.last_seq!==rxSeq-1||!rxSeq)throw Error('missing output');ended=true;finish();}
    else if(m.type==='error')fail();else throw Error('unknown type');
    return;
   }
   if(data.byteLength<16||data.byteLength>654)throw Error('frame size');
   const v=new DataView(data),n=v.getUint16(12,true),t=v.getUint32(4,true);
   if(t!==turn||t!==api.pocket_talk_turn())return;
   if(v.getUint8(0)!==1||v.getUint8(1)!==2||v.getUint16(2,true)||v.getUint32(8,true)!==rxSeq||n%2||!n||n>640||data.byteLength!==n+14||ended)throw Error('invalid frame');
   if(nodes.size>=12)throw Error('queue full');
   const audio=getAudio();if(!audio||audio.state!=='running')throw Error('audio unavailable');
   const buffer=audio.createBuffer(1,n/2,16000),pcm=buffer.getChannelData(0);
   for(let i=0;i<n/2;i++)pcm[i]=v.getInt16(14+i*2,true)/32768;
   const node=audio.createBufferSource();node.buffer=buffer;node.connect(audio.destination);
   if(!rxSeq)nextTime=audio.currentTime+.12;
   else if(nextTime<audio.currentTime)evidence.underruns++;
   nextTime=Math.max(nextTime,audio.currentTime);nodes.add(node);rxSeq++;evidence.receivedFrames++;
   evidence.maxQueued=Math.max(evidence.maxQueued,nodes.size);event(E.AUDIO,turn);
   node.onended=()=>{nodes.delete(node);node.disconnect();if(epoch!==generation)return;evidence.playedFrames++;try{send({type:'output.credit',turn,frames:1});finish();}catch{fail();}};
   node.start(nextTime);nextTime+=n/32000;
  }catch{fail();}
 }
 function command(c){
  try{
   if(c===4){
    close();const epoch=generation;
    const port=live?8882:Number(new URLSearchParams(location.search).get('talk_port')||8881);if(!Number.isInteger(port)||port<1||port>65535)throw Error('invalid local port');
    socket=new WebSocket(`ws://127.0.0.1:${port}`);socket.binaryType='arraybuffer';
    socket.onopen=()=>{if(epoch===generation)send({type:'hello',capabilities:['qa']});};
    socket.onmessage=e=>incoming(e.data,epoch);
    socket.onclose=()=>{if(epoch===generation){close();event(E.DISCONNECTED);}};
    socket.onerror=()=>{if(epoch===generation){close();event(E.DISCONNECTED);}};
   }else if(c===1){
    stopGame();stopCapture();stopPlayback();turn=api.pocket_talk_turn();seq=rxSeq=0;
    send({type:'input.begin',turn,capability:'qa',memory:'none',fixture:document.querySelector('#talk-fixture')?.value||'intro'});
    const epoch=generation;
    if(live){
     for(const id of ['live-question','live-answer']){const el=document.getElementById(id);if(el)el.textContent='';}
     microphone=captureMicrophone(pcm=>{try{
      if(epoch!==generation||api.pocket_talk_state()!==1){stopCapture();return;}
      if(socket.bufferedAmount>3200||pcm.byteLength!==640)throw Error('uplink stalled');
      const frame=new ArrayBuffer(654),v=new DataView(frame);v.setUint8(0,1);v.setUint8(1,1);v.setUint32(4,turn,true);v.setUint32(8,seq,true);v.setUint16(12,640,true);new Uint8Array(frame,14).set(new Uint8Array(pcm));socket.send(frame);seq++;evidence.sentFrames++;
     }catch{fail();}},()=>{if(epoch===generation){const el=document.querySelector('#live-answer');if(el)el.textContent='麦克风未能开启，请检查浏览器权限，再按 OK 重试。';fail();}});
     return;
    }
    capture=setInterval(()=>{try{
     if(epoch!==generation||api.pocket_talk_state()!==1){stopCapture();return;}
     if(socket.bufferedAmount>3200)throw Error('uplink stalled');
     const frame=new ArrayBuffer(654),v=new DataView(frame);v.setUint8(0,1);v.setUint8(1,1);v.setUint32(4,turn,true);v.setUint32(8,seq,true);v.setUint16(12,640,true);
     for(let i=0;i<320;i++)v.setInt16(14+i*2,Math.round(Math.sin((seq*320+i)*2*Math.PI*220/16000)*1000),true);
     socket.send(frame);seq++;evidence.sentFrames++;
    }catch{fail();}},20);
   }else if(c===2){
    stopCapture();if(!seq)throw Error('empty recording');
    // WebSocket preserves message order; bounded bufferedAmount prevents an utterance queue.
    send({type:'input.commit',turn,last_seq:seq-1});send({type:'output.credit',turn,frames:12});evidence.commits++;
   }else if(c===3){
    evidence.cancels++;stopCapture();stopPlayback();
    if(socket?.readyState===WebSocket.OPEN)send({type:'cancel',turn});
    if(api.pocket_page()!==3||api.pocket_screen_off()||api.pocket_talk_state()===5){close();event(E.DISCONNECTED);}
   }
  }catch{fail();}
 }
 return {pump(){const c=api.pocket_talk_command();if(c)command(c);},cancel(){if(api.pocket_page()===3){event(E.CANCEL);command(api.pocket_talk_command());}close();event(E.DISCONNECTED);},evidence};
}
