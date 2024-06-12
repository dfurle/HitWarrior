#include "helper.h"
#include <fstream>
#include <iostream>
#include <stdlib.h>

#include "projectDefines.h"

// #include "ap_fixed.h"



int printTiming(std::string str, std::chrono::_V2::system_clock::time_point & begin) {
  auto end = std::chrono::high_resolution_clock::now();
  int diffus = std::chrono::duration_cast < std::chrono::microseconds > (end - begin).count();
  printf(str.c_str(), diffus);
  begin = end;
  return diffus;
}

static const std::string error_message =
    "Error: Result mismatch:\n"
    "i = %d CPU result = %d Device result = %d\n";

int setupDevice(std::vector<cl::Device>& devices, cl::Device& device){
  bool found_device = false;
  // traversing all Platforms To find Xilinx Platform and targeted
  // Device in Xilinx Platform
  std::vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);
  std::cout << "Scanning Platforms" << std::endl;
  std::cout << "number: " << platforms.size() << std::endl;
  for (size_t i = 0; (i < platforms.size()) & (found_device == false); i++) {
    cl::Platform platform = platforms[i];
    std::string platformName = platform.getInfo<CL_PLATFORM_NAME>();
    std::cout << platformName << std::endl;
    if (platformName == "Xilinx") {
      devices.clear();
      platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices);
      if (devices.size()) {
        device = devices[0];
        found_device = true;
        break;
      }
    }
  }
  if (found_device == false) {
    std::cout << "Error: Unable to find Target Device "
              << device.getInfo<CL_DEVICE_NAME>() << std::endl;
    return EXIT_FAILURE;
  }
  devices.resize(1);
  return 0;
}

void setupRunSearch(cl::Program& program, cl::Context& context, cl::CommandQueue& q, Track* in, Track* out)
{
  auto begin = std::chrono::high_resolution_clock::now();

  // printf("Initialize kernel\n");
  cl::Kernel kernel(program, "runner");
  printTiming(" - kernel init %d us\n", begin);

  // Compute the size of array in bytes
  size_t in_size = sizeof(Track) * INPUTTRACKSIZE;
  size_t out_size = sizeof(Track) * INPUTTRACKSIZE;

  // std::cout << "in_hist_size:  " << in_size << std::endl;
  // std::cout << "out_size: " << out_size << std::endl;
  // printf("\n");

  // These commands will allocate memory on the Device. The cl::Buffer objects
  // can be used to reference the memory locations on the device.
  // printf("Initializing Buffers\n");
  cl::Buffer buffer_l1(context, CL_MEM_READ_ONLY, in_size);
  cl::Buffer buffer_out(context, CL_MEM_WRITE_ONLY, out_size);

  // set the kernel Arguments
  int narg = 0;
  kernel.setArg(narg++, buffer_l1);
  kernel.setArg(narg++, buffer_out);
  kernel.setArg(narg++, MIN_DIST);
  kernel.setArg(narg++, MAX_SHARED);

  // We then need to map our OpenCL buffers to get the pointers
  Track *ptr_l1 = (Track *)q.enqueueMapBuffer(buffer_l1, CL_TRUE, CL_MAP_WRITE, 0, in_size);
  Track *ptr_out = (Track *)q.enqueueMapBuffer(buffer_out, CL_TRUE, CL_MAP_READ, 0, out_size);

  printTiming(" - mapbuf %d us\n", begin);

  memcpy ( ptr_l1, in, in_size );

  printTiming(" - memcpy in %d us\n", begin);

  q.enqueueMigrateMemObjects({buffer_l1}, 0); // 0 means from host
  q.enqueueTask(kernel);
  q.enqueueMigrateMemObjects({buffer_out}, CL_MIGRATE_MEM_OBJECT_HOST);
  printTiming(" - migrate %d us\n", begin);
  q.finish();
  printTiming(" - finish %d us\n", begin);

  memcpy ( out, ptr_out, out_size );

  printTiming(" - memcpy out %d us\n", begin);

  q.enqueueUnmapMemObject(buffer_l1, ptr_l1);
  q.enqueueUnmapMemObject(buffer_out, ptr_out);

  q.finish();

  printTiming(" - finish2 %d us\n", begin);

}


