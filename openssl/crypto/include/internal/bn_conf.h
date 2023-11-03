#ifdef _MSC_VER

#ifdef _WIN64
#include "bn_conf_x64.h"
#else
#include "bn_conf_x86.h"
#endif

#else

#error TODO: Generate the rest of opensslconf variants using Configure

#endif