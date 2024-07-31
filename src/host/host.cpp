#include "helper.h"
#include <fstream>
#include <iostream>
#include <stdlib.h>

#include "projectDefines.h"
#include "data_reader.h"
#include "host.h"

// #include "ap_fixed.h"



int printTiming(std::string str, std::chrono::_V2::system_clock::time_point & begin) {
  auto end = std::chrono::high_resolution_clock::now();
  int diffus = std::chrono::duration_cast < std::chrono::microseconds > (end - begin).count();
  printf(str.c_str(), diffus);
  begin = end;
  return diffus;
}


void runSearch(CKernel& hw, Track* in, int* nTracks, nnscore_t* outScores){
  auto begin = std::chrono::high_resolution_clock::now();

  hw.write_mem(0, in);
  hw.write_mem(1, nTracks);

  // memcpy ( ptr_l1, in, in_size );
  // memcpy ( ptr_ts, inTracks, in_ts_size );

  printTiming(" - memcpy in %d us\n", begin);

  // q.enqueueMigrateMemObjects({buffer_l1}, 0); // 0 means from host
  // q.enqueueMigrateMemObjects({buffer_ts}, 0);
  // q.enqueueTask(kernel);
  // q.enqueueMigrateMemObjects({buffer_out}, CL_MIGRATE_MEM_OBJECT_HOST);
  // q.enqueueMigrateMemObjects({buffer_l1}, CL_MIGRATE_MEM_OBJECT_HOST);

  hw.migrate_buf(0);
  hw.migrate_buf(1);
  hw.enqueue_kernel();
  hw.migrate_buf(2, false);

  printTiming(" - migrate %d us\n", begin);
  // q.finish();
  hw.finish();
  printTiming(" - finish %d us\n", begin);

  // memcpy ( outScores, ptr_out, out_size );
  // memcpy ( in, ptr_l1, in_size );
  hw.read_mem(2, outScores);

  printTiming(" - memcpy out %d us\n", begin);

}

void setupRunSearch(CKernel& hw, Track* in, int* nTracks, nnscore_t* outScores){
  auto begin = std::chrono::high_resolution_clock::now();

  hw.setup_kernel("runner");
  printTiming(" - kernel init %d us\n", begin);

  // Compute the size of array in bytes
  size_t in_size = sizeof(Track) * MAX_TRACK_SIZE * BATCH_SIZE;
  size_t in_ts_size = sizeof(int) * BATCH_SIZE;
  size_t out_size = sizeof(nnscore_t) * BATCH_SIZE;

  // std::cout << "in_hist_size:  " << in_size << std::endl;
  // std::cout << "out_size: " << out_size << std::endl;
  // printf("\n");

  hw.create_buffer(in_size,    0, CL_MEM_READ_ONLY, CL_MAP_WRITE);
  hw.create_buffer(in_ts_size, 1, CL_MEM_READ_ONLY, CL_MAP_WRITE);
  hw.create_buffer(out_size,   2, CL_MEM_WRITE_ONLY, CL_MAP_READ);
  hw.get_kernel()->setArg(3, MAX_SHARED);
  hw.get_kernel()->setArg(4, BATCH_SIZE);

  // // These commands will allocate memory on the Device. The cl::Buffer objects
  // // can be used to reference the memory locations on the device.
  // // printf("Initializing Buffers\n");
  // cl::Buffer buffer_l1(*hw.context, CL_MEM_READ_ONLY, in_size);
  // cl::Buffer buffer_ts(*hw.context, CL_MEM_READ_ONLY, in_ts_size);
  // cl::Buffer buffer_out(*hw.context, CL_MEM_WRITE_ONLY, out_size);

  // // set the kernel Arguments
  // int narg = 0;
  // kernel.setArg(narg++, buffer_l1);
  // kernel.setArg(narg++, buffer_ts);
  // kernel.setArg(narg++, buffer_out);
  // kernel.setArg(narg++, BATCH_SIZE);
  // kernel.setArg(narg++, MAX_SHARED);

  // // We then need to map our OpenCL buffers to get the pointers
  // Track *ptr_l1 = (Track *)q.enqueueMapBuffer(buffer_l1, CL_TRUE, CL_MAP_WRITE, 0, in_size);
  // Track *ptr_ts = (Track *)q.enqueueMapBuffer(buffer_ts, CL_TRUE, CL_MAP_WRITE, 0, in_ts_size);
  // Track *ptr_out = (Track *)q.enqueueMapBuffer(buffer_out, CL_TRUE, CL_MAP_READ, 0, out_size);

  printTiming(" - mapbuf %d us\n", begin);


  float totaltime = 0;
  std::vector<float> timings;
  int nIterations = 100;
  for(int i = 0; i < nIterations; i++){
    printf("Run %dth iteration\n", i);
    runSearch(hw, in, nTracks, outScores);
    float time = printTiming(" - FINISHED FPGA: %d us\n", begin);
    timings.push_back(time);
    totaltime += time;
  }
  printf("Total time: %.0f\n", totaltime);
  printf("Time Per iteration: %.0f\n", totaltime/nIterations);

  for(int i = 0; i < nIterations; i++){
    printf("%.0f, ", timings[i]);
  }
  printf("\n");
  

  // q.enqueueUnmapMemObject(buffer_l1, ptr_l1);
  // q.enqueueUnmapMemObject(buffer_out, ptr_out);
  hw.unmap(0);
  hw.unmap(1);
  hw.unmap(2);

  // q.finish();
  hw.finish();

  printTiming(" - finish2 %d us\n", begin);

}

