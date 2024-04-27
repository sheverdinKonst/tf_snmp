#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "debug.h"
#include "stm32f4xx_iwdg.h"
#include "../deffines.h"
#include "../uip/uip.h"
#include "httpd.h"
#include "httpd-fs.h"
#include "httpd-cgi.h"
#include "settingsfile.h"
#include "task.h"
#include "../deffines.h"
#include "../flash/spiflash.h"
//#include "net/syslog/syslog.h"
//#include "net/syslog/cfg_syslog.h"
#include "../smtp/smtp.h"
#include "../syslog/msg_build.h"
#include "board.h"
#include "names.h"
#include "settings.h"
#include "../events/events_handler.h"
#include "../snmp/snmpd/md5.h"



#define STATE_WAITING 0
#define STATE_OUTPUT  1

#define ISO_nl       0x0a //
#define ISO_space    0x20 //" "
#define ISO_bang     0x21 //
#define ISO_percent  0x25 //"%"
#define ISO_period   0x2e //
#define ISO_slash    0x2f // "/"
#define ISO_colon    0x3a //":"
#define ISO_question 0x3f // "?"

#define HTTP_MAX_AUTH_FAIL 	30
#define HTTP_FAIL_TIMEOUT	3600 // 1 hour

u8 it_is_save=0;
u8 it_is_eeprom = 0;

//extern xTaskHandle xEepromTask;
//extern uint16_t need_update_flag[4];

//extern struct status_t status;

const char http_get[] = "GET ";
const char http_post[] = "POST ";
const char http_index_shtml[] = "/index.shtml";
const char http_content_length[] = "Content-Length:";
const char http_content_type[] = "Content-Type: ";
const char http_content_form_data[] = "multipart/form-data; ";
const char http_content_urlencoded[] = "application/x-www-form-urlencoded";
const char http_content_octet_stream[] = "application/octet-stream";
const char http_content_boundary[] = "boundary=";
const char content_disposition[] = "Content-Disposition: ";
const char content_type[] = "Content-Type: ";
const char form_data[] = "form-data";
#if AUTH_BASIC
const char http_authorization[] = "Authorization: Basic ";
#else
const char http_authorization[] = "Authorization: Digest ";
#endif
#if AUTH_BASIC
const char http_header_401[] =    "HTTP/1.0 401 UNAUTHORIZED\r\nWWW-Authenticate: Basic realm=TFortis PSW\r\n\r\n401 Unauthorized\r\n";
#else
const char http_header_401[] =    "HTTP/1.0 401 UNAUTHORIZED\r\nWWW-Authenticate: Digest "
                 "realm=\"TFortisPSW\","
                 "qop=\"auth\","
                 "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\","
                 "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\","
                 "algorithm=MD5,"
                 "stale=false"
                 "\r\n\r\n401 Unauthorized\r\n";
#endif

const char http_404_html[] = "/404.html";
const char http_header_200[] = "HTTP/1.0 200 OK\r\nConnection: close\r\n";
const char http_header_404[] = "HTTP/1.0 404 Not found\r\nConnection: close\r\n";

const char http_content_type_binary[] = "Content-type: application/octet-stream\r\n\r\n";
const char http_content_type_css[] = "Content-type: text/css\r\n\r\n";
const char http_content_type_js[] = "Content-type: text/javascript\r\n\r\n";
const char http_content_type_png[] = "Content-type: image/png\r\n\r\n";
const char http_content_type_gif[] = "Content-type: image/gif\r\n\r\n";
const char http_content_type_jpg[] = "Content-type: image/jpeg\r\n\r\n";
const char http_content_type_html[] = "Content-type: text/html\r\n\r\n";
const char http_content_type_plain[] = "Content-type: text/plain\r\n\r\n";
const char http_content_type_bak[] = "Content-type: application/octet-stream\r\n\r\n";
const char http_content_type_log[] = "Content-type: application/octet-stream\r\n\r\n";
const char http_content_type_json[] = "Content-type: application/json\r\n\r\n";
const char http_content_type_bin[] = "Content-type: application/octet-stream\r\n\r\n";


const char http_getInput[] 			= "/api/getInput";
const char http_getLink[] 			= "/api/getLink";
const char http_setPoe[] 			= "/api/setPoe";
const char http_getPoe[] 			= "/api/getPoe";
const char http_getPoePower[] 		= "/api/getPoePower";
const char http_getPortNum[] 		= "/api/getPortNum";
const char http_isUps[] 			= "/api/isUps";
const char http_getUpsStatus[] 		= "/api/getUpsStatus";
const char http_getUpsVoltage[] 	= "/api/getUpsVoltage";
const char http_getUpsEstimated[] 	= "/api/getUpsEstimated";
const char http_getNetIp[] 			= "/api/getNetIp";
const char http_getNetMac[] 		= "/api/getNetMac";
const char http_getNetMask[]		= "/api/getNetMask";
const char http_getNetGate[] 		= "/api/getNetGate";
const char http_getFwVersion[] 		= "/api/getFwVersion";
const char http_getDevType[] 		= "/api/getDevType";
const char http_getDevName[] 		= "/api/getDevName";
const char http_getDevLocation[] 	= "/api/getDevLocation";
const char http_getPortMacList[] 	= "/api/getPortMacList";
const char http_cableTesterStart[] 	= "/api/cableTesterStart";
const char http_cableTesterStatus[] = "/api/cableTesterStatus";
const char http_getSerialNum[] 		= "/api/getSerialNum";
const char http_getDevContact[] 	= "/api/getDevContact";
const char http_getUptime[] 		= "/api/getUptime";
const char http_rebootAll[] 		= "/api/rebootAll";


