#ifndef _HTTPPARSER_H_
#define _HTTPPARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HTTP_PARSER_STRICT
# define HTTP_PARSER_STRICT 1
#else
# define HTTP_PARSER_STRICT 0
#endif
    
#define HTTP_MAX_HEADER_SIZE (80*1024)

typedef struct stru_HTTPPARSER{
    /** PRIVATE **/
    unsigned char ucType : 2;
    unsigned char ucFlags : 6;
    unsigned char ucState;
    unsigned char ucHeaderState;
    unsigned int  uiIndex;
        
    unsigned int  uiNRead;
	long long xxlContentSize;
        
    /** READ-ONLY **/
    unsigned short usHttpMajor;
    unsigned short usHttpMinor;
    unsigned short usResponseCode; /* responses only */
    unsigned char ucRequestMethod;    /* requests only */
        
    /* 1 = Upgrade header was present and the parser has exited because of that.
    * 0 = No upgrade header present.
    * Should be checked when http_parser_execute() returns in addition to
    * error checking.
    */
    unsigned char ucUpgrade;
        
    /** PUBLIC **/
	void* pvData; /* A pointer to get hook to the "connection" or "socket" object */
}ST_HTTPPARSER;

typedef int (*FUNC_HTTP_DATA_CB) (ST_HTTPPARSER* pstP, unsigned char* pucData, unsigned int uiLen);
typedef int (*FUNC_HTTP_CB) (ST_HTTPPARSER* pstP);

/* Request Methods */
typedef enum enum_HTTP_METHOD
{ 
	EN_HTTP_METHOD_DELETE    = 0,
	EN_HTTP_METHOD_GET,
	EN_HTTP_METHOD_HEAD,
	EN_HTTP_METHOD_POST,
	EN_HTTP_METHOD_PUT,
   /* pathological */
	EN_HTTP_METHOD_CONNECT,
	EN_HTTP_METHOD_OPTIONS,
	EN_HTTP_METHOD_TRACE,
   /* webdav */
	EN_HTTP_METHOD_COPY,
	EN_HTTP_METHOD_LOCK,
	EN_HTTP_METHOD_MKCOL,
	EN_HTTP_METHOD_MOVE,
	EN_HTTP_METHOD_PROPFIND,
	EN_HTTP_METHOD_PROPPATCH,
	EN_HTTP_METHOD_UNLOCK,
   /* subversion */
	EN_HTTP_METHOD_REPORT,
	EN_HTTP_METHOD_MKACTIVITY,
	EN_HTTP_METHOD_CHECKOUT,
	EN_HTTP_METHOD_MERGE,
   /* upnp */
	EN_HTTP_METHOD_MSEARCH,
	EN_HTTP_METHOD_NOTIFY,
	EN_HTTP_METHOD_SUBSCRIBE,
	EN_HTTP_METHOD_UNSUBSCRIBE,

	EN_HTTP_METHOD_MAX
}EN_HTTP_METHOD;
    
    
typedef enum enum_HTTPPARSER_TYPE{
	EN_HTTPPARSER_REQUEST,
	EN_HTTPPARSER_RESPONSE,
	EN_HTTPPARSER_BOTH
}EN_HTTPPARSER_TYPE;

typedef struct stru_HTTPPARSER_SETTINGS{
    FUNC_HTTP_CB      pfunc_OnMessageBegin;
    FUNC_HTTP_DATA_CB pfunc_OnUrl;
    FUNC_HTTP_DATA_CB pfunc_OnHeaderField;
	FUNC_HTTP_DATA_CB pfunc_OnHeaderValue;
    FUNC_HTTP_CB      pfunc_OnHeadersComplete;
    FUNC_HTTP_DATA_CB pfunc_OnBody;
    FUNC_HTTP_CB      pfunc_OnMessageComplete;
}ST_HTTPPARSER_SETTINGS;

int HttpParser_Init(ST_HTTPPARSER* pParser, _UI enParserType);
unsigned int  HttpParser_Execute(ST_HTTPPARSER* pParser, ST_HTTPPARSER_SETTINGS* pSettings, unsigned char* pucData, unsigned int uiLen);
int HttpParser_KeepAlive(ST_HTTPPARSER* pParser);
unsigned char* HttpParser_MethodToStr(unsigned int enHttpMethod);

    
#ifdef __cplusplus
}
#endif
#endif
