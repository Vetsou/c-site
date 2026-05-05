#ifndef _RT_DOTENV_
#define _RT_DOTENV_

#include <stdint.h>

int32_t rt_load_dotenv(const char *path);
const char* rt_getenv_or_default(const char *name, const char *default_val);

#endif // _RT_DOTENV_