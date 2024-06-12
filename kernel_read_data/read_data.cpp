#include "projectDefines.h"


extern "C"{


void read_data(Track* inTracks, hls::stream<stream_t>& out_stream){
  // #pragma HLS DATA_PACK variable=out_stream
  #pragma HLS INTERFACE m_axi port=inTracks offset=slave bundle=aximm1

  READ_IN1_ARRAY1:
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    Track trk = inTracks[i];
    stream_t s;


    s.data = reinterpret_cast<ap_uint<16>&>(trk.NNScore);
    out_stream.write(s);
    for(int j = 0; j < NHITS; j++){
      s.data = trk.hits[j].x;
      out_stream.write(s);
      s.data = trk.hits[j].y;
      out_stream.write(s);
      s.data = trk.hits[j].z;
      out_stream.write(s);
    }
    // printf("- Read\n");
  }

  // finished data
  stream_t s1;
  s1.data = data_t(-1);
  out_stream.write(s1);
}

}
