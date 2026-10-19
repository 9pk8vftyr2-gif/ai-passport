#include "talk_device.h"
#include "talk_wire.h"
#include "audio.h"
#include "demo_radio.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_transport_tcp.h"
#include "esp_transport_ws.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "nvs.h"
#include "cJSON.h"
#include "lwip/inet.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdatomic.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef struct {int command;uint32_t turn,epoch;} command;
typedef struct {int event;uint32_t turn,epoch;char text[T_REPLY_CAP];} notification;
static QueueHandle_t commands,events;
static atomic_uint epoch;
static atomic_bool net_ready,queue_failed,abort_close;
static esp_netif_t *netif;
static bool wifi_initialized,wifi_started,handlers;
static esp_event_handler_instance_t wh,ih;
static esp_transport_handle_t tcp,ws;
static notification notice;
static char json[2049],incoming[2049];
static uint8_t wire[T_WIRE_MAX];
static void post(int event,uint32_t turn,uint32_t generation,const char *text){
    notice=(notification){.event=event,.turn=turn,.epoch=generation};
    if(text)snprintf(notice.text,sizeof(notice.text),"%s",text);
    if(xQueueSend(events,&notice,0)!=pdTRUE)atomic_store(&queue_failed,true);
}
static void memory_sample(const char *phase){
    uint32_t caps=MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT;
    ESP_LOGI("talk","%s free=%u largest=%u minimum=%u",phase,(unsigned)heap_caps_get_free_size(caps),
        (unsigned)heap_caps_get_largest_free_block(caps),(unsigned)heap_caps_get_minimum_free_size(caps));
}
static void wifi_event(void *arg,esp_event_base_t base,int32_t id,void *data){
    (void)arg;(void)data;
    if(base==IP_EVENT&&id==IP_EVENT_STA_GOT_IP)atomic_store(&net_ready,true);
    if(base==WIFI_EVENT&&id==WIFI_EVENT_STA_DISCONNECTED)atomic_store(&net_ready,false);
}
static void close_all(void){
    z_audio_talk_mode(0,0);
    if(ws){esp_transport_close(ws);esp_transport_destroy(ws);ws=NULL;}
    if(tcp){esp_transport_destroy(tcp);tcp=NULL;}
    if(wifi_started){esp_wifi_disconnect();esp_wifi_stop();wifi_started=false;}
    if(handlers){esp_event_handler_instance_unregister(WIFI_EVENT,ESP_EVENT_ANY_ID,wh);esp_event_handler_instance_unregister(IP_EVENT,IP_EVENT_STA_GOT_IP,ih);handlers=false;}
    if(wifi_initialized){esp_wifi_deinit();wifi_initialized=false;}
    if(netif){esp_netif_destroy_default_wifi(netif);netif=NULL;}
    atomic_store(&net_ready,false);
}
static bool connect_local(uint32_t generation){
    close_all();memory_sample("before-connect");
    if(demo_radio_network_prepare()!=ESP_OK||demo_radio_nvs_prepare()!=ESP_OK){return false;}
    nvs_handle_t nvs;char ssid[33]={0},password[65]={0},host[16]={0};uint16_t port=8881;
    if(nvs_open("zumi_talk",NVS_READONLY,&nvs)!=ESP_OK){return false;}
    size_t n=sizeof(ssid);esp_err_t err=nvs_get_str(nvs,"ssid",ssid,&n);
    n=sizeof(password);if(err==ESP_OK)err=nvs_get_str(nvs,"password",password,&n);
    n=sizeof(host);if(err==ESP_OK)err=nvs_get_str(nvs,"host",host,&n);
    (void)nvs_get_u16(nvs,"port",&port);nvs_close(nvs);
    struct in_addr ip;bool valid=inet_pton(AF_INET,host,&ip)==1;uint32_t address=valid?ntohl(ip.s_addr):0;
    bool local=(address>>24)==10||(address>>20)==0xac1||(address>>16)==0xc0a8;
    if(err!=ESP_OK||!ssid[0]||strlen(ssid)>32||strlen(password)>63||!local||!port){memset(password,0,sizeof(password));return false;}
    wifi_config_t cfg={0};memcpy(cfg.sta.ssid,ssid,strlen(ssid));memcpy(cfg.sta.password,password,strlen(password));memset(password,0,sizeof(password));
    netif=esp_netif_create_default_wifi_sta();if(!netif){memset(&cfg,0,sizeof(cfg));return false;}
    wifi_init_config_t init=WIFI_INIT_CONFIG_DEFAULT();
    if(esp_wifi_init(&init)!=ESP_OK){memset(&cfg,0,sizeof(cfg));return false;}wifi_initialized=true;
    if(esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,wifi_event,NULL,&wh)!=ESP_OK){memset(&cfg,0,sizeof(cfg));return false;}
    if(esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,wifi_event,NULL,&ih)!=ESP_OK){esp_event_handler_instance_unregister(WIFI_EVENT,ESP_EVENT_ANY_ID,wh);memset(&cfg,0,sizeof(cfg));return false;}
    handlers=true;
    err=esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if(err==ESP_OK)err=esp_wifi_set_mode(WIFI_MODE_STA);
    if(err==ESP_OK)err=esp_wifi_set_config(WIFI_IF_STA,&cfg);
    memset(&cfg,0,sizeof(cfg));
    if(err==ESP_OK)err=esp_wifi_start();
    if(err!=ESP_OK){return false;}wifi_started=true;
    if(esp_wifi_connect()!=ESP_OK){return false;}
    for(int i=0;i<60&&!atomic_load(&net_ready);i++){
        if(generation!=atomic_load(&epoch)){return false;}vTaskDelay(pdMS_TO_TICKS(100));
    }
    if(!atomic_load(&net_ready)||generation!=atomic_load(&epoch)||!z_audio_talk_prepare()){return false;}
    tcp=esp_transport_tcp_init();if(!tcp){return false;}
    ws=esp_transport_ws_init(tcp);if(!ws){return false;}
    esp_transport_ws_set_path(ws,"/");
    if(esp_transport_connect(ws,host,port,1000)<0){return false;}
    memory_sample("local-ws-connected");return true;
}
static bool send_text(const char *data){int n=(int)strlen(data);return ws&&esp_transport_ws_send_raw(ws,WS_TRANSPORT_OPCODES_TEXT|WS_TRANSPORT_OPCODES_FIN,data,n,100)==n;}
static bool number(cJSON *root,const char *key,uint32_t *out){
    cJSON *n=cJSON_GetObjectItemCaseSensitive(root,key);
    if(!cJSON_IsNumber(n)||n->valuedouble<0||n->valuedouble>UINT32_MAX||floor(n->valuedouble)!=n->valuedouble){return false;}
    *out=(uint32_t)n->valuedouble;return true;
}
static void worker(void *arg){
    (void)arg;uint32_t generation=0,turn=0,tx=0,rx=0,credited=0;int phase=0;bool end=false;uint64_t drain_at=0,audio_notice_at=0;
    z_pcm_frame block;int assembled=0,total=0,frame_opcode=0;uint64_t partial_at=0;
    for(;;){
        if(generation!=atomic_load(&epoch)){
            bool notify_loss=ws&&!atomic_load(&abort_close);
            if(ws&&!assembled&&!atomic_load(&abort_close)){
                snprintf(json,sizeof(json),"{\"v\":1,\"type\":\"cancel\",\"turn\":%lu}",(unsigned long)turn);
                if(!send_text(json))close_all();
            }else close_all();
            generation=atomic_load(&epoch);phase=0;turn=0;assembled=0;
            if(notify_loss&&!ws)post(T_DISCONNECTED,0,generation,NULL);
        }
        command c;
        if(xQueueReceive(commands,&c,ws?0:pdMS_TO_TICKS(20))==pdTRUE){
            if(c.epoch!=generation){if(c.epoch!=atomic_load(&epoch))continue;close_all();generation=c.epoch;phase=0;turn=0;assembled=0;}
            if(c.command==T_F_CONNECT){
                if(!connect_local(generation)||!send_text("{\"v\":1,\"type\":\"hello\",\"capabilities\":[\"qa\"]}")){goto offline;}
            }else if(c.command==T_F_BEGIN){
                turn=c.turn;if(!ws||phase){goto failure;}
                tx=rx=credited=0;end=false;drain_at=0;audio_notice_at=0;
                snprintf(json,sizeof(json),"{\"v\":1,\"type\":\"input.begin\",\"turn\":%lu,\"capability\":\"qa\",\"memory\":\"none\"}",(unsigned long)turn);
                if(!send_text(json)){goto offline;}z_audio_talk_mode(1,turn);phase=1;memory_sample("listening");
            }else if(c.command==T_F_COMMIT){
                if(!ws||phase!=1||c.turn!=turn){goto failure;}
                z_audio_talk_mode(3,turn);phase=2;
            }
        }
        if(!ws)continue;
        if(!atomic_load(&net_ready)){goto offline;}
        if(z_audio_talk_failed()){goto failure;}
        if(phase==1||phase==2){
            for(int i=0;i<T_UPLINK_FRAMES&&z_audio_capture_pop(&block);i++){
                if(block.turn!=turn)continue;
                size_t n=t_pack(wire,1,turn,tx,block.pcm,block.bytes);
                if(esp_transport_write(ws,(char*)wire,(int)n,100)!=(int)n){goto offline;}tx++;
            }
            if(phase==2&&z_audio_stopped()){
                if(!tx){goto failure;}
                snprintf(json,sizeof(json),"{\"v\":1,\"type\":\"input.commit\",\"turn\":%lu,\"last_seq\":%lu}",(unsigned long)turn,(unsigned long)(tx-1));
                if(!send_text(json)){goto offline;}
                snprintf(json,sizeof(json),"{\"v\":1,\"type\":\"output.credit\",\"turn\":%lu,\"frames\":12}",(unsigned long)turn);
                if(!send_text(json)){goto offline;}phase=3;
            }
        }
        int n=esp_transport_read(ws,incoming+assembled,(int)sizeof(incoming)-1-assembled,5);
        if(generation!=atomic_load(&epoch))continue;
        if(n<0){goto offline;}
        if(assembled&&esp_timer_get_time()-partial_at>1000000){goto failure;}
        if(n>0){
            if(!assembled){
                total=esp_transport_ws_get_read_payload_len(ws);frame_opcode=esp_transport_ws_get_read_opcode(ws);
                if(!esp_transport_ws_get_fin_flag(ws)||total<=0||total>2048){goto failure;}
                partial_at=esp_timer_get_time();
            }
            assembled+=n;if(assembled<total)continue;if(assembled!=total){goto failure;}
            n=assembled;assembled=0;int opcode=frame_opcode;
            if(opcode==WS_TRANSPORT_OPCODES_BINARY){
                const uint8_t *pcm;size_t bytes;
                if(n>=14){const uint8_t *p=(const uint8_t*)incoming;uint32_t id=(uint32_t)p[4]|((uint32_t)p[5]<<8)|((uint32_t)p[6]<<16)|((uint32_t)p[7]<<24);if(id!=turn)continue;}
                if(phase<3||end||!t_unpack((uint8_t*)incoming,(size_t)n,2,turn,rx,&pcm,&bytes)){goto failure;}
                if(phase==3){z_audio_talk_mode(2,turn);phase=4;memory_sample("speaking");}
                block.turn=turn;block.bytes=(uint16_t)bytes;memcpy(block.pcm,pcm,bytes);
                if(!z_audio_output_push(&block)){goto failure;}
                rx++;
                uint64_t now=esp_timer_get_time();
                if(!audio_notice_at||now-audio_notice_at>=200000){post(T_AUDIO,turn,generation,NULL);audio_notice_at=now;}
            }else if(opcode==WS_TRANSPORT_OPCODES_TEXT){
                incoming[n]=0;if(!t_json_bounded(incoming,(size_t)n)){goto failure;}const char *parse_end=NULL;cJSON *root=cJSON_ParseWithLengthOpts(incoming,(size_t)n+1,&parse_end,1);
                if(!root){goto failure;}
                cJSON *type=cJSON_GetObjectItemCaseSensitive(root,"type");uint32_t ver=0,id=0;bool bad=false;
                if(!number(root,"v",&ver)||ver!=1||!cJSON_IsString(type))bad=true;
                else if(!strcmp(type->valuestring,"ready")){
                    cJSON *codec=cJSON_GetObjectItemCaseSensitive(root,"audio");uint32_t rate=0,ms=0;
                    if(!cJSON_IsString(codec)||strcmp(codec->valuestring,"pcm_s16le")||!number(root,"rate",&rate)||rate!=16000||!number(root,"frame_ms",&ms)||ms!=20)bad=true;
                    else post(T_CONNECTED,0,generation,NULL);
                }else if(!number(root,"turn",&id))bad=true;
                else if(id!=turn){/* Ignore obsolete turn responses. */}
                else if(!strcmp(type->valuestring,"asr.final")||!strcmp(type->valuestring,"answer.snapshot")){
                    cJSON *text=cJSON_GetObjectItemCaseSensitive(root,"text");int ev=!strcmp(type->valuestring,"asr.final")?T_ASR:T_REPLY;
                    if(!cJSON_IsString(text)||strlen(text->valuestring)>=(ev==T_ASR?T_ASR_CAP:T_REPLY_CAP))bad=true;
                    else post(ev,turn,generation,text->valuestring);
                }else if(!strcmp(type->valuestring,"output.end")){
                    uint32_t last=0;if(!rx||!number(root,"last_seq",&last)||last!=rx-1)bad=true;else end=true;
                }else bad=true;
                cJSON_Delete(root);if(bad){goto failure;}
            }else if(opcode==WS_TRANSPORT_OPCODES_CLOSE){goto offline;}
        }
        if(phase==4){
            unsigned pending=z_audio_pending();uint32_t released=rx>pending?rx-pending:0;
            if(released>credited){
                snprintf(json,sizeof(json),"{\"v\":1,\"type\":\"output.credit\",\"turn\":%lu,\"frames\":%lu}",(unsigned long)turn,(unsigned long)(released-credited));
                if(!send_text(json)){goto offline;}credited=released;
            }
            if(end&&z_audio_drained()){
                if(!drain_at)drain_at=esp_timer_get_time();
                if(esp_timer_get_time()-drain_at>=100000){z_audio_talk_mode(0,turn);phase=0;post(T_DONE,turn,generation,NULL);memory_sample("completed");}
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2));continue;
    failure:
        post(T_FAILURE,turn,generation,NULL);
    offline:
        close_all();phase=0;assembled=0;post(T_DISCONNECTED,turn,generation,NULL);
    }
}
void z_talk_device_command(int code,uint32_t turn){
    if(code==T_F_ABORT||code==5){atomic_store(&abort_close,code==5);atomic_fetch_add(&epoch,1);z_audio_talk_mode(0,turn);return;}
    if(!commands){
        commands=xQueueCreate(8,sizeof(command));events=xQueueCreate(4,sizeof(notification));
        if(!commands||!events||xTaskCreate(worker,"zumi_talk",6144,NULL,3,NULL)!=pdPASS){
            if(commands){vQueueDelete(commands);}
            if(events){vQueueDelete(events);}commands=events=NULL;atomic_store(&queue_failed,true);return;
        }
    }
    if(code==T_F_CONNECT){atomic_store(&abort_close,true);atomic_fetch_add(&epoch,1);}
    command item={code,turn,atomic_load(&epoch)};
    if(xQueueSend(commands,&item,0)!=pdTRUE)atomic_store(&queue_failed,true);
}
void z_talk_device_poll(z_state *state){
    t_output_level(&state->talk,state->talk.turn,z_audio_output_level(state->talk.turn));
    if(atomic_exchange(&queue_failed,false)){z_talk_device_command(5,state->talk.turn);z_talk_receive(state,T_FAILURE,state->talk.turn,NULL);z_talk_receive(state,T_DISCONNECTED,state->talk.turn,NULL);return;}
    if(!events){return;}
    static notification item;
    for(int i=0;i<4&&xQueueReceive(events,&item,0)==pdTRUE;i++)
        if(item.epoch==atomic_load(&epoch))z_talk_receive(state,item.event,item.turn,item.text);
}