const char http_shtml[] = ".shtml";
const char http_css[] = ".css";
const char http_js[] = ".js";
const char http_gif[] = ".gif";
const char http_jpg[] = ".jpg";
const char http_png[] = ".png";
const char http_html[] = ".html";
const char http_bak[] = ".bak";
const char http_bin[] = ".bin";
const char http_log[] = ".log";
const char http_json[] = ".json";

const char http_username[] = "username";
const char http_uri[] = "uri";
const char http_nonce[] = "nonce";

const char http_nonce_val[] = "dcd98b7102dd2f0e8b11d0f600bfb0c093";

const char http_nc[] = "nc";
const char http_cnonce[] = "cnonce";
const char http_qop[] = "qop";
const char http_opaque[] = "opaque";
const char http_response[] = "response";

char http_passwd_enable;
static u32 progress_bar=0;
static u32 file_size=0;
static u8 http_logout_;

static char parse_contenttype(const char *type_str, char *boundary, char boundary_size){
  char type = HTTP_CONTENT_NONE;
  char *boundary_p;
  if(strncmp(type_str, http_content_form_data, sizeof(http_content_form_data)-1)==0) {
    type = HTTP_CONTENT_FORMDATA;
    boundary_p = strstr(&type_str[sizeof(http_content_form_data)-1], http_content_boundary);
    if (boundary_p){
      char *end;
      boundary_p+=sizeof(http_content_boundary)-1;
      if (boundary_p[0]=='\"') boundary_p++;
      strncpy(boundary, boundary_p, boundary_size);
      end = strchr(boundary, ';');
      if (end==NULL) end = strchr(boundary, '\r');
        if (end==NULL) return HTTP_CONTENT_NONE;
      *end = 0;
      end--;
      if (*end=='\"') *end = 0;
    }
  } else if(strncmp(type_str, http_content_urlencoded, sizeof(http_content_urlencoded)-1)==0) {
	  type = HTTP_CONTENT_URLENCODED;
  }
  return type;
}

void httpd_renew_passwd(void){
  char passwd[64];
  //get_interface_login(passwd);
  get_interface_users_username(0,passwd);

  if((passwd[0]==0)||(passwd[0]==255)){
	  http_passwd_enable = 0;
	  dev.alarm.nopass = 1;
	  set_current_user_rule(ADMIN_RULE);
  }
  else{
	  http_passwd_enable = 1;
	  dev.alarm.nopass = 0;
  }
}

#undef isspace
#if AUTH_BASIC
static int check_authorization(const char *base64pass){
  int authed=0, len;
  static uint8_t LastAuthed=0;
  char pswd[64];
  char login[64];
  char passwd_tmp[128];
  char passwd_tmp2[128];

  memset(login,0,64);
  memset(pswd,0,64);
  memset(passwd_tmp,0,128);

  get_interface_login(login);
  get_interface_passwd(pswd);


  strcat(passwd_tmp,login);
  passwd_tmp[strlen(passwd_tmp)] = ':';
  strcpy(&passwd_tmp[strlen(passwd_tmp)],pswd);

  encode64(passwd_tmp,passwd_tmp2);

  len = strlen(passwd_tmp2);
  if(strncmp(base64pass,passwd_tmp2,len) == 0)
	  authed = 1;
  else
	  authed = 0;

  if((LastAuthed!=authed)&&(authed!=0)){
	  send_events(EVENT_WEBINTERFACE_LOGIN,NULL);
  }
  LastAuthed=authed;

  return authed;
}
#endif

#if AUTH_DIGEST
static u8 get_value_auth(const char *instr,const char *varname,char *outstr){
char *ptr,*ptr2;

	ptr = strstr(instr,varname);
	if(ptr != NULL){
		ptr = strstr(ptr+strlen(varname),"\"");
		if(ptr!=NULL){
			ptr2 = strstr(ptr+1,"\"");
			if(ptr2!=0){
				memcpy(outstr,ptr+1,(ptr2-ptr-1));
				outstr[ptr2-ptr-1] = 0;
				return 0;
			}
		}
	}
	return 1;
}

static u8 get_nc_auth(const char *instr,char *outstr){
char *ptr,*ptr2;

	ptr = strstr(instr,"nc=");
	if(ptr != NULL){
		ptr2 = strstr(ptr+1,",");
		if(ptr2!=0){
			memcpy(outstr,ptr+strlen("nc="),(ptr2-ptr-strlen("nc=")));
			outstr[ptr2-ptr-strlen("nc=")] = 0;
			return 0;
		}
	}
	return 1;
}

