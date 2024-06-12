#include "projectDefines.h"

extern "C"{


void filterLowNN(hls::stream<stream_t>& in_stream, hls::stream<stream_t>& out_stream){
  #pragma HLS interface ap_ctrl_none port=return

  stream_t s = in_stream.read(); // read nnscore

  // check if end of data first (read_data will write -1 to first data stream)
  if(data_t(s.data) == data_t(-1)){
    printf("!!! Filter Found Ending!\n");
    printf("!!! Filter Sending Ending!\n");
    out_stream.write(s);
    return;
  }

  // printf("- Filtered\n");

  if(reinterpret_cast<nnscore_t&>(s.data) < MIN_THRESHOLD){
    printf("Rejecting NN: ( %.4f < %.2f )\n", float(nnscore_t(s.data)), MIN_THRESHOLD);

    // eat buffer
    for(int i = 0; i < NHITS * 3; i++){
      in_stream.read();
    }
  } else { // passed threshold
    // pass on buffer
    printf("- Filter Passed Threshold\n");
    out_stream.write(s);
    for(int i = 0; i < NHITS * 3; i++){
      stream_t s2 = in_stream.read();
      out_stream.write(s2);
    }
  }
}

}