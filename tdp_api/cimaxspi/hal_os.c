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
* @file     hal_os.c
*
* @brief    Linux operating system abstraction layer.
*
* @note     Copyright (C) 2010 RT-RK LLC 
*           All Rights Reserved
*
* @author   RT-RK CIMaX+ team
*
* @date     29/11/2010          
*/

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include <stdio.h>
#include "os.h"
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#ifdef ANDROID
#include <pthread.h>
#endif

/******************************************************************************
 * Typedefs
 *****************************************************************************/

/******************************************************************************
 * Globals
 *****************************************************************************/

/******************************************************************************
 * Functions
 *****************************************************************************/
/**
* @ingroup     CIMaX+
* @brief       Allocate memory.
*
* @param       size     Size to allocate.
* @return      Pointer to allocated memory, NULL if no more memory available.
*/
void* OS_Malloc(int32 size)
{
   return malloc(size);
}

/*
* @ingroup     CIMaX+ 
* @brief       Reallocate memory.
* 
* @param       pData    Pointer to current allocated memory.
* @param       i32Size  Size to allocate.
* @return      Pointer to allocated memory, NULL if no more memory available.
*/
void* OS_Realloc(void* pData, int32 size)
{
   return realloc(pData, size);
}

/*
* @ingroup     CIMaX+
* @brief       Free memory.
*
* @param       pData    Pointer to memory to free.
* @return      none
*/
void OS_Free(void* pData)
{
   free(pData);
}

/*
* @ingroup     CIMaX+
* @brief       Create mutex.
*
* @param       none
* @return      Pointer to created mutex, NULL if creation failed.
*/
osMutex_t* OS_MutexCreate(void)
{
   pthread_mutexattr_t attr;
   pthread_mutex_t *pMutex = NULL;
   
   pMutex = malloc(sizeof(pthread_mutex_t));
   if (pMutex == NULL)
   {
      printf("%s: malloc failed.\n", __FUNCTION__);
      return NULL;
   }
   
   pthread_mutexattr_init (&attr);
   // pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE_NP);
   if (pthread_mutex_init (pMutex, &attr) != 0)
   {
      printf("%s: mutex init failed.\n", __FUNCTION__);
      return NULL;
   }
   
   pthread_mutexattr_destroy (&attr);

   return pMutex;
}

/*
* @ingroup     CIMaX+
* @brief       Delete mutex.
*
* @param       pMutex   Pointer to mutex to delete.
* @return      none
*/
void OS_MutexDelete(osMutex_t* pMutex)
{
   pthread_mutex_destroy (pMutex);
   free(pMutex);
}

/*
* @ingroup     CIMaX+
* @brief       Lock mutex.
*
* @param       pMutex      Pointer to mutex to lock.
* @return      none
*/
void OS_MutexLock(osMutex_t* pMutex)
{
   pthread_mutex_lock (pMutex);
}

/*
* @ingroup     CIMaX+
* @brief       Unlock mutex.
*
* @param       pMutex      Pointer to mutex to unlock.
* @return      none
*/
void OS_MutexUnlock(osMutex_t* pMutex)
{
   pthread_mutex_unlock (pMutex);
}

/*
* @ingroup     CIMaX+
* @brief       Create semaphore.
*
* @param       none
* @return      Pointer to created semaphore, NULL if creation failed.
*/
osSemaphore_t* OS_SemaphoreCreate(void)
{
   int32 retVal;
   sem_t *hSemaphore = NULL;
   
   hSemaphore = malloc(sizeof(sem_t));
   if (hSemaphore == NULL)
   {
      printf("%s: malloc failed.\n", __FUNCTION__);
      return NULL;
   }

   retVal = sem_init(hSemaphore, 0, 0);
   
	if (retVal < 0)
	{
		return NULL;
	}

   return hSemaphore;
}

/*
* @ingroup     CIMaX+
* @brief       Wait for semaphore.
*
* @param       pSemaphore     Pointer to semaphore.
* @param       ms             Number of miliseconds to wait.
*
* @return      returns 0 if successfull, otherwise -1.
*/
int32 OS_SemaphoreWait(osSemaphore_t* pSemaphore, uint32 ms)
{
   int32 retVal;
   struct timespec sTs;

   clock_gettime(CLOCK_REALTIME, &sTs);
   sTs.tv_sec += ms / 1000;
   ms = ms % 1000;
   if ((sTs.tv_nsec + ms * 1000000) >= 1000000000){
      sTs.tv_nsec = (sTs.tv_nsec + ms * 1000000) % 1000000000;
      sTs.tv_sec += 1;
   } else {
      sTs.tv_nsec += ms * 1000000;
   }

   retVal = sem_timedwait(pSemaphore, &sTs);

	if (retVal < 0)
	{
		printf("-1");
		return -1;
	}
printf("1");
   return 0;
}

