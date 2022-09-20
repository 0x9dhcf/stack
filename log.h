#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <stdlib.h>

#define FLog(fmt, ...) do {\
    fprintf (stderr, "FATAL - [%s: %d]: " fmt "\n",\
            __FILE__,__LINE__, ##__VA_ARGS__);\
    exit (EXIT_FAILURE);\
} while(0)

#define ELog(fmt, ...) do {\
    fprintf (stderr, "ERROR - [%s: %d]: " fmt "\n",\
            __FILE__,__LINE__, ##__VA_ARGS__);\
} while (0)

#define ILog(fmt, ...) do {\
    fprintf (stdout, "INFO: " fmt "\n", ##__VA_ARGS__);\
} while (0)

#ifndef NDEBUG
#define DLog(fmt, ...) do {\
    fprintf (stdout, "DEBUG - [%s: %d - %s]: " fmt "\n",\
            __FILE__,__LINE__,__FUNCTION__, ##__VA_ARGS__);\
    fflush(stdout);\
} while (0)
#else
#define DLog(fmt, ...)
#endif

#endif /* __LOG_H__ */
