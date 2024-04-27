/******************************************************************************
 * Fichier Module :util.c - An IGMPv3-router implementation 
 ******************************************************************************
 * Fichier    :	util.c
 * Description: Implementation des divers routines
 *		Mars 2001
 * Date       :	May 18, 2000
 * Auteur    : lahmadi@loria.fr
 * Last Modif : Aout 2,  2001
 *
 *****************************************************************************/
#if IGMPV3_DEBUG

#include <stdarg.h>
/*#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/route.h>*/

#include "include/igmprt.h"
#include <bcmnvram.h>

int log_level;

/*
 * Set/reset the IP_MULTICAST_LOOP. Set/reset is specified by "flag".
 */
void k_set_loop(socket, flag)
	int socket;
    int flag;
{
    u_char loop;

    loop = flag;
    if (setsockopt(socket, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loop, sizeof(loop)) < 0)
    	DEBUG_MSG(IGMPV3_DEBUG,"setsockopt IP_MULTICAST_LOOP %u\n", loop);
}

/*
 * Set the IP_MULTICAST_IF option on local interface ifa.
 */
void k_set_if(socket, ifa)
    int socket;
    u_long ifa;
{
    struct in_addr adr;

    adr.s_addr = ifa;
    if (setsockopt(socket, IPPROTO_IP, IP_MULTICAST_IF,
		   (char *)&adr, sizeof(adr)) < 0)
    	DEBUG_MSG(IGMPV3_DEBUG,"setsockopt IP_MULTICAST_IF %s\n");
}


/*
 * void debug(int level)
 *
 * Write to stdout
 */
void debug(int level, const char* fmt, ...)
{
	va_list args;

	if (level < log_level)
		return;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

/*
 * u_short in_cksum(u_short *addr, int len)
 *
 * Compute the inet checksum
 */
unsigned short in_cksum(unsigned short *addr, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = addr;
    unsigned short answer = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }
    if (nleft == 1) {
        *(unsigned char*)(&answer) = *(unsigned char*)w;
        sum += answer;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    answer = ~sum;
    return (answer);
}

/*
 * interface_list_t* get_interface_list(short af, short flags, short unflags)
 *
 * Get the list of interfaces with an address from family af, and whose flags 
 * match 'flags' and don't match 'unflags'. 
 */
interface_list_t* get_interface_list(void)
{
	struct interface_list_t ifr;
	/*
    char *p, buf[10];
    interface_list_t *ifp, *ifprev, *list;
    struct sockaddr *psa;

    int sockfd;
    int i, err;

    list = ifp = ifprev = NULL;
    for (i=1; ; i++) {
        p = if_indextoname(i, buf);
        if (p == NULL)
            break;
	if (!nvram_nmatch("0","%s_bridged",p))
		continue;
	if (nvram_nmatch("0","%s_multicast",p))
		continue;
        strncpy(ifr.ifr_name, p, IFNAMSIZ);
        err = ioctl(sockfd, SIOCGIFADDR, (void*)&ifr);
        psa = &ifr.ifr_ifru.ifru_addr;
        if (err == -1 || psa->sa_family != af)
            continue;
        err = ioctl(sockfd, SIOCGIFFLAGS, (void*)&ifr);
        if (err == -1)
            continue;
        if (((ifr.ifr_flags & flags) != flags) ||
            ((ifr.ifr_flags & unflags) != 0))
            continue;
        ifp = (interface_list_t*) malloc(sizeof(*ifp));
        if (ifp) {
            strncpy(ifp->ifl_name, ifr.ifr_name, IFNAMSIZ);
            memcpy(&ifp->ifl_addr, psa, sizeof(*psa));
            ifp->ifl_next = NULL;
            if (list == NULL)
                list = ifp;
            if (ifprev != NULL)
                ifprev->ifl_next = ifp;
            ifprev = ifp;
        }
    }*/
    for(u8 i=0;i<ALL_PORT_NUM;i++){
    	if(L2F_port_conv(i)!=-1){
			ifp = (interface_list_t*) malloc(sizeof(*ifp));
			if (ifp) {
				strcpy(ifr.ifr_name,"ifname");
				ifr.ifn = L2F_port_conv(i);
			}
    	}
    }

    //close(sockfd);
    return list;
}

/*
 * void free_interface_list(interface_list_t *ifl)
 *
 * Free a list of interfaces
 */
void free_interface_list(interface_list_t *ifl)
{
    interface_list_t *ifp = ifl;

    while (ifp) {
        ifl = ifp;
        ifp = ifp->ifl_next;
        free(ifl);
    }
}

/*
 * short get_interface_flags(char *ifname)
 *
 * Get the value of the flags for a certain interface 
 */
short get_interface_flags(char *ifname)
{
    struct ifreq ifr;
    int sockfd, err;

    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd <= 0)
        return -1;
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	err = ioctl(sockfd, SIOCGIFFLAGS, (void*)&ifr);
	close(sockfd);
	if (err == -1)
		return -1;
	return ifr.ifr_flags; 
}

/*
 * short set_interface_flags(char *ifname, short flags)
 *
 * Set the value of the flags for a certain interface 
 */
short set_interface_flags(char *ifname, short flags)
{
    struct ifreq ifr;
    int sockfd, err;

    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd <= 0)
        return -1;
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_flags = flags; 
	err = ioctl(sockfd, SIOCSIFFLAGS, (void*)&ifr);
	close(sockfd);
	if (err == -1)
		return -1;
	return 0; 
}

/*
 * short get_interface_flags(char *ifname)
 *
 * Get the value of the flags for a certain interface 
 */
int get_interface_mtu(char *ifname)
{
    struct ifreq ifr;
    int sockfd, err;

    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd <= 0)
        return -1;
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	err = ioctl(sockfd, SIOCGIFMTU, (void*)&ifr);
	close(sockfd);
	if (err == -1)
		return -1;
	return ifr.ifr_mtu; 
}

/*
 * int mrouter_onoff(int sockfd, int onoff)
 *
 * Tell the kernel if a multicast router is on or off 
 */
int mrouter_onoff(int sockfd, int onoff)
{
    int err, cmd, i;

	cmd = (onoff) ? MRT_INIT : MRT_DONE; 
	i = 1;
	err = setsockopt(sockfd, IPPROTO_IP, MRT_INIT, (void*)&i, sizeof(i));
	return err; 
}


/*
 * function name: wordToOption
 * input: char *word, a pointer to the word
 * output: int; a number corresponding to the code of the word
 * operation: converts the result of the string comparisons into numerics.
 * comments: called by config_vifs_from_file()
 */
int 
wordToOption(word)
    char *word;
{
  if (EQUAL(word, ""))
    return EMPTY;
  if (EQUAL(word, "igmpversion"))
    return IGMPVERSION;
  if (EQUAL(word, "is_querier"))
    return IS_QUERIER;
  if (EQUAL(word,"upstream"))
    return UPSTREAM;
  return UNKNOWN;
}

char * next_word(s)
     char **s;
{
  char *w;
  
  w = *s;
  while (*w == ' ' || *w == '\t')
    w++;
    
  *s = w;
  for(;;) {
    switch (**s) {
    case ' '  :
    case '\t' :
      **s = '\0';
      (*s)++;
      return(w);
    case '\n' :
    case '#'  :
      **s = '\0';
      return(w);
    case '\0' :
      return(w);
    default   :
      if (isascii(**s) && isupper(**s))
	**s = tolower(**s);
      (*s)++;
    }
  }
}
#endif