static int check_authorization_digest(const char *str, u8 method){
  int authed=0;
  static uint8_t LastAuthed=0;
  char pswd[64];
  char login[64],login_auth[64],digest_uri[128],temp[5];
  char A1[128],A2[128];
  u8 HA1[16],HA2[16],GENRESPONSEhach[16];
  char GENRESPONSE[256],HA1str[33],HA2str[33],nc_val[16],cnonce_val[64],qop_val[64],response[64],opaque[64];

  MD5_CTX mdContext;

  DEBUG_MSG(AUTH_DBG,"check_authorization_digest\r\n");
  DEBUG_MSG(AUTH_DBG,"%s\r\n\r\nd",str);

  if(get_value_auth(str,http_username,login_auth)==0){
	  for(u8 i=0;i<MAX_USERS_NUM;i++){
		  if((get_interface_users_rule(i)==ADMIN_RULE)||(get_interface_users_rule(i)==USER_RULE)){
			  get_interface_users_username(i,login);
			  if(strcmp(login,login_auth) == 0){
				   set_current_user_rule(get_interface_users_rule(i));
				   DEBUG_MSG(AUTH_DBG,"FIND USERNAME %s\r\n",login_auth);
				   get_interface_users_password(i,pswd);
				   break;
			  }
		  }
	  }
  }

  if(get_value_auth(str,http_uri,digest_uri)==0){
	  DEBUG_MSG(AUTH_DBG,"FIND URI %s\r\n",digest_uri);
  }

  if(get_nc_auth(str,nc_val)==0){
	  DEBUG_MSG(AUTH_DBG,"FIND NC %s\r\n",nc_val);
  }

  if(get_value_auth(str,http_cnonce,cnonce_val)==0){
	  DEBUG_MSG(AUTH_DBG,"FIND CNONCE %s\r\n",cnonce_val);
  }

  if(get_value_auth(str,http_response,response)==0){
	  DEBUG_MSG(AUTH_DBG,"FIND RESPONSE %s\r\n",response);
  }

  //if(get_value_auth(str,http_qop,qop_val)==0){
  strcpy(qop_val,"auth");
  DEBUG_MSG(AUTH_DBG,"FIND QOP %s\r\n",qop_val);
  //}


  if(get_value_auth(str,http_opaque,opaque)==0){
	  DEBUG_MSG(AUTH_DBG,"FIND OPAQUE %s\r\n",opaque);
  }

  //make A1
  sprintf(A1,"%s:TFortisPSW:%s",login,pswd);
  MD5Init(&mdContext);
  MD5Update(&mdContext,(u8 *)A1,strlen(A1));
  MD5Final(&mdContext,HA1);

  HA1str[0]=0;
  for(u8 i=0;i<16;i++){
	  sprintf(temp,"%02x",HA1[i]);
	  strcat(HA1str,temp);
  }
  HA1str[32]=0;

  DEBUG_MSG(AUTH_DBG,"MD5 RESULT HA1:%s\r\n",HA1str);

  //make A2
  if(method == HTTP_GET)
	  sprintf(A2,"GET:%s",digest_uri);
  else if(method == HTTP_POST)
	  sprintf(A2,"POST:%s",digest_uri);
  MD5Init(&mdContext);
  MD5Update(&mdContext,(u8 *)A2,strlen(A2));
  MD5Final(&mdContext,HA2);
  HA2str[0]=0;
  for(u8 i=0;i<16;i++){
	  sprintf(temp,"%02x",HA2[i]);
	  strcat(HA2str,temp);
  }
  HA2str[32]=0;
  DEBUG_MSG(AUTH_DBG,"MD5 RESULT HA2:%s\r\n",HA2str);

  //make genresponse
  sprintf(GENRESPONSE,"%s:%s:%s:%s:%s:%s",HA1str,http_nonce_val,nc_val,cnonce_val,qop_val,HA2str);
  DEBUG_MSG(AUTH_DBG,"GENRESPONSE:%s\r\n",GENRESPONSE);
  MD5Init(&mdContext);
  MD5Update(&mdContext,(u8 *)GENRESPONSE,strlen(GENRESPONSE));
  MD5Final(&mdContext,GENRESPONSEhach);
  GENRESPONSE[0]=0;
  for(u8 i=0;i<16;i++){
	  sprintf(temp,"%02x",GENRESPONSEhach[i]);
	  strcat(GENRESPONSE,temp);
  }
  GENRESPONSE[32]=0;
  DEBUG_MSG(AUTH_DBG,"HGENRESPONSE:%s\r\n",GENRESPONSE);
  DEBUG_MSG(AUTH_DBG,"GENRESPONSE:%s\r\n",response);

  if(strcmp(GENRESPONSE,response)==0){
	  DEBUG_MSG(AUTH_DBG,"Authed Ok!\r\n");
	  set_current_username(login);
	  authed = 1;
	  if(http_logout_){
		  authed = 0;
		  http_logout_--;
	  }
  }
  else{
	  authed = 0;
	  DEBUG_MSG(AUTH_DBG,"Authed Fail\r\n");
  }

  if((LastAuthed!=authed)&&(authed!=0)){
	  send_events_u32(EVENT_WEBINTERFACE_LOGIN,0);
  }
  LastAuthed=authed;

  return authed;
}

