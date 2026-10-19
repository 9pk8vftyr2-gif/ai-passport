const {chromium}=require('playwright'),assert=require('node:assert/strict'),fs=require('node:fs');
(async()=>{
 const browser=await chromium.launch({headless:true,channel:'chrome'});
 try{
  const page=await browser.newPage({viewport:{width:1100,height:900}});
  await page.goto('http://127.0.0.1:8876/preview/');await page.waitForFunction(()=>window.pocket);
  await page.keyboard.press('Enter');await page.waitForFunction(()=>pocket.api.pocket_talk_state()===0);
  await page.locator('canvas').screenshot({path:'build/talk-mic-centered.png'});
  await page.keyboard.press('Enter');await page.waitForTimeout(400);await page.keyboard.press('Enter');
  await page.waitForFunction(()=>pocket.api.pocket_talk_state()===3);
  const seen=await page.evaluate(async()=>{
   const values=new Set();const end=performance.now()+700;
   while(performance.now()<end){pocket.render();const v=new DataView(pocket.api.memory.buffer),b=pocket.api.pocket_ops(),stride=pocket.api.pocket_stride(),n=pocket.api.pocket_render();for(let i=0;i<n;i++)if(v.getInt32(b+i*stride,true)===2)values.add(v.getInt32(b+i*stride+28,true));await new Promise(r=>setTimeout(r,20));}
   return [...values].sort();
  });
  assert.deepEqual(seen,[0,3]);
  await page.locator('canvas').screenshot({path:'build/talk-speaking-animation.png'});
  await page.keyboard.press('Enter');assert.equal(await page.evaluate(()=>pocket.api.pocket_talk_state()),0);
  const closed=await page.evaluate(()=>{pocket.render();const v=new DataView(pocket.api.memory.buffer),b=pocket.api.pocket_ops(),stride=pocket.api.pocket_stride(),n=pocket.api.pocket_render();for(let i=0;i<n;i++)if(v.getInt32(b+i*stride,true)===2)return v.getInt32(b+i*stride+28,true);});
  assert.equal(closed,0);
  fs.writeFileSync('build/talk-animation.json',JSON.stringify({status:'PASS',speakingFrames:seen,cancelRestoresOriginal:true,realMicrophone:false},null,2));
  console.log('Talk animation: PASS (both mouth frames visible, cancel restores original)');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1)});