/*
* @ingroup     CIMaX+
* @brief       Signal semaphore.
*
* @param       pSemaphore     Pointer to semaphore.
* @return      returns 0 if successfull, otherwise -1.
*/
int32 OS_SemaphoreSignal(osSemaphore_t* pSemaphore)
{
   int32 retVal;

   retVal = sem_post(pSemaphore);
   
	if (retVal < 0)
	{
		return -1;
	}

   return 0;
}

/*
* @ingroup     CIMaX+
* @brief       Delete semaphore.
*
* @param       pSemaphore  Pointer to semaphore to delete.
* @return      none
*/
void OS_SemaphoreDelete(osSemaphore_t* pSemaphore)
{
   sem_destroy(pSemaphore);
   free(pSemaphore);
}

/*
* @ingroup     CIMaX+
* @brief       Wait an amount of time in milliseconds.
*
* @param       ms       Wait time in milliseconds.
* @return      none
*/
void OS_Sleep(uint32 ms)
{
   usleep(ms*1000);
}

/*
* @ingroup     CIMaX+
* @brief       Open file.
*
* @param       pcFilename  NULL-terminated filename.
* @param       pcMode      NULL-terminated opening arguments. "r", "w", "a", "b" supported.
* @return      Pointer to open file, NULL if failure.
*/
osFile_t* OS_FileOpen(const char* pcFilename, const char* pcMode)
{
   return (osFile_t*)fopen(pcFilename, pcMode);
}

/*
* @ingroup     CIMaX+
* @brief       Close file.
*
* @param       pFile     Pointer to file.
* @return      Zero value if successfull, nonzero value if failure.
*/
int32 OS_FileClose(osFile_t* pFile)
{
   return (int32)fclose((FILE*)pFile);
}

/*
* @ingroup     CIMaX+
* @brief       Read bytes from file.
*
* @param       pData    Pointer to a block of memory.
* @param       size     Size in bytes to read.
* @param       pFile    Pointer to file.
* @return      Number of successfully read bytes.
*/
int32 OS_FileRead(void* pData, int32 size, osFile_t* pFile)
{
   return (int32)fread(pData, 1, size, (FILE*)pFile);
}

/*
* @ingroup     CIMaX+
* @brief       Write bytes to file.
*
* @param       pData    Pointer to data to write.
* @param       size     Size in bytes to write.
* @param       pFile    Pointer to file.
* @return      Number of successfully written bytes.
*/
int32 OS_FileWrite(const void* pData, int32 size, osFile_t* pFile)
{
   return (int32)fwrite(pData, 1, size, (FILE*)pFile);
}

/*
* @ingroup     CIMaX+
* @brief       Sets the file position indicator to a new position.
*
* @param       pFile          Pointer to file.
* @param       offset         Number of bytes to offset from origin.
* @param       origin         Position from where offset is added. It is specified by one of the
*                             following constant:
*                             OS_SEEK_CUR   Beginning of file
*                             OS_SEEK_END   Current position of the file pointer
*                             OS_SEEK_SET   End of file
* @return      Zero value if successfull, nonzero value if failure.
*/
int32 OS_FileSeek(osFile_t* pFile, int32 offset, int32 origin)
{
   return (int32)fseek((FILE*)pFile, (long)offset, origin);
}

/*
* @ingroup     CIMaX+
* @brief       Gets the file position indicator.
*
* @param       pFile       Pointer to file.
* @return      File position indicator if successfull, -1 if failure.
*/
int32 OS_FileTell(osFile_t* pFile)
{
   return (int32)ftell((FILE*)pFile);
}

/*
* @ingroup     CIMaX+
* @brief       Create thread.
*
* @param       pstFunction       A pointer to the application-defined function to be executed by the thread.
*                                This pointer represents the starting address of the thread.
* @param       pParam            A pointer to a variable to be passed to the thread.
* @param       stackSize         Size of the stack, in bytes.
* @return      Pointer to created thread, NULL if creation failed.
*/
osThread_t* OS_ThreadCreate(int32 (*pstFunction)(void*), void* pParam,
                                   int32       stackSize)
{
   pthread_t *pThreadId = NULL;                                                               
   void *ret;                                                                    
   
   pThreadId = malloc (sizeof(pthread_t));
   if (pThreadId == NULL)
   {
      printf("%s: malloc failed.\n", __FUNCTION__);
      return NULL;
   }
                                                                                
   if (pthread_create(pThreadId, NULL, pstFunction, pParam) != 0) 
   {
      printf("%s: creating thread failed.\n", __FUNCTION__);
      return NULL;
   }
   
   return pThreadId;
}

/*
* @ingroup     CIMaX+
* @brief       Delete thread.
*
* @param       pstThread      A pointer to the thread to delete.
* @return      Zero value if successfull, nonzero value if failure.
*/
int32 OS_ThreadDelete(osThread_t* pstThread)
{
   free(pstThread);
   return 0;
}