#endif


static unsigned short generate_part_of_file(void *state){
  struct httpd_state *s = (struct httpd_state *)state;
  if(s->file.len > uip_mss()) {
    s->len = uip_mss();
  } else {
    s->len = s->file.len;
  }
  memcpy(uip_appdata, s->file.data, s->len);
  return s->len;
}

PT_THREAD(send_part_of_file(struct httpd_state *s)){
  PSOCK_BEGIN(&s->sout);
  PSOCK_SEND(&s->sout, s->file.data, s->len);
  PSOCK_END(&s->sout);
}


PT_THREAD(send_bak(struct httpd_state *s)){
  PSOCK_BEGIN(&s->sout);
  do {
    PSOCK_GENERATOR_SEND(&s->sout, generate_part_of_file, s);
    s->file.len -= s->len;
    s->file.data += s->len;
    DEBUG_MSG(UIP_DEBUG,"send_bak %d",s->len);
  } while(s->file.len > 0);

  PSOCK_END(&s->sout);
}

PT_THREAD(send_file(struct httpd_state *s)){
  PSOCK_BEGIN(&s->sout);
  do {
    PSOCK_GENERATOR_SEND(&s->sout, generate_part_of_file, s);
    s->file.len -= s->len;
    s->file.data += s->len;
    DEBUG_MSG(UIP_DEBUG,"send_file %d",s->len);
   } while(s->file.len > 0);

  PSOCK_END(&s->sout);
}



static PT_THREAD(send_headers(struct httpd_state *s, const char *statushdr, int dofil)) {
  char *ptr;

  PSOCK_BEGIN(&s->sout);

  PSOCK_SEND_STR(&s->sout, statushdr);

  if (dofil) {

    ptr = strrchr(s->filename, ISO_period);
    if(ptr == NULL) {
    	//my
    	//PSOCK_SEND_STR(&s->sout,http_content_type_binary);
    	PSOCK_SEND_STR(&s->sout, http_content_type_html);
    } else if(strncmp(http_html, ptr, 5) == 0 ||
        strncmp(http_shtml, ptr, 6) == 0) {
    	PSOCK_SEND_STR(&s->sout, http_content_type_html);
    } else if(strncmp(http_css, ptr, 4) == 0) {
    	PSOCK_SEND_STR(&s->sout, http_content_type_css);
    } else if(strncmp(http_js, ptr, 3) == 0) {
        PSOCK_SEND_STR(&s->sout, http_content_type_js);
    } else if(strncmp(http_bak, ptr, 4) == 0) {
        PSOCK_SEND_STR(&s->sout, http_content_type_bak);
    } else if(strncmp(http_log, ptr, 4) == 0) {
        PSOCK_SEND_STR(&s->sout, http_content_type_log);
    } else if(strncmp(http_png, ptr, 4) == 0) {
    	PSOCK_SEND_STR(&s->sout, http_content_type_png);
    } else if(strncmp(http_gif, ptr, 4) == 0) {
    	PSOCK_SEND_STR(&s->sout, http_content_type_gif);
    } else if(strncmp(http_jpg, ptr, 4) == 0) {
    	PSOCK_SEND_STR(&s->sout, http_content_type_jpg);
    } else if(strncmp(http_json, ptr, 5) == 0) {
    	PSOCK_SEND_STR(&s->sout, http_content_type_json);
    } else if(strncmp(http_bin, ptr, 4) == 0) {
        PSOCK_SEND_STR(&s->sout, http_content_type_bin);
    }else {
      PSOCK_SEND_STR(&s->sout, http_content_type_plain);
    }

  }
  
  PSOCK_END(&s->sout);
}

static void next_scriptstate(struct httpd_state *s){
  char *p;
  p = strchr(s->scriptptr, ISO_nl) + 1;
  s->scriptlen -= (unsigned short)(p - s->scriptptr);
  s->scriptptr = p;
}

