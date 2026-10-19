/* Deterministic worker tests: inject stop during BSP I/O. Not a FreeRTOS scheduler test. */
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../main/zumi/audio.c"
struct fake_queue {unsigned count,used;size_t size;unsigned char *data;};
static jmp_buf done;
static int in_worker,read_action,writes,sleeps,allow_write;
static int64_t clock_us;
int64_t esp_timer_get_time(void){return clock_us;}
QueueHandle_t xQueueCreate(unsigned n,size_t size){
    QueueHandle_t q=calloc(1,sizeof(*q));assert(q);q->count=n;q->size=size;q->data=calloc(n,size);assert(q->data);return q;
}
int xQueueSend(QueueHandle_t q,const void *data,unsigned wait){(void)wait;if(q->used==q->count)return 0;memcpy(q->data+q->used++*q->size,data,q->size);return 1;}
int xQueueReceive(QueueHandle_t q,void *data,unsigned wait){
    (void)wait;if(!q->used){if(in_worker&&(q==requests||(q==downlink&&allow_write)))longjmp(done,1);return 0;}
    memcpy(data,q->data,q->size);q->used--;memmove(q->data,q->data+q->size,q->used*q->size);return 1;
}
int xQueueOverwrite(QueueHandle_t q,const void *data){q->used=0;return xQueueSend(q,data,0);}
void vQueueDelete(QueueHandle_t q){if(q){free(q->data);free(q);}}
int xTaskCreate(void (*fn)(void*),const char *name,unsigned stack,void *arg,unsigned priority,void *handle){(void)fn;(void)name;(void)stack;(void)arg;(void)priority;(void)handle;return 1;}
void vTaskDelay(unsigned ticks){(void)ticks;}
esp_err_t bsp_audio_init(void){return 0;}
esp_err_t bsp_audio_set_format(int rate,int bits,int channels){assert(rate==16000&&bits==16&&channels==1);return 0;}
void bsp_audio_set_volume(int volume){(void)volume;}
esp_err_t bsp_audio_sleep(void){sleeps++;return 0;}
esp_err_t bsp_audio_read(void *data,size_t size){
    assert(size==640);memset(data,0,size);
    if(read_action==1)z_audio_talk_mode(0,8); /* Cancel inside blocking read. */
    if(read_action==2)z_audio_talk_mode(3,7); /* Finish inside blocking read. */
    return 0;
}
esp_err_t bsp_audio_write(const void *data,size_t size){
    (void)data;assert(size<=640);writes++;if(!allow_write)z_audio_talk_mode(0,8);return 0; /* Cancel inside write. */
}
static void run(void){in_worker=1;if(!setjmp(done))worker(NULL);in_worker=0;}
int main(void){
    z_audio_start();assert(z_audio_talk_prepare());
    atomic_store(&mode,2);atomic_store(&output_turn,7);atomic_store(&output_level,800);atomic_store(&output_at,0);
    clock_us=80000;assert(z_audio_output_level(7)==800&&z_audio_output_level(8)==0);
    clock_us=81000;assert(z_audio_output_level(7)==0);atomic_store(&mode,0);
    read_action=1;z_audio_talk_mode(1,7);run();
    z_pcm_frame block={.turn=7,.bytes=640};assert(!z_audio_capture_pop(&block)&&z_audio_stopped()&&sleeps==1);
    read_action=0;z_audio_talk_mode(1,7);run(); /* Bounded queue overflow stops acquisition. */
    assert(z_audio_talk_failed()&&uplink->used==5&&z_audio_stopped());
    assert(z_audio_talk_prepare()&&!z_audio_talk_failed());
    z_audio_talk_mode(1,7);assert(xQueueSend(uplink,&block,0));
    read_action=2;run();assert(z_audio_stopped()&&uplink->used==1); /* Commit keeps only already captured blocks. */
    assert(z_audio_capture_pop(&block)&&!z_audio_capture_pop(&block));
    z_audio_talk_mode(2,7);assert(z_audio_output_push(&block)&&z_audio_output_push(&block));
    assert(z_audio_pending()==2);run();assert(writes==1&&z_audio_drained()&&z_audio_stopped());
    assert(z_audio_output_level(8)==0);
    allow_write=1;z_audio_talk_mode(2,9);block.turn=9;
    for(unsigned i=0;i<640;i+=2){block.pcm[i]=0xe0;block.pcm[i+1]=0xfc;} /* -800 PCM */
    assert(z_audio_output_push(&block));run();assert(z_audio_output_level(9)==800);
    memset(block.pcm,0,sizeof(block.pcm));assert(z_audio_output_push(&block));run();assert(z_audio_output_level(9)==0);
    z_audio_talk_mode(0,10);assert(z_audio_output_level(9)==0);
    vQueueDelete(uplink);vQueueDelete(downlink);vQueueDelete(requests);
    puts("Talk audio owner: PASS (cancel during read/write, commit drain, overflow, retry, pending counts)");
}
