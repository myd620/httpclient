#include "basetype.h"
#include "httpparser.h"

#define HTTPPARSER_MIN(a,b) ((a) < (b) ? (a) : (b))

#define HTTPPARSER_CALLBACK2(func, parser)                                               \
do {                                                                 \
   if (func) {                                          \
	  func(parser));							\
   }                                                                  \
}while (0)

#define HTTPPARSER_MARK(FOR)                                                    \
do {                                                                 \
    FOR##_mark = p;                                                    \
} while (0)

#define HTTPPARSER_CALLBACK_NOCLEAR(FOR)                                        \
do {                                                                 \
    if (FOR##_mark) {                                                  \
		if (pSettings->on_##FOR) {                                        \
			if (0 != pSettings->pfunc_##FOR(pParser,                            \
				FOR##_mark,                         \
				p - FOR##_mark))                    \
			{                                                              \
			return (p - data);                                           \
			}                                                              \
		}                                                                \
	}                                                                  \
} while (0)


#define HTTPPARSER_CALLBACK(FOR)                                                \
do {                                                                 \
    HTTPPARSER_CALLBACK_NOCLEAR(FOR);                                             \
	FOR##_mark = 0;                                                 \
} while (0)


#define STR_HTTP_PROXY_CONNECTION		"proxy-connection"
#define STR_HTTP_CONNECTION			"connection"
#define STR_HTTP_CONTENT_LENGTH		"content-length"
#define STR_HTTP_TRANSFER_ENCODING		"transfer-encoding"
#define STR_HTTP_UPGRADE				"upgrade"
#define STR_HTTP_CHUNKED				"chunked"
#define STR_HTTP_KEEP_ALIVE			"keep-alive"
#define STR_HTTP_CLOSE					"close"


static const char *s_HttpMethodStr[] =
{
	"DELETE",
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "CONNECT",
    "OPTIONS",
    "TRACE",
    "COPY",
    "LOCK",
    "MKCOL",
    "MOVE",
    "PROPFIND",
    "PROPPATCH",
    "UNLOCK",
    "REPORT",
    "MKACTIVITY",
    "CHECKOUT",
    "MERGE",
    "M-SEARCH",
    "NOTIFY",
    "SUBSCRIBE",
    "UNSUBSCRIBE"
};



static const char s_HttpTokens[256] = {
    /*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
    0,       0,       0,       0,       0,       0,       0,       0,
    /*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
    0,       0,       0,       0,       0,       0,       0,       0,
    /*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
    0,       0,       0,       0,       0,       0,       0,       0,
    /*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
    0,       0,       0,       0,       0,       0,       0,       0,
    /*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
    ' ',      '!',     '"',     '#',     '$',     '%',     '&',    '\'',
    /*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
    0,       0,      '*',     '+',      0,      '-',     '.',     '/',
    /*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
    '0',     '1',     '2',     '3',     '4',     '5',     '6',     '7',
    /*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
    '8',     '9',      0,       0,       0,       0,       0,       0,
    /*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
    0,      'a',     'b',     'c',     'd',     'e',     'f',     'g',
    /*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
    'h',     'i',     'j',     'k',     'l',     'm',     'n',     'o',
    /*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
    'p',     'q',     'r',     's',     't',     'u',     'v',     'w',
    /*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
    'x',     'y',     'z',      0,       0,       0,      '^',     '_',
    /*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
    '`',     'a',     'b',     'c',     'd',     'e',     'f',     'g',
    /* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
    'h',     'i',     'j',     'k',     'l',     'm',     'n',     'o',
    /* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
    'p',     'q',     'r',     's',     't',     'u',     'v',     'w',
    /* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
    'x',     'y',     'z',      0,      '|',     '}',     '~',       0 };


static const char s_HttpUnhex[256] =
{
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    ,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    ,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    , 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1
    ,-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1
    ,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    ,-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1
    ,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};


static const char s_HttpNormalUrlChar[256] = {
    /*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
    0,       0,       0,       0,       0,       0,       0,       0,
    /*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
    0,       0,       0,       0,       0,       0,       0,       0,
    /*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
    0,       0,       0,       0,       0,       0,       0,       0,
    /*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
    0,       0,       0,       0,       0,       0,       0,       0,
    /*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
    0,       1,       1,       0,       1,       1,       1,       1,
    /*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
    1,       1,       1,       1,       1,       1,       1,       1,
    /*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
    1,       1,       1,       1,       1,       1,       1,       1,
    /*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
    1,       1,       1,       1,       1,       1,       1,       0,
    /*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
    1,       1,       1,       1,       1,       1,       1,       1,
    /*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
    1,       1,       1,       1,       1,       1,       1,       1,
    /*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
    1,       1,       1,       1,       1,       1,       1,       1,
    /*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
    1,       1,       1,       1,       1,       1,       1,       1,
    /*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
    1,       1,       1,       1,       1,       1,       1,       1,
    /* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
    1,       1,       1,       1,       1,       1,       1,       1,
    /* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
    1,       1,       1,       1,       1,       1,       1,       1,
    /* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
    1,       1,       1,       1,       1,       1,       1,       0 };


typedef enum enum_HTTP_STATE
{ 
	EN_HTTP_STATE_DEAD = 1, /* important that this is > 0 */
    
	EN_HTTP_STATE_START_REQ_OR_RES,
    EN_HTTP_STATE_RES_OR_RESP_H,
    EN_HTTP_STATE_START_RES,
    EN_HTTP_STATE_RES_H,
	EN_HTTP_STATE_RES_HT,
    EN_HTTP_STATE_RES_HTT,
    EN_HTTP_STATE_RES_HTTP,
	EN_HTTP_STATE_RES_FIRST_HTTP_MAJOR,
    EN_HTTP_STATE_RES_HTTP_MAJOR,
	EN_HTTP_STATE_RES_FIRST_HTTP_MINOR,
	EN_HTTP_STATE_RES_HTTP_MINOR,
    EN_HTTP_STATE_RES_FIRST_STATUS_CODE,
	EN_HTTP_STATE_RES_STATUS_CODE,
    EN_HTTP_STATE_RES_STATUS,
    EN_HTTP_STATE_RES_LINE_ALMOST_DONE,
    
	EN_HTTP_STATE_START_REQ,

	EN_HTTP_STATE_REQ_METHOD,
    EN_HTTP_STATE_REQ_SPACES_BEFORE_URL,
    EN_HTTP_STATE_REQ_SCHEMA,
    EN_HTTP_STATE_REQ_SCHEMA_SLASH,
    EN_HTTP_STATE_REQ_SCHEMA_SLASH_SLASH,
	EN_HTTP_STATE_REQ_HOST,
    EN_HTTP_STATE_REQ_PORT,
    EN_HTTP_STATE_REQ_PATH,
    EN_HTTP_STATE_REQ_QUERY_STRING_START,
    EN_HTTP_STATE_REQ_QUERY_STRING,
    EN_HTTP_STATE_REQ_FRAGMENT_START,
    EN_HTTP_STATE_REQ_FRAGMENT,
    EN_HTTP_STATE_REQ_HTTP_START,
    EN_HTTP_STATE_REQ_HTTP_H,
    EN_HTTP_STATE_REQ_HTTP_HT,
    EN_HTTP_STATE_REQ_HTTP_HTT,
    EN_HTTP_STATE_REQ_HTTP_HTTP,
    EN_HTTP_STATE_REQ_FIRST_HTTP_MAJOR,
    EN_HTTP_STATE_REQ_HTTP_MAJOR,
    EN_HTTP_STATE_REQ_FIRST_HTTP_MINOR,
    EN_HTTP_STATE_REQ_HTTP_MINOR,
    EN_HTTP_STATE_REQ_LINE_SLMOST_DONE,
    
	EN_HTTP_STATE_HEADER_FIELD_START,
    EN_HTTP_STATE_HEADER_FIELD,
    EN_HTTP_STATE_HEADER_VALUE_START,
    EN_HTTP_STATE_HEADER_VALUE,
    
	EN_HTTP_STATE_HEADER_ALMOST_DONE,
    
	EN_HTTP_STATE_HEADERS_ALMOST_DONE,

	EN_HTTP_STATE_CHUNK_SIZE_START,
	EN_HTTP_STATE_CHUNK_SIZE,
	EN_HTTP_STATE_CHUNK_SIZE_ALMOST_DONE,
	EN_HTTP_STATE_CHUNK_PARAMETERS,
	EN_HTTP_STATE_CHUNK_DATA,
	EN_HTTP_STATE_CHUNK_DATA_ALMOST_DONE,
	EN_HTTP_STATE_CHUNK_DATA_DONE,
    
	EN_HTTP_STATE_BODY_IDENTITY,
    EN_HTTP_STATE_BODY_IDENTITY_EOF,

	EN_HTTP_STATE_MAX
}EN_HTTP_STATE;


#define HTTPPARSER_PARSING_HEADER(state) (state <= EN_HTTP_STATE_HEADERS_ALMOST_DONE && 0 == (pParser->ucFlags & EN_HTTPPARSER_FLAG_TRAILING))


typedef enum enum_HTTPPARSER_HEADER_STATES{
	EN_HTTPPARSER_HEADER_STATE_GENERAL = 0,
	EN_HTTPPARSER_HEADER_STATE_C,
	EN_HTTPPARSER_HEADER_STATE_CO,
	EN_HTTPPARSER_HEADER_STATE_CON,

	EN_HTTPPARSER_HEADER_STATE_MATCHING_CONNECTION,
	EN_HTTPPARSER_HEADER_STATE_MATCHING_PROXY_CONNECTION,
	EN_HTTPPARSER_HEADER_STATE_MATCHING_CONTENT_LENGTH,
	EN_HTTPPARSER_HEADER_STATE_MATCHING_TRANSFER_ENCODING,
	EN_HTTPPARSER_HEADER_STATE_MATCHING_UPGRADE,

	EN_HTTPPARSER_HEADER_STATE_CONNECTION,
	EN_HTTPPARSER_HEADER_STATE_CONTENT_LENGTH,
	EN_HTTPPARSER_HEADER_STATE_TRANSFER_ENCODING,
	EN_HTTPPARSER_HEADER_STATE_UPGRADE,
	
	EN_HTTPPARSER_HEADER_STATE_MATCHING_TRANSFER_ENCODING_CHUNKED,
	EN_HTTPPARSER_HEADER_STATE_MATCHING_CONNECTION_KEEP_ALIVE,
	EN_HTTPPARSER_HEADER_STATE_MATCHING_CONNECTION_CLOSE,

	EN_HTTPPARSER_HEADER_STATE_TRANSFER_ENCODING_CHUNKED,
	EN_HTTPPARSER_HEADER_STATE_CONNECTION_KEEP_ALIVE,
	EN_HTTPPARSER_HEADER_STATE_CONNECTION_CLOSE,

	EN_HTTPPARSER_HEADER_STATE_MAX
}EN_HTTPPARSER_HEADER_STATES;


typedef enum enum_HTTPPARSER_FLAGS{

	EN_HTTPPARSER_FLAG_CHUNKED               = 1 << 0,
    EN_HTTPPARSER_FLAG_CONNECTION_KEEP_ALIVE = 1 << 1,
    EN_HTTPPARSER_FLAG_CONNECTION_CLOSE      = 1 << 2,
    EN_HTTPPARSER_FLAG_TRAILING              = 1 << 3,
    EN_HTTPPARSER_FLAG_UPGRADE               = 1 << 4,
    EN_HTTPPARSER_FLAG_SKIPBODY              = 1 << 5,

	EN_HTTPPARSER_FLAG_MAX
}EN_HTTPPARSER_FLAGS;


#define CR '\r'
#define LF '\n'
#define HTTPPARSER_LOWER(c) (_UC)(c | 0x20)
#define HTTPPARSER_TOKEN(c) s_HttpTokens[(_UC)c]

#define HTTPPARSER_START_STATE (pParser->ucType == EN_HTTPPARSER_REQUEST ? EN_HTTP_STATE_START_REQ : EN_HTTP_STATE_START_RES)

#if HTTP_PARSER_STRICT
# define HTTP_STRICT_CHECK(cond) if (cond) goto error
# define HTTP_NEW_MESSAGE() ((HttpParser_KeepAlive(pParser) == 0) ? HTTPPARSER_START_STATE : EN_HTTP_STATE_DEAD)
#else
# define HTTP_STRICT_CHECK(cond)
# define HTTP_NEW_MESSAGE() HTTPPARSER_START_STATE
#endif



int HttpParser_Init(ST_HTTPPARSER* pParser, unsigned int enParserType)
{
	pParser->ucType = enParserType;
	pParser->ucState = (enParserType == EN_HTTPPARSER_REQUEST ? EN_HTTP_STATE_START_REQ : (enParserType == EN_HTTPPARSER_RESPONSE ? EN_HTTP_STATE_START_RES : EN_HTTP_STATE_START_REQ_OR_RES));
	pParser->uiNRead = 0;
	pParser->ucUpgrade = 0;
	pParser->ucFlags = 0;
	pParser->ucRequestMethod = 0;
	return 0;
}

unsigned int HttpParser_Execute(ST_HTTPPARSER* pParser, ST_HTTPPARSER_SETTINGS* pSettings, _UC* pucData, unsigned int uiLen)
{
	_UC* pucHeaderField_mark = 0;
	_UC* pucHeaderValue_mark = 0;
	_UC* pucFragment_mark = 0;
    _UC* pucQueryString_mark = 0;
    _UC* pucPath_mark = 0;
	_UC* pucUrl_mark = 0;

	_UC ucC, ucCh;

	_UC* pucP = pucData;
	_UC* pucPe;

	_UI xxlToRead;
	EN_HTTP_STATE enState = (EN_HTTP_STATE)pParser->ucState;
	EN_HTTPPARSER_HEADER_STATES enHeadState = (EN_HTTPPARSER_HEADER_STATES)pParser->ucHeaderState;
	_UI uiIndex = pParser->uiIndex;
	_UI uiNRead = pParser->uiNRead;

	if(uiLen == 0){
		if(enState == EN_HTTP_STATE_BODY_IDENTITY_EOF){
			if(pSettings->pfunc_OnMessageComplete){
				pSettings->pfunc_OnMessageComplete(pParser);
			}
		}
		return 0;
	}

	if(enState == EN_HTTP_STATE_HEADER_FIELD)
		pucHeaderField_mark = pucData;
	if(enState == EN_HTTP_STATE_HEADER_VALUE)
		pucHeaderValue_mark = pucData;
	if(enState == EN_HTTP_STATE_REQ_FRAGMENT)
		pucFragment_mark = pucData;
	if(enState == EN_HTTP_STATE_REQ_QUERY_STRING)
		pucQueryString_mark = pucData;
	if(enState == EN_HTTP_STATE_REQ_PATH)
		pucPath_mark = pucData;
	if(enState == EN_HTTP_STATE_REQ_PATH || enState == EN_HTTP_STATE_REQ_SCHEMA || enState == EN_HTTP_STATE_REQ_SCHEMA_SLASH 
		|| enState == EN_HTTP_STATE_REQ_SCHEMA_SLASH_SLASH || enState == EN_HTTP_STATE_REQ_PORT 
		|| enState == EN_HTTP_STATE_REQ_QUERY_STRING_START || enState == EN_HTTP_STATE_REQ_QUERY_STRING || 
		enState == EN_HTTP_STATE_REQ_HOST || enState == EN_HTTP_STATE_REQ_FRAGMENT_START || enState == EN_HTTP_STATE_REQ_FRAGMENT)
		pucUrl_mark = pucData;

	for(pucP = pucData, pucPe = pucData + uiLen; pucP != pucPe; ++pucP){
		ucCh = *pucP;
		if(enState <= EN_HTTP_STATE_HEADERS_ALMOST_DONE && 0 == (pParser->ucFlags & EN_HTTPPARSER_FLAG_TRAILING)){
			++uiNRead;
			if(uiNRead > HTTP_MAX_HEADER_SIZE) goto error;
		}

		switch(enState){
			case EN_HTTP_STATE_DEAD:
				goto error;
			case EN_HTTP_STATE_START_REQ_OR_RES:
				{
					if(ucCh == CR || ucCh == LF)
						break;
					pParser->ucFlags = 0;
					pParser->xxlContentSize = -1;
					if(pSettings->pfunc_OnMessageBegin){
						pSettings->pfunc_OnMessageBegin(pParser);
					}
					if(ucCh == 'H'){
						enState = EN_HTTP_STATE_RES_OR_RESP_H;
					}else{
						pParser->ucType = EN_HTTPPARSER_REQUEST;
						goto start_req_method_assign;
					}
					break;
				}
			case EN_HTTP_STATE_RES_OR_RESP_H:
				{
					if(ucCh == 'T'){
						pParser->ucType = EN_HTTPPARSER_RESPONSE;
						enState = EN_HTTP_STATE_RES_HT;
					}else{
						if(ucCh != 'E') goto error;
						pParser->ucType = EN_HTTPPARSER_REQUEST;
						pParser->ucRequestMethod = EN_HTTP_METHOD_HEAD;
						uiIndex = 2;
						enState = EN_HTTP_STATE_REQ_METHOD;
					}
					break;
				}
			case EN_HTTP_STATE_START_RES:
				{
					pParser->ucFlags = 0;
					pParser->xxlContentSize = -1;
					if(pSettings->pfunc_OnMessageBegin){
						pSettings->pfunc_OnMessageBegin(pParser);
					}
					switch(ucCh){
						case 'H':
							enState = EN_HTTP_STATE_RES_H;
							break;
						case CR:
						case LF:
							break;
						default:
							goto error;
					}
					break;
				}
			case EN_HTTP_STATE_RES_H:
				{
					HTTP_STRICT_CHECK(ucCh != 'T');
					enState = EN_HTTP_STATE_RES_HT;
					break;
				}
			case EN_HTTP_STATE_RES_HT:
				{
					HTTP_STRICT_CHECK(ucCh != 'T');
					enState = EN_HTTP_STATE_RES_HTT;
					break;
				}
			case EN_HTTP_STATE_RES_HTT:
				{
					HTTP_STRICT_CHECK(ucCh != 'P');
					enState = EN_HTTP_STATE_RES_HTTP;
					break;
				}
			case EN_HTTP_STATE_RES_HTTP:
				{
					HTTP_STRICT_CHECK(ucCh != '/');
					enState = EN_HTTP_STATE_RES_FIRST_HTTP_MAJOR;
					break;
				}
			case EN_HTTP_STATE_RES_FIRST_HTTP_MAJOR:
				{
					if(ucCh < '0' || ucCh > '9') goto error;
					pParser->usHttpMajor = ucCh - '0';
					enState = EN_HTTP_STATE_RES_HTTP_MAJOR;
					break;
				}
			case EN_HTTP_STATE_RES_HTTP_MAJOR:
				{
					if(ucCh == '.'){
						enState = EN_HTTP_STATE_RES_FIRST_HTTP_MINOR;
						break;
					}
					if(ucCh < '0' || ucCh > '9') goto error;
					pParser->usHttpMajor *= 10;
					pParser->usHttpMajor += ucCh - '0';
					if(pParser->usHttpMajor > 999) goto error;
					break;
				}
			case EN_HTTP_STATE_RES_FIRST_HTTP_MINOR:
				{
					if(ucCh < '0' || ucCh > '9') goto error;
					pParser->usHttpMinor = ucCh - '0';
					enState = EN_HTTP_STATE_RES_HTTP_MINOR;
					break;
				}
			case EN_HTTP_STATE_RES_HTTP_MINOR:
				{
					if(ucCh == ' '){
						enState = EN_HTTP_STATE_RES_FIRST_STATUS_CODE;
						break;
					}
					if(ucCh < '0' || ucCh > '9') goto error;
					pParser->usHttpMinor *= 10;
					pParser->usHttpMinor += ucCh - '0';
					if(pParser->usHttpMinor > 999) goto error;
					break;
				}
			case EN_HTTP_STATE_RES_FIRST_STATUS_CODE:
				{
					if(ucCh == ' ') break;
					if(ucCh < '0' || ucCh > '9') goto error;
					pParser->usResponseCode  = ucCh - '0';
					enState = EN_HTTP_STATE_RES_STATUS_CODE;
					break;
				}
			case EN_HTTP_STATE_RES_STATUS_CODE:
				{
					if(ucCh < '0' || ucCh > '9'){
						switch(ucCh){
							case ' ':
								enState = EN_HTTP_STATE_RES_STATUS;
								break;
							case CR:
								enState = EN_HTTP_STATE_RES_LINE_ALMOST_DONE;
								break;
							case LF:
								enState = EN_HTTP_STATE_HEADER_FIELD_START;
								break;
							default:
								goto error;
						}
						break;
					}
					pParser->usResponseCode *= 10;
					pParser->usResponseCode += ucCh - '0';
					if(pParser->usResponseCode > 999) goto error;
					break;
				}
			case EN_HTTP_STATE_RES_STATUS:
				{
					if(ucCh == CR){
						enState = EN_HTTP_STATE_RES_LINE_ALMOST_DONE;
					}else if(ucCh == LF){
						enState = EN_HTTP_STATE_HEADER_FIELD_START;
					}
					break;
				}
			case EN_HTTP_STATE_RES_LINE_ALMOST_DONE:
				{
					HTTP_STRICT_CHECK(ucCh != LF);
					enState = EN_HTTP_STATE_HEADER_FIELD_START;
					break;
				}
			case EN_HTTP_STATE_START_REQ:
				{
					if(ucCh == CR || ucCh == LF) break;
					pParser->ucFlags = 0; pParser->xxlContentSize = -1;
					if(pSettings->pfunc_OnMessageBegin){
						pSettings->pfunc_OnMessageBegin(pParser);
					}
					if(ucCh < 'A' || ucCh > 'Z') goto error;
start_req_method_assign:
					pParser->ucRequestMethod = 0;
					uiIndex = 1;
					switch(ucCh){
						case 'C': pParser->ucRequestMethod = EN_HTTP_METHOD_CONNECT; break;
						case 'D': pParser->ucRequestMethod = EN_HTTP_METHOD_DELETE; break;
						case 'G': pParser->ucRequestMethod = EN_HTTP_METHOD_GET; break;
						case 'H': pParser->ucRequestMethod = EN_HTTP_METHOD_HEAD; break;
						case 'L': pParser->ucRequestMethod = EN_HTTP_METHOD_LOCK; break;
						case 'M': pParser->ucRequestMethod = EN_HTTP_METHOD_MKCOL; break;
						case 'N': pParser->ucRequestMethod = EN_HTTP_METHOD_NOTIFY; break;
						case 'O': pParser->ucRequestMethod = EN_HTTP_METHOD_OPTIONS; break;
						case 'P': pParser->ucRequestMethod = EN_HTTP_METHOD_POST; break;
						case 'R': pParser->ucRequestMethod = EN_HTTP_METHOD_REPORT; break;
						case 'S': pParser->ucRequestMethod = EN_HTTP_METHOD_SUBSCRIBE; break;
						case 'T': pParser->ucRequestMethod = EN_HTTP_METHOD_TRACE; break;
						case 'U': pParser->ucRequestMethod = EN_HTTP_METHOD_UNLOCK; break;
						default: goto error;
					}
					enState = EN_HTTP_STATE_REQ_METHOD;
					break;
				}
			case EN_HTTP_STATE_REQ_METHOD:
				{
					_UC* pucMatcher = HttpParser_MethodToStr(pParser->ucRequestMethod);
					if(ucCh == '\0') goto error;
					if(ucCh == ' ' && pucMatcher[uiIndex] == '\0'){
						enState = EN_HTTP_STATE_REQ_SPACES_BEFORE_URL;
					}else if(ucCh == pucMatcher[uiIndex]){ ;
					}else if(pParser->ucRequestMethod == EN_HTTP_METHOD_CONNECT){
						if(uiIndex == 1 && ucCh == 'H'){
							pParser->ucRequestMethod = EN_HTTP_METHOD_CHECKOUT;
						}else if(uiIndex == 2 && ucCh == 'P'){
							pParser->ucRequestMethod = EN_HTTP_METHOD_COPY;
						}
					}else if(pParser->ucRequestMethod == EN_HTTP_METHOD_MKCOL){
						if(uiIndex == 1 && ucCh == 'O'){
							pParser->ucRequestMethod = EN_HTTP_METHOD_MOVE;
						}else if(uiIndex == 1 && ucCh == 'E'){
							pParser->ucRequestMethod = EN_HTTP_METHOD_MERGE;
						}else if(uiIndex == 1 && ucCh == '-'){
							pParser->ucRequestMethod = EN_HTTP_METHOD_MSEARCH;
						}else if(uiIndex == 2 && ucCh == 'A'){
							pParser->ucRequestMethod = EN_HTTP_METHOD_MKACTIVITY;
						}
					}else if(uiIndex == 1 && pParser->ucRequestMethod == EN_HTTP_METHOD_POST && ucCh == 'R'){
						pParser->ucRequestMethod = EN_HTTP_METHOD_PROPFIND;
					}else if(uiIndex == 1 && pParser->ucRequestMethod == EN_HTTP_METHOD_POST && ucCh == 'U'){
						pParser->ucRequestMethod = EN_HTTP_METHOD_PUT;
					}else if(uiIndex == 2 && pParser->ucRequestMethod == EN_HTTP_METHOD_UNLOCK && ucCh == 'S'){
						pParser->ucRequestMethod = EN_HTTP_METHOD_UNSUBSCRIBE;
					}else if(uiIndex == 4 && pParser->ucRequestMethod == EN_HTTP_METHOD_PROPFIND && ucCh == 'P'){
						pParser->ucRequestMethod = EN_HTTP_METHOD_PROPPATCH;
					}else{
						goto error;
					}
					++uiIndex;
					break;
				}
			case EN_HTTP_STATE_REQ_SPACES_BEFORE_URL:
				{
					if(ucCh == ' ') break;
					if(ucCh == '/' || ucCh == '*'){
						pucUrl_mark = pucP;
						pucPath_mark = pucP;
						enState = EN_HTTP_STATE_REQ_PATH;
						break;
					}
					ucC = HTTPPARSER_LOWER(ucCh);
					if(ucC >= 'a' && ucC <= 'z'){
						pucUrl_mark = pucP;
						enState = EN_HTTP_STATE_REQ_SCHEMA;
						break;
					}
					goto error;
				}
			case EN_HTTP_STATE_REQ_SCHEMA:
				{
					ucC = HTTPPARSER_LOWER(ucCh);
					if(ucC > 'a' && ucC <= 'z') break;
					if(ucCh == ':'){
						enState = EN_HTTP_STATE_REQ_SCHEMA_SLASH;
						break;
					}else if(ucCh == '.'){
						enState = EN_HTTP_STATE_REQ_HOST;
						break;
					}else if(ucCh >= '0' && ucCh <= '9'){
						enState = EN_HTTP_STATE_REQ_HOST;
						break;
					}
					goto error;
				}
			case EN_HTTP_STATE_REQ_SCHEMA_SLASH:
				{
					HTTP_STRICT_CHECK(ucCh != '/');
					enState = EN_HTTP_STATE_REQ_SCHEMA_SLASH_SLASH;
					break;
				}
			case EN_HTTP_STATE_REQ_SCHEMA_SLASH_SLASH:
				{
					HTTP_STRICT_CHECK(ucCh != '/');
					enState = EN_HTTP_STATE_REQ_HOST;
					break;
				}
			case EN_HTTP_STATE_REQ_HOST:
				{
					ucC = HTTPPARSER_LOWER(ucCh);
					if(ucC >= 'a' && ucC <= 'z') break;
					if((ucCh >= '0' && ucCh <= '9') || ucCh == '.' || ucCh == '-') break;
					switch(ucCh){
						case ':':
							enState = EN_HTTP_STATE_REQ_PORT;
							break;
						case '/':
							{
								pucPath_mark = pucP;
								enState = EN_HTTP_STATE_REQ_PATH;
								break;
							}
						case ' ':
							{
								if(pSettings->pfunc_OnUrl){
									pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
									pucUrl_mark = 0;
								}
								enState = EN_HTTP_STATE_REQ_HTTP_START;
								break;
							}
						default:
							goto error;
					}
					break;
				}
			case EN_HTTP_STATE_REQ_PORT:
				{
					if(ucCh >= '0' && ucCh <= '9') break;
					switch(ucCh){
						case '/':
							{
								pucPath_mark = pucP;
								enState = EN_HTTP_STATE_REQ_PATH;
								break;
							}
						case ' ':
							{
								if(pSettings->pfunc_OnUrl){
									pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
									pucUrl_mark = 0;
								}
								enState = EN_HTTP_STATE_REQ_HTTP_START;
								break;
							}
						default:
							goto error;
					}
				}
			case EN_HTTP_STATE_REQ_PATH:
				{
					if(s_HttpNormalUrlChar[ucCh]) break;
					switch(ucCh){
						case ' ':
							{
								if(pSettings->pfunc_OnUrl){
									pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
									pucUrl_mark = 0;
								}
								enState = EN_HTTP_STATE_REQ_HTTP_START;
								break;
							}
						case CR:
							{
								if(pSettings->pfunc_OnUrl){
									pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
									pucUrl_mark = 0;
								}
								pParser->usHttpMajor = 0;
								pParser->usHttpMinor = 9;
								enState = EN_HTTP_STATE_REQ_LINE_SLMOST_DONE;
								break;
							}
						case LF:
							{
								if(pSettings->pfunc_OnUrl){
									pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
									pucUrl_mark = 0;
								}
								pParser->usHttpMajor = 0;
								pParser->usHttpMinor = 9;
								enState = EN_HTTP_STATE_HEADER_FIELD_START;
								break;
							}
						case '?':
							enState = EN_HTTP_STATE_REQ_QUERY_STRING_START;
							break;
						case '#':
							enState = EN_HTTP_STATE_REQ_FRAGMENT_START;
							break;
						default:
							goto error;
					}
					break;
				}
			case EN_HTTP_STATE_REQ_QUERY_STRING_START:
				{
					if(s_HttpNormalUrlChar[ucCh]){
						pucQueryString_mark = pucP;
						enState = EN_HTTP_STATE_REQ_QUERY_STRING;
						break;
					}
					switch(ucCh){
						case '?': break;
						case ' ':
							{
								if(pSettings->pfunc_OnUrl){
									pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
									pucUrl_mark = 0;
								}
								enState = EN_HTTP_STATE_REQ_HTTP_START;
								break;
							}
						case CR:
							{
								if(pSettings->pfunc_OnUrl){
									pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
									pucUrl_mark = 0;
								}
								pParser->usHttpMajor = 0;
								pParser->usHttpMinor = 9;
								enState = EN_HTTP_STATE_REQ_LINE_SLMOST_DONE;
								break;
							}
						case LF:
							{
								if(pSettings->pfunc_OnUrl){
									pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
									pucUrl_mark = 0;
								}
								pParser->usHttpMajor = 0;
								pParser->usHttpMinor = 9;
								enState = EN_HTTP_STATE_HEADER_FIELD_START;
								break;
							}
						case '#':
							enState = EN_HTTP_STATE_REQ_FRAGMENT_START;
							break;
						default: goto error;
					}
					break;
				}
			case EN_HTTP_STATE_REQ_QUERY_STRING:
				{
					if(s_HttpNormalUrlChar[ucCh]) break;
					switch(ucCh){
						case '?': break;
						case ' ':
							{
								if(pSettings->pfunc_OnUrl){
									pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
									pucUrl_mark = 0;
								}
								enState = EN_HTTP_STATE_REQ_HTTP_START;
								break;
							}
						case CR:
							{
								if(pSettings->pfunc_OnUrl){
									pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
									pucUrl_mark = 0;
								}
								pParser->usHttpMajor = 0;
								pParser->usHttpMinor = 9;
								enState = EN_HTTP_STATE_REQ_LINE_SLMOST_DONE;
								break;
							}
						case LF:
							{
								if(pSettings->pfunc_OnUrl){
									pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
									pucUrl_mark = 0;
								}
								pParser->usHttpMajor = 0;
								pParser->usHttpMinor = 9;
								enState = EN_HTTP_STATE_HEADER_FIELD_START;
								break;
							}
						case '#':
							enState = EN_HTTP_STATE_REQ_FRAGMENT_START;
							break;
						default: goto error;
					}
				}
			case EN_HTTP_STATE_REQ_FRAGMENT_START:
				{
					if(s_HttpNormalUrlChar[ucCh]){
						pucFragment_mark = pucP;
						enState = EN_HTTP_STATE_REQ_FRAGMENT;
						break;
					}
					switch(ucCh){
						case ' ':
							{
								if(pSettings->pfunc_OnUrl){
									pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
									pucUrl_mark = 0;
								}
								enState = EN_HTTP_STATE_REQ_HTTP_START;
								break;
							}
						case CR:
							{
								if(pSettings->pfunc_OnUrl){
									pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
									pucUrl_mark = 0;
								}
								pParser->usHttpMajor = 0;
								pParser->usHttpMinor = 9;
								enState = EN_HTTP_STATE_REQ_LINE_SLMOST_DONE;
								break;
							}
						case LF:
							{
								if(pSettings->pfunc_OnUrl){
									pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
									pucUrl_mark = 0;
								}
								pParser->usHttpMajor = 0;
								pParser->usHttpMinor = 9;
								enState = EN_HTTP_STATE_HEADER_FIELD_START;
								break;
							}
						case '?': 
							pucFragment_mark = pucP;
							enState = EN_HTTP_STATE_REQ_FRAGMENT;
							break;
						case '#': break;
						default: goto error;
					}
					break;
				}
			case EN_HTTP_STATE_REQ_FRAGMENT:
				{
					if(s_HttpNormalUrlChar[ucCh]) break;
					switch(ucCh){
						case ' ':
							{
								if(pSettings->pfunc_OnUrl){
									pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
									pucUrl_mark = 0;
								}
								enState = EN_HTTP_STATE_REQ_HTTP_START;
								break;
							}
						case CR:
							{
								if(pSettings->pfunc_OnUrl){
									pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
									pucUrl_mark = 0;
								}
								pParser->usHttpMajor = 0;
								pParser->usHttpMinor = 9;
								enState = EN_HTTP_STATE_REQ_LINE_SLMOST_DONE;
								break;
							}
						case LF:
							{
								if(pSettings->pfunc_OnUrl){
									pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
									pucUrl_mark = 0;
								}
								pParser->usHttpMajor = 0;
								pParser->usHttpMinor = 9;
								enState = EN_HTTP_STATE_HEADER_FIELD_START;
								break;
							}
						case '?': break;
						case '#': break;
						default: goto error;
					}
					break;
				}
			case EN_HTTP_STATE_REQ_HTTP_START:
				{
					switch(ucCh){
						case 'H':
							enState = EN_HTTP_STATE_REQ_HTTP_H;
							break;
						case ' ':
							break;
						default: goto error;
					}
					break;
				}
			case EN_HTTP_STATE_REQ_HTTP_H:
				{
					HTTP_STRICT_CHECK(ucCh != 'T');
					enState = EN_HTTP_STATE_REQ_HTTP_HT;
					break;
				}
			case EN_HTTP_STATE_REQ_HTTP_HT:
				{
					HTTP_STRICT_CHECK(ucCh != 'T');
					enState = EN_HTTP_STATE_REQ_HTTP_HTT;
					break;
				}
			case EN_HTTP_STATE_REQ_HTTP_HTT:
				{
					HTTP_STRICT_CHECK(ucCh != 'P');
					enState = EN_HTTP_STATE_REQ_HTTP_HTTP;
					break;
				}
			case EN_HTTP_STATE_REQ_HTTP_HTTP:
				{
					HTTP_STRICT_CHECK(ucCh != '/');
					enState = EN_HTTP_STATE_REQ_FIRST_HTTP_MAJOR;
					break;
				}
			case EN_HTTP_STATE_REQ_FIRST_HTTP_MAJOR:
				{
					if(ucCh < '1' || ucCh > '9') goto error;
					pParser->usHttpMajor = ucCh - '0';
					enState = EN_HTTP_STATE_REQ_HTTP_MAJOR;
					break;
				}
			case EN_HTTP_STATE_REQ_HTTP_MAJOR:
				{
					if(ucCh == '.'){
						enState = EN_HTTP_STATE_REQ_FIRST_HTTP_MINOR;
						break;
					}
					if(ucCh < '0' || ucCh > '9') goto error;
					pParser->usHttpMajor *= 10;
					pParser->usHttpMajor += ucCh - '0';
					if(pParser->usHttpMajor > 999) goto error;
					break;
				}
			case EN_HTTP_STATE_REQ_FIRST_HTTP_MINOR:
				{
					if(ucCh < '1' || ucCh > '9') goto error;
					pParser->usHttpMinor = ucCh - '0';
					enState = EN_HTTP_STATE_REQ_HTTP_MINOR;
					break;
				}
			case EN_HTTP_STATE_REQ_HTTP_MINOR:
				{
					if(ucCh == CR){
						enState = EN_HTTP_STATE_REQ_LINE_SLMOST_DONE;
						break;
					}else if(ucCh == LF){
						enState = EN_HTTP_STATE_HEADER_FIELD_START;
						break;
					}
					if(ucCh < '0' || ucCh > '9') goto error;
					pParser->usHttpMinor *= 10;
					pParser->usHttpMinor += ucCh - '0';
					if(pParser->usHttpMinor > 999) goto error;
					break;
				}
			case EN_HTTP_STATE_REQ_LINE_SLMOST_DONE:
				{
					if(ucCh != LF) goto error;
					enState = EN_HTTP_STATE_HEADER_FIELD_START;
					break;
				}
			case EN_HTTP_STATE_HEADER_FIELD_START:
				{
					if(ucCh == ' ') break;
					if(ucCh == CR){
						enState = EN_HTTP_STATE_HEADERS_ALMOST_DONE;
						break;
					}else if(ucCh == LF){
						enState = EN_HTTP_STATE_HEADERS_ALMOST_DONE;
						goto headers_almost_done;
					}
					ucC = s_HttpTokens[ucCh];
					if(!ucC) goto error;
					pucHeaderField_mark = pucP;
					uiIndex = 0;
					enState = EN_HTTP_STATE_HEADER_FIELD;
					switch(ucC){
						case 'c':
							enHeadState = EN_HTTPPARSER_HEADER_STATE_C;
							break;
						case 'p':
							enHeadState = EN_HTTPPARSER_HEADER_STATE_MATCHING_PROXY_CONNECTION;
							break;
						case 't':
							enHeadState = EN_HTTPPARSER_HEADER_STATE_MATCHING_TRANSFER_ENCODING;
							break;
						case 'u':
							enHeadState = EN_HTTPPARSER_HEADER_STATE_MATCHING_UPGRADE;
							break;
						default:
							enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
							break;
					}
					break;
				}
			case EN_HTTP_STATE_HEADER_FIELD:
				{
					if(ucCh == ' ') break;
					ucC = s_HttpTokens[ucCh];
					if(ucC){
						switch(enHeadState){
							case EN_HTTPPARSER_HEADER_STATE_GENERAL: break;
							case EN_HTTPPARSER_HEADER_STATE_C:
								++uiIndex;
								enHeadState = (ucC == 'o' ? EN_HTTPPARSER_HEADER_STATE_CO : EN_HTTPPARSER_HEADER_STATE_GENERAL);
								break;
							case EN_HTTPPARSER_HEADER_STATE_CO:
								++uiIndex;
								enHeadState = (ucC == 'n' ? EN_HTTPPARSER_HEADER_STATE_CON : EN_HTTPPARSER_HEADER_STATE_GENERAL);
								break;
							case EN_HTTPPARSER_HEADER_STATE_CON:
								{
									++uiIndex;
									switch(ucC){
										case 'n':
											enHeadState = EN_HTTPPARSER_HEADER_STATE_MATCHING_CONNECTION;
											break;
										case 't':
											enHeadState = EN_HTTPPARSER_HEADER_STATE_MATCHING_CONTENT_LENGTH;
											break;
										default:
											enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
											break;
									}
									break;
								}
							case EN_HTTPPARSER_HEADER_STATE_MATCHING_CONNECTION:
								{
									++uiIndex;
									if((uiIndex > sizeof(STR_HTTP_CONNECTION) - 1) || (ucC != STR_HTTP_CONNECTION[uiIndex])){
										enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
									}else if(uiIndex == sizeof(STR_HTTP_CONNECTION) - 2){
										enHeadState = EN_HTTPPARSER_HEADER_STATE_CONNECTION;
									}
									break;
								}
							case EN_HTTPPARSER_HEADER_STATE_MATCHING_PROXY_CONNECTION:
								{
									++uiIndex;
									if((uiIndex > sizeof(STR_HTTP_PROXY_CONNECTION) - 1) || (ucC != STR_HTTP_PROXY_CONNECTION[uiIndex])){
										enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
									}else if(uiIndex == sizeof(STR_HTTP_PROXY_CONNECTION) - 2){
										enHeadState = EN_HTTPPARSER_HEADER_STATE_CONNECTION;
									}
									break;
								}
							case EN_HTTPPARSER_HEADER_STATE_MATCHING_CONTENT_LENGTH:
								{
									++uiIndex;
									if((uiIndex > sizeof(STR_HTTP_CONTENT_LENGTH) - 1) || (ucC != STR_HTTP_CONTENT_LENGTH[uiIndex])){
										enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
									}else if(uiIndex == sizeof(STR_HTTP_CONTENT_LENGTH) - 2){
										enHeadState = EN_HTTPPARSER_HEADER_STATE_CONTENT_LENGTH;
									}
									break;
								}
							case EN_HTTPPARSER_HEADER_STATE_MATCHING_TRANSFER_ENCODING:
								{
									++uiIndex;
									if((uiIndex > sizeof(STR_HTTP_TRANSFER_ENCODING) - 1) || (ucC != STR_HTTP_TRANSFER_ENCODING[uiIndex])){
										enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
									}else if(uiIndex == sizeof(STR_HTTP_TRANSFER_ENCODING) - 2){
										enHeadState = EN_HTTPPARSER_HEADER_STATE_TRANSFER_ENCODING;
									}
									break;
								}
							case EN_HTTPPARSER_HEADER_STATE_MATCHING_UPGRADE:
								{
									++uiIndex;
									if((uiIndex > sizeof(STR_HTTP_UPGRADE) - 1) || (ucC != STR_HTTP_UPGRADE[uiIndex])){
										enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
									}else if(uiIndex == sizeof(STR_HTTP_UPGRADE) - 2){
										enHeadState = EN_HTTPPARSER_HEADER_STATE_UPGRADE;
									}
									break;
								}
							case EN_HTTPPARSER_HEADER_STATE_CONNECTION:
							case EN_HTTPPARSER_HEADER_STATE_CONTENT_LENGTH:
							case EN_HTTPPARSER_HEADER_STATE_TRANSFER_ENCODING:
							case EN_HTTPPARSER_HEADER_STATE_UPGRADE:
								if(ucCh != ' ') enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
								break;
							default: break; // unknown header.
						}
						break;
					}
					if(ucCh == ':'){
						if(pSettings->pfunc_OnHeaderField){
							pSettings->pfunc_OnHeaderField(pParser, pucHeaderField_mark, SAFE_UI(pucP - pucHeaderField_mark));
							pucHeaderField_mark = 0;
						}
						enState = EN_HTTP_STATE_HEADER_VALUE_START;
						break;
					}else if(ucCh == CR){
						enState = EN_HTTP_STATE_HEADER_ALMOST_DONE;
						if(pSettings->pfunc_OnHeaderField){
							pSettings->pfunc_OnHeaderField(pParser, pucHeaderField_mark, SAFE_UI(pucP - pucHeaderField_mark));
							pucHeaderField_mark = 0;
						}
						break;
					}else if(ucCh == LF){
						if(pSettings->pfunc_OnHeaderField){
							pSettings->pfunc_OnHeaderField(pParser, pucHeaderField_mark, SAFE_UI(pucP - pucHeaderField_mark));
							pucHeaderField_mark = 0;
						}
						enState = EN_HTTP_STATE_HEADER_FIELD_START;
						break;
					}
					goto error;
				}
			case EN_HTTP_STATE_HEADER_VALUE_START:
				{
					if(ucCh == ' ') break;
					pucHeaderValue_mark = pucP;
					enState = EN_HTTP_STATE_HEADER_VALUE;
					uiIndex = 0;
					ucC = HTTPPARSER_LOWER(ucCh);
					if(ucCh == CR){
						if(pSettings->pfunc_OnHeaderValue){
							pSettings->pfunc_OnHeaderValue(pParser, pucHeaderValue_mark, SAFE_UI(pucP - pucHeaderValue_mark));
							pucHeaderValue_mark = 0;
						}
						enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
						enState = EN_HTTP_STATE_HEADER_ALMOST_DONE;
					}else if(ucCh == LF){
						if(pSettings->pfunc_OnHeaderValue){
							pSettings->pfunc_OnHeaderValue(pParser, pucHeaderValue_mark, SAFE_UI(pucP - pucHeaderValue_mark));
							pucHeaderValue_mark = 0;
						}
						enState = EN_HTTP_STATE_HEADER_FIELD_START;
						break;
					}
					switch(enHeadState){
						case EN_HTTPPARSER_HEADER_STATE_UPGRADE:
							{
								pParser->ucUpgrade |= EN_HTTPPARSER_FLAG_UPGRADE;
								enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
								break;
							}
						case EN_HTTPPARSER_HEADER_STATE_TRANSFER_ENCODING:
							{
								if(ucC == 'c'){
									enHeadState = EN_HTTPPARSER_HEADER_STATE_MATCHING_TRANSFER_ENCODING_CHUNKED;
								}else{
									enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
								}
								break;
							}
						case EN_HTTPPARSER_HEADER_STATE_CONTENT_LENGTH:
							{
								if(ucCh < '0' || ucCh > '9') goto error;
								pParser->xxlContentSize = ucCh - '0';
								break;
							}
						case EN_HTTPPARSER_HEADER_STATE_CONNECTION:
							{
								if(ucC == 'k'){
									enHeadState = EN_HTTPPARSER_HEADER_STATE_MATCHING_CONNECTION_KEEP_ALIVE;
								}else if(ucC == 'c'){
									enHeadState = EN_HTTPPARSER_HEADER_STATE_MATCHING_CONNECTION_CLOSE;
								}else{
									enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
								}
								break;
							}
						default:
							enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
							break;
					}
					break;
				}
			case EN_HTTP_STATE_HEADER_VALUE:
				{
					ucC = HTTPPARSER_LOWER(ucCh);
					if(ucCh == CR){
						if(pSettings->pfunc_OnHeaderValue){
							pSettings->pfunc_OnHeaderValue(pParser, pucHeaderValue_mark, SAFE_UI(pucP - pucHeaderValue_mark));
							pucHeaderValue_mark = 0;
						}
						//enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
						enState = EN_HTTP_STATE_HEADER_ALMOST_DONE;
					}else if(ucCh == LF){
						if(pSettings->pfunc_OnHeaderValue){
							pSettings->pfunc_OnHeaderValue(pParser, pucHeaderValue_mark, SAFE_UI(pucP - pucHeaderValue_mark));
							pucHeaderValue_mark = 0;
						}
						goto header_almost_done;
						break;
					}
					switch(enHeadState){
						case EN_HTTPPARSER_HEADER_STATE_GENERAL: break;
						case EN_HTTPPARSER_HEADER_STATE_CONNECTION:
						case EN_HTTPPARSER_HEADER_STATE_TRANSFER_ENCODING:
							break;
						case EN_HTTPPARSER_HEADER_STATE_CONTENT_LENGTH:
							{
								if(ucCh == ' ') break;
								if(ucCh == CR){
									enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
									break;
								}
								if(ucCh < '0' || ucCh > '9') goto error;
								pParser->xxlContentSize *= 10;
								pParser->xxlContentSize += ucCh - '0';
								break;
							}
						case EN_HTTPPARSER_HEADER_STATE_MATCHING_TRANSFER_ENCODING_CHUNKED:
							{
								++uiIndex;
								if((uiIndex > sizeof(STR_HTTP_CHUNKED) -1) || ucC != STR_HTTP_CHUNKED[uiIndex]){
									enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
								}else if(uiIndex == sizeof(STR_HTTP_CHUNKED) - 2) {
									enHeadState = EN_HTTPPARSER_HEADER_STATE_TRANSFER_ENCODING_CHUNKED;
								}
								break;
							}
						case EN_HTTPPARSER_HEADER_STATE_MATCHING_CONNECTION_KEEP_ALIVE:
							{
								++uiIndex;
								if((uiIndex > sizeof(STR_HTTP_KEEP_ALIVE) -1) || ucC != STR_HTTP_KEEP_ALIVE[uiIndex]){
									enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
								}else if(uiIndex == sizeof(STR_HTTP_KEEP_ALIVE) - 2) {
									enHeadState = EN_HTTPPARSER_HEADER_STATE_CONNECTION_KEEP_ALIVE;
								}
								break;
							}
						case EN_HTTPPARSER_HEADER_STATE_MATCHING_CONNECTION_CLOSE:
							{
								++uiIndex;
								if((uiIndex > sizeof(STR_HTTP_CLOSE) -1) || ucC != STR_HTTP_CLOSE[uiIndex]){
									enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
								}else if(uiIndex == sizeof(STR_HTTP_CLOSE) - 2) {
									enHeadState = EN_HTTPPARSER_HEADER_STATE_CONNECTION_CLOSE;
								}
								break;
							}
						case EN_HTTPPARSER_HEADER_STATE_TRANSFER_ENCODING_CHUNKED:
                            break;
						case EN_HTTPPARSER_HEADER_STATE_CONNECTION_KEEP_ALIVE:
						case EN_HTTPPARSER_HEADER_STATE_CONNECTION_CLOSE:
							if(ucCh != ' ')  enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
							break;
						default:
							enState = EN_HTTP_STATE_HEADER_VALUE;
							enHeadState = EN_HTTPPARSER_HEADER_STATE_GENERAL;
							break;
					}
					break;
				}
			case EN_HTTP_STATE_HEADER_ALMOST_DONE:
header_almost_done:
				{
					HTTP_STRICT_CHECK(ucCh != LF);
					enState = EN_HTTP_STATE_HEADER_FIELD_START;
					switch(enHeadState){
						case EN_HTTPPARSER_HEADER_STATE_CONNECTION_KEEP_ALIVE:
							pParser->ucFlags |= EN_HTTPPARSER_FLAG_CONNECTION_KEEP_ALIVE;
							break;
						case EN_HTTPPARSER_HEADER_STATE_CONNECTION_CLOSE:
							pParser->ucFlags |= EN_HTTPPARSER_FLAG_CONNECTION_CLOSE;
							break;
						case EN_HTTPPARSER_HEADER_STATE_TRANSFER_ENCODING_CHUNKED:
							pParser->ucFlags |= EN_HTTPPARSER_FLAG_CHUNKED;
							break;
						default:
							break;
					}
					break;
				}
			case EN_HTTP_STATE_HEADERS_ALMOST_DONE:
headers_almost_done:
				{
					HTTP_STRICT_CHECK(ucCh != LF);
					if(pParser->ucFlags & EN_HTTPPARSER_FLAG_TRAILING){
						if(pSettings->pfunc_OnMessageComplete){
							pSettings->pfunc_OnMessageComplete(pParser);
						}
						enState = HTTP_NEW_MESSAGE();
						break;
					}
					uiNRead = 0;
					if(pParser->ucFlags & EN_HTTPPARSER_FLAG_UPGRADE || pParser->ucRequestMethod == EN_HTTP_METHOD_CONNECT){
						pParser->ucUpgrade = 1;
					}
					if(pSettings->pfunc_OnHeadersComplete){
						if(pSettings->pfunc_OnHeadersComplete(pParser) == 0){
							//break;
						}else{
							pParser->ucFlags |= EN_HTTPPARSER_FLAG_SKIPBODY;
							//break;
						}
					}
					if(pParser->ucUpgrade){
						if(pSettings->pfunc_OnMessageComplete){
							pSettings->pfunc_OnMessageComplete(pParser);
						}
						return SAFE_UI(pucP - pucData);
					}
					if(pParser->ucFlags & EN_HTTPPARSER_FLAG_SKIPBODY){
						if(pSettings->pfunc_OnMessageComplete){
							pSettings->pfunc_OnMessageComplete(pParser);
						}
						enState = HTTP_NEW_MESSAGE();
					}else if(pParser->ucFlags & EN_HTTPPARSER_FLAG_CHUNKED){
						enState = EN_HTTP_STATE_CHUNK_SIZE_START;
					}else{
						if(pParser->xxlContentSize == 0){
							if(pSettings->pfunc_OnMessageComplete){
								pSettings->pfunc_OnMessageComplete(pParser);
							}
							enState = HTTP_NEW_MESSAGE();
						}else if(pParser->xxlContentSize > 0){
							enState = EN_HTTP_STATE_BODY_IDENTITY;
						}else{
							if((pParser->ucType == EN_HTTPPARSER_REQUEST) || (HttpParser_KeepAlive(pParser) == 0)){
								if(pSettings->pfunc_OnMessageComplete){
									pSettings->pfunc_OnMessageComplete(pParser);
								}
								enState = HTTP_NEW_MESSAGE();
							}else{
								enState = EN_HTTP_STATE_BODY_IDENTITY_EOF;
							}
						}
					}
					break;
				}
			case EN_HTTP_STATE_BODY_IDENTITY:
				{
					xxlToRead = SAFE_UI(pucPe - pucP);
					if(xxlToRead > 0){
						if(pSettings->pfunc_OnBody){
							pSettings->pfunc_OnBody(pParser, pucP, xxlToRead);
						}
						pucP += xxlToRead - 1;
						pParser->xxlContentSize -= xxlToRead;
						if(pParser->xxlContentSize <= 0){
							if(pSettings->pfunc_OnMessageComplete){
								pSettings->pfunc_OnMessageComplete(pParser);
							}
							enState = HTTP_NEW_MESSAGE();
						}
					}
					break;
				}
			case EN_HTTP_STATE_BODY_IDENTITY_EOF:
				{
					xxlToRead = SAFE_UI(pucPe - pucP);
					if(xxlToRead > 0){
						if(pSettings->pfunc_OnBody){
							pSettings->pfunc_OnBody(pParser, pucP, xxlToRead);
						}
						pucP += xxlToRead - 1;
					}
					break;
				}
			case EN_HTTP_STATE_CHUNK_SIZE_START:
				{
					char c = s_HttpUnhex[ucCh];
					if(c == -1) goto error;
					pParser->xxlContentSize = c;
					enState = EN_HTTP_STATE_CHUNK_SIZE;
					break;
				}
			case EN_HTTP_STATE_CHUNK_SIZE:
				{
					char c = s_HttpUnhex[ucCh];
					if(ucCh == CR){
						enState = EN_HTTP_STATE_CHUNK_SIZE_ALMOST_DONE;
						break;
					}
					if(c == -1){
						if(ucCh == ':' || ucCh == ' '){
							enState = EN_HTTP_STATE_CHUNK_PARAMETERS;
							break;
						}
						goto error;
					}
					pParser->xxlContentSize *= 16;
					pParser->xxlContentSize += c;
					break;
				}
			case EN_HTTP_STATE_CHUNK_PARAMETERS:
				{
					if(ucCh == CR){
						enState = EN_HTTP_STATE_CHUNK_SIZE_ALMOST_DONE;
						break;
					}
					break;
				}
			case EN_HTTP_STATE_CHUNK_SIZE_ALMOST_DONE:
				{
					HTTP_STRICT_CHECK(ucCh != LF);
					if(pParser->xxlContentSize == 0){
						pParser->ucFlags |= EN_HTTPPARSER_FLAG_TRAILING;
						enState = EN_HTTP_STATE_HEADER_FIELD_START;
					}else{
						enState = EN_HTTP_STATE_CHUNK_DATA;
					}
					break;
				}
			case EN_HTTP_STATE_CHUNK_DATA:
				{
					xxlToRead = (_UI)(MIN_NUM(pucPe - pucP, pParser->xxlContentSize));
					if(xxlToRead > 0){
						if(pSettings->pfunc_OnBody){
							pSettings->pfunc_OnBody(pParser, pucP, xxlToRead);
							pucP += xxlToRead - 1;
						}
					}
					if(xxlToRead == pParser->xxlContentSize){
						enState = EN_HTTP_STATE_CHUNK_DATA_ALMOST_DONE;
					}
					pParser->xxlContentSize -= xxlToRead;
					break;
				}
			case EN_HTTP_STATE_CHUNK_DATA_ALMOST_DONE:
				{
					HTTP_STRICT_CHECK(ucCh != CR);
					enState = EN_HTTP_STATE_CHUNK_DATA_DONE;
					break;
				}
			case EN_HTTP_STATE_CHUNK_DATA_DONE:
				{
					HTTP_STRICT_CHECK(ucCh != LF);
					enState = EN_HTTP_STATE_CHUNK_SIZE_START;
					break;
				}
			default:
				goto error;
		}
	}
	if(pucHeaderField_mark){
		if(pSettings->pfunc_OnHeaderField){
			pSettings->pfunc_OnHeaderField(pParser, pucHeaderField_mark, SAFE_UI(pucP - pucHeaderField_mark));
		}
	}
	if(pucHeaderValue_mark){
		if(pSettings->pfunc_OnHeaderValue){
			pSettings->pfunc_OnHeaderValue(pParser, pucHeaderValue_mark, SAFE_UI(pucP - pucHeaderValue_mark));
		}
	}
	if(pucUrl_mark){
		if(pSettings->pfunc_OnUrl){
			pSettings->pfunc_OnUrl(pParser, pucUrl_mark, SAFE_UI(pucP - pucUrl_mark));
		}
	}
	pParser->ucState = enState;
	pParser->ucHeaderState = enHeadState;
	pParser->uiIndex = uiIndex;
	pParser->uiNRead = uiNRead;
	return uiLen;
  
error:
	pParser->ucState = EN_HTTP_STATE_DEAD;
	return SAFE_UI(pucP - pucData);
}

int HttpParser_KeepAlive(ST_HTTPPARSER* pParser)
{
	if (pParser->usHttpMajor > 0 && pParser->usHttpMinor > 0) {
		if (pParser->ucFlags & EN_HTTPPARSER_FLAG_CONNECTION_CLOSE){
			return -1;
        } else {
            return 0;
        }
    } else {
		if (pParser->ucFlags & EN_HTTPPARSER_FLAG_CONNECTION_KEEP_ALIVE){
            return 0;
        } else {
			return -1;
        }
    }
}

_UC * HttpParser_MethodToStr(unsigned int enHttpMethod)
{
	return (_UC*)s_HttpMethodStr[enHttpMethod];
}
