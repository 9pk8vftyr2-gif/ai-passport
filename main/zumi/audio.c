#include "audio.h"
#include "sound.h"
#include "bsp_audio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdatomic.h>
static QueueHandle_t requests, uplink, downlink;
static atomic_uint revision, talk_turn, pending, acknowledged;
static atomic_int mode;
static atomic_bool failed;
static void worker(void *arg) {
    (void)arg;int event=0,frame=0,previous_mode=0;bool ready=false;
    z_pcm_frame block;int16_t pcm[256];
    for(;;) {
        unsigned rev=atomic_load(&revision);int current=atomic_load(&mode);
        if(current!=previous_mode){
            event=0;frame=0;
            if(previous_mode&&ready){(void)bsp_audio_sleep();ready=false;}
            previous_mode=current;
        }
        atomic_store(&acknowledged,rev);
        if(current) {
            if(!ready){
                if(bsp_audio_init()!=ESP_OK||bsp_audio_set_format(16000,16,1)!=ESP_OK){atomic_store(&failed,true);atomic_store(&mode,0);continue;}
                bsp_audio_set_volume(60);ready=true;
                /* Initialization can block; recheck stop/mode before starting I/O. */
                continue;
            }
            if(current==1){
                block.turn=atomic_load(&talk_turn);block.bytes=T_FRAME_BYTES;
                esp_err_t err=bsp_audio_read(block.pcm,T_FRAME_BYTES);
                if(rev!=atomic_load(&revision)||atomic_load(&mode)!=1||block.turn!=atomic_load(&talk_turn))continue;
                if(err!=ESP_OK||xQueueSend(uplink,&block,0)!=pdTRUE){atomic_store(&failed,true);atomic_store(&mode,0);}
            }else if(xQueueReceive(downlink,&block,pdMS_TO_TICKS(20))==pdTRUE){
                if(block.turn==atomic_load(&talk_turn)&&rev==atomic_load(&revision)){
                    if(bsp_audio_write(block.pcm,block.bytes)!=ESP_OK){atomic_store(&failed,true);atomic_store(&mode,0);}
                }
                /* Queued and in-flight blocks each release exactly one pending count. */
                atomic_fetch_sub(&pending,1);
            }
            continue;
        }
        if(!event){if(xQueueReceive(requests,&event,pdMS_TO_TICKS(20))!=pdTRUE)continue;frame=0;}
        int newer;if(xQueueReceive(requests,&newer,0)==pdTRUE){event=newer;frame=0;}
        if(!event)continue;
        if(!ready){
            if(bsp_audio_init()!=ESP_OK||bsp_audio_set_format(Z_SOUND_RATE,16,1)!=ESP_OK){event=0;continue;}
            bsp_audio_set_volume(60);ready=true;continue;
        }
        int count=z_sound_frames(event)-frame;if(count>256)count=256;
        if(count<=0){event=0;continue;}
        for(int i=0;i<count;i++)pcm[i]=z_sound_sample(event,frame+i);
        if(bsp_audio_write(pcm,(size_t)count*sizeof(pcm[0]))!=ESP_OK){(void)bsp_audio_sleep();ready=false;event=0;continue;}
        frame+=count;if(frame>=z_sound_frames(event))event=0;
    }
}
void z_audio_start(void){
    if(requests){return;}requests=xQueueCreate(1,sizeof(int));if(!requests){return;}
    if(xTaskCreate(worker,"zumi_sound",4096,NULL,4,NULL)!=pdPASS){vQueueDelete(requests);requests=NULL;}
}
void z_audio_play(int event){if(requests)(void)xQueueOverwrite(requests,&event);}
bool z_audio_talk_prepare(void){
    if(!requests){return false;}
    for(int i=0;i<120&&!z_audio_stopped();i++)vTaskDelay(pdMS_TO_TICKS(10));
    if(!z_audio_stopped()){return false;}atomic_store(&failed,false);
    if(!uplink)uplink=xQueueCreate(T_UPLINK_FRAMES,sizeof(z_pcm_frame));
    if(!downlink)downlink=xQueueCreate(T_DOWNLINK_FRAMES,sizeof(z_pcm_frame));
    return uplink&&downlink;
}
void z_audio_talk_mode(int next,uint32_t turn){
    z_audio_play(Z_SOUND_NONE);atomic_store(&talk_turn,turn);atomic_fetch_add(&revision,1);
    atomic_store(&mode,next==3?0:next);if(next==1||next==2)atomic_store(&failed,false);
    /* Queue items carry turn IDs; no queue reset races with a blocked codec read/write. */
    z_pcm_frame block;
    if(next!=3&&uplink)while(xQueueReceive(uplink,&block,0)==pdTRUE){}
    if(downlink)while(xQueueReceive(downlink,&block,0)==pdTRUE)atomic_fetch_sub(&pending,1);
}
bool z_audio_capture_pop(z_pcm_frame *out){return uplink&&xQueueReceive(uplink,out,0)==pdTRUE;}
bool z_audio_output_push(const z_pcm_frame *frame){
    if(!downlink){return false;}atomic_fetch_add(&pending,1);
    if(xQueueSend(downlink,frame,0)==pdTRUE){return true;}
    atomic_fetch_sub(&pending,1);return false;
}
bool z_audio_drained(void){return atomic_load(&pending)==0;}
bool z_audio_talk_failed(void){return atomic_load(&failed);}
unsigned z_audio_pending(void){return atomic_load(&pending);}

bool z_audio_stopped(void){return atomic_load(&mode)==0&&atomic_load(&acknowledged)==atomic_load(&revision);}