int main(int argc, char *argv[]) {

  // TARGET_DEVICE macro needs to be passed from gcc command line
  if (argc < 2 || argc > 3) {
    std::cout << "Usage: " << argv[0] << "<xclbin>" << std::endl;
    return EXIT_FAILURE;
  }

  CKernel hw;
  printf("setting up device\n");
  hw.setup_device("u250");
  printf("loading xcl\n");
  hw.load_xclbin(argv[1]);
  // hw.setup_kernel("runner");


  printf("INITIALIZING DATA\n");

  Track* inTracks = new Track[MAX_TRACK_SIZE * BATCH_SIZE];
  int* nTracks = new int[BATCH_SIZE];
  nnscore_t* outScores = new nnscore_t[BATCH_SIZE];
  if(1 == read_data(inTracks, nTracks, outScores, "../tb_files/data/formatted_data.csv"))
    return 1;

  printf("\n---=== Running Kernel ===---\n\n");

  auto begin = std::chrono::high_resolution_clock::now();
  printTiming("Initializing:\n  %d us\n", begin);

  printf(" ┌------- sending %d batches with %d tracks\n",BATCH_SIZE,MAX_TRACK_SIZE);
  printf(" |          total tracks: %d or (%.2fKB)\n",BATCH_SIZE*MAX_TRACK_SIZE, (sizeof(Track)*BATCH_SIZE*MAX_TRACK_SIZE+sizeof(int)*BATCH_SIZE)/1024.);
  // setupRunSearch(program, context, q, inputTracks, outTracks);
  setupRunSearch(hw, inTracks, nTracks, outScores);
  std::cout << " └>Total on FPGA+transfer run\n";
  int timingFPGA = printTiming("%d us\n", begin);
  printf("\n---=== Finished Kernel ===---\n\n");
  std::cout << std::endl;

  printf("Pred Outs:\n");
  int counter = 0;
  for (int i = 0; i < MAX_TRACK_SIZE; i++) {
    // if(float(inTracks[i].NNScore) < 0.5){
    if(float(outScores[i]) < 0.5){
      continue;
    }
    printf("ID: %d : Count: %d | NNScore: %f\n", i, counter++, float(outScores[i]));
    for(int j = 0; j < NHITS; j++){
      printf(" %.2f %.2f %.2f\n",float(inTracks[i].hits[j].x), float(inTracks[i].hits[j].y), float(inTracks[i].hits[j].z));
    }
    printf("\n");
  }

  std::ofstream outputFile("../tb_files/tb_output.dat");
  if(outputFile.is_open()){
    for (int i = 0; i < MAX_TRACK_SIZE; i++) {
      // if(float(outTracks[i].NNScore) < 0.5){
      if(float(outScores[i]) < 0.5){
        continue;
      }
      // outputFile << outTracks[i].NNScore << "\n";
      // for(int j = 0; j < NHITS; j++){
      //   outputFile << outTracks[i].hits[j].x << " ";
      //   outputFile << outTracks[i].hits[j].y << " ";
      //   outputFile << outTracks[i].hits[j].z << "\n";
      // }
      outputFile << inTracks[i].NNScore << "\n";
      for(int j = 0; j < NHITS; j++){
        outputFile << inTracks[i].hits[j].x << " ";
        outputFile << inTracks[i].hits[j].y << " ";
        outputFile << inTracks[i].hits[j].z << "\n";
      }
    }
  }
  outputFile.close();

  std::cout << std::endl;

  std::cout << "Finished" << std::endl;
  return 0;
}
