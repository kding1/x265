// Copyright 2021, Intel Corporation. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of the copyright holder nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.


#include <stdlib.h>
#include <stdio.h>
#include "venclib.h"
#include "venc_inc.h"
#include "venc_util.h"

#define QP_MAX_MAX      69 /* max allowed QP to be output by rate control */

sync_queue<venc_msg_type> venc_msq_queue; 


// lambda = pow(2, (double)q / 6 - 2);
double g_x265_lambda_tab[QP_MAX_MAX + 1] =
{
    0.2500, 0.2806, 0.3150, 0.3536, 0.3969,
    0.4454, 0.5000, 0.5612, 0.6300, 0.7071,
    0.7937, 0.8909, 1.0000, 1.1225, 1.2599,
    1.4142, 1.5874, 1.7818, 2.0000, 2.2449,
    2.5198, 2.8284, 3.1748, 3.5636, 4.0000,
    4.4898, 5.0397, 5.6569, 6.3496, 7.1272,
    8.0000, 8.9797, 10.0794, 11.3137, 12.6992,
    14.2544, 16.0000, 17.9594, 20.1587, 22.6274,
    25.3984, 28.5088, 32.0000, 35.9188, 40.3175,
    45.2548, 50.7968, 57.0175, 64.0000, 71.8376,
    80.6349, 90.5097, 101.5937, 114.0350, 128.0000,
    143.6751, 161.2699, 181.0193, 203.1873, 228.0701,
    256.0000, 287.3503, 322.5398, 362.0387, 406.3747,
    456.1401, 512.0000, 574.7006, 645.0796, 724.0773
};

// lambda2 = 0.038 * exp(0.234 * QP)
double g_x265_lambda2_tab[QP_MAX_MAX + 1] =
{
    0.0380, 0.0480, 0.0606, 0.0766, 0.0968,
    0.1224, 0.1547, 0.1955, 0.2470, 0.3121,
    0.3944, 0.4984, 0.6299, 0.7959, 1.0058,
    1.2710, 1.6061, 2.0295, 2.5646, 3.2408,
    4.0952, 5.1749, 6.5393, 8.2633, 10.4419,
    13.1949, 16.6736, 21.0695, 26.6244, 33.6438,
    42.5138, 53.7224, 67.8860, 85.7838, 108.4003,
    136.9794, 173.0933, 218.7284, 276.3949, 349.2649,
    441.3467, 557.7054, 704.7413, 890.5425, 1125.3291,
    1422.0160, 1796.9227, 2270.6714, 2869.3215, 3625.8023,
    4581.7251, 5789.6717, 7316.0868, 9244.9328, 11682.3084,
    14762.2847, 18654.2798, 23572.3779, 29787.1055, 37640.3119,
    47563.9728, 60103.9523, 75950.0283, 95973.8349, 121276.8079,
    153250.7703, 193654.4919, 244710.4321, 309226.9897, 390752.9823
};

void *enc_init(int bitrate, int w, int h, int gop, int mode, char *yuv_path)
{
    enc_ctx *ctx = (enc_ctx *)malloc(sizeof(enc_ctx));
    ctx->frm_idx = 0;
    printf("enc_init, ctx = %p, bitrate = %d, w = %d, h = %d, gop = %d, mode = %d, yuv = %s.\n",
           (void *)ctx, bitrate, w, h, gop, mode, yuv_path);
    return ctx;
}

void enc_free(void *state)
{
    printf("enc_free, ctx = %p.\n", state);
}

bool enc_get_state(enc_state_buf *es, void *state)
{
    printf("enc_get_state, ctx = %p, get es qp = %d, sad = %.4f.\n", state, es->qp, es->sad);

    es->qp = 1;
    es->sad = 2.0f;
    for (int i = 0; i < 4; i++) {
        es->psnr[i] = i;
    }
    es->ssim = 4.0f;

    return true;
}

bool enc_set_lambda(float lambda_v, void *state)
{
    printf("enc_set_lambda, ctx = %p, lamdbda = %.4f.\n", state, lambda_v);
    return true;
}

bool enc_cur_frame(float *psnr, float *ssim, void *state)
{
    // pls return false if no more frame exists.
    enc_ctx *ctx = (enc_ctx *)state;
    for (int i = 0; i < 4; i++) {
        psnr[i] = i;
    }
    *ssim = 10.0f;
    printf("enc_cur_frame, ctx = %p, idx = %d.\n", (void *)ctx, ctx->frm_idx);

    ctx->frm_idx++;

    return ctx->frm_idx <= 10;
}
