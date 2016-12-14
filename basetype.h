#ifndef _BASE_TYPE_H_
#define _BASE_TYPE_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


typedef unsigned char	   _UC;
typedef unsigned short	   _US;
typedef unsigned int	   _UI;
typedef int 			   _INT;
typedef unsigned long long _LLID;
typedef unsigned long long _XXLSIZE;
typedef void			   _VOID;
typedef void			  *_VPTR;



#define  SAFE_UI(_val)       (_UI)(unsigned long)(_val)
#define  MIN_NUM(a,b)        (((a) > (b)) ? (b) :(a))
#define  MAX_NUM(a,b)        (((a) > (b)) ? (a) :(b))


#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable:4996)
#define SOCK            SOCKET 
#define MUTEX_TYPE      CRITICAL_SECTION
#define MUTEX_INIT(x)   InitializeCriticalSection((LPCRITICAL_SECTION)&x)
#define MUTEX_CLEAN(x)   DeleteCriticalSection((LPCRITICAL_SECTION)&x)
#define MUTEX_LOCK(x)     EnterCriticalSection((LPCRITICAL_SECTION)&x)
#define MUTEX_UNLOCK(x)  LeaveCriticalSection((LPCRITICAL_SECTION)&x)
#define THREAD_ID        GetCurrentThreadId()
#define SLEEP(a)             Sleep(a)
#define CLOSESOCKET(a)    closesocket(a)
#else
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define SOCK                  int 
#define MUTEX_TYPE            pthread_mutex_t
#define MUTEX_INIT(mutex)     pthread_mutex_init(&mutex, NULL)
#define MUTEX_CLEAN(mutex)    pthread_mutex_destroy(&mutex)
#define MUTEX_LOCK(mutex)     pthread_mutex_lock(&mutex)
#define MUTEX_UNLOCK(mutex)   pthread_mutex_unlock(&mutex)
#define SLEEP(a)              usleep((a)*1000)
#define CLOSESOCKET(a)        close(a)
#endif



#endif

