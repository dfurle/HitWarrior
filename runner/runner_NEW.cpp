#include "projectDefines.h"

#include <iostream>

#define RUN_PIPELINE


extern "C"{

#define NUM_HITS 3
#define DATA_LENGTH 10

void NN_sample(hls::vector<Hit, NUM_HITS> hits, Hit* out){
  for(int i = 0; i < hits.size(); i++){
    out->x = hits[i].x;
    out->y = hits[i].y;
    out->z = hits[i].z;
  }
}

void compute(hls::stream<Hit>& in_stream, hls::stream<Hit>& out_stream){
  #pragma HLS INTERFACE ap_ctrl_none port=return
  Hit out;
  out.x = 0;
  out.y = 0;
  out.z = 0;


  for(int i = 0; i < NUM_HITS; i++){
    Hit in = in_stream.read();
    out.x += in.x;
    out.y += in.y;
    out.z += in.z;
  }

  out_stream.write(out);
}

void mem_read(Hit* data, hls::stream<Hit>& out_stream){
  for(int j = 0; j < DATA_LENGTH; j++){
    for(int i = 0; i < NUM_HITS; i++){
      Hit h = data[i + j * NUM_HITS];
      out_stream.write(h);
    }
  }
}

void mem_write(Hit* data, hls::stream<Hit>& stream){
  for(int j = 0; j < DATA_LENGTH; j++){
    data[j] = stream.read();
  }
}
}