#ifndef _HTTPCLIENT_PRV_H_
#define _HTTPCLIENT_PRV_H_


#include "httpparser.h"

#define HTTP_BUF_SIZE   4096

typedef enum enum_HTTP_SYNC_TYPE{
    EN_HTTP_SYNC_TYPE_ASYNC = 0,
    EN_HTTP_SYNC_TYPE_SYNC = 1,
}EN_HTTP_SYNC_TYPE;


typedef struct stru_SOCKBUF{
    _US offset;
    _US length;
    _UC payload[HTTP_BUF_SIZE];
    struct stru_SOCKBUF *next;
}ST_SOCKBUF;


#define BUF_LEN(np)      (np->length)
#define BUF_OFF(np)      (np->offset)
#define BUF_PTR(np)	     (np->payload + (np)->offset)
#define BUF_END(np)      (BUF_PTR(np) + np->length)

typedef struct stru_HTTPCLIENT_SLOT{
    _UC       ucSendFlag;
    _UC       ucSyncFlag;
    _UC       ucNeedCancel;
    _UC       ucStatus;
    _INT      iSlotIndex;
    SOCK      sockHttp;
    _UC       ucRequestIP[32];
    _US       usRequestPort;
    _UC       ucNeedDel;
    _UC       ucFillChar;
    _INT        iTimeout;
    time_t              timeCreate;
    ST_SOCKBUF*         pstSendSockBuf;
    ST_HTTPPARSER_SETTINGS* pstParseSetting;
    ST_HTTPPARSER* stParser;
    PFN_RECV            pfuncRecvData;
    PFN_FINISHED        pfuncFinished;
    PFN_FAILED          pfuncFailed;
    _VOID*               vpBase;
    struct stru_HTTPCLIENT_SLOT *next;
}ST_HTTPCLIENT_SLOT;


typedef struct stru_HTTPCLIENT_BASE{
    _UC                  ucRunFlag;
    _UC                  ucFillChar1;
    _UC                  ucFillChar2;
    _UC                  ucFillChar3;
    _INT                 iNowSlotCount;
    _UC                  ucProductName[256];
    _UC                  ucProductVersion[256];
#ifdef WIN32
    HANDLE               httptid;
#else
    pthread_t            httptid;
#endif
    fd_set               fdWSet;
    fd_set               fdRSet;
    MUTEX_TYPE           hMutexCount;
    MUTEX_TYPE           hMutexIdelSlot;
    MUTEX_TYPE           hMutexSlot;
    MUTEX_TYPE           hMutexBuf;
    ST_SOCKBUF*          pstHttpBufList;
    ST_HTTPCLIENT_SLOT*  stIdelSlotList;
    ST_HTTPCLIENT_SLOT*  stSlotList;
}ST_HTTPCLIENT_BASE;


_INT HttpMessageBeginCB(ST_HTTPPARSER* p);
_INT HttpHeaderFieldCB(ST_HTTPPARSER* p, _UC* pucBuf, _UI uiLen);
_INT HttpHeaderValueCB(ST_HTTPPARSER* p, _UC* pucBuf, _UI uiLen);
_INT HttpHeaderCompleteCB(ST_HTTPPARSER* p);
_INT HttpBodyCB(ST_HTTPPARSER* p, _UC* pucBuf, _UI uiLen);
_INT HttpMessageComplete(ST_HTTPPARSER* p);
_INT HttpSyncBodyCB(ST_HTTPPARSER* p, _UC* pucBuf, _UI uiLen);
_INT HttpSyncMessageComplete(ST_HTTPPARSER* p);

ST_SOCKBUF* Http_Malloc_SockBuf(_INT iCount);
_VOID Http_Free_SockBuf(ST_SOCKBUF* pstSockBuf);
ST_HTTPCLIENT_SLOT* HttpClientBase_GetSlot(ST_HTTPCLIENT_BASE* pstBase);
_VOID HttpClientSlot_Init(ST_HTTPCLIENT_SLOT* pstSlot);
_VOID HttpClientSlot_Clean(ST_HTTPCLIENT_SLOT* pstSlot);
_VOID HttpClientSlot_Destroy(ST_HTTPCLIENT_SLOT** ppstSlot);
_VOID* HttpClient_RecvThread(_VOID* vpParam);
_INT HttpProcessLoop(ST_HTTPCLIENT_BASE* pstBase, time_t timeNow);
_INT HttpProcessSlot(ST_HTTPCLIENT_BASE* pstBase, time_t timeNow);
_INT HttpClientSlot_SendBuf(ST_HTTPCLIENT_SLOT* pstSlot);
_UI  HttpCreateSyncHandleID();
_INT HttpClientSlot_ProcessFailed(ST_HTTPCLIENT_SLOT* pstSlot,_UI uiErrCode);
_INT HttpClientSlot_SetCancel(_UI uiHandleID);
ST_HTTPCLIENT_SLOT* HttpClientSlot_CreateSocket(_UC* pucUrlip, _US usUrlport);
_INT HttpClientSlot_ProcessSyncFinished(ST_SOCKBUF* pstRecvBuf, _UC** ppucOutBuffer, _UI* puiOutLen);
_INT HttpClientSlot_CopyPostRequest(ST_HTTPCLIENT_SLOT* pstSlot, _UC* pucUrlip, _US usUrlport, _UC* pucUrl, _UC* pucParam, _UI uiLen);

#endif
