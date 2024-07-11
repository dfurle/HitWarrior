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

// void setupRunSearch(cl::Program& program, cl::Context& context, cl::CommandQueue& q, Track* in, Track* out)
void setupRunSearch(cl::Program& program, cl::Context& context, cl::CommandQueue& q, Track* in)
{
  auto begin = std::chrono::high_resolution_clock::now();

  // printf("Initialize kernel\n");
  cl::Kernel kernel(program, "runner");
  printTiming(" - kernel init %d us\n", begin);

  // Compute the size of array in bytes
  size_t in_size = sizeof(Track) * MAX_TRACK_SIZE;
  // size_t out_size = sizeof(Track) * MAX_TRACK_SIZE;

  // std::cout << "in_hist_size:  " << in_size << std::endl;
  // std::cout << "out_size: " << out_size << std::endl;
  // printf("\n");

  // These commands will allocate memory on the Device. The cl::Buffer objects
  // can be used to reference the memory locations on the device.
  // printf("Initializing Buffers\n");
  cl::Buffer buffer_l1(context, CL_MEM_READ_ONLY, in_size);
  // cl::Buffer buffer_out(context, CL_MEM_WRITE_ONLY, out_size);

  // set the kernel Arguments
  int narg = 0;
  kernel.setArg(narg++, buffer_l1);
  // kernel.setArg(narg++, buffer_out);
  // kernel.setArg(narg++, MIN_DIST);
  kernel.setArg(narg++, MAX_SHARED);
  kernel.setArg(narg++, 32);

  // We then need to map our OpenCL buffers to get the pointers
  Track *ptr_l1 = (Track *)q.enqueueMapBuffer(buffer_l1, CL_TRUE, CL_MAP_WRITE, 0, in_size);
  // Track *ptr_out = (Track *)q.enqueueMapBuffer(buffer_out, CL_TRUE, CL_MAP_READ, 0, out_size);

  printTiming(" - mapbuf %d us\n", begin);

  memcpy ( ptr_l1, in, in_size );

  printTiming(" - memcpy in %d us\n", begin);

  q.enqueueMigrateMemObjects({buffer_l1}, 0); // 0 means from host
  q.enqueueTask(kernel);
  // q.enqueueMigrateMemObjects({buffer_out}, CL_MIGRATE_MEM_OBJECT_HOST);
  q.enqueueMigrateMemObjects({buffer_l1}, CL_MIGRATE_MEM_OBJECT_HOST);
  printTiming(" - migrate %d us\n", begin);
  q.finish();
  printTiming(" - finish %d us\n", begin);

  // memcpy ( out, ptr_out, out_size );
  memcpy ( in, ptr_l1, in_size );

  printTiming(" - memcpy out %d us\n", begin);

  q.enqueueUnmapMemObject(buffer_l1, ptr_l1);
  // q.enqueueUnmapMemObject(buffer_out, ptr_out);

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

  Track* inTracks = new Track[MAX_TRACK_SIZE];
  // Track* outTracks = new Track[MAX_TRACK_SIZE];


  // Input hit list to search
  int count = 0;
  std::ifstream trackFile("../tb_files/data/formatted_data.csv");
  if(trackFile.is_open()){
    // Read the input file
    std::string hitline;
    // skip header
    std::getline(trackFile, hitline);
    while(true){ // assume it never ends... bad but eh
      for(int h = 0; h < NHITS; h++){ // 5 hits
        if(!std::getline(trackFile, hitline)){
          printf("END OF FILE\n");
          break;
      }
        std::stringstream linestream(hitline);
        std::string s;

        std::getline(linestream, s, ',');
        int trackid = std::stoi(s);

        std::getline(linestream, s, ',');
        int hitid = std::stoi(s);

        std::getline(linestream, s, ',');
        float pt = std::stof(s);

        std::getline(linestream, s, ',');
        int truth = std::stoi(s);

        std::getline(linestream, s, ',');
        float nnscore = std::stof(s);

        std::getline(linestream, s, ',');
        float x = std::stof(s);

        std::getline(linestream, s, ',');
        float y = std::stof(s);

        std::getline(linestream, s, ',');
        float z = std::stof(s);

        // printf("%d %d :  %.2f %.2f %.2f\n", count, h, x, y, z);

        inTracks[count].hits[h].x = x;
        inTracks[count].hits[h].y = y;
        inTracks[count].hits[h].z = z;
        inTracks[count].NNScore = nnscore_t(nnscore); // redundant but eh
      }
      count++;
      if(count >= MAX_TRACK_SIZE) break;
    }
  } else {
    printf("\n\nFailed to open one of the files!!!\n\n\n");
    trackFile.close();
    return 0;
  }
  trackFile.close();

  printf("\n---=== Running Kernel ===---\n\n");

  auto begin = std::chrono::high_resolution_clock::now();
  printTiming("Initializing:\n  %d us\n", begin);

  printf(" ┌------- sending 100 tracks\n");
  // setupRunSearch(program, context, q, inputTracks, outTracks);
  setupRunSearch(program, context, q, inTracks);
  std::cout << " └>Total on FPGA+transfer run\n";
  int timingFPGA = printTiming("%d us\n", begin);
  printf("\n---=== Finished Kernel ===---\n\n");
  std::cout << std::endl;

  printf("Pred Outs:\n");
  int counter = 0;
  for (int i = 0; i < MAX_TRACK_SIZE; i++) {
    if(float(inTracks[i].NNScore) < 0.5){
      continue;
    }
    printf("ID: %d : Count: %d | NNScore: %f\n", i, counter++, float(inTracks[i].NNScore));
    for(int j = 0; j < NHITS; j++){
      printf(" %d %d %d\n",int(inTracks[i].hits[j].x), int(inTracks[i].hits[j].y), int(inTracks[i].hits[j].z));
    }
    printf("\n");
  }

  std::ofstream outputFile("../tb_files/tb_output.dat");
  if(outputFile.is_open()){
    for (int i = 0; i < MAX_TRACK_SIZE; i++) {
      // if(float(outTracks[i].NNScore) < 0.5){
      if(float(inTracks[i].NNScore) < 0.5){
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
