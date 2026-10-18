const {chromium}=require('playwright');
const assert=require('node:assert/strict');
(async()=>{
 const browser=await chromium.launch({headless:true,channel:'chrome',args:['--use-fake-ui-for-media-stream','--use-fake-device-for-media-stream']});
 try{
  for(const scenario of ['normal','denied','late']){
   const page=await browser.newPage();
   await page.addInitScript(scenario=>{
    window.micRequests=0;window.testStreams=[];window.inputFrames=0;
    const original=navigator.mediaDevices.getUserMedia.bind(navigator.mediaDevices);
    navigator.mediaDevices.getUserMedia=async options=>{
     window.micRequests++;
     if(scenario==='denied')throw new DOMException('Denied for test','NotAllowedError');
     if(scenario==='late')await new Promise(resolve=>window.releaseMic=resolve);
     const stream=await original(options);window.testStreams.push(stream);return stream;
    };
    class LocalSocket{
     static OPEN=1;
     constructor(){this.readyState=1;this.bufferedAmount=0;setTimeout(()=>this.onopen?.(),0);}
     send(raw){
      if(typeof raw!=='string'){inputFrames++;return;}
      const m=JSON.parse(raw);
      if(m.type==='hello')setTimeout(()=>this.onmessage?.({data:JSON.stringify({v:1,type:'ready',backend:'xiaozhi',audio:'pcm_s16le',rate:16000,frame_ms:20})}),0);
     }
     close(){this.readyState=3;}
    }
    window.WebSocket=LocalSocket;
   },scenario);
   await page.goto('http://127.0.0.1:8876/preview/?talk=xiaozhi');await page.waitForFunction(()=>window.pocket);
   assert.equal(await page.evaluate(()=>micRequests),0);
   await page.keyboard.press('Enter');await page.waitForFunction(()=>pocket.api.pocket_talk_state()===0);
   assert.equal(await page.evaluate(()=>micRequests),0);
   await page.keyboard.press('Enter');
   if(scenario==='denied'){
    await page.waitForFunction(()=>pocket.api.pocket_talk_state()===5);assert.equal(await page.evaluate(()=>inputFrames),0);
   }else{
    if(scenario==='normal')await page.waitForFunction(()=>inputFrames>3);
    else await page.waitForFunction(()=>!!window.releaseMic);
    await page.evaluate(()=>window.dispatchEvent(new Event('blur')));
    if(scenario==='late')await page.evaluate(()=>releaseMic());
    await page.waitForFunction(()=>testStreams.length===1&&testStreams[0].getTracks().every(t=>t.readyState==='ended'));
    const frames=await page.evaluate(()=>inputFrames);await page.waitForTimeout(150);assert.equal(await page.evaluate(()=>inputFrames),frames);
    assert.equal(await page.evaluate(()=>pocket.api.pocket_talk_state()),4);
   }
   await page.close();
  }
  console.log('Microphone safety: PASS (button-only, denied, blur cancellation, late permission cleanup); no cloud or real mic');
 }finally{await browser.close();}
})().catch(e=>{console.error(e.message);process.exit(1)});
