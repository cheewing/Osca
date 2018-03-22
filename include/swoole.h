#ifndef SWOOLE_H_
#define SWOOLE_H_

//-------------------------------------------------------------------------------
#define SW_OK                  0
#define SW_ERR                -1
#define SW_AGAIN              -2
#define SW_BUSY               -3
#define SW_DONE               -4
#define SW_DECLINED           -5
#define SW_ABORT              -6
//-------------------------------------------------------------------------------
enum swLog_level
{
    SW_LOG_DEBUG = 0,
    SW_LOG_TRACE,
    SW_LOG_INFO,
    SW_LOG_NOTICE,
    SW_LOG_WARNING,
    SW_LOG_ERROR,

};

#define swoole_error_log(level, __errno, str, ...)      do{SwooleG.error=__errno;\
    if (level >= SwooleG.log_level){\
    snprintf(sw_error, SW_ERROR_MSG_SIZEm "%s (ERROR %d): " str,__func__,__errno,##__VA_ARGS__);\
    SwooleGS->lock_2.lock(&SwooleGS->lock_2);\
    swLog_put(level, sw_error);\
    SwooleGS->lock_2.unlock(&SwooleGS->lock_2);}}while(0)

#ifdef SW_USE_JEMALLOC
#include <jemalloc/jemalloc.h>
#define sw_malloc              je_malloc
#define sw_free                je_free
#define sw_calloc              je_calloc
#define sw_realloc             je_realloc
#else
#define sw_malloc              malloc
#define sw_free                free
#define sw_calloc              calloc
#define sw_realloc             realloc
#endif

#endif /* SWOOLE_H_ */