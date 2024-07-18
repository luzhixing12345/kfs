#ifndef COMMON_H
#define COMMON_H

#define FUSE_USE_VERSION 31

#include <errno.h>
#include <fuse3/fuse.h>
#include <stdbool.h>

#define ALIGN_TO(__n, __align)                              \
    ({                                                      \
        typeof(__n) __ret;                                  \
        if ((__n) % (__align)) {                            \
            __ret = ((__n) & (~((__align)-1))) + (__align); \
        } else                                              \
            __ret = (__n);                                  \
        __ret;                                              \
    })

#define UNUSED(__x) ((void)(__x))
#define MIN(x, y)              \
    ({                         \
        typeof(x) __x = (x);   \
        typeof(y) __y = (y);   \
        __x < __y ? __x : __y; \
    })

#define STATIC_ASSERT(e) static char const static_assert[(e) ? 1 : -1] = {'!'}

#include <limits.h>

#endif
