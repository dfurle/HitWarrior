# Hit Warrior

## Compiling

```
source /path/to/Vitis/yyyy.v/settings64.sh
source /path/to/xrt/setup.sh
cd build
make
source export.sh        # if running emulation
./host.exe SearchKernel.xclbin
```

## Important files

- `build/Makefile`
  - various parameters such as sw_emu/hw_emu/hw compilation
  - which xilinx platform to use, etc
- `src/common/projectDefines.h`
  - contains input track size and some other parameters
- `src/common/design.cfg`
  - used during .xo linking
  - provides interface information between kernels
- `src/kernels/runner/hls_config.cfg`
  - holds information on how to compile the kernel
- `tb_files_convert_tracks.py`
  - used to generate the datafile from `recoTracks.txt` which wasn't pushed to the github
- `tb_files/data_converter.ipynb`
  - plots and prints of various tests after running the kernel
- `nnscore_network/training.ipynb`
  - retrain the nnscore network, hls4ml seems a bit broken with latest version of keras

## Problems

- nnscore network currently takes up 120% DSP - figure out why or how to reduce
