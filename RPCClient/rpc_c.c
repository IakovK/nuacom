#if !defined(_M_IA64) && !defined(_M_AMD64)
#include "..\common\Win32\Nuacom_c.c"
#endif
#if defined(_M_AMD64)
#include "..\common\x64\Nuacom_c.c"
#endif
