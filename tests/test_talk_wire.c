#include "talk_wire.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void){
 uint8_t packet[T_WIRE_MAX],pcm[640]={0};const uint8_t *data;size_t n;
 assert(t_pack(packet,1,42,3,pcm,640)==654);
 assert(t_unpack(packet,654,1,42,3,&data,&n)&&n==640);
 assert(!t_unpack(packet,653,1,42,3,&data,&n));
 assert(!t_unpack(packet,654,2,42,3,&data,&n));
 assert(!t_unpack(packet,654,1,41,3,&data,&n));
 assert(!t_unpack(packet,654,1,42,4,&data,&n));
 packet[2]=1;assert(!t_unpack(packet,654,1,42,3,&data,&n));
 assert(!t_pack(packet,1,42,3,pcm,641));assert(!t_pack(packet,1,42,3,pcm,1));
 assert(!t_json_bounded("[[[[[0]]]]]",11));assert(!t_json_bounded("{",1));
 assert(t_json_bounded("{\"text\":\"[ok]\"}",15));
 for(int size=0;size<654;size++){memset(packet,0,sizeof(packet));assert(!t_unpack(packet,(size_t)size,1,42,3,&data,&n));}
 puts("Talk wire: PASS (bounds, sequence, identity, parser depth)");
}
