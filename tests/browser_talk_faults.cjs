const {chromium}=require('playwright');
const {spawn}=require('node:child_process');
const assert=require('node:assert/strict');
(async()=>{
 const browser=await chromium.launch({headless:true,channel:'chrome'});
 try{
  for(const [i,fault] of ['error','bad-frame','timeout','disconnect'].entries()){
   const port=18881+i,server=spawn('build/talk-venv/bin/python',['tools/talk_mock_server.py','--port',String(port),'--fault',fault]);
   try{
    await new Promise((resolve,reject)=>{server.stdout.once('data',resolve);server.once('error',reject);server.once('exit',code=>reject(Error(`server exit ${code}`)));});
    const page=await browser.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
    await page.goto(`http://127.0.0.1:8876/preview/?talk_port=${port}`);await page.waitForFunction(()=>window.pocket);
    await page.keyboard.press('Enter');await page.waitForFunction(()=>pocket.api.pocket_talk_state()===0);
    await page.keyboard.press('Enter');await page.waitForTimeout(150);await page.keyboard.press('Enter');
    if(fault==='timeout')await page.evaluate(()=>{for(let i=0;i<201;i++)pocket.api.pocket_tick(100);pocket.render();});
    await page.waitForFunction(s=>pocket.api.pocket_talk_state()===s,fault==='disconnect'?4:5);
    const received=await page.evaluate(()=>pocket.talkClient.evidence.receivedFrames);
    await page.waitForTimeout(250);assert.equal(await page.evaluate(()=>pocket.talkClient.evidence.receivedFrames),received);
    if(fault!=='disconnect')await page.keyboard.press('Enter');
    assert.equal(await page.evaluate(()=>pocket.api.pocket_talk_state()),4);
    await page.keyboard.press('Enter');await page.waitForFunction(()=>pocket.api.pocket_talk_state()===0);
    assert.deepEqual(errors,[]);await page.close();console.log(`Talk fault recovery: PASS (${fault})`);
   }finally{server.kill('SIGTERM');}
  }
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1)});
