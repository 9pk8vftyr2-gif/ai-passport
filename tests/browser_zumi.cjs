const {chromium}=require('playwright');
const assert=require('node:assert/strict'),fs=require('node:fs'),cp=require('node:child_process');
cp.execFileSync(process.env.CC||'cc',['-std=c11','-O2','-Wall','-Wextra','-Werror','-Imain/zumi','tests/zumi_difficulty.c','main/zumi/core.c','main/zumi/talk_model.c','-o','build/zumi_difficulty']);
const levels=JSON.parse(cp.execFileSync('build/zumi_difficulty',{encoding:'utf8'}));
fs.writeFileSync('build/zumi-difficulty.json',JSON.stringify(levels,null,2));
(async()=>{
 const browser=await chromium.launch({headless:true,channel:process.env.PLAYWRIGHT_CHANNEL||'chrome'});
 const page=await browser.newPage({viewport:{width:1440,height:1000}}),errors=[];
 await page.addInitScript(()=>{
  window.soundEvidence=[];
  const create=AudioContext.prototype.createBufferSource;
  AudioContext.prototype.createBufferSource=function(){const node=create.call(this),start=node.start.bind(node),stop=node.stop.bind(node);
   node.start=(...args)=>{const samples=node.buffer.getChannelData(0);window.soundEvidence.push({frames:node.buffer.length,rate:node.buffer.sampleRate,peak:samples.reduce((m,n)=>Math.max(m,Math.abs(n)),0)});return start(...args);};
   node.stop=(...args)=>{window.soundEvidence.push({stopped:true});return stop(...args);};return node;};
 });
 page.on('pageerror' ,e=>errors.push(e.message));
 await page.goto(process.env.ZUMI_PREVIEW_URL||'http://127.0.0.1:8876/preview/');await page.waitForFunction(()=>window.pocket).catch(async e=>{console.error("Preview initialization:",errors,await page.locator("#status").textContent());throw e;});
 const state=()=>page.evaluate(()=>({page:pocket.api.pocket_page(),moves:pocket.api.pocket_moves(),won:pocket.api.pocket_won(),level:pocket.api.pocket_level(),cursor:pocket.api.pocket_cursor(),board:pocket.api.pocket_board()}));
 const tap=key=>page.keyboard.press(key);
 const home=async()=>{await page.keyboard.down('Enter');await page.waitForTimeout(550);await page.keyboard.up('Enter');assert.equal((await state()).page,0);};
 await page.screenshot({path:'build/browser-home.png',fullPage:true});
 await tap('Enter');assert.equal((await state()).page,3);await home();
 await tap('ArrowDown');await tap('ArrowDown');await tap('Enter');assert.equal((await state()).page,4);
 await tap('Enter');assert.equal((await state()).page,0);
 await tap('ArrowDown');await tap('Enter');assert.equal((await state()).page,1);
 await page.locator('canvas').screenshot({path:'build/browser-play-title.png'});
 await tap('Enter');assert.equal((await state()).page,2);
 await page.screenshot({path:'build/browser-link.png'});
 const labels=()=>page.evaluate(()=>{const a=pocket.api,n=a.pocket_render(),base=a.pocket_ops(),stride=a.pocket_stride();return Array.from({length:n},(_,i)=>new TextDecoder().decode(new Uint8Array(a.memory.buffer,base+i*stride+36,64)).split('\0')[0]);});
 const solar=await labels();assert(solar.includes('Solar power > House'));assert(!solar.includes('S')&&!solar.includes('H'));
 const first=levels[0],tiles=[...first.initial];
 for(let visit=0;visit<16&&!(await state()).won;visit++){
  const cursor=(await state()).cursor;
  for(let turns=0;turns<4&&tiles[cursor]!==first.solution[cursor]&&!(await state()).won;turns++){
   tiles[cursor]=((tiles[cursor]<<1)|(tiles[cursor]>>3))&15;await tap('Enter');
  }
  if(!(await state()).won)await tap('ArrowDown');
 }
 assert.equal((await state()).won,1);assert.equal((await state()).moves,first.minimum_turns);
 await page.waitForFunction(()=>window.soundEvidence.some(e=>e.frames===17440));
 assert(await page.evaluate(()=>soundEvidence.some(e=>e.rate===16000&&e.peak>0.03&&e.peak<0.15)));
 await page.screenshot({path:'build/browser-link-complete.png'});
 await tap('Enter');assert.equal((await state()).level,1);assert.equal((await state()).page,2);assert.equal((await state()).moves,0);
 assert(await page.evaluate(()=>soundEvidence.some(e=>e.stopped)));
 await page.context().setOffline(true);
 await tap('Enter');assert.equal((await state()).moves,1);await tap('ArrowDown');await tap('ArrowUp');
 await page.evaluate(()=>{for(let i=0;i<500;i++)pocket.api.pocket_tick(100);});assert.equal((await state()).page,2);
 await page.waitForFunction(()=>window.soundEvidence.some(e=>e.frames===1440));
 await home();
 assert(await page.evaluate(()=>soundEvidence.some(e=>e.stopped)));
 await tap('ArrowDown');await tap('Enter');while((await state()).level!==2)await tap('ArrowDown');await tap('Enter');
 const wind=await labels();assert(wind.includes('Wind power > House'));
 await page.screenshot({path:'build/browser-wind.png'});await home();
 await page.evaluate(()=>{for(let i=0;i<600;i++)pocket.api.pocket_tick(100);pocket.render();});
 assert.equal(await page.evaluate(()=>pocket.api.pocket_screen_off()),1);
 assert(await page.evaluate(()=>{const p=document.querySelector('canvas').getContext('2d').getImageData(0,0,240,320).data;return p.every((v,i)=>i%4===3?v===255:v===0);}));
 await page.locator('canvas').screenshot({path:'build/browser-screen-off.png'});
 const previous=await page.evaluate(()=>pocket.api.pocket_wake_card());
 await tap('Enter');assert.equal((await state()).page,5);
 assert.notEqual(await page.evaluate(()=>pocket.api.pocket_wake_card()),previous);
 await page.waitForTimeout(400);
 const greeting=await page.evaluate(()=>{const a=pocket.api,n=a.pocket_render(),v=new DataView(a.memory.buffer),b=a.pocket_ops(),stride=a.pocket_stride();return Array.from({length:n},(_,i)=>({kind:v.getInt32(b+i*stride,true),opacity:v.getInt32(b+i*stride+32,true),text:new TextDecoder().decode(new Uint8Array(a.memory.buffer,b+i*stride+36,64)).split('\0')[0]}));});
 assert(greeting.some(x=>x.kind===3));assert(greeting.every(x=>x.opacity===255));
 await page.locator('canvas').screenshot({path:'build/browser-wake.png'});
 await page.waitForTimeout(3000);assert.equal((await state()).page,0);
 await page.setViewportSize({width:390,height:844});assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
 await page.getByRole('button',{name:'DOWN',exact:true}).click();await page.getByRole('button',{name:'OK',exact:true}).click();
 assert.equal((await state()).page,1);await page.getByRole('button',{name:'OK',exact:true}).click();
 await page.screenshot({path:'build/browser-mobile.png',fullPage:true});await home();await tap('ArrowDown');await tap('ArrowDown');await tap('Enter');
 await page.screenshot({path:'build/browser-garden-mobile.png',fullPage:true});
 assert.deepEqual(errors,[]);
 // Idle blanking and wake also apply inside a puzzle; wake never rotates a tile.
 await home();await tap('ArrowDown');await tap('Enter');await tap('Enter');
 await tap('Enter');await tap('ArrowDown');
 const gameBefore=await state();
 await page.evaluate(()=>{for(let i=0;i<600;i++)pocket.api.pocket_tick(100);pocket.render();});
 assert.equal(await page.evaluate(()=>pocket.api.pocket_screen_off()),1);
 await tap('ArrowUp');assert.equal((await state()).page,5);assert.equal((await state()).moves,gameBefore.moves);
 await page.evaluate(()=>{for(let i=0;i<30;i++)pocket.api.pocket_tick(100);pocket.render();});
 assert.deepEqual(await state(),gameBefore);
 await page.waitForTimeout(300);
 await page.locator('canvas').screenshot({path:'build/browser-game-resumed.png'});
 await tap('Enter');assert.equal((await state()).moves,gameBefore.moves+1);
 await home();
 const seen=new Set();let lastCard=-1;
 for(let n=0;n<128&&seen.size<8;n++) {
  const chosen=await page.evaluate(()=>{const a=pocket.api;for(let i=0;i<600;i++)a.pocket_tick(100);a.pocket_button(0,0);a.pocket_button(0,4);a.pocket_tick(100);a.pocket_tick(100);a.pocket_tick(100);pocket.render();return a.pocket_wake_card();});
  assert.notEqual(chosen,lastCard);lastCard=chosen;seen.add(chosen);
  await page.locator('canvas').screenshot({path:`build/browser-wake-card-${chosen+1}.png`});
  await page.evaluate(()=>{for(let i=0;i<31;i++)pocket.api.pocket_tick(100);pocket.render();});
 }
 assert.equal(seen.size,8);
 fs.writeFileSync('build/browser-results.json',JSON.stringify({status:'PASS',browser:'Chrome',checks:['home','Talk offline entry and Garden placeholder','single game entry','solve level 1','next level','offline rotation','untimed thinking','long OK release','60 second screen off on all pages','eight offline wake cards','no adjacent repeat','three second return to preserved game or home','mobile overflow','physical controls','full English energy labels','real Web Audio PCM playback','sound cancellation on exit'],pageErrors:errors},null,2));
 await browser.close();console.log('Browser Demo: PASS (desktop + mobile, eight offline wake cards, no page errors)');
})().catch(e=>{console.error(e);process.exit(1)});
