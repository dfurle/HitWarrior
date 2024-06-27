
# name of .xcl file
HLS_NAME := SearchKernel
# xilinx platform to be used
PLATFORM := xilinx_u250_gen3x16_xdma_4_1_202210_1
# emulation mode (sw_emu, hw_emu, hw)
EMU_MODE := hw

########

WORK := $(realpath ../)
LDFLAGS := -L${XILINX_XRT}/lib
LDFLAGS += -lxilinxopencl -lxrt_core -lxrt++ -lxrt_coreutil
# LDFLAGS += -pthread   #doesnt seem to be needed, but if anything breaks try uncomment this first!
DESIGN_CONFIG := ${WORK}/design_runner.cfg
KERNEL_WORK_DIR := runner_kernel
main: ${HLS_NAME}.xclbin ./host.exe export.sh

.PHONY: host compile link clean all

all: host compile link

HOST_SRC := ${WORK}/host.cpp
HOST_INCLUDE := -I${XILINX_XRT}/include/
HOST_INCLUDE += -I${XILINX_HLS}/include/
HOST_INCLUDE += -I${WORK}/
host.exe: $(HOST_SRC)
	g++ -g -std=c++17 -Wall -O0 $(HOST_SRC) ${HOST_INCLUDE} -o ./host.exe  ${LDFLAGS}

RUNNER_KERNEL := runner
RUNNER_SRC := ${WORK}/runner/runner.cpp
RUNNER_CFG := ${WORK}/runner/hls_config.cfg
${RUNNER_KERNEL}.xo: ${RUNNER_SRC} ${RUNNER_CFG}
	mkdir -p ./kernels
	cd ./kernels && \
	v++ -c --mode hls --config ${RUNNER_CFG} && \
	mv ${RUNNER_KERNEL}/${RUNNER_KERNEL}.xo ..


csim: ${RUNNER_SRC} ${RUNNER_CFG}
	vitis-run --mode hls --csim --config ${RUNNER_CFG} --work_dir ./kernels/runner

# v++ -l -t $(EMU_MODE) --config ${WORK}/runner/config.cfg ../${RUNNER_KERNEL}.xo -o $(HLS_NAME).xclbin

${HLS_NAME}.xclbin: ${RUNNER_KERNEL}.xo  ${DESIGN_CONFIG} emconfig.json
	mkdir -p ./xclbin
	cd ./xclbin && \
	v++ -l -t $(EMU_MODE) --platform $(PLATFORM) --config ${DESIGN_CONFIG} ../${RUNNER_KERNEL}.xo -o $(HLS_NAME).xclbin && \
	mv ${HLS_NAME}.xclbin ..

export.sh: ${HLS_NAME}.xclbin
	@echo "export XCL_EMULATION_MODE=$(EMU_MODE)" > export.sh
	echo "do 'source export.sh' if EMU_MODE was changed"

emconfig.json:
	emconfigutil --platform $(PLATFORM)

host:
	rm -f host.exe
	${MAKE}

link:
	rm -f ${HLS_NAME}.xclbin
	${MAKE}

compile:
	rm -f  ${RUNNER_KERNEL}.xo
	${MAKE}

clean:
	rm -rf ./.Xil/
	rm -rf ./.run/
	rm -rf ./kernels
	rm -rf ./xclbin
	rm -rf ${READ_KERNEL}
	rm -rf ${WRITE_KERNEL}
	rm -rf ${FILTER_KERNEL}
	rm -rf ${SEARCH_KERNEL}
	rm -f *.xo
	rm -f *.xclbin
	rm -f *.exe
	rm -f emconfig.json
	rm -f vitis-comp.json
	rm -f export.sh
	rm -f host.exe

