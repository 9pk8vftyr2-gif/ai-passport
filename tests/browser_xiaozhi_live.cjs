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
  const record=async()=>{await page.keyboard.press('Enter');await page.waitForTimeout(4200);await page.keyboard.press('Enter');};
  for(let n=1;n<=2;n++){
   await record();await page.waitForFunction(n=>pocket.talkClient.evidence.done===n,n,{timeout:70000});
   assert(await page.evaluate(()=>pocket.talkClient.evidence.receivedFrames===pocket.talkClient.evidence.playedFrames));
  }
  await record();await page.waitForFunction(()=>pocket.api.pocket_talk_state()===3);
  const start=Date.now();await page.keyboard.press('Enter');
  assert.equal(await page.evaluate(()=>pocket.api.pocket_talk_state()),0);
  const interruptMs=Date.now()-start;
  const playedAfterCancel=await page.evaluate(()=>pocket.talkClient.evidence.playedFrames);
  await page.waitForTimeout(350);assert.equal(await page.evaluate(()=>pocket.talkClient.evidence.playedFrames),playedAfterCancel);
  await record();await page.waitForFunction(()=>pocket.talkClient.evidence.done===3,{},{timeout:70000});
  const evidence=await page.evaluate(()=>({...pocket.talkClient.evidence,micRequests,tracksStopped:testStreams.every(s=>s.getTracks().every(t=>t.readyState==='ended')),questionPresent:!!document.querySelector('#live-question').textContent,answerPresent:!!document.querySelector('#live-answer').textContent}));
  assert(evidence.sentFrames>50&&evidence.receivedFrames>0&&evidence.receivedFrames>=evidence.playedFrames&&evidence.maxQueued<=36&&evidence.tracksStopped&&evidence.questionPresent&&evidence.answerPresent);
  assert.equal(evidence.micRequests,4);assert.equal(evidence.done,3);assert.equal(evidence.cancels,1);assert.deepEqual(errors,[]);
  // Keep only counts in the receipt, never private utterances or account identifiers.
  fs.writeFileSync('build/xiaozhi-live-browser.json',JSON.stringify({status:'PASS',input:'public synthetic greeting; no real microphone',sequence:'two complete turns, interrupt, recovered complete turn',interruptMs,evidence,errors},null,2));
  await page.screenshot({path:'build/xiaozhi-live-browser.png',fullPage:true});
  console.log('Official XiaoZhi desktop round trip: PASS',evidence);
 }finally{await browser.close();}
})().catch(error=>{console.error(error.message);process.exit(1)});
