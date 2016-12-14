
#include "httpclient.h"
#include "httpclient_prv.h"

static ST_HTTPCLIENT_BASE* s_stHttpBase = 0;
static ST_HTTPPARSER_SETTINGS s_stAsyncHttpParserSettings = {
    HttpMessageBeginCB,
    NULL,
    HttpHeaderFieldCB,
    HttpHeaderValueCB,
    HttpHeaderCompleteCB,
    HttpBodyCB,
    HttpMessageComplete
};

static ST_HTTPPARSER_SETTINGS s_stSyncHttpParserSettings = {
    0, 0, 0, 0, 0, HttpSyncBodyCB, HttpSyncMessageComplete
};



_INT HttpMessageBeginCB(ST_HTTPPARSER* pstP)
{
    return 0;
}

_INT HttpHeaderFieldCB(ST_HTTPPARSER* pstP, _UC* buf, _UI len)
{
    return 0;
}

_INT HttpHeaderValueCB(ST_HTTPPARSER* pstP, _UC* buf, _UI len)
{
    return 0;
}

_INT HttpHeaderCompleteCB(ST_HTTPPARSER* pstP)
{
    return 0;
}

_INT HttpBodyCB(ST_HTTPPARSER* pstP, _UC* buf, _UI len)
{
    ST_HTTPCLIENT_SLOT* stSlot = (ST_HTTPCLIENT_SLOT*)(pstP->pvData);
    if (stSlot->ucSyncFlag == 0)
    {
        if (stSlot->pfuncRecvData)
        {
            (stSlot->pfuncRecvData)(buf, len, stSlot->vpBase);
        }
    }
    return 0;
}

_INT HttpMessageComplete(ST_HTTPPARSER* pstP)
{
    ST_HTTPCLIENT_SLOT* stSlot = (ST_HTTPCLIENT_SLOT*)(pstP->pvData);
    if (stSlot->ucSyncFlag == 0)
    {
        stSlot->ucStatus = EN_HTTP_STATUS_FINISHED;
    }
    return 0;
}

_INT HttpSyncBodyCB(ST_HTTPPARSER* pstP, _UC* buf, _UI len)
{
    ST_SOCKBUF* synb = (ST_SOCKBUF*)(pstP->pvData);
    _UI tmplen1 = 0;
    while (synb->next)
    {
        synb = synb->next;
    }
    tmplen1 = HTTP_BUF_SIZE - BUF_LEN(synb);
    if (tmplen1 > len)
    {
        memcpy(BUF_END(synb), buf, len);
        BUF_LEN(synb) += len;
    }
    else
    {
        ST_SOCKBUF* syntmp;
        _UI tmplen2 = 0;
        if (tmplen1 > 0)
        {
            memcpy(BUF_END(synb), buf, tmplen1);
            BUF_LEN(synb) += tmplen1;
        }
        MUTEX_LOCK(s_stHttpBase->hMutexBuf);
        syntmp = s_stHttpBase->pstHttpBufList;
        s_stHttpBase->pstHttpBufList = syntmp->next;
        MUTEX_UNLOCK(s_stHttpBase->hMutexBuf);
        tmplen2 = len - tmplen1;
        memcpy(BUF_END(syntmp), buf + tmplen1, tmplen2);
        BUF_LEN(syntmp) += tmplen2;
        synb->next = syntmp;
    }
    return 0;
}

_INT HttpSyncMessageComplete(ST_HTTPPARSER* p)
{
    return 0;
}


int socketinit()
{
#ifdef WIN32
    WSADATA wsaData;
    int iResult;
    u_long iMode = 0;
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR)
        printf("Error at WSAStartup()\n");
#endif
    return 0;
}

int socketdeinit()
{
#ifdef WIN32
    WSACleanup();
#endif
    return 0;
}

_INT Httpclient_Init()
{
    if (s_stHttpBase == 0)
    {
        s_stHttpBase = (ST_HTTPCLIENT_BASE*)malloc(sizeof(ST_HTTPCLIENT_BASE));
        s_stHttpBase->ucRunFlag = 0;
        s_stHttpBase->iNowSlotCount = 1;
        s_stHttpBase->pstHttpBufList = Http_Malloc_SockBuf(5);
        MUTEX_INIT(s_stHttpBase->hMutexCount);
        MUTEX_INIT(s_stHttpBase->hMutexIdelSlot);
        MUTEX_INIT(s_stHttpBase->hMutexSlot);
        MUTEX_INIT(s_stHttpBase->hMutexBuf);
        s_stHttpBase->stIdelSlotList = NULL;
        s_stHttpBase->stSlotList = NULL;
    }
    socketinit();
    if (s_stHttpBase->ucRunFlag == 0)
    {
#ifdef WIN32
        DWORD ThreadId = 0;
        s_stHttpBase->httptid = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)HttpClient_RecvThread, NULL, 0, &ThreadId);
        if (!s_stHttpBase->httptid)
#else
        if(0 != pthread_create(&s_stHttpBase->httptid, NULL,HttpClient_RecvThread, NULL))
#endif
        {
            return -1;
        }
    }
    return 0;
}

