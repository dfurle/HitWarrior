#include "projectDefines.h"

#include <iostream>

#define RUN_PIPELINE


extern "C"{

void searchHit(Hit* inputHistList, Hit* searchHitList, Hit* outList, int size_list, int size_search)
{

  // Loop over each hit and find the closest hit
  SEARCHLOOP: for(int i = 0; i < SEARCHHITSIZE; i++)
  {
    float closestDistance = 100000000000;

    float localX =  searchHitList[i].x;
    float localY =  searchHitList[i].y;
    float localZ =  searchHitList[i].z;

    // std::cout<<localX<<" "<<localY<<" "<<localZ<<std::endl;

    Hit closestHit;
    DISTANCESEARCH: for(int j = 0; j < INPUTHITSIZE; j++)
    {

      Hit compareHit = inputHistList[j];
      float distanceX = compareHit.x - localX;
      float distanceY = compareHit.y - localY;
      float distanceZ = compareHit.z - localZ;

      float distance = distanceX*distanceX + distanceY*distanceY + distanceZ*distanceZ;

      if(distance < closestDistance)
      {
        closestHit = compareHit;
        closestDistance = distance;

      }
      if(j <= size_search - 1)
        break;
    }
    outList[i] = closestHit;
    if(i <= size_list - 1)
      break;  
  }

}


void read_data(Hit* full_array1, Hit* full_array2, Hit* array1, Hit* array2, int size){
  READ_IN1_ARRAY1:
  for(int i = 0; i < INPUTHITSIZE; i++){
    array1[i] = full_array1[i];
    array2[i] = full_array2[i];
    if(i >= size - 1)
      break;
  }
}

void write_data(Hit* full_output, Hit* output, int size){
WRITE_OUTPUT:
  for(int i = 0; i < SEARCHHITSIZE; i++){
    full_output[i] = output[i];
    if(i >= size - 1)
      break;
  }
}

void runner(Hit* inputHistList, Hit* searchHitList, Hit* outList, int size_list, int size_search)
{
  // #pragma HLS PIPELINE
  // #pragma HLS DATAFLOW
  // #pragma HLS INTERFACE m_axi port=inputHistList offset=slave bundle=aximm1
  // #pragma HLS INTERFACE m_axi port=searchHitList offset=slave bundle=aximm1
  // #pragma HLS INTERFACE m_axi port=outList offset=slave bundle=aximm1

  #pragma HLS INTERFACE ap_ctrl_none port=return


  // ---=== might be better for certain situations, but will limit maximum size ===---
  Hit inputHist_array[INPUTHITSIZE];
  Hit searchHit_array[SEARCHHITSIZE];
  Hit out_array[RETURNHITSIZE];
  #pragma HLS ARRAY_PARTITION variable=inputHist_array complete
  #pragma HLS ARRAY_PARTITION variable=searchHit_array complete
  #pragma HLS ARRAY_PARTITION variable=out_array complete
  read_data(inputHistList, searchHitList, inputHist_array, searchHit_array, size_list);
  searchHit(inputHist_array, searchHit_array, out_array, size_list, size_search);
  write_data(outList, out_array, size_list);
  
  // ---=== otherwise just run something like this ===---
  // searchHit(inputHistList, searchHitList, outList);

}


}