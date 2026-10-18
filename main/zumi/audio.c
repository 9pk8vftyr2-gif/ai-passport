#include "audio.h"
#include "sound.h"
#include "bsp_audio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
static QueueHandle_t requests;
static void worker(void *arg) {
    (void)arg;
    int event=0,frame=0;bool ready=false;
    int16_t pcm[256];
    for(;;) {
        if(!event) {
            if(xQueueReceive(requests,&event,portMAX_DELAY)!=pdTRUE) continue;
            frame=0;
        }
        int newer;
        if(xQueueReceive(requests,&newer,0)==pdTRUE) {event=newer;frame=0;}
        if(!event) continue;
        if(!ready) {
            if(bsp_audio_init()!=ESP_OK || bsp_audio_set_format(Z_SOUND_RATE,16,1)!=ESP_OK) {
                ESP_LOGW("zumi","Sound unavailable; game remains playable");event=0;continue;
            }
            bsp_audio_set_volume(60);ready=true;
            /* Initialization may block: honor a newer cancellation first. */
            continue;
        }
        int count=z_sound_frames(event)-frame;if(count>256) count=256;
        if(count<=0){event=0;continue;}
        for(int i=0;i<count;i++) pcm[i]=z_sound_sample(event,frame+i);
        if(bsp_audio_write(pcm,(size_t)count*sizeof(pcm[0]))!=ESP_OK) {
            ESP_LOGW("zumi","Sound write failed");
            (void)bsp_audio_sleep();ready=false;event=0;continue;
        }
        frame+=count;
        if(frame>=z_sound_frames(event)) event=0;
    }
}
void z_audio_start(void) {
    if(requests) return;
    requests=xQueueCreate(1,sizeof(int));
    if(!requests) return;
    if(xTaskCreate(worker,"zumi_sound",4096,NULL,4,NULL)!=pdPASS) {
        vQueueDelete(requests);requests=NULL;
    }
}
void z_audio_play(int event) { if(requests) (void)xQueueOverwrite(requests,&event); }