_INT Httpclient_Destroy()
{
    ST_HTTPCLIENT_SLOT* pstSlot = NULL;
    if (s_stHttpBase == 0)
        return -1;

    s_stHttpBase->ucRunFlag = 0;

    MUTEX_LOCK(s_stHttpBase->hMutexSlot);
    pstSlot = s_stHttpBase->stSlotList;
    while (pstSlot)
    {
        if (pstSlot->ucStatus == EN_HTTP_STATUS_CONNECTING || pstSlot->ucStatus == EN_HTTP_STATUS_PROCESSING)
        {
            pstSlot->ucStatus = EN_HTTP_STATUS_FAILED;
        }
        if (pstSlot->sockHttp != -1)
        {
#ifdef WIN32
            shutdown(pstSlot->sockHttp, SD_BOTH);
#else
            shutdown(pstSlot->sockHttp, SHUT_RDWR);
#endif
            CLOSESOCKET(pstSlot->sockHttp);
            pstSlot->sockHttp = -1;
        }
        pstSlot = pstSlot->next;
    }
    MUTEX_UNLOCK(s_stHttpBase->hMutexSlot);
#ifdef WIN32
    WaitForSingleObject(s_stHttpBase->httptid, INFINITE);
    CloseHandle(s_stHttpBase->httptid);
#else
    pthread_join(s_stHttpBase->httptid,NULL);
#endif
    MUTEX_LOCK(s_stHttpBase->hMutexSlot);
    while (s_stHttpBase->stSlotList)
    {
        pstSlot = s_stHttpBase->stSlotList;
        s_stHttpBase->stSlotList = pstSlot->next;
        HttpClientSlot_Destroy(&pstSlot);
    }
    MUTEX_UNLOCK(s_stHttpBase->hMutexSlot);

    MUTEX_LOCK(s_stHttpBase->hMutexIdelSlot);
    while (s_stHttpBase->stIdelSlotList)
    {
        pstSlot = s_stHttpBase->stIdelSlotList;
        s_stHttpBase->stIdelSlotList = pstSlot->next;
        HttpClientSlot_Destroy(&pstSlot);
    }
    MUTEX_UNLOCK(s_stHttpBase->hMutexIdelSlot);
    MUTEX_CLEAN(s_stHttpBase->hMutexCount);
    MUTEX_CLEAN(s_stHttpBase->hMutexSlot);
    MUTEX_CLEAN(s_stHttpBase->hMutexBuf);
    MUTEX_CLEAN(s_stHttpBase->hMutexIdelSlot);
    Http_Free_SockBuf(s_stHttpBase->pstHttpBufList);
    free(s_stHttpBase);
    s_stHttpBase = 0;
    socketdeinit();
    return 0;
}

ST_SOCKBUF* Http_Malloc_SockBuf(_INT iCount)
{
    _INT iLoop = 0;
    ST_SOCKBUF* pstSockBuf = NULL;
    ST_SOCKBUF* pstTmp = NULL;
    if (iCount <= 0)
        return NULL;

    for (iLoop = 0; iLoop < iCount; iLoop++)
    {
        pstTmp = (ST_SOCKBUF*)malloc(iCount*sizeof(ST_SOCKBUF));
        memset(pstTmp, iCount*sizeof(ST_SOCKBUF), 0);
        pstTmp->next = pstSockBuf;
        pstSockBuf = pstTmp;
    }
    return pstSockBuf;
}

_VOID Http_Free_SockBuf(ST_SOCKBUF* pstSockBuf)
{
    ST_SOCKBUF* pstSlot = pstSockBuf;
    while (pstSlot)
    {
        pstSockBuf = pstSlot->next;
        free(pstSlot);
        pstSlot = pstSockBuf;
    }
}

ST_SOCKBUF* Http_Pop_SockBuf()
{
    ST_SOCKBUF* pstSockBuf = NULL;
    MUTEX_LOCK(s_stHttpBase->hMutexBuf);
    if (s_stHttpBase->pstHttpBufList == NULL)
    {
        s_stHttpBase->pstHttpBufList = Http_Malloc_SockBuf(5);
        if (s_stHttpBase->pstHttpBufList == NULL)
        {
            MUTEX_UNLOCK(s_stHttpBase->hMutexBuf);
            return NULL;
        }
    }
    pstSockBuf = s_stHttpBase->pstHttpBufList;
    s_stHttpBase->pstHttpBufList = pstSockBuf->next;
    MUTEX_UNLOCK(s_stHttpBase->hMutexBuf);
    return pstSockBuf;
}

_VOID Http_Push_SockBuf(ST_SOCKBUF* pstSockBuf)
{
    MUTEX_LOCK(s_stHttpBase->hMutexBuf);
    pstSockBuf->next = s_stHttpBase->pstHttpBufList;
    s_stHttpBase->pstHttpBufList = pstSockBuf;
    MUTEX_UNLOCK(s_stHttpBase->hMutexBuf);
}




ST_HTTPCLIENT_SLOT* HttpClientBase_GetSlot(ST_HTTPCLIENT_BASE* pstBase)
{
    ST_HTTPCLIENT_SLOT* pstSlot = NULL;
    MUTEX_LOCK(s_stHttpBase->hMutexIdelSlot);
    if (s_stHttpBase->stIdelSlotList)
    {
        pstSlot = s_stHttpBase->stIdelSlotList;
        s_stHttpBase->stIdelSlotList = pstSlot->next;
    }
    MUTEX_UNLOCK(s_stHttpBase->hMutexIdelSlot);
    if (!pstSlot)
    {
        pstSlot = (ST_HTTPCLIENT_SLOT*)malloc(sizeof(ST_HTTPCLIENT_SLOT));
        if (pstSlot)
        {
            HttpClientSlot_Init(pstSlot);
        }
    }
    return pstSlot;
}

