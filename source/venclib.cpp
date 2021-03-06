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

#include "venclib.h"
#include "x265.h"
#include "x265cli.h"
#include "constants.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "venc_inc.h"
#include "venc_util.h"


#define QP_MAX_MAX      69 /* max allowed QP to be output by rate control */

using namespace std;
using namespace X265_NS;

extern int venc_main(int argc, char **argv);

sync_queue< shared_ptr<venc_msg_node> > venc_tx_queue;

sync_queue< shared_ptr<venc_msg_node> > venc_rx_queue;


// lambda = pow(2, (double)q / 6 - 2);
static double g_x265_lambda_tab[QP_MAX_MAX + 1] =
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
static double g_x265_lambda2_tab[QP_MAX_MAX + 1] =
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

vector<string> argvv = {"x265", "--input", "foreman_cif.yuv", "--fps", "30", "--input-res", "352x288", "-o", "out.265",
                        "--bitrate", "256", "--vbv-maxrate", "256", "--vbv-bufsize", "512",
                        "--psnr", "--ssim", "--bframes", "0", "--profile", "main", "--rc-lookahead", "0", "--keyint", "0",
                        "--frame-threads", "1", "--slices", "1",
                        "--tune", "zerolatency", "--preset", "ultrafast", "-r", "recon.bin"};

void* venc_main_pthread(void* p)
{
    std::vector<char*> argv;

    for (const auto& arg : argvv) {
        argv.push_back((char*)arg.data());
    }
    argv.push_back(nullptr);

    venc_main(argv.size() - 1, argv.data());
}

void *enc_init(int bitrate, int w, int h, int gop, int mode, char *yuv_path, char *bs_path)
{
    enc_ctx *ctx = (enc_ctx *)malloc(sizeof(enc_ctx));

    argvv[2] = string(yuv_path);
    argvv[6] = to_string(w) + "x" + to_string(h);
    argvv[8] = string(bs_path);
    argvv[10] = to_string(bitrate);
    argvv[12] = to_string(bitrate);
    argvv[14] = to_string(bitrate * 2);

    ctx->frm_idx = 0;

    // start the worker thread
    pthread_create(&ctx->venc_worker, NULL, venc_main_pthread, NULL);

    return ctx;
}

void enc_free(void *state)
{
    enc_ctx *ctx = (enc_ctx *)state;
    pthread_join(ctx->venc_worker, NULL);
    free(ctx);
}

bool enc_get_state(enc_state_buf *es, void *state)
{
    printf("enc_get_state, ctx = %p, get es qp = %d, sad = %.4f.\n", state, es->qp, es->sad);

    return true;
}

bool enc_set_lambda(float rdqp_lambda, float sadqp_lambda, void *state)
{
    // sadqp_lambda is reserved for now.
    for (int i = 0; i <= QP_MAX_MAX; i++) {
        x265_lambda_tab[i] = rdqp_lambda * g_x265_lambda_tab[i];
        x265_lambda2_tab[i] = rdqp_lambda * g_x265_lambda2_tab[i];
    }

    return true;
}

bool enc_cur_frame(float rdqp_lambda, float sadqp_lambda, void *state)
{
    // sadqp_lambda is reserved for now.
    // pls return false if no more frame exists.
    shared_ptr < venc_msg_node > msg = make_shared< venc_msg_node >();
    enc_ctx *ctx = (enc_ctx *)state;

    // step 0: send the lambda to worker
    msg->type = TYPE_LAMBDA_READY;
    msg->lambda_ratio = rdqp_lambda;
    // x265_log(NULL, X265_LOG_INFO, "sending lambda = %4f.", msg->lambda_ratio);
    printf("sending lambda = %4f.\n", msg->lambda_ratio);
    venc_tx_queue.put(msg);
    printf("sent queue size = %d.\n", venc_tx_queue.size());
    // step 1: get encoder result
    msg = venc_rx_queue.get();
    // x265_log(NULL, X265_LOG_INFO, "received current encoder state.");
    printf("received current encoder state, type = %d.\n", msg->type);

    return msg->type != TYPE_EOF;
}
