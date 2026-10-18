// Explicit opt-in live service test. Sends only a public synthetic fixture.
const {chromium}=require('playwright');
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
if(process.env.ZUMI_TEST_OFFICIAL!=='1')throw Error('Set ZUMI_TEST_OFFICIAL=1 only after binding and confirming free service');
(async()=>{
 const browser=await chromium.launch({headless:true,channel:'chrome',args:['--use-fake-ui-for-media-stream','--use-fake-device-for-media-stream',`--use-file-for-fake-audio-capture=${path.resolve('build/talk-mock/cloud-question.wav')}%noloop`]}),page=await browser.newPage({viewport:{width:1100,height:1000}}),errors=[];
 try{
  page.on('pageerror',e=>errors.push(e.message));
  await page.addInitScript(()=>{
   window.micRequests=0;window.testStreams=[];
   const original=navigator.mediaDevices.getUserMedia.bind(navigator.mediaDevices);
   navigator.mediaDevices.getUserMedia=async options=>{window.micRequests++;const stream=await original(options);window.testStreams.push(stream);return stream;};
  });
  await page.goto('http://127.0.0.1:8876/preview/?talk=xiaozhi');await page.waitForFunction(()=>window.pocket);
  await page.keyboard.press('Enter');await page.waitForFunction(()=>pocket.api.pocket_talk_state()===0);
  assert.equal(await page.evaluate(()=>micRequests),0);
  await page.keyboard.press('Enter');await page.waitForTimeout(4200);await page.keyboard.press('Enter');
  await page.waitForFunction(()=>pocket.talkClient.evidence.done===1,{},{timeout:70000});
  const evidence=await page.evaluate(()=>({...pocket.talkClient.evidence,micRequests,tracksStopped:testStreams.every(s=>s.getTracks().every(t=>t.readyState==='ended')),questionPresent:!!document.querySelector('#live-question').textContent,answerPresent:!!document.querySelector('#live-answer').textContent}));
  assert(evidence.sentFrames>50&&evidence.receivedFrames>0&&evidence.receivedFrames===evidence.playedFrames&&evidence.maxQueued<=12&&evidence.tracksStopped&&evidence.questionPresent&&evidence.answerPresent);
  assert.equal(evidence.micRequests,1);assert.deepEqual(errors,[]);
  // Keep only counts in the receipt, never private utterances or account identifiers.
  fs.writeFileSync('build/xiaozhi-live-browser.json',JSON.stringify({status:'PASS',input:'public synthetic greeting; no real microphone',evidence,errors},null,2));
  await page.screenshot({path:'build/xiaozhi-live-browser.png',fullPage:true});
  console.log('Official XiaoZhi desktop round trip: PASS',evidence);
 }finally{await browser.close();}
})().catch(error=>{console.error(error.message);process.exit(1)});