int main(int argc, char *argv[]) {

  // TARGET_DEVICE macro needs to be passed from gcc command line
  if (argc < 2 || argc > 3) {
    std::cout << "Usage: " << argv[0] << "<xclbin>" << std::endl;
    return EXIT_FAILURE;
  }
  // Creates a vector of DATA_SIZE elements with an initial value of 10 and 32
  // using customized allocator for getting buffer alignment to 4k boundary
  std::vector<cl::Device> devices;
  cl::Device device;
  if(setupDevice(devices, device) == EXIT_FAILURE){
    return EXIT_FAILURE;
  }

  // Creating Context and Command Queue for selected device
  cl::Context context(device);
  cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE);

  // ======= Found Device =======================================================================================

  // Load rotation xclbin
  char *rxclbinFilename = argv[1];
  std::cout << "Loading: '" << rxclbinFilename << "'\n";
  std::ifstream rbin_file(rxclbinFilename, std::ifstream::binary);
  rbin_file.seekg(0, rbin_file.end);
  unsigned rnb = rbin_file.tellg();
  rbin_file.seekg(0, rbin_file.beg);
  char *rbuf = new char[rnb];
  rbin_file.read(rbuf, rnb);


  // Creating Program from Binary File
  cl::Program::Binaries bins;
  bins.push_back({rbuf, rnb});
  cl::Program program(context, devices, bins);

  // This call will get the kernel object from program. A kernel is an
  // OpenCL function that is executed on the FPGA.

  printf("INITIALIZING DATA\n");

  Track* inputTracks = new Track[INPUTTRACKSIZE];
  Track* outTracks = new Track[INPUTTRACKSIZE];

  // Input hit list to search
  int count = 0;
  std::ifstream trackFile("../tb_files/tb_track_data.dat");
  std::ifstream scoreFile("../tb_files/tb_NNscore.dat");
  if(trackFile.is_open() && scoreFile.is_open()){
    // Read the input file
    std::string trackLine, scoreLine;
    while(std::getline(trackFile, trackLine) && std::getline(scoreFile, scoreLine)){
      std::stringstream linestream(trackLine);
      std::string s;

      double NNScore = std::stof(scoreLine); // get NN score from the other file
      inputTracks[count].NNScore = NNScore;
      // inputTracks[count].flag_delete = ap_int<2>(0);

      outTracks[count].NNScore = 0;
      // outTracks[count].flag_delete = ap_int<2>(0);
      for(int i = 0; i < NHITS; i++){
        outTracks[count].hits[i].x = 0;
        outTracks[count].hits[i].y = 0;
        outTracks[count].hits[i].y = 0;
      }

      // store it in a float
      std::vector<double> inValue;
      while (std::getline(linestream, s, ' ')){
        inValue.push_back(std::stof(s));
      }

      // Organize it into hits
      for(int i = 0; i < NHITS; i++){
        inputTracks[count].hits[i].x = int(inValue[i + 0 * NHITS]); // TODO: is int() needed?
        inputTracks[count].hits[i].y = int(inValue[i + 1 * NHITS]);
        inputTracks[count].hits[i].z = int(inValue[i + 2 * NHITS]);
      }
      count++;
      if(count >= INPUTTRACKSIZE) break;
    }
  } else {
    printf("\n\nFailed to open input files!\n\n");
    trackFile.close();
    scoreFile.close();
    return 0;
  }
  trackFile.close();
  scoreFile.close();

  printf("\n---=== Running Kernel ===---\n\n");

  auto begin = std::chrono::high_resolution_clock::now();
  printTiming("Initializing:\n  %d us\n", begin);

  printf(" ┌------- sending 100 tracks\n");
  setupRunSearch(program, context, q, inputTracks, outTracks);
  std::cout << " └>Total on FPGA+transfer run\n";
  int timingFPGA = printTiming("%d us\n", begin);
  printf("\n---=== Finished Kernel ===---\n\n");
  std::cout << std::endl;

  // printf("Pred Outs:\n");
  // for (int i = 0; i < INPUTTRACKSIZE; i++) {
  //   if(float(outTracks[i].NNScore) < 0.5){
  //     break;
  //   }
  //   printf("ID: %d | NNScore: %f\n", i, float(outTracks[i].NNScore));
  //   for(int j = 0; j < NHITS; j++){
  //     printf(" %d %d %d\n",int(outTracks[i].hits[j].x), int(outTracks[i].hits[j].y), int(outTracks[i].hits[j].z));
  //   }
  //   printf("\n");
  // }

  std::ofstream outputFile("../tb_files/tb_output.dat");
  if(outputFile.is_open()){
    for (int i = 0; i < INPUTTRACKSIZE; i++) {
      if(float(outTracks[i].NNScore) < 0.5){
        break;
      }
      outputFile << outTracks[i].NNScore << "\n";
      for(int j = 0; j < NHITS; j++){
        outputFile << outTracks[i].hits[j].x << " ";
        outputFile << outTracks[i].hits[j].y << " ";
        outputFile << outTracks[i].hits[j].z << "\n";
      }
    }
  }
  outputFile.close();

  std::cout << std::endl;

  std::cout << "Finished" << std::endl;
  return 0;
}