static PT_THREAD(handle_script(struct httpd_state *s)){
  char *ptr;
  
  PT_BEGIN(&s->scriptpt);
  
  while(s->file.len > 0) {
    /* Check if we should start executing a script. */
    if(*s->file.data == ISO_percent &&
       *(s->file.data + 1) == ISO_bang) {
      s->scriptptr = s->file.data + 3;
      s->scriptlen = s->file.len - 3;
      if(*(s->scriptptr - 1) == ISO_colon) {

    	//защищаем  httpd_fs_open
		//taskENTER_CRITICAL();
        httpd_fs_open(s->scriptptr + 1, &s->file);
		//taskEXIT_CRITICAL();

        PT_WAIT_THREAD(&s->scriptpt, send_file(s));
      } else {
    	  PT_WAIT_THREAD(&s->scriptpt, httpd_cgi(s->scriptptr)(s, s->scriptptr));
      }
      next_scriptstate(s);
      
      /* The script is over, so we reset the pointers and continue
        sending the rest of the file. */
      s->file.data = s->scriptptr;
      s->file.len = s->scriptlen;
    } else {
      /* See if we find the start of script marker in the block of HTML
          to be sent. */
      if(s->file.len > uip_mss()) s->len = uip_mss();
        else s->len = s->file.len;
      if(*s->file.data == ISO_percent) ptr = strchr(s->file.data + 1, ISO_percent);
        else ptr = strchr(s->file.data, ISO_percent);
      if(ptr != NULL && ptr != s->file.data) {
        s->len = (int)(ptr - s->file.data);
        if(s->len >= uip_mss()) s->len = uip_mss();
      }
      PT_WAIT_THREAD(&s->scriptpt, send_part_of_file(s));
      s->file.data += s->len;
      s->file.len -= s->len;
    }
  }
  PT_END(&s->scriptpt);
}

static PT_THREAD(handle_bak(struct httpd_state *s)){
  char *ptr;

  PT_BEGIN(&s->scriptpt);
  
  while(s->file.len > 0) {
    /* Check if we should start executing a script. */
    if(*s->file.data == ISO_percent &&
       *(s->file.data + 1) == ISO_bang) {
      s->scriptptr = s->file.data + 3;
      s->scriptlen = s->file.len - 3;
      if(*(s->scriptptr - 1) == ISO_colon) {
    	  	httpd_fs_open(s->scriptptr + 1, &s->file);
		   	PT_WAIT_THREAD(&s->scriptpt, send_file(s));
      } else {
    	  	PT_WAIT_THREAD(&s->scriptpt, bak_cgi(s->scriptptr)(s, s->scriptptr));
      }
      next_scriptstate(s);

      /* The script is over, so we reset the pointers and continue
        sending the rest of the file. */
      s->file.data = s->scriptptr;
      s->file.len = s->scriptlen;
    } else {
      /* See if we find the start of script marker in the block of HTML
          to be sent. */
      if(s->file.len > uip_mss()) s->len = uip_mss();
        else s->len = s->file.len;
      if(*s->file.data == ISO_percent) ptr = strchr(s->file.data + 1, ISO_percent);
        else ptr = strchr(s->file.data, ISO_percent);
      if(ptr != NULL && ptr != s->file.data) {
        s->len = (int)(ptr - s->file.data);
        if(s->len >= uip_mss()) s->len = uip_mss();
      }
      PT_WAIT_THREAD(&s->scriptpt, send_part_of_file(s));
      s->file.data += s->len;
      s->file.len -= s->len;
    }
  }
  PT_END(&s->scriptpt);
}

static int out_param_char(struct httpd_state *s, char c){
	if (s->param_ind>=sizeof(s->param)-1) return -1;
	s->param[s->param_ind++] = c;
	return 0;
}

