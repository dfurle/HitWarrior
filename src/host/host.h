#pragma once
#include "helper.h"
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <vector>

#include "projectDefines.h"


class CKernel{
private:
  cl::CommandQueue* q;
  std::vector<cl::Device> devices;
  cl::Device* device;
  cl::Program* program;
  cl::Kernel* kernel;
  cl::Context* context;

  struct Var{
    cl::Buffer buf;
    size_t size;
    void* ptr;
  };
  std::vector<Var> args;

public:
  CKernel(){

  }

  void create_buffer(size_t size, int narg, cl_mem_flags mem_flags, cl_map_flags map_flags){
    if(args.size() < narg){
      printf("Please initialize in argument order; out of order currently not implemented\n");
      return;
    }
    Var v;
    v.size = size;
    v.buf = cl::Buffer(*context, mem_flags, size);
    kernel->setArg(narg, v.buf);
    v.ptr = q->enqueueMapBuffer(v.buf, CL_TRUE, map_flags, 0, size);
    args.push_back(v);
  }

  cl::Kernel* get_kernel(){
    return kernel;
  }
  
  void migrate_buf(int narg, bool from_host=true){
    cl_mem_migration_flags f = 0;
    if(!from_host)
      f = CL_MIGRATE_MEM_OBJECT_HOST;
    q->enqueueMigrateMemObjects({args[narg].buf}, f);
  }
  void unmap(int narg){
    q->enqueueUnmapMemObject(args[narg].buf, args[narg].ptr);
  }

  void write_mem(int narg, void* data){
    memcpy(args[narg].ptr, data, args[narg].size);
  }
  void read_mem(int narg, void* data){
    memcpy(data, args[narg].ptr, args[narg].size);
  }

  void enqueue_kernel(){
    q->enqueueTask(*kernel);
  }
  void finish(){
    q->finish();
  }


  // device_name_filter : "u280" or other
  int setup_device(std::string device_name_filter){

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
        std::cout << "number Devices: " << devices.size() << std::endl;
        for(size_t d = 0; d < devices.size(); d++) {
          std::string deviceName = devices[0].getInfo<CL_DEVICE_NAME>();
          std::cout << "Device: (" << d << "): " << deviceName << std::endl;
          if(deviceName.find(device_name_filter) != std::string::npos){
            printf("found device\n");
            device = &devices[0];
            found_device = true;
            printf("-done\n");
            break;
          } else {
            printf("erasing\n");
            devices.erase(devices.begin());
            printf("-done\n");
          }
        }
      }
    }
    if (found_device == false) {
      std::cout << "Error: Unable to find Target Device "
                << device->getInfo<CL_DEVICE_NAME>() << std::endl;
      return EXIT_FAILURE;
    }
    devices.resize(1);


    printf("context\n");
    context = new cl::Context(*device);
    printf("command queue\n");
    q = new cl::CommandQueue(*context, *device, CL_QUEUE_PROFILING_ENABLE);
    return 0;
  }


  void load_xclbin(char* xclbin_name){
    std::cout << "Loading: '" << xclbin_name << "'\n";
    std::ifstream rbin_file(xclbin_name, std::ifstream::binary);
    rbin_file.seekg(0, rbin_file.end);
    unsigned rnb = rbin_file.tellg();
    rbin_file.seekg(0, rbin_file.beg);
    char *rbuf = new char[rnb];
    rbin_file.read(rbuf, rnb);


    // Creating Program from Binary File
    cl::Program::Binaries bins;
    bins.push_back({rbuf, rnb});
    program = new cl::Program(*this->context, devices, bins);
  }

  void setup_kernel(std::string kernel_name){
    kernel = new cl::Kernel(*this->program, "runner");
  }




};