
#include "venc_inc.h"
#include <stdio.h>

#ifdef RARC_ENCODER
int main(int argc, char **argv)
#else
int dummy(int argc, char **argv)
#endif
{
    printf("EVERYTIHNG IS OKAY.\n");
    return 0;
}