static PT_THREAD(handle_input(struct httpd_state *s)){
  int i;
  static u32 k=0;
  static uint32_t offs, erased_offs;

  PSOCK_BEGIN(&s->sin);
  
  PSOCK_READTO(&s->sin, ISO_space);
  if(strncmp(s->inputbuf, http_get, 4) == 0) {
    s->request_type = HTTP_GET;
  } else if(strncmp(s->inputbuf, http_post, 5) == 0) {
    s->request_type = HTTP_POST;
  } else {
    PSOCK_CLOSE_EXIT(&s->sin);
  }
  s->contenttype = HTTP_CONTENT_NONE;
  

  s->contenttype = HTTP_CONTENT_URLENCODED;//HTTP_CONTENT_NONE;

  PSOCK_READTO(&s->sin, ISO_space);
  if(s->inputbuf[0] != ISO_slash) {
    PSOCK_CLOSE_EXIT(&s->sin);
  }
  
  if(s->inputbuf[1] == ISO_space) {
    strncpy(s->filename, http_index_shtml, sizeof(s->filename));
  } else {
    s->inputbuf[PSOCK_DATALEN(&s->sin) - 1] = 0;
    
    for(i=0 ; s->inputbuf[i]!=0 && s->inputbuf[i] != ISO_question ; i++);
    if( s->inputbuf[i] == ISO_question ) {
      s->inputbuf[i]=0;
      strncpy(s->param,&s->inputbuf[i+1],sizeof(s->param)-1);
    } else s->param[0]=0;
    strncpy(s->filename, &s->inputbuf[0], sizeof(s->filename));
  }
  
  while(1) {
    PSOCK_READTO(&s->sin, ISO_nl);
    s->inputbuf[PSOCK_DATALEN(&s->sin)] = 0;
    if ((s->inputbuf[0] == '\n') || (s->inputbuf[0] == '\r')) break;
    if(strncmp(s->inputbuf, http_content_length, sizeof(http_content_length)-1) == 0) 
      s->content_length = strtol(&s->inputbuf[sizeof(http_content_length)-1], NULL, 10);
#if AUTH_BASIC
    if(strncmp(s->inputbuf, http_authorization, sizeof(http_authorization)-1) == 0)
      s->is_authorized = check_authorization(&s->inputbuf[ sizeof(http_authorization)-1 ]);
#else
    //auth DIGEST
    if (http_passwd_enable == 1) {
		if(strncmp(s->inputbuf, http_authorization, sizeof(http_authorization)-1) == 0)
		  s->is_authorized = check_authorization_digest(&s->inputbuf[sizeof(http_authorization)-1 ],s->request_type);
    }else
    	s->is_authorized = 0;
#endif
    if(strncmp(s->inputbuf, http_content_type, sizeof(http_content_type)-1) == 0) {
      s->contenttype = parse_contenttype(&s->inputbuf[ sizeof(http_content_type)-1 ], s->boundary, sizeof(s->boundary)-1);
      if (s->contenttype==HTTP_CONTENT_NONE) PSOCK_CLOSE_EXIT(&s->sin);
    }
  }
  
  if(s->request_type != HTTP_POST)
	  s->content_length = 0;
  
  if (s->content_length){
	  //printf("post\r\n");

	  if (s->contenttype==HTTP_CONTENT_URLENCODED){
		  s->param_ind = 0;
		  while (s->content_length > 0) {
			  s->sin.bufsize = s->content_length > (sizeof(s->inputbuf)-1) ? (sizeof(s->inputbuf)-1) : s->content_length;
			  if (s->sin.bufsize>(sizeof(s->param)-1-s->param_ind)) s->sin.bufsize = sizeof(s->param)-1-s->param_ind;
			  PSOCK_READBUF(&s->sin);
			  s->content_length -= PSOCK_DATALEN(&s->sin);
			  strncpy(&s->param[s->param_ind], s->inputbuf, PSOCK_DATALEN(&s->sin));
			  s->param_ind+=PSOCK_DATALEN(&s->sin);
			  if (s->param_ind>=sizeof(s->param)-1) break;
			}
	  } else if (s->contenttype==HTTP_CONTENT_FORMDATA){
		    s->param_ind = 0;
		    s->parse_state = HTTP_PARSE_IDLE;

		    //printf("multipart form data\r\n");

			while (s->content_length > 0) {

			//printf("11 content_length\r\n");

			  char *cp;
			  PSOCK_READTO(&s->sin, ISO_nl);
			  s->inputbuf[PSOCK_DATALEN(&s->sin)] = 0;
			  if (s->content_length<PSOCK_DATALEN(&s->sin)) s->content_length = 0;
				else s->content_length -= PSOCK_DATALEN(&s->sin);
			  if (strncmp(s->inputbuf, "--", 2)==0) {
				if (strncmp(&s->inputbuf[2], s->boundary, strlen(s->boundary))==0) {
				  if (strncmp(&s->inputbuf[2+strlen(s->boundary)], "--", 2)==0){
					  s->parse_state = HTTP_PARSE_IDLE;
					  continue;
				  } else {
					  s->parse_state = HTTP_PARSE_WAIT_DISPOSITION;
					  continue;
				  }
				}
			  }
			  if (s->parse_state==HTTP_PARSE_WAIT_DISPOSITION){
				  if (strncmp(s->inputbuf, content_disposition, sizeof(content_disposition)-1)==0) {

					  //printf("content disposition\r\n");

					  cp = &s->inputbuf[sizeof(content_disposition)-1];
					  if (strncmp(cp, form_data, sizeof(form_data)-1)==0){
						  cp = strstr(cp, "name=\"");
						  if (cp==NULL){s->parse_state = HTTP_PARSE_IDLE; continue;}
						  cp+=strlen("name=\"");
						  if (strncmp(cp, "updatefile", 10)==0) {
							  s->parse_state = HTTP_PARSE_WAIT_OCTET_STREM;
							  //printf("content disposition updatefile\r\n");
							  continue;
						  }
						  if (s->param_ind!=0) out_param_char(s, '&');
						  while((*cp)&&(*cp!='\"')) if (out_param_char(s, *cp++)<0) break;
						  if (*cp=='\"') s->parse_state = HTTP_PARSE_WAIT_NL1;
						  out_param_char(s, '=');
					  }
				  } else s->parse_state = HTTP_PARSE_IDLE;
			  } else
			  if (s->parse_state==HTTP_PARSE_WAIT_NL1){
				  s->parse_state = HTTP_PARSE_WAIT_PARAM;
			  } else
			  if (s->parse_state==HTTP_PARSE_WAIT_PARAM){
				  cp = s->inputbuf;
				  while((*cp)&&(*cp!='\r')) if (out_param_char(s, *cp++)<0) break;
				  s->param[s->param_ind] = 0;
				  s->parse_state = HTTP_PARSE_IDLE;
			  } else
			  if (s->parse_state==HTTP_PARSE_WAIT_OCTET_STREM){
				  //printf("content type 2\r\n");
				  if (strncmp(s->inputbuf, content_type, sizeof(content_type)-1)==0) {
					  cp = &s->inputbuf[sizeof(content_type)-1];
					  if (strncmp(cp, http_content_octet_stream, sizeof(http_content_octet_stream)-1)==0){
						s->parse_state = HTTP_PARSE_WAIT_UPLOAD;
						//printf("content type http_content_octet_stream\r\n");
					  }
				  } else s->parse_state = HTTP_PARSE_IDLE;
			  } else
			  if (s->parse_state==HTTP_PARSE_WAIT_UPLOAD){
  				  uint32_t esize;
				  uint16_t len;
				  //DEBUG_MSG(PRINTF_DEB,"it_is_eeprom = %d %d\r\n",it_is_eeprom,it_is_save);

				  if (update_state==UPDATE_STATE_IDLE){
					  if(it_is_save==1 || it_is_eeprom==1){
						  spi_flash_properties(NULL,NULL,&esize);
						  erased_offs = 0;
						  spi_flash_erase(FL_TMP_START,esize);
						  it_is_save=0;
						  it_is_eeprom = 0;
						  //IWDG_ReloadCounter();
					  }
					  else
					  {
						  //erase all temp place
						  //spi_flash_properties(NULL,NULL,&esize);
						  //erased_offs = 0;
						  //for(u8 i=0;i<(FL_TMP_END/(esize*2));i++){
						  //	  spi_flash_erase(FL_TMP_START+esize*i,esize);
						  //    IWDG_ReloadCounter();
						  //}
					  }
					  //update_state = UPDATE_STATE_UPLOAD;
					  update_state=UPDATE_STATE_UPLOADING;
					  timer_set(&update_timer, HTTP_UPDATE_TIMER*MSEC);
				  } else
					  PSOCK_CLOSE_EXIT(&s->sout);


				  offs = 0;
				  file_size = s->content_length;
				  //DEBUG_MSG(PRINTF_DEB,"s->content_length %lu\r\n",s->content_length);

				  while (s->content_length > 0) {
					  s->sin.bufsize = s->content_length > (sizeof(s->inputbuf)-1) ? (sizeof(s->inputbuf)-1) : s->content_length;
					  PSOCK_READBUF(&s->sin);
					  len = PSOCK_DATALEN(&s->sin);
					  s->content_length -= len;

					  if (update_state==UPDATE_STATE_UPLOADING){
						  /*if((offs+len) >= erased_offs){
							  //spi_flash_properties(NULL,NULL,&esize);
							  //spi_flash_erase(FL_TMP_START+erased_offs,esize);
							  //DEBUG_MSG(PRINTF_DEB,"erase %lu/%lu\r\n",offs,erased_offs);
							  //erased_offs += esize;
							  vTaskDelay(10);
							  IWDG_ReloadCounter();
						  }*/
						  if((FL_TMP_START + offs)<(FL_TMP_END-esize))
							  spi_flash_write(FL_TMP_START + offs, len, s->inputbuf);

						  timer_restart(&update_timer);
						  set_progress(offs);
					  }
					  k++;
					  offs+=len;

					  //DEBUG_MSG(PRINTF_DEB,"download %d/%lu\r\n",len,file_size);
				  }


				  //DEBUG_MSG(PRINTF_DEB,"download compleat\r\n");
				  s->parse_state = HTTP_PARSE_WAIT_UPLOADED;
				  update_state = UPDATE_STATE_UPLOADED;
			  }
			}
	  }
  }

  s->state = STATE_OUTPUT;
  
  PSOCK_END(&s->sin);
}