_VOID HttpClientSlot_Init(ST_HTTPCLIENT_SLOT* pstSlot)
{
    if (pstSlot)
    {
        pstSlot->ucSyncFlag = 0;
        pstSlot->ucNeedCancel = 0;
        pstSlot->ucStatus = 0;
        pstSlot->ucNeedDel = 0;
        pstSlot->iSlotIndex = 0;
        pstSlot->sockHttp = -1;
        memset(pstSlot->ucRequestIP, 0, 32);
        pstSlot->usRequestPort = 0;
        pstSlot->iTimeout = 30;
        pstSlot->stParser = (ST_HTTPPARSER*)malloc(sizeof(ST_HTTPPARSER));
        if (pstSlot->stParser){
            HttpParser_Init(pstSlot->stParser, EN_HTTPPARSER_RESPONSE);
            pstSlot->stParser->pvData = NULL;
        }
        pstSlot->pstSendSockBuf = NULL;
        pstSlot->pfuncRecvData = 0;
        pstSlot->pfuncFinished = 0;
        pstSlot->pfuncFailed = 0;
        pstSlot->pstParseSetting = NULL;
        pstSlot->vpBase = 0;
    }
}

_VOID HttpClientSlot_Clean(ST_HTTPCLIENT_SLOT* pstSlot)
{
    if (pstSlot)
    {
        pstSlot->ucSyncFlag = 0;
        pstSlot->ucNeedCancel = 0;
        pstSlot->ucStatus = 0;
        pstSlot->ucNeedDel = 0;
        pstSlot->iSlotIndex = 0;
        pstSlot->sockHttp = -1;
        memset(pstSlot->ucRequestIP, 0, 32);
        pstSlot->usRequestPort = 0;
        pstSlot->iTimeout = 30;
        if (pstSlot->stParser){
            pstSlot->stParser->pvData = NULL;
            HttpParser_Init(pstSlot->stParser, EN_HTTPPARSER_RESPONSE);
        }
        Http_Push_SockBuf(pstSlot->pstSendSockBuf);
        pstSlot->pstSendSockBuf = NULL;
        pstSlot->pfuncRecvData = 0;
        pstSlot->pfuncFinished = 0;
        pstSlot->pfuncFailed = 0;
        pstSlot->pstParseSetting = NULL;
        pstSlot->vpBase = 0;
    }
}

_VOID HttpClientSlot_Destroy(ST_HTTPCLIENT_SLOT** ppstSlot)
{
    if (ppstSlot && *ppstSlot)
    {
        if ((*ppstSlot)->pstSendSockBuf)
        {
            Http_Push_SockBuf((*ppstSlot)->pstSendSockBuf);
            (*ppstSlot)->pstSendSockBuf = NULL;
        }
        if ((*ppstSlot)->stParser)
        {
            free(((*ppstSlot)->stParser));
            (*ppstSlot)->stParser = 0;
        }
        free(*ppstSlot);
        *ppstSlot = 0;
    }
}

_VOID* HttpClient_RecvThread(_VOID* vpParam)
{
    time_t timeNow;
    _INT retLoop = 0;
    _INT retSlot = 0;
    s_stHttpBase->ucRunFlag = 1;
    while (s_stHttpBase->ucRunFlag == 1)
    {
        timeNow = time(NULL);
        retLoop = HttpProcessLoop(s_stHttpBase, timeNow);
        retSlot = HttpProcessSlot(s_stHttpBase, timeNow);
        if (retLoop != 0 || retSlot != 0)
        {
            SLEEP(100);
        }
    }

    return NULL;
}

