使用c语言一个跨平台的http库
对外提供的API:
_INT Httpclient_Init();  //初始化
_INT Httpclient_Destroy(); //销毁

//以下是异步接口
_INT Httpclient_SendAsyncGetRequest(_UC* pucUrlip,_US usUrlport,_UC* pucUrl, PFN_RECV pfuncRecv, PFN_FINISHED pfuncFinished, PFN_FAILED pfuncFailed, _VOID* vpUserPtr,_UI* puiHandler);
_INT Httpclient_SendAsyncPostRequest(_UC* pucUrlip, _US usUrlport, _UC* pucUrl, _UC* pucParam, _UI uiLen, PFN_RECV pfuncRecv, PFN_FINISHED pfuncFinished, PFN_FAILED pfuncFailed, _VPTR vpUserPtr, _UI* puiHandler);
_INT Httpclient_CancelAsyncRequest(_UI uiHandler);

//以下是同步接口
_UI HttpCreateSyncHandleID();
_INT HttpCloseSyncHandleID(_UI uiHandleID);
_INT Httpclient_SendSyncGetRequest(_UI uiHandleID, _UC* pucUrlip, _US usUrlport,_UC *pucHost, _UC* pucUrl, _UI uiTimeoutSecond, _UC** ppucOutBuffer, _UI* puiOutLen,_UC* pucStatus);
_INT Httpclient_SendSyncPostRequest(_UI uiHandleID, _UC* pucUrlip, _US usUrlport, _UC* pucUrl, _UC* pucParam, _UI uiLen, _UI uiTimeoutSecond, _UC** ppucOutBuffer, _UI* puiOutLen, _UC* pucStatus);
