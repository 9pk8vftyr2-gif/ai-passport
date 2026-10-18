const $=s=>document.querySelector(s);
const E={OK:0,CANCEL:1,CONNECTED:2,DISCONNECTED:3,ASR:4,REPLY:5,AUDIO:6,DONE:7,FAILURE:8,OVERFLOW:9};
const names=['Idle','Listening','Processing','Speaking','Offline','Error'];
const {instance}=await WebAssembly.instantiate(await (await fetch('talk.wasm')).arrayBuffer(),{wasi_snapshot_preview1:{}});
const a=instance.exports;a._initialize?.();a.talk_init();
const encoder=new TextEncoder(),decoder=new TextDecoder();
let timers=[],online=true,home=false,scroll=0,lastState=-1,last=performance.now(),held=null;
function clearTimers(){for(const id of timers)clearTimeout(id);timers=[];}
function later(ms,fn){timers.push(setTimeout(fn,ms));}
function read(ptr,cap){const b=new Uint8Array(a.memory.buffer,ptr,cap),end=b.indexOf(0);return decoder.decode(b.subarray(0,end<0?cap:end));}
function emit(event,turn=a.talk_turn(),text){
 if(text!==undefined){const b=new Uint8Array(a.memory.buffer,a.talk_buffer(),1536);b.fill(0);b.set(encoder.encode(text).subarray(0,1535));a.talk_text(event,turn);}
 else a.talk_event(event,turn);
 effect();render();
}
function effect(){
 const f=a.talk_effect(),turn=a.talk_turn();
 if(f===1||f===3)clearTimers();
 if(f===2&&$('#auto').checked){
  later(700,()=>emit(E.ASR,turn,'What can solar power do?'));
  later(1400,()=>emit(E.REPLY,turn,'Sunlight can power a home. Solar panels turn light into electricity.'));
  later(2000,()=>emit(E.AUDIO,turn));
  for(let t=2500;t<6500;t+=500)later(t,()=>emit(E.AUDIO,turn));
  later(6500,()=>emit(E.DONE,turn));
 }
 if(f===4&&online)later(500,()=>emit(E.CONNECTED));
}
function render(){
 const s=a.talk_state();if(s!==lastState){scroll=0;lastState=s;}
 $('#screen').dataset.state=String(s);$('#state-label').textContent=home?'Home':names[s];
 $('#network').textContent=online?'ONLINE':'OFFLINE';
 $('.portrait img').src=`../../assets/images/zumi-pocket/zumi-${s===1||s===2?1:s===3?2:0}.png`;
 const errors=['','Response timed out.','Connection lost.','Please try again.','Service unavailable.','Audio buffer full.'];
 const asr=read(a.talk_asr(),1024),reply=read(a.talk_reply(),1536);
 const text=home?'Talk     Play     Garden':s===0?(reply||'Ready when you are.'):s===1?'Press OK when finished.':s===2?(asr||'Sending your question...'):s===3?(reply||'Zumi is answering.'):s===4?(a.talk_connecting()?'Trying to reconnect...':'Connect to start talking.'):(errors[a.talk_error()]||'Please try again.');
 $('#message').textContent=text;
 $('#caption').textContent=home?'BACK TO HOME':s===2?'YOU SAID':s===3||reply?'ZUMI SAYS':s===1?'LISTENING TO YOU':s===5?'TRY AGAIN':'YOUR MOMENT';
 $('#indicator').textContent=home?'MIC OFF':s===1?`REC  ${String(Math.floor(a.talk_elapsed()/1000)).padStart(2,'0')} / 15s`:s===3?'VOICE PLAYBACK':'MIC OFF';
 $('#cta').textContent=home?'OK · Open Talk':['OK · Start talking','OK · Finish & send','OK · Cancel','OK · Stop speaking',a.talk_connecting()?'Connecting...':'OK · Reconnect','OK · Try again'][s];
 $('#message').scrollTop=scroll*18;
 $('#status').textContent=home?'已取消本轮对话；现有首页与小游戏见左侧返回链接。':`状态模拟：${names[s]} · 实际麦克风与扬声器未启用`;
 $('#network-toggle').textContent=online?'模拟断网':'恢复网络';
}
function key(key){
 if(key==='ok'){if(home){home=false;emit(online?E.CONNECTED:E.DISCONNECTED);}else emit(E.OK);}
 else {scroll=Math.max(0,scroll+(key==='down'?1:-1));render();}
}
function down(key){if(held)return;held={key,long:false};if(key==='ok')held.timer=setTimeout(()=>{if(held){held.long=true;home=true;emit(E.CANCEL);location.assign("../");}},500);}
function up(){if(!held)return;const h=held;held=null;clearTimeout(h.timer);if(!h.long)key(h.key);}
for(const b of document.querySelectorAll('[data-key]')){
 b.addEventListener('pointerdown',e=>{b.setPointerCapture(e.pointerId);down(b.dataset.key);});b.addEventListener('pointerup',up);
 b.addEventListener('pointercancel',()=>{if(held)clearTimeout(held.timer);held=null;});
}
addEventListener('keydown',e=>{const key={Enter:'ok',ArrowUp:'up',ArrowDown:'down'}[e.key];if(key&&!e.repeat&&e.target.tagName!=='INPUT'){e.preventDefault();down(key);}});
addEventListener('keyup',e=>{if(['Enter','ArrowUp','ArrowDown'].includes(e.key)){e.preventDefault();up();}});
addEventListener('blur',()=>{if(held)clearTimeout(held.timer);held=null;});
$('#network-toggle').onclick=()=>{online=!online;clearTimers();emit(online?E.CONNECTED:E.DISCONNECTED);};
$('#failure').onclick=()=>emit(E.FAILURE);
$('#timeout').onclick=()=>{clearTimers();if(a.talk_state()===0){emit(E.OK);emit(E.OK);}a.talk_tick(20000);effect();render();};
$('#open').onclick=()=>{clearTimers();home=false;emit(E.CANCEL);emit(online?E.CONNECTED:E.DISCONNECTED);};
emit(E.CONNECTED); // Mock readiness only; no socket, microphone or TTS provider.
setInterval(()=>{const now=performance.now();a.talk_tick(Math.floor(now-last));last=now;effect();render();},100);
window.talk={api:a,event:emit,key,names,read,reset(){clearTimers();a.talk_init();home=false;online=true;emit(E.CONNECTED);}};
