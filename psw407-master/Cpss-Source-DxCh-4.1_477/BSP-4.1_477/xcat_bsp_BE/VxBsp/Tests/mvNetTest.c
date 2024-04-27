/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
        used to endorse or promote products derived from this software without
        specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include "presteraSwitchEnd.h"
#include "mvSysHwConfig.h"
#include "mvGndReg.h"
#include "mvGnd.h"
#include "mvGndOsIf.h"
#include "mvGndHwIf.h"
#include "mvGenSyncPool.h"
#include "mvNetDrvCommon.h"
#include "iv.h"
#include "ftpLib.h"

extern STATUS 	close (int fd);
extern int 	read (int fd, char *buffer, size_t maxbytes);
extern int 	write (int fd, char *buffer, size_t nbytes);

#define BUFFER_READ_SIZE 512
#define FPGA_ALLOCATED_MEM 0x00600000
#define MAX_MEM_BLOCK 2
#define FTP_HOST_NAME "host"
#define FTP_USER_NAME "anonymous"
#define FTP_PASSWORD "target"
#define FTP_PATH "."
#define FTP_FILE_NAME "vxWorks.st"

int loaded_file_size = 0;	/* the loaded ftp file */

int debugPrintFile = 0;
long file_pointer = 0L;
long file_length = 0L;
char *file_buffer = NULL;

int ftpFileGet(char *, char *, char *, char *, char *);

int debugFtpTestStr(char* s) {
    mvOsPrintf("\nFile name %s %s\n",s,FTP_FILE_NAME);
    return ftpFileGet(FTP_HOST_NAME,FTP_USER_NAME,FTP_PASSWORD,FTP_PATH,s);
}

int debugFtpLoop(MV_U32 loopsNum) {
    MV_U32 i, status = 0;

    if (loopsNum > 50)
    {
        loopsNum = 50;
    }

    for (i = 0; i < loopsNum; i++)
    {
        status = ftpFileGet(FTP_HOST_NAME,FTP_USER_NAME,FTP_PASSWORD,FTP_PATH,FTP_FILE_NAME);
        mvOsDelay(1000);
    }

    return status; /* return last status */
}

int debugFtpLoopEternal(MV_VOID) {

    while (1)
    {
        ftpFileGet(FTP_HOST_NAME,FTP_USER_NAME,FTP_PASSWORD,FTP_PATH,FTP_FILE_NAME);
    }

#ifndef _DIAB_TOOL
    return 0; /* make the compiler happy */
#endif
}

int debugFtpTest() {
    return ftpFileGet(FTP_HOST_NAME,FTP_USER_NAME,FTP_PASSWORD,FTP_PATH,FTP_FILE_NAME);
}
int ftpFileGet(char* hostName,char* user, char* password,
             char* path,char* fileNam) {

    int fd;         /* file descriptor */
    int errFd;      /* for receiving standard error messages from Unix */
    FILE *fp = 0;   /* temp file descriptor */
    int num_of_bytes = 0;      /* used to read file length */
    int num_of_buffers = 0;    /* used to read file length */
    int end_of_file = 0;       /* used to read file length */
    int i,j,k;                   /* loop index */
    char command [100];
    char *memoryBuffer[MAX_MEM_BLOCK] = {0}; /* used to read size of file */
    int  memoryBufferRealMemory[MAX_MEM_BLOCK] = {0};
    int  memoryBufferIndex = 0;  /* memoryBuffer              */
    char *ptrMemoryBuffer;

    file_length = 0;

    /* start ftp */
    if (ftpXfer (hostName, user, password, "", "RETR %s",
                 path, fileNam, &errFd, &fd) == ERROR) {
        mvOsPrintf("\nftp fail\n");
        return 1;
    }

    /* open file that will be loaded  */
    if ((fp = fdopen(fd, "r")) == (FILE *)(-1)) {
        mvOsPrintf("\nCould not open file for downloadable image\n");
        return 1;
    }

    memoryBuffer[0] = (char *) mvOsMalloc((size_t) FPGA_ALLOCATED_MEM);
    if (NULL == memoryBuffer[0])
    {
        mvOsPrintf("\nFaile d allocate memory to file_buffer\n");
        return 1;
    }
    ptrMemoryBuffer = memoryBuffer[0];

    /* read file and put results in allocated memory */
    while (!end_of_file)
    {
      if ((memoryBufferRealMemory[memoryBufferIndex] + BUFFER_READ_SIZE) >=
          FPGA_ALLOCATED_MEM) {
        if ((memoryBufferIndex + 1) >= MAX_MEM_BLOCK)
        {
            mvOsPrintf("\nCould not allocate more memory - need to update driver\n");
            return 1;
        }

        /* allocate next buffer */
        memoryBufferIndex++;
        memoryBuffer[memoryBufferIndex] =
            (char *) mvOsMalloc((size_t) FPGA_ALLOCATED_MEM);
        if (NULL == memoryBuffer[memoryBufferIndex])
        {
            mvOsPrintf("\nFaile d allocate memory to file_buffer\n");
            return 1;
        }
        mvOsPrintf("\nAllocate next memory buffer \n");
        ptrMemoryBuffer = memoryBuffer[memoryBufferIndex];
      }

      num_of_bytes = fread (ptrMemoryBuffer, 1,BUFFER_READ_SIZE, fp);

      memoryBufferRealMemory[memoryBufferIndex] += num_of_bytes;

      ptrMemoryBuffer += BUFFER_READ_SIZE;

      /* is this the last buffer? */
      if(num_of_bytes == BUFFER_READ_SIZE)
        num_of_buffers++;
      else
        end_of_file = 1;
    }

    /* calculate image size */
    file_length = (num_of_buffers * BUFFER_READ_SIZE) + num_of_bytes;

    mvOsPrintf("Done.\n\r");
    mvOsPrintf("Ftp file size is %d bytes.\n\r", file_length);

    /* Empty the Data Socket before close. PC FTP server hangs otherwise */
    while ((read (fd, command, sizeof (command))) > 0);

    /* acknolage host that transfer is complete */
    if (ftpReplyGet(errFd, TRUE) != /*FTP_COMPLETE*/2) {
        mvOsPrintf("ftpReplyGet error\n");
        return 1;
    }

    /* close FTP sesion */
    if (ftpCommand (errFd, "QUIT",0,0,0,0,0,0) != /*FTP_COMPLETE*/2) {

      mvOsPrintf("ftpCommand QUIT error\n");
      return 1;
    }

    close(fd);
    close (errFd);
    fclose (fp);

    /* allocate memory for the file buffer */
    file_buffer = (char *) mvOsMalloc((size_t) file_length);
    if (NULL == file_buffer)
    {
        mvOsPrintf("\nFaile d allocate memory to file_buffer\n");
        return 1;
    }

    /* copy file from memory */
    for (i = 0, j = 0; i < MAX_MEM_BLOCK; i++)
    {
        for (k = 0; k < memoryBufferRealMemory[i]; k++)
        {
            file_buffer[j] = memoryBuffer[i][k];
            j++;
        }

        if (memoryBuffer[i] != NULL)
        {
            mvOsPrintf("\nfree memoryBuffer\n");
            mvOsFree(memoryBuffer[i]);
        }
    }

    if (debugPrintFile)
    {
        mvOsPrintf("\nfile = %s",file_buffer);

    }

    return 0;
}

