import {captureMicrophone} from './talk_microphone.js';
import {createPlayback} from './talk_playback.js?v=20261019-envelope2';
// Default remains a synthetic local fixture. Official mode is explicitly selected.
export function createTalkClient(api,getAudio,stopGame){
 const live=new URLSearchParams(location.search).get('talk')==='xiaozhi';
 const E={OK:0,CANCEL:1,CONNECTED:2,DISCONNECTED:3,ASR:4,REPLY:5,AUDIO:6,DONE:7,FAILURE:8,OVERFLOW:9};
 let socket=null,generation=0,capture=null,microphone=null,turn=0,seq=0,rxSeq=0,playback=null,playbackReady=null,playbackRevision=0,queued=0,ended=false,committing=false,lastAudioAt=0;
 const evidence={connections:0,maxArrivalGapMs:0,sentFrames:0,receivedFrames:0,playedFrames:0,maxQueued:0,underruns:0,commits:0,cancels:0,done:0};
 let outputWindow=12,retryAt=0,retryDelay=2000;
 let check=null,lastCheckAt=0;
 function showCheck(force=true){
  const now=performance.now();if(!force&&now-lastCheckAt<200)return;lastCheckAt=now;
  if(!live||!check)return;
  const el=document.querySelector('#talk-check-current');
  if(el)el.textContent=`第 ${check.number} 轮 · ${check.phase}\n接收 / 播放：${check.received} / ${check.played} 帧\n缓冲耗尽：${check.underruns} 次\n最大到达间隔：${check.maxGap} ms · 最大队列：${check.maxQueued} 帧`;
  const audio=getAudio(),info=document.querySelector('#talk-check-audio');
  if(audio&&info)info.textContent=`输出：${audio.sampleRate} Hz · ${audio.state}\n浏览器报告延迟：处理 ${Math.round((audio.baseLatency||0)*1000)} ms / 输出 ${audio.outputLatency==null?'未知':Math.round(audio.outputLatency*1000)+' ms'}`;
  const stop=document.querySelector('#talk-check-stop');if(stop)stop.textContent=check.stop?`停止原因：${check.stop}`:'';
 }
 function checkStop(reason){if(check&&check.phase!=='完整播放'){check.stop=reason;showCheck();}}
 const enc=new TextEncoder();
 function connectionStatus(text){const el=document.querySelector('#live-connection');if(live&&el)el.textContent=text;}
 function event(e,t=api.pocket_talk_turn(),text=''){
  if(e===E.DISCONNECTED&&check&&!check.stop)checkStop('连接断开');
  if(live&&(e===E.ASR||e===E.REPLY)){
   const output=document.querySelector(e===E.ASR?'#live-question':'#live-answer');if(output)output.textContent=text;
   // The firmware font/wrapping is ASCII-only. Desktop Chinese stays in the transcript.
   if(/[^\x20-\x7e]/.test(text))text=e===E.ASR?'Question received.':'Answer shown below.';
  }
  const b=enc.encode(text);if(b.length>=1536){fail();return;}
  const dest=new Uint8Array(api.memory.buffer,api.pocket_talk_buffer(),1536);dest.fill(0);dest.set(b);api.pocket_talk_event(e,t);
 }
 function send(obj){if(socket?.readyState!==WebSocket.OPEN)throw Error('local relay offline');socket.send(JSON.stringify({v:1,...obj}));}
 function captureStatus(kind){const el=document.querySelector('#live-capture');if(el)el.textContent=({preparing:'麦克风准备中，请稍候…',ready:'可以说话了 · 再按 OK 发送',quiet:'声音较轻，请靠近麦克风',loud:'声音偏响，请稍微远离麦克风',sending:'录音已结束，正在发送…',off:'麦克风已关闭'})[kind]||'';}
 function stopCapture(){clearInterval(capture);capture=null;committing=false;microphone?.stop();microphone=null;if(live)captureStatus('off');}
 function stopPlayback(){playbackRevision++;playback?.stop();playback=null;playbackReady=null;queued=0;ended=false;api.pocket_talk_level(turn,0);}
 function preparePlayback(){
  const revision=playbackRevision,epoch=generation,activeTurn=turn,audio=getAudio();
  if(!audio||audio.state!=='running')throw Error('audio unavailable');
  playbackReady=createPlayback(audio,outputWindow,live,report=>{
   if(revision!==playbackRevision||epoch!==generation||activeTurn!==api.pocket_talk_turn())return;
   queued-=report.consumed;evidence.playedFrames+=report.consumed;evidence.underruns+=report.underruns;
   if(check){check.played+=report.consumed;check.underruns+=report.underruns;if(report.consumed)check.phase="播放中";}
   api.pocket_talk_level(activeTurn,report.level);
   try{
    if(report.consumed)send({type:'output.credit',turn:activeTurn,frames:report.consumed});
    if(report.done&&ended&&queued===0){ended=false;evidence.done++;if(check)check.phase="完整播放";event(E.DONE,activeTurn);}
    showCheck(report.done);
   }catch{fail();}
  },()=>{if(revision===playbackRevision&&epoch===generation)fail();}).then(player=>{
   if(revision!==playbackRevision||epoch!==generation){player.stop();return false;}
   playback=player;return true;
  });
  // Observe loading errors even if recording is cancelled before commit.
  playbackReady.catch(()=>{if(revision===playbackRevision&&epoch===generation)fail();});
 }
 function close(){connectionStatus('语音连接已关闭，返回 Talk 后自动连接');retryAt=performance.now()+retryDelay;generation++;stopCapture();stopPlayback();const old=socket;socket=null;if(old){old.onclose=null;old.onmessage=null;old.close();}}
 function fail(){if(check)check.phase="错误";checkStop("服务或音频错误");event(E.FAILURE);close();event(E.DISCONNECTED);}
 function incoming(data,epoch){
  if(epoch!==generation)return;
  try{
   if(typeof data==='string'){
    if(enc.encode(data).length>2048)throw Error('oversize JSON');const m=JSON.parse(data);if(m.v!==1)throw Error('version');
    if(m.type==='ready'){if(m.audio!=='pcm_s16le'||m.rate!==16000||m.frame_ms!==20||(live&&m.backend!=='xiaozhi'))throw Error('format');outputWindow=live?(m.output_window??12):12;if(!Number.isInteger(outputWindow)||outputWindow<12||outputWindow>36)throw Error('window');retryDelay=2000;evidence.connections++;connectionStatus('已连接小智 · 按 OK 开始说话');event(E.CONNECTED);return;}
    if(m.turn!==turn||turn!==api.pocket_talk_turn())return;
    if(m.type==='asr.final'||m.type==='answer.snapshot'){
     if(typeof m.text!=='string'||enc.encode(m.text).length>(m.type==='asr.final'?1023:1535))throw Error('text bounds');
     event(m.type==='asr.final'?E.ASR:E.REPLY,turn,m.text);
    }else if(m.type==='output.end'){if(m.last_seq!==rxSeq-1||!rxSeq)throw Error('missing output');ended=true;playback.end();}
    else if(m.type==='error')fail();else throw Error('unknown type');
    return;
   }
   if(data.byteLength<16||data.byteLength>654)throw Error('frame size');
   const v=new DataView(data),n=v.getUint16(12,true),t=v.getUint32(4,true);
   if(t!==turn||t!==api.pocket_talk_turn())return;
   if(v.getUint8(0)!==1||v.getUint8(1)!==2||v.getUint16(2,true)||v.getUint32(8,true)!==rxSeq||n%2||!n||n>640||data.byteLength!==n+14||ended)throw Error('invalid frame');
   if(queued>=outputWindow||!playback)throw Error('queue unavailable');
   const now=performance.now();if(lastAudioAt){const gap=Math.round(now-lastAudioAt);evidence.maxArrivalGapMs=Math.max(evidence.maxArrivalGapMs,gap);if(check)check.maxGap=Math.max(check.maxGap,gap);}lastAudioAt=now;
   const pcm=new Float32Array(n/2);
   for(let i=0;i<n/2;i++)pcm[i]=v.getInt16(14+i*2,true)/32768;
   queued++;rxSeq++;evidence.receivedFrames++;evidence.maxQueued=Math.max(evidence.maxQueued,queued);
   if(check){check.received++;check.maxQueued=Math.max(check.maxQueued,queued);}
   event(E.AUDIO,turn);playback.push(pcm);
  }catch{fail();}
 }
 function command(c){
  try{
   if(c===4){
    close();connectionStatus('正在连接小智…');const epoch=generation;
    const port=live?8882:Number(new URLSearchParams(location.search).get('talk_port')||8881);if(!Number.isInteger(port)||port<1||port>65535)throw Error('invalid local port');
    socket=new WebSocket(`ws://127.0.0.1:${port}`);socket.binaryType='arraybuffer';
    socket.onopen=()=>{if(epoch===generation)send({type:'hello',capabilities:['qa'],...(live?{output_window:36}:{})});};
    socket.onmessage=e=>incoming(e.data,epoch);
    socket.onclose=()=>{if(epoch===generation){close();retryDelay=Math.min(retryDelay*2,15000);connectionStatus('连接暂时中断，正在自动重连…');event(E.DISCONNECTED);}};
    socket.onerror=()=>{if(epoch===generation){close();retryDelay=Math.min(retryDelay*2,15000);connectionStatus('连接暂时中断，正在自动重连…');event(E.DISCONNECTED);}};
   }else if(c===1){
    stopGame();stopCapture();stopPlayback();turn=api.pocket_talk_turn();seq=rxSeq=0;lastAudioAt=0;check={number:(check?.number||0)+1,phase:"录音中",received:0,played:0,underruns:0,maxGap:0,maxQueued:0,stop:""};showCheck();preparePlayback();
    send({type:'input.begin',turn,capability:'qa',memory:'none',fixture:document.querySelector('#talk-fixture')?.value||'intro'});
    const epoch=generation;
    if(live){
     for(const id of ['live-question','live-answer']){const el=document.getElementById(id);if(el)el.textContent='';}
     microphone=captureMicrophone(pcm=>{try{
      if(epoch!==generation||api.pocket_talk_turn()!==turn||!(api.pocket_talk_state()===1||(committing&&api.pocket_talk_state()===2))){stopCapture();return;}
      if(socket.bufferedAmount>3200||pcm.byteLength!==640)throw Error('uplink stalled');
      const frame=new ArrayBuffer(654),v=new DataView(frame);v.setUint8(0,1);v.setUint8(1,1);v.setUint32(4,turn,true);v.setUint32(8,seq,true);v.setUint16(12,640,true);new Uint8Array(frame,14).set(new Uint8Array(pcm));socket.send(frame);seq++;evidence.sentFrames++;
     }catch{fail();}},()=>{if(epoch===generation){const el=document.querySelector('#live-answer');if(el)el.textContent='麦克风未能开启，请检查浏览器权限，再按 OK 重试。';fail();}},captureStatus);
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
    if(check){check.phase="等待回复";showCheck();}
    if(live){
     const epoch=generation,recordingTurn=turn,activeMic=microphone;committing=true;captureStatus('sending');
     (async()=>{try{
      const drained=await activeMic?.finish();
      const outputReady=await playbackReady;
      if(epoch!==generation||turn!==recordingTurn||api.pocket_talk_turn()!==recordingTurn||api.pocket_talk_state()!==2||!committing)return;
      committing=false;microphone=null;
      if(!drained||!outputReady||!seq)throw Error('empty or interrupted recording');
      send({type:'input.commit',turn,last_seq:seq-1});send({type:'output.credit',turn,frames:outputWindow});evidence.commits++;captureStatus('off');
     }catch{if(epoch===generation&&turn===recordingTurn)fail();}})();
     return;
    }
    stopCapture();if(!seq)throw Error('empty recording');
    // WebSocket preserves message order; bounded bufferedAmount prevents an utterance queue.
    const revision=playbackRevision,recordingTurn=turn;
    playbackReady.then(ready=>{if(!ready||revision!==playbackRevision||recordingTurn!==api.pocket_talk_turn())return;try{send({type:'input.commit',turn,last_seq:seq-1});send({type:'output.credit',turn,frames:outputWindow});evidence.commits++;}catch{fail();}}).catch(()=>{});
   }else if(c===3){
    if(check&&check.phase!=="完整播放"){check.phase="已中断";if(!check.stop)check.stop="按键打断或离开 Talk";showCheck();}
    evidence.cancels++;stopCapture();stopPlayback();
    if(socket?.readyState===WebSocket.OPEN)send({type:'cancel',turn});
    if(api.pocket_page()!==3||api.pocket_screen_off()||[4,5].includes(api.pocket_talk_state())){close();if(api.pocket_talk_state()!==4)event(E.DISCONNECTED);}
   }
  }catch{fail();}
 }
 return {pump(){const c=api.pocket_talk_command();if(c)command(c);else if(live&&!socket&&api.pocket_page()===3&&!api.pocket_screen_off()&&api.pocket_talk_state()===4&&!document.hidden&&document.hasFocus()&&performance.now()>=retryAt){event(E.OK);command(api.pocket_talk_command());}},resume(){retryAt=0;},cancel(reason="离开页面"){checkStop(reason);if(api.pocket_page()===3){event(E.CANCEL);command(api.pocket_talk_command());}close();event(E.DISCONNECTED);},evidence};
}
