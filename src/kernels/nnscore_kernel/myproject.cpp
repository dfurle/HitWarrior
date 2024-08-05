#include <iostream>

#include "myproject.h"
#include "parameters.h"

void myproject(
    input_t input_1[N_INPUT_1_1],
    result_t layer9_out[N_LAYER_8]
) {

    // hls-fpga-machine-learning insert IO
    #pragma HLS ARRAY_RESHAPE variable=input_1 complete dim=0
    #pragma HLS ARRAY_PARTITION variable=layer9_out complete dim=0
    #pragma HLS INTERFACE ap_vld port=input_1,layer9_out 
    #pragma HLS PIPELINE 

// #ifndef __SYNTHESIS__
//     static bool loaded_weights = false;
//     if (!loaded_weights) {
//         // hls-fpga-machine-learning insert load weights
//         nnet::load_weights_from_txt<dense_weight_t, 240>(w2, "w2.txt");
//         nnet::load_weights_from_txt<dense_bias_t, 16>(b2, "b2.txt");
//         nnet::load_weights_from_txt<dense_1_weight_t, 256>(w4, "w4.txt");
//         nnet::load_weights_from_txt<dense_1_bias_t, 16>(b4, "b4.txt");
//         nnet::load_weights_from_txt<dense_2_weight_t, 128>(w6, "w6.txt");
//         nnet::load_weights_from_txt<dense_2_bias_t, 8>(b6, "b6.txt");
//         nnet::load_weights_from_txt<dense_3_weight_t, 8>(w8, "w8.txt");
//         nnet::load_weights_from_txt<dense_3_bias_t, 1>(b8, "b8.txt");
//         loaded_weights = true;
//     }
// #endif

    // ****************************************
    // NETWORK INSTANTIATION
    // ****************************************

    // hls-fpga-machine-learning insert layers

    layer2_t layer2_out[N_LAYER_2];
    #pragma HLS ARRAY_PARTITION variable=layer2_out complete dim=0
    nnet::dense<input_t, layer2_t, config2>(input_1, layer2_out, w2, b2); // dense

    layer3_t layer3_out[N_LAYER_2];
    #pragma HLS ARRAY_PARTITION variable=layer3_out complete dim=0
    nnet::relu<layer2_t, layer3_t, relu_config3>(layer2_out, layer3_out); // dense_relu

    layer4_t layer4_out[N_LAYER_4];
    #pragma HLS ARRAY_PARTITION variable=layer4_out complete dim=0
    nnet::dense<layer3_t, layer4_t, config4>(layer3_out, layer4_out, w4, b4); // dense_1

    layer5_t layer5_out[N_LAYER_4];
    #pragma HLS ARRAY_PARTITION variable=layer5_out complete dim=0
    nnet::relu<layer4_t, layer5_t, relu_config5>(layer4_out, layer5_out); // dense_1_relu

    layer6_t layer6_out[N_LAYER_6];
    #pragma HLS ARRAY_PARTITION variable=layer6_out complete dim=0
    nnet::dense<layer5_t, layer6_t, config6>(layer5_out, layer6_out, w6, b6); // dense_2

    layer7_t layer7_out[N_LAYER_6];
    #pragma HLS ARRAY_PARTITION variable=layer7_out complete dim=0
    nnet::relu<layer6_t, layer7_t, relu_config7>(layer6_out, layer7_out); // dense_2_relu

    layer8_t layer8_out[N_LAYER_8];
    #pragma HLS ARRAY_PARTITION variable=layer8_out complete dim=0
    nnet::dense<layer7_t, layer8_t, config8>(layer7_out, layer8_out, w8, b8); // dense_3

    nnet::sigmoid<layer8_t, result_t, sigmoid_config9>(layer8_out, layer9_out); // dense_3_sigmoid

}
