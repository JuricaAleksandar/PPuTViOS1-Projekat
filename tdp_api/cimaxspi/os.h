/*
   Copyright notice
   � 2005 RT-RK LLC. All rights reserved
   This software and related documentation (the "Software") are intellectual
   property owned by RT-RK and are copyright of RT-RK, unless specifically
   noted otherwise.
   Any use of the Software is permitted only pursuant to the terms of the license
   agreement, if any, which accompanies, is included with or applicable to the
   Software ("License Agreement") or upon express written consent of RT-RK. Any
   copying, reproduction or redistribution of the Software in whole or in part by
   any means not in accordance with the License Agreement or as agreed in writing
   by RT-RK is expressly prohibited.
   THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE
   AGREEMENT. EXCEPT AS WARRANTED IN THE LICENSE AGREEMENT THE SOFTWARE IS
   DELIVERED "AS IS" AND RT-RK HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS
   WITH REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIT ENJOYMENT, TITLE AND
   NON-INFRINGEMENT OF ANY THIRD PARTY INTELLECTUAL PROPERTY OR OTHER RIGHTS WHICH
   MAY RESULT FROM THE USE OR THE INABILITY TO USE THE SOFTWARE.
   IN NO EVENT SHALL RT-RK BE LIABLE FOR INDIRECT, INCIDENTAL, CONSEQUENTIAL,
   PUNITIVE, SPECIAL OR OTHER DAMAGES WHATSOEVER INCLUDING WITHOUT LIMITATION,
   DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS
   INFORMATION, AND THE LIKE, ARISING OUT OF OR RELATING TO THE USE OF OR THE
   INABILITY TO USE THE SOFTWARE, EVEN IF RT-RK HAS BEEN ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGES, EXCEPT PERSONAL INJURY OR DEATH RESULTING FROM
   RT-RK'S NEGLIGENCE.
*/
/**
* @file     os.h
*
* @brief    Operating system abstraction layer.
*
* @note     Copyright (C) 2010 RT-RK LLC 
*           All Rights Reserved
*
* @author   RT-RK CIMaX+ team
*
* @date     20/11/2010          
*/

#ifndef __OS_H__
#define __OS_H__

/******************************************************************************
 * Includes
 *****************************************************************************/
#ifdef WIN32
   #include <stdio.h>
   #include <stdarg.h>
   #include <winsock.h>
   #define SHUT_RDWR   2
   typedef int socklen_t;
#else
   #include <errno.h>
   #include <sys/socket.h>
   #include <netinet/in.h>
   #include <arpa/inet.h>

   typedef int SOCKET;
   #define closesocket close
   int WSAGetLastError(void);
#endif

#include <stdlib.h>
#include <string.h>

/******************************************************************************
 * Defines
 *****************************************************************************/
#ifndef __OS_FUNDAMENTAL_TYPES__
#define __OS_FUNDAMENTAL_TYPES__
  #undef    TRUE
  #define   TRUE              1
  #undef    FALSE
  #define   FALSE             0
  #undef    NULL
  #define   NULL              0

  #ifndef WIN32
  typedef unsigned char       BOOL;
  #endif

  typedef signed char         int8;
  typedef unsigned char       uint8;
  typedef unsigned char       byte;
  typedef signed short        int16;
  typedef unsigned short      uint16;
  typedef unsigned short      word;
  typedef signed long         int32;
  typedef unsigned long       uint32;
  typedef unsigned long       dword;
  typedef signed long long    int64;
  typedef unsigned long long  uint64;

  #define OS_SEEK_CUR    1
  #define OS_SEEK_END    2
  #define OS_SEEK_SET    0

#endif

#define osFile_tScanf fscanf

/******************************************************************************
 * Typedefs
 *****************************************************************************/
typedef void  osMutex_t;
typedef void  osSemaphore_t;
typedef void  osFile_t;
typedef void  osThread_t;

/******************************************************************************
 * Globals
 *****************************************************************************/
/******************************************************************************
 * Functions
 *****************************************************************************/
void*          OS_Malloc(int32 size);
void*          OS_Realloc(void* pData, int32 size);
void           OS_Free(void* pData);

osMutex_t*     OS_MutexCreate(void);
void           OS_MutexDelete(osMutex_t* pMutex);
void           OS_MutexLock(osMutex_t* pMutex);
void           OS_MutexUnlock(osMutex_t* pMutex);

osSemaphore_t* OS_SemaphoreCreate(void);
int32          OS_SemaphoreWait(osSemaphore_t* pSemaphore, uint32 ms);
int32          OS_SemaphoreSignal(osSemaphore_t* pSemaphore);
void           OS_SemaphoreDelete(osSemaphore_t* pSemaphore);

void           OS_Sleep(uint32 ms);

osFile_t*      OS_FileOpen(const char* pcFilename, const char* pcMode);
int32          OS_FileClose(osFile_t* pFile);
int32          OS_FileRead(void* pData, int32 size, osFile_t* pFile);
int32          OS_FileWrite(const void* pData, int32 size, osFile_t* pFile);
int32          OS_FileSeek(osFile_t* pFile, int32 offset, int32 origin);
int32          OS_FileTell(osFile_t* pFile);

osThread_t*    OS_ThreadCreate(int32 (*pstFunction)(void*), void* pParam,
                              int32 stackSize);
int32          OS_ThreadDelete(osThread_t* pThread);


#endif /* __OS_H__ */