static PT_THREAD(handle_output(struct httpd_state *s)){
  char *ptr;
  int ret;
  
  PT_BEGIN(&s->outputpt);

	if (http_passwd_enable == 1) {
		if (!(s->is_authorized)) {
		  PT_WAIT_THREAD(&s->outputpt, send_headers(s, http_header_401, 0));
		  goto endhandleoutput;
		}
	}



	ret=httpd_fs_open(s->filename, &s->file);
    if(ret == 0)
		  httpd_fs_open(http_404_html, &s->file);



    if(strncmp(s->filename,"/api",4)==0){



    	if((strncmp(s->filename, http_getInput,strlen(http_getInput)-1)==0)||
    		(strncmp(s->filename, http_getLink,strlen(http_getLink)-1)==0)||
    		(strncmp(s->filename, http_setPoe,strlen(http_setPoe)-1)==0)||
			(strncmp(s->filename, http_getPoe,strlen(http_getPoe)-1)==0)||
			(strncmp(s->filename, http_getPoePower,strlen(http_getPoePower)-1)==0)||
			(strncmp(s->filename, http_getPortNum,strlen(http_getPortNum)-1)==0)||
			(strncmp(s->filename, http_isUps,strlen(http_isUps)-1)==0)||
			(strncmp(s->filename, http_getUpsStatus,strlen(http_getUpsStatus)-1)==0)||
			(strncmp(s->filename, http_getUpsVoltage,strlen(http_getUpsVoltage)-1)==0)||
			(strncmp(s->filename, http_getUpsEstimated,strlen(http_getUpsEstimated)-1)==0)||
			(strncmp(s->filename, http_getNetIp,strlen(http_getNetIp)-1)==0)||
			(strncmp(s->filename, http_getNetMac,strlen(http_getNetMac)-1)==0)||
			(strncmp(s->filename, http_getNetMask,strlen(http_getNetMask)-1)==0)||
			(strncmp(s->filename, http_getNetGate,strlen(http_getNetGate)-1)==0)||
			(strncmp(s->filename, http_getFwVersion,strlen(http_getFwVersion)-1)==0)||
			(strncmp(s->filename, http_getDevType,strlen(http_getDevType)-1)==0)||
			(strncmp(s->filename, http_getDevName,strlen(http_getDevName)-1)==0)||
			(strncmp(s->filename, http_getDevLocation,strlen(http_getDevLocation)-1)==0)||
			(strncmp(s->filename, http_getPortMacList,strlen(http_getPortMacList)-1)==0)||
			(strncmp(s->filename, http_cableTesterStart,strlen(http_cableTesterStart)-1)==0)||
			(strncmp(s->filename, http_cableTesterStatus,strlen(http_cableTesterStatus)-1)==0)||
			(strncmp(s->filename, http_getSerialNum,strlen(http_getSerialNum)-1)==0)||
			(strncmp(s->filename, http_getUptime,strlen(http_getUptime)-1)==0)||
			(strncmp(s->filename, http_getDevContact,strlen(http_getDevContact)-1)==0)||
			(strncmp(s->filename, http_rebootAll,strlen(http_rebootAll)-1)==0)){



    		PT_WAIT_THREAD(&s->outputpt, send_headers(s, http_header_200, 1));
    		PT_INIT(&s->scriptpt);
    		PT_WAIT_THREAD(&s->outputpt, handle_script(s));
    	}

    }else {
        PT_WAIT_THREAD(&s->outputpt, send_headers(s, http_header_200, 1));

        ptr = strchr(s->filename, ISO_period);

        //для shtml и json
        if(ptr != NULL && ((strncmp(ptr, http_shtml, 6)==0) || (strncmp(ptr, http_json, 5) == 0))) {
        	PT_INIT(&s->scriptpt);
        	PT_WAIT_THREAD(&s->outputpt, handle_script(s));
        }else if(ptr != NULL && strncmp(ptr, http_bak, 4) == 0) {
        	PT_INIT(&s->scriptpt);
        	PT_WAIT_THREAD(&s->outputpt, handle_bak(s));
        }else if(ptr != NULL && strncmp(ptr, http_log, 4) == 0) {
         	PT_INIT(&s->scriptpt);
         	PT_WAIT_THREAD(&s->outputpt, handle_bak(s));
        }else if(ptr != NULL && strncmp(ptr, http_bin, 4) == 0) {
         	PT_INIT(&s->scriptpt);
         	PT_WAIT_THREAD(&s->outputpt, handle_bak(s));
        }else{
        	PT_WAIT_THREAD(&s->outputpt, send_file(s));
        }

    }


  
endhandleoutput:
  PSOCK_CLOSE(&s->sout);
  PT_END(&s->outputpt);
}

