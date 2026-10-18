#include "talk_wire.h"
#include <string.h>
static uint32_t get32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void put32(uint8_t *p,uint32_t n){for(int i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
size_t t_pack(uint8_t *out,int type,uint32_t turn,uint32_t seq,const void *pcm,size_t bytes){
    if(!out||!pcm||(type!=1&&type!=2)||!bytes||bytes>T_FRAME_BYTES||bytes%2){return 0;}
    out[0]=1;out[1]=(uint8_t)type;out[2]=out[3]=0;put32(out+4,turn);put32(out+8,seq);
    out[12]=(uint8_t)bytes;out[13]=(uint8_t)(bytes>>8);memcpy(out+14,pcm,bytes);return bytes+14;
}
bool t_unpack(const uint8_t *in,size_t bytes,int type,uint32_t turn,uint32_t seq,const uint8_t **pcm,size_t *length){
    if(!in||!pcm||!length||bytes<16||bytes>T_WIRE_MAX){return false;}
    size_t n=(size_t)in[12]+((size_t)in[13]<<8);
    if(in[0]!=1||in[1]!=type||in[2]||in[3]||get32(in+4)!=turn||get32(in+8)!=seq||
       !n||n%2||n>T_FRAME_BYTES||bytes!=n+14)return false;
    *pcm=in+14;*length=n;return true;
}
bool t_json_bounded(const char *text,size_t bytes){
    if(!text||!bytes||bytes>2048){return false;}
    int depth=0,tokens=0;bool quoted=false,escape=false;
    for(size_t i=0;i<bytes;i++){
        unsigned char c=(unsigned char)text[i];
        if(!c||c>=128){return false;} /* Local English fixture contract, no CJK font yet. */
        if(escape){escape=false;continue;}
        if(quoted&&c=='\\'){escape=true;continue;}
        if(c=='"'){quoted=!quoted;if(quoted&&++tokens>48){return false;}continue;}
        if(quoted)continue;
        if(c=='{'||c=='['){if(++depth>4||++tokens>48){return false;}}
        if(c=='}'||c==']'){if(--depth<0){return false;}}
        if(c==','&&++tokens>48){return false;}
    }
    return !quoted&&!escape&&depth==0;
}