_INT HttpProcessLoop(ST_HTTPCLIENT_BASE* pstBase, time_t timeNow)
{
    _INT iMaxFd = 0;
    _INT iReady = 0;
    _INT iret = 0;
    struct timeval  stTimeout = { 0 };
    _UC ucRecvBuffer[HTTP_BUF_SIZE];
    _INT iRecvLen = 0;
    _INT iMiSec = 1000;
    ST_HTTPCLIENT_SLOT* pstSlot;
    if (!pstBase || pstBase->ucRunFlag != 1){
        return -1;
    }
    FD_ZERO(&pstBase->fdRSet);
    FD_ZERO(&pstBase->fdWSet);

    MUTEX_LOCK(pstBase->hMutexSlot);
    pstSlot = pstBase->stSlotList;
    while (pstSlot)
    {
        if (pstSlot->ucNeedCancel || (pstSlot->sockHttp == -1))
        {
            pstSlot = pstSlot->next;
            continue;
        }
        if ((pstSlot->ucStatus == EN_HTTP_STATUS_PROCESSING) || (pstSlot->ucStatus == EN_HTTP_STATUS_CONNECTING))
        {
            iMaxFd = MAX_NUM(iMaxFd, (_INT)(pstSlot->sockHttp));
            if (pstSlot->ucStatus == EN_HTTP_STATUS_CONNECTING)
            {
                FD_SET(pstSlot->sockHttp, &pstBase->fdWSet);
            }
            else if (pstSlot->ucStatus == EN_HTTP_STATUS_PROCESSING)
            {
                FD_SET(pstSlot->sockHttp, &pstBase->fdRSet);
            }
        }
        pstSlot = pstSlot->next;
    }
    MUTEX_UNLOCK(s_stHttpBase->hMutexSlot);

    if (iMaxFd > 0)
    {
        stTimeout.tv_sec = 1;
        stTimeout.tv_usec = 0;
        iret = select(iMaxFd + 1, &pstBase->fdRSet, &pstBase->fdWSet, NULL, &stTimeout);
        if (iret == -1)
        {
            return -1;
        }

        if (iret > 0)
        {
            pstSlot = pstBase->stSlotList;
            MUTEX_LOCK(pstBase->hMutexSlot);
            while (pstSlot)
            {
                if ((pstSlot->ucNeedCancel) || (pstSlot->sockHttp == -1))
                {
                    pstSlot = pstSlot->next;
                    continue;
                }
                if (pstSlot->ucStatus == EN_HTTP_STATUS_CONNECTING)
                {
                    if (FD_ISSET(pstSlot->sockHttp, &pstBase->fdWSet))
                    {
                        _INT err;
                        _INT len = sizeof(err);
                        _INT code = 0;
#ifdef WIN32
                        code = getsockopt(pstSlot->sockHttp, SOL_SOCKET, SO_ERROR, (char*)(&err), &len);
#else
                        code = getsockopt(pstSlot->sockHttp, SOL_SOCKET, SO_ERROR, (char*)(&err), (socklen_t*)&len);
#endif
                        if (code < 0 || err)
                        {
                            pstSlot->ucStatus = EN_HTTP_STATUS_CONERR;
                        }
                        else
                            pstSlot->ucStatus = EN_HTTP_STATUS_PROCESSING;
                    }
                }
                else if (pstSlot->ucStatus == EN_HTTP_STATUS_PROCESSING)
                {
                    if (FD_ISSET(pstSlot->sockHttp, &pstBase->fdRSet))
                    {
                        memset(ucRecvBuffer, 0, HTTP_BUF_SIZE);
                        iRecvLen = recv(pstSlot->sockHttp, ucRecvBuffer, HTTP_BUF_SIZE, 0);
                        if (iRecvLen <= 0)
                        {
                            pstSlot->ucStatus = EN_HTTP_STATUS_RECVERR;
                        }
                        else if (iRecvLen > 0)
                        {
                            _UI uiLen = HttpParser_Execute(pstSlot->stParser, pstSlot->pstParseSetting, ucRecvBuffer, iRecvLen);
                            if (uiLen != (_UI)iRecvLen){
                                pstSlot->ucStatus = EN_HTTP_STATUS_FAILED;
                            }
                            else if (pstSlot->stParser->usResponseCode > 299){
                                pstSlot->ucStatus = EN_HTTP_STATUS_ERRCODE;
                            }
                            else if (pstSlot->stParser->xxlContentSize == 0){
                                pstSlot->ucStatus = EN_HTTP_STATUS_FINISHED;
                            }
                        }
                    }
                }
                pstSlot = pstSlot->next;
            }
            MUTEX_UNLOCK(s_stHttpBase->hMutexSlot);
        }
    }
    else
    {
        SLEEP(200);
    }
    return 0;
}

_INT HttpProcessSlot(ST_HTTPCLIENT_BASE* pstBase, time_t timeNow)
{
    ST_HTTPCLIENT_SLOT* pstSlotPre = NULL;
    ST_HTTPCLIENT_SLOT* pstSlotNext = NULL;
    ST_HTTPCLIENT_SLOT* pstSlot = NULL;
    if (!pstBase || pstBase->ucRunFlag != 1){
        return -1;
    }
    MUTEX_LOCK(pstBase->hMutexSlot);
    pstSlot = pstBase->stSlotList;
    while (pstSlot)
    {
        if (pstSlot->ucNeedDel == 1)
        {
            if (pstSlot->sockHttp != -1)
            {
                CLOSESOCKET(pstSlot->sockHttp);
                pstSlot->sockHttp = -1;
            }
            if (pstSlotPre)
            {
                pstSlotPre->next = pstSlot->next;
            }
            else
            {
                pstBase->stSlotList = pstSlot->next;
            }
            pstSlotNext = pstSlot->next;
            HttpClientSlot_Clean(pstSlot);
            MUTEX_LOCK(s_stHttpBase->hMutexIdelSlot);
            pstSlot->next = s_stHttpBase->stIdelSlotList;
            s_stHttpBase->stIdelSlotList = pstSlot;
            MUTEX_UNLOCK(s_stHttpBase->hMutexIdelSlot);
            pstSlot = pstSlotNext;
            continue;
        }

        if (pstSlot->ucNeedCancel == 1)
        {
            pstSlot->ucStatus = EN_HTTP_STATUS_CANCELED;
        }
        if (pstSlot->ucStatus == EN_HTTP_STATUS_CONNECTING)
        {
            if (timeNow - pstSlot->timeCreate > pstSlot->iTimeout)
            {
                pstSlot->ucStatus = EN_HTTP_STATUS_CONTIMEOUT;
            }
        }
        else if (pstSlot->ucStatus == EN_HTTP_STATUS_PROCESSING)
        {
            if (timeNow - pstSlot->timeCreate > pstSlot->iTimeout)
            {
                pstSlot->ucStatus = EN_HTTP_STATUS_TIMEOUT;
            }
            else if (pstSlot->ucSendFlag == 1)
            {
                HttpClientSlot_SendBuf(pstSlot);
            }
        }
        else if (pstSlot->ucStatus == EN_HTTP_STATUS_FINISHED)
        {
            MUTEX_UNLOCK(pstBase->hMutexSlot);
            if (pstSlot->pfuncFinished) pstSlot->pfuncFinished(pstSlot->vpBase);
            if (pstSlot->sockHttp != -1){
                CLOSESOCKET(pstSlot->sockHttp);
                pstSlot->sockHttp = -1;
            }
            if (pstSlot->ucSyncFlag == EN_HTTP_SYNC_TYPE_ASYNC){
                pstSlot->ucNeedDel = 1;
            }
            MUTEX_LOCK(pstBase->hMutexSlot);
        }
        else if (pstSlot->ucStatus != EN_HTTP_STATUS_INIT)
        {
            MUTEX_UNLOCK(pstBase->hMutexSlot);
            HttpClientSlot_ProcessFailed(pstSlot, pstSlot->ucStatus);
            MUTEX_LOCK(pstBase->hMutexSlot);
        }
        pstSlotPre = pstSlot;
        pstSlot = pstSlot->next;
    }
    MUTEX_UNLOCK(pstBase->hMutexSlot);
    return 0;
}

