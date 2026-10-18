// Reuse Eco Pop's exact atlas crops; compile small RGB565 icons for offline use.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),sharp=require('sharp');
const root=path.resolve(__dirname,'..'),dir=path.join(root,'assets/images/zumi-pocket/energy-icons');
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const specs=[['solar','clay-matte-v1.png',78,60,441],['wind','wind-matte-sky-v1.png',98,117,1059],['house','clay-matte-v1.png',1013,530,460]];
(async()=>{
 let c='#include "energy_icons.h"\n',assets=[];
 for(let lit=0;lit<2;lit++)for(let k=0;k<3;k++){
  const [name,file,left,top,size]=specs[k],source=fs.readFileSync(path.join(dir,'source',file)),id=lit*3+k;
  const raw=await sharp(source).extract({left,top,width:size,height:size}).resize(32,32).flatten({background:lit?'#D5E9BA':'#E8EBDD'}).removeAlpha().raw().toBuffer();
  const pixels=Buffer.alloc(2048),rgb=Buffer.alloc(3072);
  for(let i=0;i<1024;i++){
   const r=raw[i*3]>>3,g=raw[i*3+1]>>2,b=raw[i*3+2]>>3;
   pixels.writeUInt16LE((r<<11)|(g<<5)|b,i*2);
   rgb[i*3]=(r<<3)|(r>>2);rgb[i*3+1]=(g<<2)|(g>>4);rgb[i*3+2]=(b<<3)|(b>>2);
  }
  const png=await sharp(rgb,{raw:{width:32,height:32,channels:3}}).png().toBuffer();
  const output=`${name}-${lit?'on':'off'}.png`;fs.writeFileSync(path.join(dir,output),png);
  c+=`static const uint8_t pixels_${id}[]={\n`;
  for(let i=0;i<pixels.length;i+=24)c+=' '+[...pixels.subarray(i,i+24)].join(',')+',\n';
  c+='};\n';assets.push({id,name,lit:!!lit,file:output,sha256:hash(png),rgb565_sha256:hash(pixels),bytes:pixels.length,source:file,source_sha256:hash(source),crop:[left,top,size]});
 }
 c+='const lv_image_dsc_t zumi_energy_icons[6]={\n';
 for(let i=0;i<6;i++)c+=` {.header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_RGB565,.w=32,.h=32,.stride=64},.data_size=2048,.data=pixels_${i}},\n`;
 c+='};\n';fs.writeFileSync(path.join(root,'main/zumi/energy_icons.c'),c);
 fs.writeFileSync(path.join(dir,'manifest.json'),JSON.stringify({source_project:'ZCO Eco Pop',source_revision:'a0504f97fb74a94ea864361f539e5832a16b268f',source_path:'game/carbon-pop/icons',crop_reference:'docs/evidence/carbon-pop-matte/alignment.json',note:'Original artwork reused at user request. House uses Eco Pop building/demand art. House crop y=529.5 rounds to 530. No claim of a new third-party redistribution license.',flash_bytes:12288,width:32,height:32,assets},null,2)+'\n');
 console.log('Eco Pop icons: 6 RGB565 variants, 12,288 Flash bytes');
})().catch(e=>{console.error(e);process.exit(1)});
