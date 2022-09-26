#ifndef mtots_config_h
#define mtots_config_h


#define DEBUG_TRACE_EXECUTION  0
#define DEBUG_PRINT_CODE       0
#define DEBUG_STRESS_GC        1
#define DEBUG_LOG_GC           0


#define MAX_PATH_LENGTH        4096
#define MAX_ELIF_CHAIN_COUNT     64
#define MAX_IDENTIFIER_LENGTH   128
#define FREAD_BUFFER_SIZE      8192


#define MTOTS_FILE_EXTENSION ".mtots"


/****************************************************************
 * C version
 ****************************************************************/

#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1900)

/* ##### C++11 and above ##### */
#define NORETURN [[ noreturn ]]
#define STATIC_INLINE static inline

#else /* __cplusplus */

#if __STDC_VERSION__ >= 199901L

/* ##### C99 and above ##### */
#define NORETURN _Noreturn
#define STATIC_INLINE static inline

#else /* __STDC_VERSION__ */

/* ##### Assume C89 only ##### */
#define NORETURN
#define STATIC_INLINE

#endif /* __STDC_VERSION__ */

#endif /* __cplusplus */

#endif/*mtots_config_h*/

/****************************************************************
 * OS version
 ****************************************************************/

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

#define OS_NAME "windows"

#elif __APPLE__

#include <TargetConditionals.h>

#if TARGET_OS_IPHONE

#define OS_NAME "iphone"

#elif TARGET_OS_MAC

#define OS_NAME "macos"

#endif

#elif __ANDROID__

#define OS_NAME "android"

#elif __linux__

#define OS_NAME "linux"

#elif __unix__

#define OS_NAME "unix"

#elif defined(_POSIX_VERSION)

#define OS_NAME "posix"

#else

#define OS_NAME "unknown"

#endif
