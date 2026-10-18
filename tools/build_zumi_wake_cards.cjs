// Offline asset conversion only: approved card -> native RGB565, no runtime decoder.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),sharp=require('sharp');
const root=path.resolve(__dirname,'..'),dir=path.join(root,'assets/images/zumi-pocket/wake-cards');
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
(async()=>{
 const provenance=JSON.parse(fs.readFileSync(path.join(dir,'source/provenance.json')));
 const cards=[],buffers=[];
 for(const entry of provenance.cards){
  const input=fs.readFileSync(path.join(dir,'source',entry.source));
  if(hash(input)!==entry.source_sha256)throw Error('Approved source changed: '+entry.source);
  const raw=await sharp(input).resize(240,320,{fit:'fill'}).removeAlpha().raw().toBuffer();
  const rgb565=Buffer.alloc(240*320*2),quantized=Buffer.alloc(raw.length);
  for(let i=0;i<240*320;i++){
   const r=raw[i*3]>>3,g=raw[i*3+1]>>2,b=raw[i*3+2]>>3;
   rgb565.writeUInt16LE((r<<11)|(g<<5)|b,i*2);
   quantized[i*3]=(r<<3)|(r>>2);quantized[i*3+1]=(g<<2)|(g>>4);quantized[i*3+2]=(b<<3)|(b>>2);
  }
  const name=`card-${entry.id+1}.png`;
  const png=await sharp(quantized,{raw:{width:240,height:320,channels:3}}).png().toBuffer();
  fs.writeFileSync(path.join(dir,name),png);buffers.push(rgb565);
  cards.push({...entry,file:name,sha256:hash(png),rgb565_sha256:hash(rgb565),rgb565_bytes:rgb565.length,offset:entry.id*rgb565.length});
 }
 const binary=Buffer.concat(buffers);fs.writeFileSync(path.join(dir,'wake_cards.rgb565'),binary);
 fs.writeFileSync(path.join(dir,'manifest.json'),JSON.stringify({width:240,height:320,format:'RGB565 little-endian',count:8,flash_bytes:binary.length,runtime_full_frame_buffers:0,sha256:hash(binary),cards},null,2)+'\n');
 console.log(`Wake cards: ${cards.length}, ${binary.length} Flash bytes, 240x320 RGB565.`);
})().catch(e=>{console.error(e);process.exit(1);});
