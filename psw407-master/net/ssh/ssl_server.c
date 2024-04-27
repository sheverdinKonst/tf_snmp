/**
  ******************************************************************************
  * @file    SSL_server.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    31-October-2011
  * @brief   SSL Server main task
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************  
  */
/* Includes ------------------------------------------------------------------*/


#include "deffines.h"
#if SSL_USE

#include <stdlib.h>
/* PolarSSL includes */
#include "polarssl/include/polarssl/net.h"
#include "polarssl/include/polarssl/ssl.h"
#include "polarssl/include/polarssl/havege.h"
#include "polarssl/include/polarssl/certs.h"
#include "polarssl/include/polarssl/x509.h"
#include "ssl_server.h"
#include "stm32f4xx_rng.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define DEBUG_LEVEL 0  /* Set DEBUG_LEVEL if you want to enable SSL debug
                         option, this should be set to 2, 3, 4 or 5 */
                         
#define HTTP_RESPONSE "<p><p>Successful connection using: %s\r\n"

/* Format of dynamic web page */
#define PAGE_START \
"<html>\
<head>\
</head>\
<BODY onLoad=\"window.setTimeout(&quot;location.href='index.html'&quot;,1000)\" bgcolor=\"#FFFFFF\" text=\"#000000\">\
<font size=\"4\" color=\"#0000FF\"><b>STM32F417xx : SSL Server Demo using HW Crypto</font></b></i>\
<br><br><pre>\r\nPage Hits = "

#define PAGE_END \
" \r\n</pre><br><br><hr>\
<font size=\"3\" color=\"#808080\">All rights reserved �2011 STMicroelectronics\
\r\n</font></BODY>\
</html>"

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* SSL structures */
//rng_state rngs;
ssl_context ssl;
ssl_session ssn;
x509_cert srvcert;
rsa_context rsa;
uint32_t nPageHits = 0;

/* Private functions ---------------------------------------------------------*/
/*
 * Computing a "safe" DH-1024 prime can take a very
 * long time, so a precomputed value is provided below.
 * You may run dh_genprime to generate a new value.
 */

char *my_dhm_P = 
  "E4004C1F94182000103D883A448B3F80" \
  "2CE4B44A83301270002C20D0321CFD00" \
  "11CCEF784C26A400F43DFB901BCA7538" \
  "F2C6B176001CF5A0FD16D2C48B1D0C1C" \
  "F6AC8E1DA6BCC3B4E1F96B0564965300" \
  "FFA1D0B601EB2800F489AA512C4B248C" \
  "01F76949A60BB7F00A40B1EAB64BDD48" \
  "E8A700D60B7F1200FA8E77B0A979DABF";

char *my_dhm_G = "4";

/*
 * Sorted by order of preference
 */
int my_ciphersuites[] =
{
  SSL_EDH_RSA_AES_256_SHA,
  SSL_EDH_RSA_CAMELLIA_256_SHA,
  SSL_EDH_RSA_AES_128_SHA,
  SSL_EDH_RSA_CAMELLIA_128_SHA,
  SSL_EDH_RSA_DES_168_SHA,
  SSL_RSA_AES_256_SHA,
  SSL_RSA_CAMELLIA_256_SHA,
  SSL_RSA_AES_128_SHA,
  SSL_RSA_CAMELLIA_128_SHA,
  SSL_RSA_DES_168_SHA,
  SSL_RSA_RC4_128_SHA,
  SSL_RSA_RC4_128_MD5,
  0
};

void my_debug(void *ctx, int level, const char *str)
{
  if(level < DEBUG_LEVEL)
  {
    printf("%s", str);  
  }
}

/*
 * These session callbacks use a simple chained list
 * to store and retrieve the session information.
 */
ssl_session *s_list_1st = NULL;
ssl_session *cur, *prv;

