// Offline jitter/reconnect tests with fake media; no real microphone/cloud.
const {chromium}=require('playwright'),assert=require('node:assert/strict'),fs=require('node:fs');
(async()=>{
 const browser=await chromium.launch({headless:true,channel:'chrome',args:['--use-fake-ui-for-media-stream','--use-fake-device-for-media-stream']});
 try{
  const page=await browser.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
  await page.addInitScript(()=>{
   window.micRequests=0;window.localSockets=[];
   const original=navigator.mediaDevices.getUserMedia.bind(navigator.mediaDevices);
   navigator.mediaDevices.getUserMedia=opts=>{micRequests++;return original(opts);};
   class LocalSocket{
    static OPEN=1;
    constructor(){this.readyState=1;this.bufferedAmount=0;this.seq=0;this.credit=0;localSockets.push(this);setTimeout(()=>this.onopen?.(),0);}
    message(m){this.onmessage?.({data:JSON.stringify({v:1,...m})});}
    send(raw){
     if(typeof raw!=='string')return;
     const m=JSON.parse(raw);
     if(m.type==='hello')setTimeout(()=>this.message({type:'ready',backend:'xiaozhi',audio:'pcm_s16le',rate:16000,frame_ms:20,output_window:36}),0);
     if(m.type==='input.begin'){this.turn=m.turn;this.seq=0;this.credit=0;this.active=false;}
     if(m.type==='input.commit')this.active=true;
     if(m.type==='output.credit'){this.credit+=m.frames;this.drain();}
     if(m.type==='cancel'){this.active=false;clearTimeout(this.timer);}
    }
    drain(){
     if(this.paused||!this.active||this.readyState!==1)return;
     while(this.credit&&this.seq<240){
      this.credit--;const frame=new ArrayBuffer(654),v=new DataView(frame);v.setUint8(0,1);v.setUint8(1,2);v.setUint32(4,this.turn,true);v.setUint32(8,this.seq,true);v.setUint16(12,640,true);
      for(let i=0;i<320;i++)v.setInt16(14+2*i,Math.sin((this.seq*320+i)*2*Math.PI*220/16000)*1000,true);
      this.seq++;this.onmessage?.({data:frame});
      if(this.seq%36===0&&this.seq<240){this.paused=true;this.timer=setTimeout(()=>{this.paused=false;this.drain();},250);return;}
     }
     if(this.seq===240){this.active=false;this.message({type:'output.end',turn:this.turn,last_seq:239});}
    }
    close(){this.readyState=3;clearTimeout(this.timer);}
    disconnect(){this.close();this.onclose?.();}
   }
   window.WebSocket=LocalSocket;
  });
  await page.goto('http://127.0.0.1:8876/preview/?talk=xiaozhi');await page.waitForFunction(()=>window.pocket);
  await page.keyboard.press('Enter');await page.waitForFunction(()=>pocket.api.pocket_talk_state()===0);
  assert.equal(await page.evaluate(()=>micRequests),0);
  await page.keyboard.press('Enter');await page.waitForFunction(()=>pocket.talkClient.evidence.sentFrames>5);await page.keyboard.press('Enter');
  await page.waitForFunction(()=>pocket.api.pocket_talk_state()===3,{},{timeout:8000}).catch(async e=>{console.log(await page.evaluate(()=>({state:pocket.api.pocket_talk_state(),evidence:pocket.talkClient.evidence,mic:document.querySelector('#live-capture').textContent,answer:document.querySelector('#live-answer').textContent,sockets:localSockets.map(s=>({seq:s.seq,credit:s.credit,active:s.active,ready:s.readyState}))})));throw e;});
  // Audio already scheduled ahead must tolerate a busy UI thread.
  await page.evaluate(()=>{const until=performance.now()+180;while(performance.now()<until){}});
  await page.waitForFunction(()=>pocket.talkClient.evidence.done===1);
  const playback=await page.evaluate(()=>({...pocket.talkClient.evidence}));
  assert.equal(playback.receivedFrames,240);assert.equal(playback.playedFrames,240);assert.equal(playback.underruns,0);assert(playback.maxQueued<=36);
  await page.evaluate(()=>localSockets.at(-1).disconnect());
  await page.waitForFunction(()=>pocket.talkClient.evidence.connections===2,{},{timeout:6000});
  assert.equal(await page.evaluate(()=>micRequests),1); // Reconnect never opens a microphone.
  await page.evaluate(()=>window.dispatchEvent(new Event('blur')));
  assert.equal(await page.evaluate(()=>pocket.api.pocket_talk_state()),4);
  await page.evaluate(()=>window.dispatchEvent(new Event('focus')));
  await page.waitForFunction(()=>pocket.talkClient.evidence.connections===3);
  assert.equal(await page.evaluate(()=>micRequests),1);
  assert.deepEqual(errors,[]);
  fs.writeFileSync('build/talk-resilience.json',JSON.stringify({status:'PASS',input:'fake media; no cloud',jitterMs:250,uiStallMs:180,playback,automaticReconnects:2,microphoneReopened:false},null,2));
  console.log('Talk resilience: PASS',playback);
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1)});
