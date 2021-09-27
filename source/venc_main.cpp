
#include "venclib.h"
#include "common.h"

#include <stdio.h>
#include <string>

using namespace std;

int main(int argc, char **argv)
{
    void *state = NULL;
    bool eof = false;
    float lambda = 1.0f, psnr = 0.0f, ssim = 0.0f;
    enc_state_buf es;
    int bitrate, w, h;

    if (argc != 5) {
        printf("wrong input: bitrate w h yuv_path.\n");
        return -1;
    }

    bitrate = atoi(argv[1]);
    w = atoi(argv[2]);
    h = atoi(argv[3]);

    state = enc_init(bitrate, w, h, 30, 0, argv[4]);
    while(!eof) {
        eof = ! enc_cur_frame(lambda, &psnr, &ssim, state);
        enc_get_state(&es, state);
    }
    enc_free(state);

    printf("EVERYTIHNG IS OKAY.\n");
    return 0;
}
