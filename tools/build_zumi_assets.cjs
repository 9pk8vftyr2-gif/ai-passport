// Compile the approved vector into small flash-resident RGB565 sprites.
const fs = require('node:fs');
const path = require('node:path');
const sharp = require('sharp');
const crypto = require('node:crypto');
const root = path.resolve(__dirname,'..');
const dir=path.join(root,'assets/images/zumi-pocket');
const original=fs.readFileSync(path.join(dir,'zumi-original.svg'),'utf8');
const faces=[original,
 original.replace(/<g id="face-eyes"[\s\S]*?<\/g>/,'<g id="face-eyes"><ellipse cx="191" cy="320" rx="8" ry="13" fill="#111C20"/><ellipse cx="304" cy="320" rx="8" ry="13" fill="#111C20"/></g>').replace(/<g id="face-mouth"[\s\S]*?<\/g>/,'<g id="face-mouth"><path d="M225 353 Q248 368 274 353" fill="none" stroke="#111C20" stroke-width="8" stroke-linecap="round"/></g>'),
 original.replace(/<g id="face-eyes"[\s\S]*?<\/g>/,'<g id="face-eyes"><path d="M169 324 Q191 296 213 324 M282 324 Q304 296 326 324" fill="none" stroke="#111C20" stroke-width="9" stroke-linecap="round"/></g>'),
 original.replace(/<g id="face-mouth"[\s\S]*?<\/g>/,'<g id="face-mouth"><path d="M225 353 Q248 368 274 353" fill="none" stroke="#111C20" stroke-width="8" stroke-linecap="round"/></g>')];
(async()=>{
 let c='#include "sprites.h"\n';
 const hashes=[];
 for(let i=0;i<faces.length;i++) {
  const png=await sharp(Buffer.from(faces[i])).resize(78,92,{fit:'contain',background:'#F6F3E9'}).flatten({background:'#F6F3E9'}).png().toBuffer();
  fs.writeFileSync(path.join(dir,`zumi-${i}.png`),png);
  const raw=await sharp(png).removeAlpha().raw().toBuffer();
  const bytes=[];
  for(let p=0;p<raw.length;p+=3) {const v=((raw[p]>>3)<<11)|((raw[p+1]>>2)<<5)|(raw[p+2]>>3);bytes.push(v&255,v>>8);}
  c+=`static const uint8_t pixels_${i}[] = {\n`;
  for(let p=0;p<bytes.length;p+=24)c+=' '+bytes.slice(p,p+24).map(n=>'0x'+n.toString(16).padStart(2,'0')).join(',')+',\n';
  c+='};\n';
  hashes.push({name:`zumi-${i}.png`,sha256:crypto.createHash('sha256').update(png).digest('hex'),rgb565_bytes:bytes.length});
 }
 c+='const lv_image_dsc_t zumi_sprites[4] = {\n';
 for(let i=0;i<faces.length;i++)c+=` {.header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_RGB565,.w=78,.h=92,.stride=156},.data_size=14352,.data=pixels_${i}},\n`;
 c+='};\n'; fs.writeFileSync(path.join(root,'main/zumi/sprites.c'),c);
 fs.writeFileSync(path.join(dir,'manifest.json'),JSON.stringify({source:'ZUMI_2D_FRONT_IVORY_v1.1.svg',source_sha256:crypto.createHash('sha256').update(original).digest('hex'),character:'Zumi is a boy. Existing ivory 2D master; no redesign.',expressions:['happy (original)','focused (face-only derivative)','cheering (face-only derivative)','speaking closed mouth (original eyes/body, mouth-only derivative)'],license:'ZCO character artwork; supplied for this project. No additional third-party license is granted.',assets:hashes},null,2)+'\n');
})();
