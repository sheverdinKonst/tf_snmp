#include "syscalls.h"
//#include "uart.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "vcp/usbd_cdc_vcp.h"
#include "deffines.h"
size_t   __malloc_margin = 256;
char    *__brkval;



ssize_t _read_r(struct _reent *r, int fd, void *ptr, size_t len)
{
    //return uart_read_r(r, fd, ptr, len);
#if PRINTF_DEB
    return VCP_DataRx((uint8_t *)ptr,(uint32_t)len);
#else
    return 0;
#endif
    
}


ssize_t _write_r(struct _reent *r, int fd, const void *ptr, size_t len)
{

#if PRINTF_DEB
    return VCP_DataTx((uint8_t *)ptr,(uint32_t)len);
#else
    return len;

#endif
}


off_t _lseek_r(struct _reent *r, int fd, _off_t ptr, int dir)
{
//    fd  = fd;
//    ptr = ptr;
//    dir = dir;
    r->_errno = ENOSYS;
    return -1;
}


int _close_r(struct _reent *r, int fd)
{
//    fd = fd;
    r->_errno = ENOSYS;
    return -1;
}


int _fstat_r(struct _reent *r, int fd, struct stat *st)
{
    if ((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO)) {
      st->st_mode = S_IFCHR;
      return 0;
    }

    r->_errno = ENOSYS;
    return -1;
}


int _isatty_r(struct _reent *r, int fd)
{
//    r  = r;
//    fd = fd;
    return 1;
}


void *_sbrk_r(struct _reent *r, ptrdiff_t incr)
{
    extern char end;   // provided by the linker script

    if (__brkval == 0)
        __brkval = &end;

    if (__brkval + incr > (char*)__get_MSP() - __malloc_margin) {
        r->_errno = ENOMEM;
        return (void*)-1;
    }

    void *ret = __brkval;
    __brkval += incr;

    return ret;
}


int _kill_r(struct _reent *r, int pid, int signal)
{
    r->_errno = ENOSYS;
    return -1;
}


int _getpid_r(struct _reent *r)
{
    r->_errno = ENOSYS;
    return -1;
}


void __malloc_lock(struct _reent *r)
{
    vPortEnterCritical();
}


void __malloc_unlock(struct _reent *r)
{
    vPortExitCritical();
}