_INT HttpClientSlot_ProcessFailed(ST_HTTPCLIENT_SLOT* pstSlot, _UI uiErrCode)
{
    if (pstSlot->pfuncFailed) pstSlot->pfuncFailed(pstSlot->vpBase, uiErrCode);
    if (pstSlot->sockHttp != -1)
    {
        CLOSESOCKET(pstSlot->sockHttp);
        pstSlot->sockHttp = -1;
    }
    if (pstSlot->ucSyncFlag == EN_HTTP_SYNC_TYPE_ASYNC){
        pstSlot->ucNeedDel = 1;
    }
    return 0;
}

_INT HttpClientSlot_SendBuf(ST_HTTPCLIENT_SLOT* pstSlot)
{
    _INT ilen = 0;
    ST_SOCKBUF* pstSendBuffer = NULL;
    _UI uiSendLen = 0;
    if (!pstSlot || pstSlot->ucStatus != EN_HTTP_STATUS_PROCESSING || pstSlot->ucNeedCancel)
    {
        return -1;
    }
    pstSendBuffer = pstSlot->pstSendSockBuf;
    if (pstSendBuffer == NULL)
    {
        pstSlot->ucSendFlag = 0;
        return -1;
    }
    while (BUF_LEN(pstSendBuffer) > 0)
    {
        uiSendLen = BUF_LEN(pstSendBuffer);
        ilen = send(pstSlot->sockHttp, BUF_PTR(pstSendBuffer), uiSendLen, 0);
        if (ilen < 0)
        {
#ifdef WIN32
            if (errno == WSAEWOULDBLOCK)
#else
            if(errno == EWOULDBLOCK)
#endif
            {
                SLEEP(500);
                break;
            }
            else
            {
                return -1;
            }
        }
        BUF_OFF(pstSendBuffer) += ilen;
        BUF_LEN(pstSendBuffer) -= ilen;

        if (BUF_LEN(pstSendBuffer) <= 0)
        {
            pstSlot->pstSendSockBuf = pstSendBuffer->next;
            pstSendBuffer->next = NULL;
            Http_Push_SockBuf(pstSendBuffer);
            pstSendBuffer = pstSlot->pstSendSockBuf;
            if (pstSendBuffer == NULL)
            {
                pstSlot->ucSendFlag = 0;
                return 0;
            }
        }
    }
    return 0;
}

_INT HttpClientSlot_SetCancel(_UI uiHandleID)
{
    ST_HTTPCLIENT_SLOT* pstSlot;
    if (uiHandleID == 0)
    {
        return -1;
    }
    if (!s_stHttpBase){
        return -1;
    }
    MUTEX_LOCK(s_stHttpBase->hMutexSlot);
    pstSlot = s_stHttpBase->stSlotList;
    while (pstSlot)
    {
        if (pstSlot->iSlotIndex == uiHandleID)
        {
            pstSlot->vpBase = NULL;
            pstSlot->pfuncFailed = NULL;
            pstSlot->pfuncFinished = NULL;
            pstSlot->pfuncRecvData = NULL;
            pstSlot->ucNeedCancel = 1;
            break;
        }
    }
    MUTEX_UNLOCK(s_stHttpBase->hMutexSlot);
    return -1;
}

_INT Httpclient_CancelAsyncRequest(_UI uiHandler)
{
    return HttpClientSlot_SetCancel(uiHandler);
}

ST_HTTPCLIENT_SLOT* HttpClientSlot_CreateSocket(_UC* pucUrlip, unsigned short usUrlport)
{
    _INT ret = 0;
    _INT sockfd = -1;
    _INT len = sizeof(struct sockaddr_in);
    _UI iEnable = 0;
    struct sockaddr_in addr;
    ST_HTTPCLIENT_SLOT* pstSlot;

    addr.sin_family = AF_INET;
    inet_pton(AF_INET, (char *)pucUrlip, &addr.sin_addr);
    addr.sin_port = htons(usUrlport);

    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        return NULL;
#ifdef WIN32
    ioctlsocket(sockfd, FIONBIO, (unsigned long *)&iEnable);
#else
    fcntl(sockfd , F_SETFL, (fcntl(sockfd,F_GETFL) | O_NONBLOCK));
#endif

    ret = connect(sockfd, (struct sockaddr*)&addr, len);
    pstSlot = HttpClientBase_GetSlot(s_stHttpBase);
    if (!pstSlot)
    {
        CLOSESOCKET(sockfd);
        return NULL;
    }
    pstSlot->sockHttp = sockfd;
    pstSlot->iTimeout = 30;
    pstSlot->ucStatus = EN_HTTP_STATUS_CONNECTING;
    pstSlot->timeCreate = time(NULL);
    pstSlot->pstSendSockBuf = Http_Pop_SockBuf();
    pstSlot->ucSendFlag = 1;

    if (pstSlot->pstSendSockBuf == NULL){
        CLOSESOCKET(sockfd);
        HttpClientSlot_Destroy(&pstSlot);
        pstSlot = NULL;
        return NULL;
    }
    memset(pstSlot->pstSendSockBuf, 0, sizeof(ST_SOCKBUF));
    if (pstSlot->stParser == NULL){
        CLOSESOCKET(sockfd);
        HttpClientSlot_Destroy(&pstSlot);
        pstSlot = NULL;
        return NULL;
    }

    if (ret == 0)
    {
        pstSlot->ucStatus = EN_HTTP_STATUS_PROCESSING;
    }
    strncpy((char*)pstSlot->ucRequestIP, (char*)pucUrlip, 31);
    pstSlot->usRequestPort = usUrlport;
    return pstSlot;
}