static void handle_connection(struct httpd_state *s){
  handle_input(s);
  if(s->state == STATE_OUTPUT) {
    handle_output(s);
  }
}

void httpd_appcall(void){
  struct httpd_state *s = (struct httpd_state *)&(uip_conn->appstate.httpd_state);

	if(uip_hostaddr[0] == 0 && uip_hostaddr[1]==0)
		return;

    if(uip_closed() || uip_aborted() || uip_timedout()) {
    	if ((s->parse_state==HTTP_PARSE_WAIT_UPLOAD)||(s->parse_state==HTTP_PARSE_WAIT_UPLOADED))
    		update_state = UPDATE_STATE_IDLE;
    } else if(uip_connected()) {
        PSOCK_INIT(&s->sin, s->inputbuf, sizeof(s->inputbuf) - 1);
        PSOCK_INIT(&s->sout, s->inputbuf, sizeof(s->inputbuf) - 1);
        PT_INIT(&s->outputpt);
        s->state = STATE_WAITING;
        s->parse_state = 0;
        s->timer = 0;
        s->param_ind = 0;
        s->content_length = 0;
        s->is_authorized = 0;
        handle_connection(s);
      } else if(s != NULL) {
          if(uip_poll()) {
            ++s->timer;
            if(s->timer >= 30) uip_abort();
          } else s->timer = 0;
          handle_connection(s);
      } else {
        uip_abort();
      }
}

void httpd_init(void) {
  httpd_renew_passwd();
  uip_listen(HTONS(80));
}

void tosave(void){
	it_is_save=1;
}

void nosave(void){
	it_is_save=0;
}

void set_progress(u32 offset){
	progress_bar = offset;
}
u32 get_progress(void){
	if(file_size != 0){
		return (u32)(progress_bar*100/file_size);
	}
	return 0;
}

void http_logout(void){
	http_logout_ = 1;
}
