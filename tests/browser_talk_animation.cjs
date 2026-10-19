const {chromium}=require('playwright'),assert=require('node:assert/strict'),fs=require('node:fs');
(async()=>{
 const browser=await chromium.launch({headless:true,channel:'chrome'});
 try{
  const page=await browser.newPage({viewport:{width:1100,height:900}}),errors=[];
  page.on('pageerror',e=>errors.push(e.message));
  await page.addInitScript(()=>{
   class LocalSocket{
    static OPEN=1;
    constructor(){this.readyState=1;this.bufferedAmount=0;setTimeout(()=>this.onopen?.(),0);}
    send(raw){if(typeof raw!=='string')return;const m=JSON.parse(raw);
     if(m.type==='hello')this.message({type:'ready',audio:'pcm_s16le',rate:16000,frame_ms:20});
     if(m.type==='input.begin'){this.turn=m.turn;this.seq=0;this.active=true;}
     if(m.type==='cancel')this.active=false;
     if(m.type==='output.credit'&&this.active){
      for(let j=0;j<m.frames&&this.seq<120;j++){
       const frame=new ArrayBuffer(654),v=new DataView(frame);v.setUint8(0,1);v.setUint8(1,2);v.setUint32(4,this.turn,true);v.setUint32(8,this.seq,true);v.setUint16(12,640,true);
       for(let i=0;i<320;i++)v.setInt16(14+2*i,this.seq>=35&&this.seq<75?0:Math.sin((this.seq*320+i)*2*Math.PI*220/16000)*6000,true);
       this.seq++;this.onmessage?.({data:frame});
      }
      if(this.seq===120){this.active=false;this.message({type:'output.end',turn:this.turn,last_seq:119});}
     }
    }
    message(m){this.onmessage?.({data:JSON.stringify({v:1,...m})});}
    close(){this.readyState=3;}
   }
   window.WebSocket=LocalSocket;
  });
  await page.goto('http://127.0.0.1:8876/preview/');await page.waitForFunction(()=>window.pocket);
  await page.keyboard.press('Enter');await page.waitForFunction(()=>pocket.api.pocket_talk_state()===0);
  await page.locator('canvas').screenshot({path:'build/talk-mic-centered.png'});
  const portrait=()=>page.evaluate(()=>{const api=pocket.api,n=api.pocket_render(),v=new DataView(api.memory.buffer),b=api.pocket_ops(),stride=api.pocket_stride();for(let i=0;i<n;i++)if(v.getInt32(b+i*stride,true)===2)return v.getInt32(b+i*stride+28,true);});
  await page.keyboard.press('Enter');await page.waitForTimeout(300);await page.keyboard.press('Enter');
  await page.waitForFunction(()=>pocket.talkClient.evidence.playedFrames>=10);assert.equal(await portrait(),0);
  await page.locator('canvas').screenshot({path:'build/talk-envelope-speaking.png'});
  await page.waitForFunction(()=>pocket.talkClient.evidence.playedFrames>=42);assert.equal(await portrait(),3);
  await page.waitForTimeout(350);assert.equal(await portrait(),3); // Silent across >2 old 140 ms cycles.
  await page.locator('canvas').screenshot({path:'build/talk-envelope-silent.png'});
  await page.waitForFunction(()=>pocket.talkClient.evidence.playedFrames>=85);assert.equal(await portrait(),0);
  await page.keyboard.press('Enter');assert.equal(await page.evaluate(()=>pocket.api.pocket_talk_state()),0);
  const after=await page.evaluate(()=>pocket.talkClient.evidence.playedFrames);await page.waitForTimeout(200);
  assert.equal(await page.evaluate(()=>pocket.talkClient.evidence.playedFrames),after);assert.equal(await portrait(),0);
  assert.deepEqual(errors,[]);
  fs.writeFileSync('build/talk-animation.json',JSON.stringify({status:'PASS',input:'synthetic tone / 800 ms silence / tone; no real microphone',mouthFrames:[0,3,0],silentObservationMs:350,cancelStopsOutput:true,errors},null,2));
  console.log('Talk animation: PASS (audio-driven open / silent closed / open, cancel stops output)');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1)});
