#ifdef PHOTO_NETWORK
#include "photo_network.h"
#include <curl/curl.h>
#include <dswifi9.h>
#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static NetStage stage;
static CURLM *multi;static CURL *easy;static struct curl_slist *headers;
static char base[240],token[80],url[320],job[33],message[160];
static unsigned char response[4096],*upload;static size_t used,upload_size;
static time_t began,next_poll;static bool wifi_started,curl_started;
static const char probe[]="Photo Worlds encrypted round trip v1";
static void cleanup_request(void){if(easy){if(multi)curl_multi_remove_handle(multi,easy);curl_easy_cleanup(easy);easy=NULL;}if(multi){curl_multi_cleanup(multi);multi=NULL;}curl_slist_free_all(headers);headers=NULL;}
void photo_network_close(void){cleanup_request();free(upload);upload=NULL;upload_size=0;if(wifi_started){Wifi_DisconnectAP();Wifi_DisableWifi();wifi_started=false;}if(curl_started){curl_global_cleanup();curl_started=false;}memset(token,0,sizeof(token));}
static void fail(const char*s){snprintf(message,sizeof(message),"%s",s);stage=NET_FAILED;photo_network_close();}
static size_t receive(char*data,size_t size,size_t n,void*arg){(void)arg;if(size&&n>sizeof(response)/size)return 0;size_t len=size*n;if(len>sizeof(response)-used-1)return 0;memcpy(response+used,data,len);used+=len;response[used]=0;return len;}
static bool start_request(const char *path,const void*body,size_t size){
 cleanup_request();used=0;snprintf(url,sizeof(url),"%s%s",base,path);
 multi=curl_multi_init();easy=curl_easy_init();if(!multi||!easy){fail("Network memory unavailable");return false;}
 char authorization[128];snprintf(authorization,sizeof(authorization),"Authorization: Bearer %s",token);
 headers=curl_slist_append(headers,authorization);headers=curl_slist_append(headers,"Content-Type: application/octet-stream");headers=curl_slist_append(headers,"Expect:");
 curl_easy_setopt(easy,CURLOPT_URL,url);curl_easy_setopt(easy,CURLOPT_HTTPHEADER,headers);
 curl_easy_setopt(easy,CURLOPT_HTTP_VERSION,CURL_HTTP_VERSION_1_1);
 curl_easy_setopt(easy,CURLOPT_SSL_VERIFYHOST,2L);curl_easy_setopt(easy,CURLOPT_SSL_VERIFYPEER,1L);
 curl_easy_setopt(easy,CURLOPT_CAINFO,"sd:/photo-worlds/service-ca.pem");
 curl_easy_setopt(easy,CURLOPT_FOLLOWLOCATION,0L);curl_easy_setopt(easy,CURLOPT_PROTOCOLS_STR,"https");
 curl_easy_setopt(easy,CURLOPT_CONNECTTIMEOUT,15L);curl_easy_setopt(easy,CURLOPT_TIMEOUT,30L);
 curl_easy_setopt(easy,CURLOPT_WRITEFUNCTION,receive);curl_easy_setopt(easy,CURLOPT_WRITEDATA,NULL);
 if(body){curl_easy_setopt(easy,CURLOPT_POST,1L);curl_easy_setopt(easy,CURLOPT_POSTFIELDS,body);curl_easy_setopt(easy,CURLOPT_POSTFIELDSIZE,(long)size);}
 if(curl_multi_add_handle(multi,easy)!=CURLM_OK){fail("Could not begin transfer");return false;}return true;
}
static bool line(FILE*f,char*out,size_t size){if(!fgets(out,(int)size,f))return false;char*p=strpbrk(out,"\r\n");if(p)*p=0;return out[0]!=0;}
bool photo_network_begin(const char*path){
 photo_network_close();stage=NET_IDLE;used=0;
 FILE*f=fopen("sd:/photo-worlds/network.cfg","r");if(!f){fail("Setup needed: network.cfg missing");return false;}
 bool valid=line(f,base,sizeof(base))&&line(f,token,sizeof(token));fclose(f);
 if(!valid||strncmp(base,"https://",8)||strpbrk(base," \t?#@")||strlen(token)!=64){fail("Network setup is invalid");return false;}
 for(unsigned i=0;i<64;i++)if(!((token[i]>='0'&&token[i]<='9')||(token[i]>='a'&&token[i]<='f'))){fail("Device pairing is invalid");return false;}
 size_t l=strlen(base);if(base[l-1]=='/')base[l-1]=0;
 f=fopen("sd:/photo-worlds/service-ca.pem","rb");if(!f){fail("Trusted server certificate missing");return false;}fclose(f);
 f=fopen(path,"rb");if(!f){fail("Saved capture could not open");return false;}
 fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);
 if(size!=98316&&size!=98320){fclose(f);fail("Saved capture is incomplete");return false;}
 upload=malloc((size_t)size);if(!upload){fclose(f);fail("Photo memory unavailable");return false;}
 bool ok=fread(upload,1,(size_t)size,f)==(size_t)size;fclose(f);upload_size=(size_t)size;if(!ok){fail("Could not read the saved photo");return false;}
 if(!isDSiMode()){fail("Start in DSi mode through Unlaunch");return false;}
 if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK){fail("TLS initialization failed");return false;}
 curl_started=true;
 if(!Wifi_CheckInit()&&!Wifi_InitDefault(INIT_ONLY|WIFI_ATTEMPT_DSI_MODE)){fail("DSi WiFi initialization failed");return false;}
 wifi_started=true;Wifi_AutoConnect();began=time(NULL);stage=NET_CONNECT;strcpy(message,"Connecting to saved WiFi");return true;
}
void photo_network_tick(bool cancel){
 if(cancel){photo_network_close();stage=NET_CANCELLED;strcpy(message,"Cancelled; photo stays saved");return;}
 if(stage<NET_CONNECT||stage>NET_FETCH)return;
 time_t now=time(NULL);if(now-began>180){fail("Timed out; photo saved. A retries");return;}
 if(stage==NET_CONNECT){int status=Wifi_AssocStatus();if(status==ASSOCSTATUS_CANNOTCONNECT||now-began>35){fail("WiFi failed; check hotspot settings");return;}if(status!=ASSOCSTATUS_ASSOCIATED)return;
  stage=NET_PROBE;strcpy(message,"Checking encrypted connection");start_request("/v1/probe",probe,sizeof(probe)-1);return;}
 if(stage==NET_WAIT){if(now<next_poll)return;char path[64];snprintf(path,sizeof(path),"/v1/jobs/%s",job);stage=NET_FETCH;start_request(path,NULL,0);return;}
 int running=0;if(curl_multi_perform(multi,&running)!=CURLM_OK){fail("Transfer interrupted; A retries");return;}
 if(running)return;
 int queued;CURLMsg*event=curl_multi_info_read(multi,&queued);if(!event||event->msg!=CURLMSG_DONE)return;
 CURLcode code=event->data.result;long status=0;curl_easy_getinfo(easy,CURLINFO_RESPONSE_CODE,&status);cleanup_request();
 if(code!=CURLE_OK){fail(code==CURLE_PEER_FAILED_VERIFICATION?"Certificate check failed; check clock":"Secure transfer failed; A retries");return;}
 if(stage==NET_PROBE){if(status!=200||used!=sizeof(probe)-1||memcmp(response,probe,used)){fail("Server pairing check failed");return;}stage=NET_UPLOAD;strcpy(message,"Sending photo securely");start_request("/v1/worlds",upload,upload_size);return;}
 if(stage==NET_UPLOAD){if(status!=202||used!=32){fail("Generation request rejected; check Mac");return;}for(unsigned i=0;i<32;i++)if(!((response[i]>='0'&&response[i]<='9')||(response[i]>='a'&&response[i]<='f'))){fail("Invalid server reply");return;}memcpy(job,response,32);job[32]=0;free(upload);upload=NULL;upload_size=0;}
 else if(stage==NET_FETCH&&status==200){stage=NET_READY;strcpy(message,"Checking generated game");photo_network_close();return;}
 else if(stage==NET_FETCH&&status!=202){fail("Could not generate; photo saved");return;}
 stage=NET_WAIT;next_poll=now+2;strcpy(message,"Building and checking your world");
}
NetStage photo_network_stage(void){return stage;}
const char*photo_network_message(void){return message;}
const unsigned char*photo_network_result(size_t*size){*size=used;return response;}
#endif
