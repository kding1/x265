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

#ifndef VENCLIB_H
#define VENCLIB_H

#ifdef __cplusplus
extern "C"{
#endif

typedef struct {
    float rdqp_lambda;  // rdQp lambda used to encode previous frame
    float sadqp_lambda; // sad lambda used to encode previous frame
    int qp;             // qp used to encode previous frame
    int encoded_bits;   // already encoded bits for current gop
    float psnr[4];      // previous frame psnr for yuv, y, u and v
    float ssim;         // previous frame ssid

    float sad;          // current frame sad
    int cu_type[3];     // current frame cu type: intra, inter, skip

} enc_state_buf;

void *enc_init(int bitrate, int w, int h, int gop, int mode, char *yuv_path, char *bs_path);

void enc_free(void *state);

// get the state buffer prior to encoding current buffer
// return:
//      es: encoder state buffer provided by caller
//      return false if eos
bool enc_get_state(enc_state_buf *es, void *state);

// based on state, RL to estimate the best lambda value to encode current buffer
// return:
//      return false if eos
bool enc_set_lambda(float rdqp_lambda, float sadqp_lambda, void *state);

// with the lambda, trigger the encoder to encode current frame
// return:
//      return false if eos
bool enc_cur_frame(float rdqp_lambda, float sadqp_lambda, void *state);

#ifdef __cplusplus
}
#endif

#endif // VENCLIB_H
