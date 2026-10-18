const {chromium}=require('playwright');
const assert=require('node:assert/strict');
const fs=require('node:fs');
(async()=>{
 const browser=await chromium.launch({headless:true,channel:process.env.PLAYWRIGHT_CHANNEL||'chrome'});
 const page=await browser.newPage({viewport:{width:1440,height:1000}});
 const errors=[];page.on('pageerror',e=>errors.push(e.message));
 await page.goto(process.env.ZUMI_PREVIEW_URL||'http://127.0.0.1:8876/preview/');
 await page.waitForFunction(()=>window.pocket);
 const state=()=>page.evaluate(()=>({page:pocket.api.pocket_page(),score:pocket.api.pocket_score(),lives:pocket.api.pocket_lives()}));
 const tap=key=>page.keyboard.press(key);
 const home=async()=>{await page.keyboard.down('Enter');await page.waitForTimeout(550);await page.keyboard.up('Enter');assert.equal((await state()).page,0);};
 await page.screenshot({path:'build/browser-home.png',fullPage:true});
 await tap('Enter');assert.equal((await state()).page,4);await tap('Enter');
 await tap('ArrowDown');await tap('ArrowDown');await tap('Enter');assert.equal((await state()).page,5);await home();
 await tap('ArrowDown');await tap('Enter');assert.equal((await state()).page,1);
 await tap('Enter');assert.equal((await state()).page,2);
 await page.evaluate(()=>{const a=pocket.api;while(a.pocket_lane()>a.pocket_target())a.pocket_button(0,0);while(a.pocket_lane()<a.pocket_target())a.pocket_button(1,0);});
 await tap('Enter');assert.equal((await state()).score,10);
 await page.screenshot({path:'build/browser-carbon.png'});
 await home();await tap('ArrowDown');await tap('Enter');await tap('ArrowDown');await tap('Enter');assert.equal((await state()).page,3);
 await page.waitForTimeout(1400);await tap('Enter');await page.screenshot({path:'build/browser-journey.png'});
 // Running modules, local sprites and games remain usable without networking.
 await page.context().setOffline(true);await home();await tap('ArrowDown');await tap('Enter');await tap('Enter');assert.equal((await state()).page,2);
 await page.evaluate(()=>{for(let i=0;i<60;i++)pocket.api.pocket_tick(100);pocket.render();});assert.equal((await state()).page,6);
 await tap('Enter');assert.equal((await state()).page,2);await home();
 await page.setViewportSize({width:390,height:844});
 assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);
 // Mouse/physical-button surface, no screen taps.
 await page.getByRole('button',{name:'DOWN',exact:true}).click();
 await page.getByRole('button',{name:'OK',exact:true}).click();assert.equal((await state()).page,1);
 await page.screenshot({path:'build/browser-mobile.png',fullPage:true});
 assert.deepEqual(errors,[]);
 fs.writeFileSync('build/browser-results.json',JSON.stringify({status:'PASS',browser:'Chromium',checks:['home','Talk stub','Garden stub','Carbon score','long-press release','Journey jump','offline gameplay','loss and replay','mobile overflow','physical buttons'],pageErrors:errors},null,2));
 await browser.close();console.log('Browser Demo: PASS (10 checks, desktop + mobile, offline, no page errors)');
})().catch(e=>{console.error(e);process.exit(1)});
