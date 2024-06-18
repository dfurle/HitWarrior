#include "projectDefines.h"

extern "C"{


void write_data(Track* outTracks, hls::stream<stream_t>& in_stream){
  #pragma HLS INTERFACE m_axi port=outTracks offset=slave bundle=aximm1
  #pragma HLS INTERFACE mode=axis port=in_stream


  int counter = 0;
  while(true){
    stream_t s;
    s = in_stream.read();
    if(data_t(s.data) == data_t(-1)){
      // finished
      return;
    }
    
    Track t;
    t.NNScore = reinterpret_cast<nnscore_t&>(s.data);
    for(int i = 0; i < NHITS; i++){
      Hit h;
      stream_t s1;
      s1 = in_stream.read();
      h.x = data_t(s1.data);
      s1 = in_stream.read();
      h.y = data_t(s1.data);
      s1 = in_stream.read();
      h.z = data_t(s1.data);
      t.hits[i] = h;
    }
    outTracks[counter++] = t;
  }
}

}