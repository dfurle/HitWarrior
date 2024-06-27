#include "projectDefines.h"

extern "C"{


void filterLowNN(hls::stream<stream_t>& in_stream, hls::stream<stream_t>& out_stream){
  #pragma HLS interface ap_ctrl_none port=return
  #pragma HLS INTERFACE mode=axis port=in_stream
  #pragma HLS INTERFACE mode=axis port=out_stream


  stream_t s = in_stream.read(); // read nnscore

  // check if end of data first (read_data will write -1 to first data stream)
  if(data_t(s.data) == data_t(-1)){
    out_stream.write(s);
    return;
  }

  // printf("- Filtered\n");

  if(reinterpret_cast<nnscore_t&>(s.data) < MIN_THRESHOLD){

    // eat buffer
    for(int i = 0; i < NHITS * 3; i++){
      in_stream.read();
    }
  } else { // passed threshold
    // pass on buffer
    out_stream.write(s);
    for(int i = 0; i < NHITS * 3; i++){
      stream_t s2 = in_stream.read();
      out_stream.write(s2);
    }
  }
}

}