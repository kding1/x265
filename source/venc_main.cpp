
#include "venclib.h"
#include "common.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    void *state = NULL;
    bool eof = false;
    float lambda = 1.0f, psnr = 0.0f, ssim = 0.0f;
    enc_state_buf es;

    state = enc_init(10000000, 352, 288, 30, 0, "./foreman_cif.yuv");
    while(!eof) {
        eof = ! enc_cur_frame(lambda, &psnr, &ssim, state);
        enc_get_state(&es, state);
    }
    enc_free(state);

    printf("EVERYTIHNG IS OKAY.\n");
    return 0;
}