static int my_get_session(ssl_context *ssl)
{
  time_t t = time(NULL);

  if(ssl->resume == 0)
    return(1);

  cur = s_list_1st;
  prv = NULL;

  while(cur != NULL)
  {
    prv = cur;
    cur = cur->next;

    if(ssl->timeout != 0 && t - prv->start > ssl->timeout)
      continue;

    if( ssl->session->ciphersuite != prv->ciphersuite ||
        ssl->session->length != prv->length)
      continue;

    if(memcmp( ssl->session->id, prv->id, prv->length ) != 0)
      continue;

    memcpy(ssl->session->master, prv->master, 48);
      return(0);
  }

  return(1);
}

static int my_set_session(ssl_context *ssl)
{
  time_t t = time(NULL);

  cur = s_list_1st;
  prv = NULL;

  while(cur != NULL)
  {
    if(ssl->timeout != 0 && t - cur->start > ssl->timeout)
      break; /* expired, reuse this slot */

    if(memcmp( ssl->session->id, cur->id, cur->length ) == 0)
      break; /* client reconnected */

    prv = cur;
    cur = cur->next;
  }

  if(cur == NULL)
  {
    cur = (ssl_session *) malloc(sizeof(ssl_session));
    if(cur == NULL)
      return(1);

    if(prv == NULL)
      s_list_1st = cur;
    else  prv->next  = cur;
  }

  memcpy(cur, ssl->session, sizeof(ssl_session));

  return(0);
}

/**
  * @brief  SSL Server task.
  * @param  pvParameters not used
  * @retval None
  */
