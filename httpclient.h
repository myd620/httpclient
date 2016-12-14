#ifndef _HTTPCLIENT_H_
#define _HTTPCLIENT_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "basetype.h"

typedef enum enum_HTTP_STATUS{
    EN_HTTP_STATUS_INIT             = 0,
    EN_HTTP_STATUS_CONNECTING       = 1,
    EN_HTTP_STATUS_PROCESSING       = 2,
    EN_HTTP_STATUS_FINISHED         = 3,
    EN_HTTP_STATUS_FAILED           = 4, //  Êý¾Ý ´íÎó
    EN_HTTP_STATUS_CANCELED         = 5,
    EN_HTTP_STATUS_TIMEOUT          = 6,
    EN_HTTP_STATUS_ERRCODE          = 7,
    EN_HTTP_STATUS_CONERR           = 8,
    EN_HTTP_STATUS_CONTIMEOUT       = 9,
    EN_HTTP_STATUS_RECVERR          = 10,
    EN_HTTP_STATUS_PARERR           = 11,
    EN_HTTP_STATUS_SOCKETERR        = 12,
}EN_HTTP_STATUS;


typedef _VOID (*PFN_RECV)(_UC* pucData, _UI uiLen, _VOID* vpUserPtr);
typedef _VOID (*PFN_FINISHED)(_VOID* vpUserPtr);
typedef _VOID (*PFN_FAILED)(_VOID* vpUserPtr,_UI uiCode);


_INT Httpclient_Init();

_INT Httpclient_Destroy();

_INT Httpclient_SendAsyncGetRequest(_UC* pucUrlip,_US usUrlport,_UC* pucUrl, PFN_RECV pfuncRecv, PFN_FINISHED pfuncFinished, PFN_FAILED pfuncFailed, _VOID* vpUserPtr,_UI* puiHandler);

_INT Httpclient_SendAsyncPostRequest(_UC* pucUrlip, _US usUrlport, _UC* pucUrl, _UC* pucParam, _UI uiLen, PFN_RECV pfuncRecv, PFN_FINISHED pfuncFinished, PFN_FAILED pfuncFailed, _VPTR vpUserPtr, _UI* puiHandler);

_UI HttpCreateSyncHandleID();

_INT HttpCloseSyncHandleID(_UI uiHandleID);

_INT Httpclient_CancelAsyncRequest(_UI uiHandler);

_INT Httpclient_SendSyncGetRequest(_UI uiHandleID, _UC* pucUrlip, _US usUrlport,_UC *pucHost, _UC* pucUrl, _UI uiTimeoutSecond, _UC** ppucOutBuffer, _UI* puiOutLen,_UC* pucStatus);

_INT Httpclient_SendSyncPostRequest(_UI uiHandleID, _UC* pucUrlip, _US usUrlport, _UC* pucUrl, _UC* pucParam, _UI uiLen, _UI uiTimeoutSecond, _UC** ppucOutBuffer, _UI* puiOutLen, _UC* pucStatus);

#ifdef __cplusplus
}
#endif

#endif
