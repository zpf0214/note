// a.cc

#include "b.h"

#ifdef USE_FIX
    #define FIX_VALUE 1
#else
    #define FIX_VALUE 0
#endif

void a_function() {
    // 调用 b.cc 中的函数 f，并传递 FIX_VALUE 作为参数
    f(FIX_VALUE);
}
