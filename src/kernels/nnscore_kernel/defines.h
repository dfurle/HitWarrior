#ifndef DEFINES_H_
#define DEFINES_H_

#include "ap_fixed.h"
#include "ap_int.h"
#include "nnet_utils/nnet_types.h"
#include <cstddef>
#include <cstdio>

// hls-fpga-machine-learning insert numbers
#define N_INPUT_1_1 15
#define N_LAYER_2 16
#define N_LAYER_2 16
#define N_LAYER_4 16
#define N_LAYER_4 16
#define N_LAYER_6 8
#define N_LAYER_6 8
#define N_LAYER_8 1
#define N_LAYER_8 1

// hls-fpga-machine-learning insert layer-precision
typedef ap_fixed<32,11> input_t;
typedef ap_fixed<16,6> dense_accum_t;
typedef ap_fixed<16,6> layer2_t;
typedef ap_fixed<16,6> dense_weight_t;
typedef ap_fixed<16,6> dense_bias_t;
typedef ap_uint<1> layer2_index;
typedef ap_fixed<16,6> layer3_t;
typedef ap_fixed<18,8> dense_relu_table_t;
typedef ap_fixed<16,6> dense_1_accum_t;
typedef ap_fixed<16,6> layer4_t;
typedef ap_fixed<16,6> dense_1_weight_t;
typedef ap_fixed<16,6> dense_1_bias_t;
typedef ap_uint<1> layer4_index;
typedef ap_fixed<16,6> layer5_t;
typedef ap_fixed<18,8> dense_1_relu_table_t;
typedef ap_fixed<16,6> dense_2_accum_t;
typedef ap_fixed<16,6> layer6_t;
typedef ap_fixed<16,6> dense_2_weight_t;
typedef ap_fixed<16,6> dense_2_bias_t;
typedef ap_uint<1> layer6_index;
typedef ap_fixed<16,6> layer7_t;
typedef ap_fixed<18,8> dense_2_relu_table_t;
typedef ap_fixed<16,6> dense_3_accum_t;
typedef ap_fixed<16,6> layer8_t;
typedef ap_fixed<16,6> dense_3_weight_t;
typedef ap_fixed<16,6> dense_3_bias_t;
typedef ap_uint<1> layer8_index;
typedef ap_fixed<16,6> result_t;
typedef ap_fixed<18,8> dense_3_sigmoid_table_t;

#endif