void ssl_server(void *pvParameters)
{
  int ret, len;
  int listen_fd;
  int client_fd;
  unsigned char buf[1024];
 
  memset(&srvcert, 0, sizeof( x509_cert));
    
  /* 1. Load the certificates and private RSA key */
  DEBUG_MSG(SSL_DEBUG,"\n\r Loading the server certificates and key...");
    
  /*
   * This demonstration program uses embedded test certificates.
   * Instead, you may want to use x509parse_crtfile() to read the
   * server and CA certificates, as well as x509parse_keyfile().
  */
  ret = x509parse_crt(&srvcert, (unsigned char *) test_srv_crt, strlen(test_srv_crt));
  if(ret != 0)
  {
	DEBUG_MSG(SSL_DEBUG," failed\n  !  x509parse_crt returned %d\n\r", ret);
    goto exit;
  }
  ret = x509parse_crt(&srvcert, (unsigned char *) test_ca_crt, strlen(test_ca_crt));
  if(ret != 0)
  {
	DEBUG_MSG(SSL_DEBUG," failed\n  !  x509parse_crt returned %d\n\r", ret);
    goto exit;
  }
  rsa_init( &rsa, RSA_PKCS_V15, 0 );
  ret =  x509parse_key(&rsa, (unsigned char *) test_srv_key, strlen(test_srv_key), NULL, 0);
  if( ret != 0 )
  {
	DEBUG_MSG(SSL_DEBUG," failed\n  !  x509parse_key returned %d\n\r", ret);
    goto exit;
  }
  DEBUG_MSG(SSL_DEBUG," ok\n\r");

  /* 2. Setup the listening TCP socket */
  DEBUG_MSG(SSL_DEBUG,"\n\r Bind to https port ...");
  
  /* Bind the connection to https port : 443 */ 
  ret = net_bind(&listen_fd, NULL, 443); 
  if(ret != 0)
  {
	DEBUG_MSG(SSL_DEBUG," failed\n  ! net_bind returned %d\n\r", ret);
    goto exit;
  }
  DEBUG_MSG(SSL_DEBUG," ok\n\r");
    
  /* 3. Wait until a client connects */
  for(;;)
  {
    /* LED2 on : Bint to https port done */ 
    //STM_EVAL_LEDOn(LED2);
    
    DEBUG_MSG(SSL_DEBUG,"\n\r Waiting for a remote connection ...");
    
    /* Accept the remote connection */ 
    ret = net_accept(listen_fd, &client_fd, NULL);
    if(ret != 0)
    {
      /* Connection failed */
      DEBUG_MSG(SSL_DEBUG," failed\n  ! net_accept returned %d\n\n", ret);
      goto exit;
    }
    DEBUG_MSG(SSL_DEBUG," ok\n");

    /* LED1 on : Accept the remote connection done */ 
    //STM_EVAL_LEDOn(LED1);

    /* 4. Initialize the session data */
    DEBUG_MSG(SSL_DEBUG,"\n\r Setting up the RNG and SSL data....");

    /* Initialize the SSL context */
    ret = ssl_init(&ssl);
    if(ret != 0)
    {
      /* SSL initialization failed */
      DEBUG_MSG(SSL_DEBUG," failed\n  ! ssl_init returned %d\n\n", ret);
      goto accept;
    }
    DEBUG_MSG(SSL_DEBUG," ok\n");

    /* Set the current session as SSL server */
    ssl_set_endpoint(&ssl, SSL_IS_SERVER);
 
    /* No certificate verification */
    ssl_set_authmode(&ssl, SSL_VERIFY_NONE);

    /* Set the random number generator callback function */
//  ssl_set_rng(&ssl, RandVal, &rngs);

    /* Set the debug callback function */
    ssl_set_dbg(&ssl, my_debug, stdout);
  
    /* Set read and write callback functions */
    ssl_set_bio(&ssl, net_recv, &client_fd, net_send, &client_fd);
    
    /* Set the session callback functions */
    ssl_set_scb(&ssl, my_get_session, my_set_session);

    /* The list of ciphersuites to be used in this session */
    ssl_set_ciphersuites(&ssl, my_ciphersuites);
  
    /* Set the session resuming flag, timeout and session context */
    ssl_set_session(&ssl, 1, 0, &ssn);
    memset(&ssn, 0, sizeof(ssl_session));

    /* Set the data required to verify peer certificate */
    ssl_set_ca_chain(&ssl, srvcert.next, NULL, NULL);

    /* Set own certificate and private key */
    ssl_set_own_cert(&ssl, &srvcert, &rsa);

    /* Set the Diffie-Hellman public P and G values */ 
    ssl_set_dh_param(&ssl, my_dhm_P, my_dhm_G);

    /* 5. Handshake protocol */
    DEBUG_MSG(SSL_DEBUG,"\n\r Performing the SSL/TLS handshake...");

    /* Perform the SSL handshake protocol */
    while((ret = ssl_handshake(&ssl)) != 0)
    {
      if(ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE)
      {
    	DEBUG_MSG(SSL_DEBUG," failed\n  ! ssl_handshake returned %d\n\n", ret);
        goto accept;
      }
    }
    DEBUG_MSG(SSL_DEBUG," ok\n");

    /* 6. Read the HTTP Request */
    DEBUG_MSG(SSL_DEBUG,"\n\r <= Read from client :");
    do
    {
      len = sizeof(buf) - 1;
      memset(buf, 0, sizeof(buf));

      /* Read decrypted application data */
      ret = ssl_read(&ssl, (unsigned char*)buf, len);

      if(ret == POLARSSL_ERR_NET_WANT_READ || ret == POLARSSL_ERR_NET_WANT_WRITE)
        continue;

      if(ret <= 0)
      {
        switch(ret)
        {
          case POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY:
        	DEBUG_MSG(SSL_DEBUG,"\n\r connection was closed \n");
            break;

          case POLARSSL_ERR_NET_CONN_RESET:
        	DEBUG_MSG(SSL_DEBUG,"\n\r connection was reset by peer\n");
            break;

          default:
        	DEBUG_MSG(SSL_DEBUG,"\n\r ssl_read returned %d\n", ret);
            break;
        }
        break;
      }
      len = ret;
      /* Display the length of read data */
      DEBUG_MSG(SSL_DEBUG,"\n\r Successfully read %d bytes from client \n\r",len);
    }while(0);

    /* 7. Write the Response */
    DEBUG_MSG(SSL_DEBUG,"\n\r => Write to client :\n\r");

    /* Send the dynamic html page */    
    ssl_DynPage(&ssl);

    /* LED3 on : End of connection */ 
    //STM_EVAL_LEDOn(LED3);
    
    /* Close the connection */
    ssl_close_notify(&ssl);
    goto accept;

exit:
   /* Close and delete the current session data: certificate, RSA key and SSL session */
    x509_free(&srvcert);
    rsa_free(&rsa);
    cur = s_list_1st;
    while(cur != NULL)
    {
      prv = cur;
      cur = cur->next;
      memset(prv, 0, sizeof(ssl_session));
      free(prv);
    }
    memset(&ssl, 0, sizeof(ssl_context));

accept:
    /* Wait 200s until next retry */
    vTaskDelay(200); 

    /* Turn off LED1, LED2 and LED3 */
    //STM_EVAL_LEDOff(LED1);
    //STM_EVAL_LEDOff(LED2);
    //STM_EVAL_LEDOff(LED3);

    /* Close and free the SSL context */
    net_close(client_fd);
    ssl_free(&ssl);
  }
}           

