
# name of .xcl file
HLS_NAME := SearchKernel
# xilinx platform to be used
PLATFORM := xilinx_u250_gen3x16_xdma_4_1_202210_1
# emulation mode (sw_emu, hw_emu, hw)
EMU_MODE := sw_emu

########

SRC_COMMON := $(realpath ../src/common)
SRC_HOST := $(realpath ../src/host)
SRC_KERNELS := $(realpath ../src/kernels)
BASE := $(realpath ../)
LDFLAGS := -L${XILINX_XRT}/lib
LDFLAGS += -lxilinxopencl -lxrt_core -lxrt++ -lxrt_coreutil
# LDFLAGS += -pthread   #doesnt seem to be needed, but if anything breaks try uncomment this first!
# KERNEL_WORK_DIR := runner_kernel   # TODO: figure out what this was?

main: ${HLS_NAME}.xclbin ./host.exe export.sh

.PHONY: host compile link clean all

all: host compile link


# ---=== Runner Kernel ===---
# TODO: figure out how to make simpler/seperate makefiles that will build each one, I know that is possible...
RUNNER_KERNEL := runner
RUNNER_SRC := ${SRC_KERNELS}/runner/runner.cpp
RUNNER_CFG := ${SRC_KERNELS}/runner/hls_config.cfg    # this contains the references to .cpp files, the file to be updated aswell
# TODO: figure out a simpler way, i think there is a --temp-dir flag for `v++` ?
${RUNNER_KERNEL}.xo: ${RUNNER_SRC} ${RUNNER_CFG}
	mkdir -p ./kernels
	cd ./kernels && \
	v++ -c --mode hls --config ${RUNNER_CFG} && \
	mv ${RUNNER_KERNEL}/${RUNNER_KERNEL}.xo ..

csim: ${RUNNER_SRC} ${RUNNER_CFG}
	vitis-run --mode hls --csim --config ${RUNNER_CFG} --work_dir ./kernels/runner
# ---===---===---


# ---=== XCLBIN generation ===---
DESIGN_CONFIG := ${SRC_COMMON}/design.cfg
${HLS_NAME}.xclbin: ${RUNNER_KERNEL}.xo ${DESIGN_CONFIG} emconfig.json
	mkdir -p ./xclbin
	cd ./xclbin && \
	v++ -l -t $(EMU_MODE) --platform $(PLATFORM) --config ${DESIGN_CONFIG} ../${RUNNER_KERNEL}.xo -o $(HLS_NAME).xclbin && \
	mv ${HLS_NAME}.xclbin ..

export.sh: ${HLS_NAME}.xclbin
	@echo "export XCL_EMULATION_MODE=$(EMU_MODE)" > export.sh
	echo "do 'source export.sh' if EMU_MODE was changed"

emconfig.json:
	emconfigutil --platform $(PLATFORM)
# ---===---===---


# ---=== Host executable ===---
HOST_SRC := ${SRC_HOST}/host.cpp # may be bad naming here, HOST_SRC and SRC_HOST are pointing to differnt things...
HOST_INCLUDE := -I${XILINX_XRT}/include/
HOST_INCLUDE += -I${XILINX_HLS}/include/
HOST_INCLUDE += -I${SRC_COMMON}/
HOST_INCLUDE += -I${SRC_HOST}/
host.exe: $(HOST_SRC)
	g++ -g -std=c++17 -Wall -O0 $(HOST_SRC) ${HOST_INCLUDE} -o ./host.exe  ${LDFLAGS}
# ---===---===---


# TODO: make a better way to generate just the required things, cuz this will just remove files and regenerate everything
host:
	rm -f host.exe
	${MAKE}

link:
	rm -f ${HLS_NAME}.xclbin
	${MAKE}

compile:
	rm -f  *.xo
	${MAKE}

clean:
	rm -rf ./.Xil/
	rm -rf ./.run/
	rm -rf ./kernels
	rm -rf ./xclbin
	rm -f *.xo
	rm -f *.xclbin
	rm -f *.exe
	rm -f emconfig.json
	rm -f vitis-comp.json
	rm -f export.sh
	rm -f host.exe

