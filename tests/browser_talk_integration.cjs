const {chromium}=require('playwright');
const assert=require('node:assert/strict'),fs=require('node:fs');
(async()=>{
 const browser=await chromium.launch({headless:true,channel:'chrome'}),page=await browser.newPage({viewport:{width:1100,height:900}}),errors=[];
 page.on('pageerror',e=>errors.push(e.message));
 await page.goto('http://127.0.0.1:8876/preview/');await page.waitForFunction(()=>window.pocket);
 const state=()=>page.evaluate(()=>pocket.api.pocket_talk_state());
 const tap=()=>page.keyboard.press('Enter');
 const home=async()=>{await page.keyboard.down('Enter');await page.waitForTimeout(550);await page.keyboard.up('Enter');};
 await tap();await page.waitForFunction(()=>pocket.api.pocket_talk_state()===0);
 await tap();assert.equal(await state(),1);await page.waitForTimeout(600);await tap();
 await page.waitForFunction(()=>pocket.api.pocket_talk_state()===3);
 await page.locator('canvas').screenshot({path:'build/talk-integrated-speaking.png'});
 await page.waitForFunction(()=>pocket.api.pocket_talk_state()===0,{},{timeout:15000});
 let result=await page.evaluate(()=>pocket.talkClient.evidence);assert(result.sentFrames>10&&result.receivedFrames>50&&result.receivedFrames===result.playedFrames&&result.maxQueued<=12&&result.underruns===0&&result.done===1);
 // Interrupt drains immediately and late chunks cannot restart playback.
 await tap();await page.waitForTimeout(250);await tap();await page.waitForFunction(()=>pocket.api.pocket_talk_state()===3);await tap();
 assert.equal(await state(),0);await page.waitForTimeout(500);assert.equal(await state(),0);
 // Losing focus closes the session and exposes Offline, so reconnect never fails on a stale online flag.
 await page.evaluate(()=>window.dispatchEvent(new Event('blur')));
 assert.equal(await state(),4);await tap();await page.waitForFunction(()=>pocket.api.pocket_talk_state()===0);
 // A new turn after interruption still starts, and leaving aborts it.
 await tap();await page.waitForTimeout(250);await tap();await page.waitForFunction(()=>pocket.api.pocket_talk_state()===3);await home();
 assert.equal(await page.evaluate(()=>pocket.api.pocket_page()),0);
 // Offline game resumes unchanged after its wake card.
 await page.keyboard.press('ArrowDown');await tap();await tap();await tap();
 const saved=await page.evaluate(()=>[pocket.api.pocket_board(),pocket.api.pocket_moves(),pocket.api.pocket_cursor()]);
 await page.evaluate(()=>{for(let i=0;i<600;i++)pocket.api.pocket_tick(100);pocket.render();});
 assert.equal(await page.evaluate(()=>pocket.api.pocket_screen_off()),1);
 await tap();await page.waitForTimeout(3300);
 assert.deepEqual(await page.evaluate(()=>[pocket.api.pocket_board(),pocket.api.pocket_moves(),pocket.api.pocket_cursor()]),saved);
 assert.equal(await page.evaluate(()=>pocket.api.pocket_page()),2);
 await home();await tap();await page.waitForFunction(()=>pocket.api.pocket_talk_state()===0);
 await page.evaluate(()=>{for(let i=0;i<600;i++)pocket.api.pocket_tick(100);pocket.render();});
 assert.equal(await page.evaluate(()=>pocket.api.pocket_screen_off()),1);await tap();await page.waitForTimeout(3300);assert.equal(await page.evaluate(()=>pocket.api.pocket_page()),0);
 assert.deepEqual(errors,[]);result=await page.evaluate(()=>pocket.talkClient.evidence);
 fs.writeFileSync('build/talk-integration-browser.json',JSON.stringify({status:'PASS',evidence:result,errors},null,2));
 console.log('Integrated Talk: PASS (local WS, PCM playback, interruption, game + wake regression)',result);
 await browser.close();
})().catch(e=>{console.error(e);process.exit(1)});
