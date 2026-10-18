const canvas=document.querySelector('canvas'),ctx=canvas.getContext('2d');
const status=document.querySelector('#status');
const sprites=await Promise.all([0,1,2].map(i=>new Promise((resolve,reject)=>{const im=new Image(); im.onload=()=>resolve(im);im.onerror=reject;im.src=`../assets/images/zumi-pocket/zumi-${i}.png`;})));
let api;
try {
 const response=await fetch('pocket.wasm'); if(!response.ok)throw Error('请先运行 tools/build_preview.sh');
 const bytes=await response.arrayBuffer();
 const {instance}=await WebAssembly.instantiate(bytes,{wasi_snapshot_preview1:{clock_time_get:()=>52,fd_write:()=>0,fd_close:()=>0,fd_seek:()=>0,proc_exit:code=>{throw Error(`WASM exit ${code}`);}}});
 api=instance.exports;api._initialize?.();api.pocket_init(0x5A554D49);
 status.textContent='本地预览 · 游戏逻辑与固件共享 · 无服务连接';
} catch(e) {status.textContent=`加载失败：${e.message}`;throw e;}
let audioContext, soundNode, soundGeneration=0;
function unlockAudio(){
 try {const Audio=window.AudioContext||window.webkitAudioContext;if(!Audio)return;
 audioContext ||= new Audio();if(audioContext.state==='suspended')audioContext.resume().catch(()=>{});}catch{}
}
function stopSound(){soundGeneration++;if(soundNode){try{soundNode.stop();}catch{}soundNode.disconnect();soundNode=null;}}
function playSound(event){
 if(!audioContext || document.hidden)return;
 stopSound();
 const frames=api.pocket_sound_frames(event),buffer=audioContext.createBuffer(1,frames,16000),samples=buffer.getChannelData(0);
 for(let frame=0;frame<frames;frame+=256){const ptr=api.pocket_sound_chunk(event,frame),pcm=new Int16Array(api.memory.buffer,ptr,256);
 for(let i=0;i<Math.min(256,frames-frame);i++)samples[frame+i]=pcm[i]/32768;}
 const node=audioContext.createBufferSource();node.buffer=buffer;node.connect(audioContext.destination);soundNode=node;
 node.onended=()=>{node.disconnect();if(soundNode===node)soundNode=null;};
 const generation=soundGeneration;
 audioContext.resume().then(()=>{if(generation===soundGeneration)node.start();}).catch(()=>stopSound());
}
const color=n=>'#'+n.toString(16).padStart(6,'0');
const decoder=new TextDecoder();
function render(){
 const sound=api.pocket_take_sound();
 if(api.pocket_page()!==2) {if(soundNode)stopSound();}
 else if(sound)playSound(sound);
 canvas.style.filter=api.pocket_dimmed()?'brightness(0.29)':'none';
 const n=api.pocket_render(),base=api.pocket_ops(),stride=api.pocket_stride();
 const data=new DataView(api.memory.buffer);
 ctx.save();ctx.beginPath();ctx.roundRect(0,0,240,320,30);ctx.clip();
 for(let i=0;i<n;i++){
  const p=base+i*stride,a=Array.from({length:9},(_,j)=>data.getInt32(p+j*4,true));
  const [kind,x,y,w,h,c,r,size,opacity]=a;ctx.fillStyle=color(c);ctx.globalAlpha=opacity/255;
  if(kind===0){ctx.beginPath();ctx.roundRect(x,y,w,h,r);ctx.fill();}
  else if(kind===1){const b=new Uint8Array(api.memory.buffer,p+36,64);const end=b.indexOf(0);const text=decoder.decode(b.subarray(0,end<0?64:end));ctx.font=`${size}px Arial`;ctx.textBaseline='top';ctx.fillText(text,x,y);}
  else ctx.drawImage(sprites[size%3],x,y,w,h);
 }
 ctx.restore();
}
let held=null,longTimer=null;
function press(key){if(held!==null)return;unlockAudio();held=key;api.pocket_button(key,0);document.querySelector(`[data-key="${key}"]`).classList.add('pressed');if(key===2)longTimer=setTimeout(()=>{api.pocket_button(2,3);render();},500);render();}
function release(cancel=false){if(held===null)return;clearTimeout(longTimer);const key=held;held=null;document.querySelector(`[data-key="${key}"]`).classList.remove('pressed');if(cancel&&key===2)api.pocket_button(2,3);api.pocket_button(key,4);render();}
for(const button of document.querySelectorAll('button')){
 button.addEventListener('pointerdown',e=>{e.preventDefault();button.setPointerCapture(e.pointerId);press(Number(button.dataset.key));});
 button.addEventListener('pointerup',()=>release());button.addEventListener('pointercancel',()=>release(true));
}
const keyMap={ArrowUp:0,ArrowDown:1,Enter:2,' ':2};
window.addEventListener('keydown',e=>{if(e.target.tagName==='BUTTON'&&!e.key.startsWith('Arrow'))return;if(e.key in keyMap){e.preventDefault();if(!e.repeat)press(keyMap[e.key]);}});
window.addEventListener('keyup',e=>{if(e.key in keyMap){e.preventDefault();if(held===keyMap[e.key])release();}});
// Keyboard activation of the physical button controls supports assistive input.
for(const b of document.querySelectorAll('button')){
 b.addEventListener('keydown',e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();if(!e.repeat)press(Number(b.dataset.key));}});
 b.addEventListener('keyup',e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();release();}});
}
window.addEventListener('blur',()=>{release(true);stopSound();});
let last=performance.now();
function frame(now){const dt=Math.floor(now-last);last+=dt;if(!document.hidden){api.pocket_tick(dt);render();}requestAnimationFrame(frame);}
document.addEventListener('visibilitychange',()=>{last=performance.now();if(document.hidden){release(true);stopSound();audioContext?.suspend().catch(()=>{});}});
window.pocket={api,render};render();requestAnimationFrame(frame);