_INT Httpclient_SendAsyncGetRequest(_UC* pucUrlip, unsigned short usUrlport, _UC* pucUrl, PFN_RECV pfuncRecv, PFN_FINISHED pfuncFinished, PFN_FAILED pfuncFailed, _VPTR vpUserPtr, _UI* puiHandler)
{
    _UI uiSendLen;
    ST_HTTPCLIENT_SLOT* pstSlot;
    if (!s_stHttpBase || s_stHttpBase->ucRunFlag == 0)
    {
        if (pfuncFailed) pfuncFailed(vpUserPtr, EN_HTTP_STATUS_PARERR);
        return -1;
    }
    if (pucUrl == NULL || strlen((char*)pucUrl) > 3000)
    {
        if (pfuncFailed) pfuncFailed(vpUserPtr, EN_HTTP_STATUS_PARERR);
        return -1;
    }
    pstSlot = HttpClientSlot_CreateSocket(pucUrlip, usUrlport);
    if (pstSlot == NULL){
        if (pfuncFailed) pfuncFailed(vpUserPtr, EN_HTTP_STATUS_SOCKETERR);
        return -1;
    }

    pstSlot->vpBase = vpUserPtr;
    pstSlot->pfuncFailed = pfuncFailed;
    pstSlot->pfuncFinished = pfuncFinished;
    pstSlot->pfuncRecvData = pfuncRecv;
    pstSlot->ucSyncFlag = EN_HTTP_SYNC_TYPE_ASYNC;
    pstSlot->pstParseSetting = &s_stAsyncHttpParserSettings;
    pstSlot->stParser->pvData = pstSlot;
    pstSlot->iSlotIndex = HttpCreateSyncHandleID();
    *puiHandler = pstSlot->iSlotIndex;

    memset(pstSlot->pstSendSockBuf->payload, 0, HTTP_BUF_SIZE);
    sprintf((char *)BUF_PTR(pstSlot->pstSendSockBuf), "GET %s HTTP/1.1\r\nHost: %s:%u\r\nAccept: */*\r\nAccept-Language: zh-cn\r\nContent-Type: application/json\r\nConnection: keep-alive\r\n\r\n", pucUrl, pucUrlip, usUrlport);
    uiSendLen = strlen((char *)BUF_PTR(pstSlot->pstSendSockBuf));
    BUF_LEN(pstSlot->pstSendSockBuf) += uiSendLen;

    MUTEX_LOCK(s_stHttpBase->hMutexSlot);
    pstSlot->next = s_stHttpBase->stSlotList;
    s_stHttpBase->stSlotList = pstSlot;
    MUTEX_UNLOCK(s_stHttpBase->hMutexSlot);

    return 0;
}

_INT Httpclient_SendAsyncPostRequest(_UC* pucUrlip, unsigned short usUrlport, _UC* pucUrl, _UC* pucParam, _UI uiLen, PFN_RECV pfuncRecv, PFN_FINISHED pfuncFinished, PFN_FAILED pfuncFailed, _VPTR vpUserPtr, _UI* puiHandler)
{
    ST_HTTPCLIENT_SLOT* pstSlot;
    if (!s_stHttpBase || s_stHttpBase->ucRunFlag == 0)
    {
        if (pfuncFailed) pfuncFailed(vpUserPtr, EN_HTTP_STATUS_PARERR);
        return -1;
    }
    if (pucUrl == NULL || strlen((char*)pucUrl) > 3000)
    {
        if (pfuncFailed) pfuncFailed(vpUserPtr, EN_HTTP_STATUS_PARERR);
        return -1;
    }
    pstSlot = HttpClientSlot_CreateSocket(pucUrlip, usUrlport);
    if (pstSlot == NULL)
    {
        if (pfuncFailed) pfuncFailed(vpUserPtr, EN_HTTP_STATUS_SOCKETERR);
        return -1;
    }

    pstSlot->vpBase = vpUserPtr;
    pstSlot->pfuncFailed = pfuncFailed;
    pstSlot->pfuncFinished = pfuncFinished;
    pstSlot->pfuncRecvData = pfuncRecv;
    pstSlot->ucSyncFlag = EN_HTTP_SYNC_TYPE_ASYNC;
    pstSlot->pstParseSetting = &s_stAsyncHttpParserSettings;
    pstSlot->stParser->pvData = pstSlot;
    pstSlot->iSlotIndex = HttpCreateSyncHandleID();
    *puiHandler = pstSlot->iSlotIndex;
    HttpClientSlot_CopyPostRequest(pstSlot, pucUrlip, usUrlport, pucUrl, pucParam, uiLen);
    MUTEX_LOCK(s_stHttpBase->hMutexSlot);
    pstSlot->next = s_stHttpBase->stSlotList;
    s_stHttpBase->stSlotList = pstSlot;
    MUTEX_UNLOCK(s_stHttpBase->hMutexSlot);

    return 0;
}

_UI HttpCreateSyncHandleID()
{
    _UI uiHandleID = 0;
    MUTEX_LOCK(s_stHttpBase->hMutexCount);
    uiHandleID = s_stHttpBase->iNowSlotCount;
    ++(s_stHttpBase->iNowSlotCount);
    if (s_stHttpBase->iNowSlotCount == 0x7FFFFFFF)
        s_stHttpBase->iNowSlotCount = 1;
    MUTEX_UNLOCK(s_stHttpBase->hMutexCount);
    return uiHandleID;
}