/**
  * @brief  Create and send a dynamic Web Page.  This page contains the list of 
  *         running tasks and the number of page hits. 
  * @param  ssl the SSL context
  * @retval None
  */
void ssl_DynPage(ssl_context *ssl)
{
  portCHAR buf[1024];
  portCHAR pagehits[10];
  portCHAR getcipher[100];
  uint32_t len = 0;

  memset(buf, 0, 1024);

  /* Update the hit count */
  nPageHits++;
  sprintf( pagehits, "%d", nPageHits );
  sprintf( (char *) getcipher, HTTP_RESPONSE, ssl_get_ciphersuite(ssl));   
  
  /* Generate the dynamic page */
  strcpy(buf, PAGE_START);

  /* Page header */
  strcat(buf, pagehits);
  strcat((char *) buf, "<br><pre>** The list of tasks and their status **");
  strcat((char *) buf, "<br><pre>---------------------------------------------"); 
  strcat((char *) buf, "<br>Name          State  Priority  Stack   Num" );
  strcat((char *) buf, "<br>---------------------------------------------"); 
    
  /* The list of tasks and their status */
  vTaskList((signed char *)buf + strlen(buf));
  strcat((char *) buf, "<br>---------------------------------------------"); 
  strcat((char *) buf, "<br>B : Blocked, R : Ready, D : Deleted, S : Suspended");

  /* The current cipher used */
  strcat(buf, getcipher);

  /* Page footer */
  strcat(buf, PAGE_END);
  
  /* Send the dynamically generated page */
  len = ssl_write(ssl, (unsigned char *)buf, strlen(buf));

  /* Display the length of application data */
  printf( "\n Successfully write %d bytes to client\n\r", len);
}

/**
  * @brief  This function is used to send messages with size upper 16k bytes.
  * @param  ssl  SSL context
  * @param  data buffer holding the data
  * @param  len  how many bytes must be written
  * @retval None
  */
void ssl_sendframes( ssl_context *ssl, char *data, int datalen )
{
  int index = 0;
  int k = 0;
  int lastframe, nbrframes;

  /* Calculate the number of frames */
  nbrframes = datalen / 16000; 
  
  /* Send nbrframes frames */
  while(nbrframes > 0)
  {
    index = k * 16000;
    ssl_write( ssl, (unsigned char *)(data + index), 16000 );
    nbrframes--;
    k++;
  }
  /* Send the last frame */
  index = k * 16000;
  lastframe = datalen % 16000 ;
  ssl_write( ssl, (unsigned char *)(data + index), lastframe );
}

/**
  * @brief  Returns a 32-bit random number.
  * @param  arg not used
  * @retval 32-bit random number
  */
int RandVal(void* arg){

  uint32_t ret; 

  /* Wait until random number is ready */
  while(RNG_GetFlagStatus(RNG_FLAG_DRDY)== RESET);
  
  /* Get the random number */       
  ret = RNG_GetRandomNumber();

  /* Return the random number */ 
  return(ret);
}

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

#endif