_INT HttpCloseSyncHandleID(_UI uiHandleID)
{
    return HttpClientSlot_SetCancel(uiHandleID);
}

_INT HttpClientSlot_ProcessSyncFinished(ST_SOCKBUF* pstRecvBuf, _UC** ppucOutBuffer, _UI* puiOutLen)
{
    ST_SOCKBUF* pstTmpBuf = NULL;
    _UI uiRecvLen = 0;
    _INT iRet = -1;
    pstTmpBuf = pstRecvBuf;
    while (pstTmpBuf && BUF_LEN(pstTmpBuf) > 0)
    {
        uiRecvLen += BUF_LEN(pstTmpBuf);
        pstTmpBuf = pstTmpBuf->next;
    }
    if (uiRecvLen > 0)
    {
        _INT iTmpHaslen = 0;
        pstTmpBuf = pstRecvBuf;
        *ppucOutBuffer = (_UC*)malloc(uiRecvLen + 1);
        if ((*ppucOutBuffer) == NULL)
        {
            return -1;
        }
        *puiOutLen = uiRecvLen;
        memset(*ppucOutBuffer, 0, uiRecvLen + 1);
        while (pstTmpBuf)
        {
            memcpy(*ppucOutBuffer + iTmpHaslen, BUF_PTR(pstTmpBuf), BUF_LEN(pstTmpBuf));
            iTmpHaslen += BUF_LEN(pstTmpBuf);
            pstTmpBuf = pstTmpBuf->next;
        }
        iRet = 0;
    }
    return iRet;
}

_INT HttpClientSlot_CopyPostRequest(ST_HTTPCLIENT_SLOT* pstSlot, _UC* pucUrlip, unsigned short usUrlport, _UC* pucUrl, _UC* pucParam, _UI uiLen)
{
    _UI uiHeadLen = 0;
    _UI uiLen1 = uiLen;
    _UI uiTmp = 0;
    _UI uiSendLen = 0;
    _UI uiAllSendLen = 0;
    ST_SOCKBUF* pstHead = NULL;
    ST_SOCKBUF* pstTmp = NULL;
    ST_SOCKBUF* pstTmp1 = NULL;

    pstHead = Http_Pop_SockBuf();
    memset(pstHead, 0, sizeof(ST_SOCKBUF));

    if (uiLen <= HTTP_BUF_SIZE)
    {
        memcpy(BUF_PTR(pstHead), pucParam, uiLen);
        BUF_LEN(pstHead) = uiLen;
    }
    else
    {
        memcpy(BUF_PTR(pstHead), pucParam, HTTP_BUF_SIZE);
        BUF_LEN(pstHead) = HTTP_BUF_SIZE;
        uiLen1 -= HTTP_BUF_SIZE;
        uiSendLen += HTTP_BUF_SIZE;
        pstTmp = pstHead;
        while (uiLen1)
        {
            if (uiLen1 >= HTTP_BUF_SIZE)
            {
                uiTmp = HTTP_BUF_SIZE;
            }
            else
            {
                uiTmp = uiLen1;
            }
            pstTmp1 = Http_Pop_SockBuf();
            memset(pstTmp1, 0, sizeof(ST_SOCKBUF));
            memcpy(BUF_PTR(pstTmp1), pucParam + uiSendLen, uiTmp);
            uiSendLen += uiTmp;
            uiLen -= uiTmp;
            pstTmp->next = pstTmp1;
            pstTmp = pstTmp1;
        }
    }


    BUF_OFF(pstSlot->pstSendSockBuf) = 0;
    BUF_LEN(pstSlot->pstSendSockBuf) = 0;
    memset(pstSlot->pstSendSockBuf->payload, 0, HTTP_BUF_SIZE);
    sprintf((char*)BUF_END(pstSlot->pstSendSockBuf), "POST %s HTTP/1.1\r\nHost: %s:%u\r\nContent-Length: %d\r\nAccept: */*\r\nAccept-Language: zh-cn\r\nContent-Type: application/json\r\nConnection: keep-alive\r\n\r\n", pucUrl, pucUrlip, usUrlport, uiLen);
    uiHeadLen = strlen((char*)BUF_PTR(pstSlot->pstSendSockBuf));
    BUF_LEN(pstSlot->pstSendSockBuf) = uiHeadLen;
    uiAllSendLen = uiLen + uiHeadLen;
    if (uiAllSendLen <= HTTP_BUF_SIZE)
    {
        memcpy(BUF_END(pstSlot->pstSendSockBuf), BUF_PTR(pstHead), BUF_LEN(pstHead));
        BUF_LEN(pstSlot->pstSendSockBuf) += uiLen;
        Http_Push_SockBuf(pstHead);
    }
    else
    {
        pstSlot->pstSendSockBuf->next = pstHead;
    }
    return 0;
}

_INT Httpclient_SendSyncGetRequest(_UI uiHandleID, _UC* pucUrlip, unsigned short usUrlport, _UC *pucHost, _UC* pucUrl, _UI uiTimeoutSecond, _UC** ppucOutBuffer, _UI* puiOutLen, _UC* pucStatus)
{
    ST_SOCKBUF* pstRecvBuf;
    _UI uiSendLen;
    ST_HTTPCLIENT_SLOT* pstSlot;
    _INT iRet = -1;
    *pucStatus = 0;
    if (!s_stHttpBase || s_stHttpBase->ucRunFlag == 0){
        return -1;
    }
    if (pucUrl == NULL || strlen((char*)pucUrl) > 3000){
        return -1;
    }
    pstRecvBuf = Http_Pop_SockBuf();
    memset(pstRecvBuf, 0, sizeof(ST_SOCKBUF));
    if (!pstRecvBuf){
        return -1;
    }
    pstSlot = HttpClientSlot_CreateSocket(pucUrlip, usUrlport);
    if (pstSlot == NULL){
        return -1;
    }
    pstSlot->iTimeout = uiTimeoutSecond;
    pstSlot->vpBase = NULL;
    pstSlot->pfuncFailed = NULL;
    pstSlot->pfuncFinished = NULL;
    pstSlot->pfuncRecvData = NULL;
    pstSlot->ucSyncFlag = EN_HTTP_SYNC_TYPE_SYNC;
    pstSlot->pstParseSetting = &s_stSyncHttpParserSettings;
    pstSlot->stParser->pvData = pstRecvBuf;
    pstSlot->iSlotIndex = uiHandleID;

    if (pucHost == NULL)
        sprintf((char*)BUF_PTR(pstSlot->pstSendSockBuf), "GET %s HTTP/1.1\r\nHost: %s:%u\r\nAccept: */*\r\nAccept-Language: zh-cn\r\nContent-Type: application/json\r\nConnection: keep-alive\r\n\r\n",
        pucUrl, pucUrlip, usUrlport);
    else
        sprintf((char*)BUF_PTR(pstSlot->pstSendSockBuf), "GET %s HTTP/1.1\r\nHost: %s\r\nAccept: */*\r\nAccept-Language: zh-cn\r\nConnection: keep-alive\r\n\r\n",
        pucUrl, pucHost);
    uiSendLen = strlen((char*)BUF_PTR(pstSlot->pstSendSockBuf));
    BUF_LEN(pstSlot->pstSendSockBuf) += uiSendLen;

    MUTEX_LOCK(s_stHttpBase->hMutexSlot);
    pstSlot->next = s_stHttpBase->stSlotList;
    s_stHttpBase->stSlotList = pstSlot;
    MUTEX_UNLOCK(s_stHttpBase->hMutexSlot);

    while (pstSlot->ucStatus == EN_HTTP_STATUS_CONNECTING || pstSlot->ucStatus == EN_HTTP_STATUS_PROCESSING)
    {
        SLEEP(200);
    }

    if (pstSlot->ucStatus == EN_HTTP_STATUS_FINISHED)
    {
        iRet = HttpClientSlot_ProcessSyncFinished(pstRecvBuf, ppucOutBuffer, puiOutLen);
        pstSlot->ucNeedDel = 1;
        Http_Push_SockBuf(pstRecvBuf);
        return iRet;
    }
    pstSlot->ucNeedDel = 1;
    Http_Push_SockBuf(pstRecvBuf);
    *pucStatus = pstSlot->ucStatus;
    return -1;
}

_INT Httpclient_SendSyncPostRequest(_UI uiHandleID, _UC* pucUrlip, unsigned short usUrlport, _UC* pucUrl, _UC* pucParam, _UI uiLen, _UI uiTimeoutSecond, _UC** ppucOutBuffer, _UI* puiOutLen, _UC* pucStatus)
{
    ST_SOCKBUF* pstRecvBuf;
    ST_HTTPCLIENT_SLOT* pstSlot;
    _INT iRet = -1;
    *pucStatus = 0;
    if (!s_stHttpBase || s_stHttpBase->ucRunFlag == 0)
    {
        return -1;
    }
    if (pucUrl == NULL || strlen((char*)pucUrl) > 3000)
    {
        return -1;
    }

    pstRecvBuf = Http_Pop_SockBuf();
    memset(pstRecvBuf, 0, sizeof(ST_SOCKBUF));
    if (!pstRecvBuf)
    {
        return -1;
    }
    pstSlot = HttpClientSlot_CreateSocket(pucUrlip, usUrlport);
    if (pstSlot == NULL){
        return -1;
    }
    pstSlot->iTimeout = uiTimeoutSecond;
    pstSlot->vpBase = NULL;
    pstSlot->pfuncFailed = NULL;
    pstSlot->pfuncFinished = NULL;
    pstSlot->pfuncRecvData = NULL;
    pstSlot->ucSyncFlag = EN_HTTP_SYNC_TYPE_SYNC;
    pstSlot->pstParseSetting = &s_stSyncHttpParserSettings;
    pstSlot->stParser->pvData = pstRecvBuf;
    pstSlot->iSlotIndex = uiHandleID;

    HttpClientSlot_CopyPostRequest(pstSlot, pucUrlip, usUrlport, pucUrl, pucParam, uiLen);

    MUTEX_LOCK(s_stHttpBase->hMutexSlot);
    pstSlot->next = s_stHttpBase->stSlotList;
    s_stHttpBase->stSlotList = pstSlot;
    MUTEX_UNLOCK(s_stHttpBase->hMutexSlot);

    while (pstSlot->ucStatus == EN_HTTP_STATUS_CONNECTING || pstSlot->ucStatus == EN_HTTP_STATUS_PROCESSING)
    {
        SLEEP(200);
    }
    if (pstSlot->ucStatus == EN_HTTP_STATUS_FINISHED)
    {
        iRet = HttpClientSlot_ProcessSyncFinished(pstRecvBuf, ppucOutBuffer, puiOutLen);
        Http_Push_SockBuf(pstRecvBuf);
        pstSlot->ucNeedDel = 1;
        return iRet;
    }
    pstSlot->ucNeedDel = 1;
    Http_Push_SockBuf(pstRecvBuf);
    *pucStatus = pstSlot->ucStatus;
    return -1;
}
