
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


# TODO: figure out how to make simpler/seperate makefiles that will build each one, I know that is possible...
# TODO: figure out a simpler way to seperate into various folders, i think there is a --temp-dir flag for `v++` ?

# ---=== Write Data Kernel ===---
WRITE_KERNEL := write_data
WRITE_SRC := ${SRC_KERNELS}/${WRITE_KERNEL}/${WRITE_KERNEL}.cpp
WRITE_CONFIG := ${SRC_KERNELS}/${WRITE_KERNEL}/hls_config.cfg
${WRITE_KERNEL}.xo: ${WRITE_SRC} ${WRITE_CONFIG}
	mkdir -p ./${WRITE_KERNEL}
	cd ./${WRITE_KERNEL} && \
	v++ -c --mode hls --config ${WRITE_CONFIG} && \
	mv  ${WRITE_KERNEL}/${WRITE_KERNEL}.xo ..
	
# ---=== Read Data Kernel ===---
READ_KERNEL := read_data
READ_SRC := ${SRC_KERNELS}/${READ_KERNEL}/${READ_KERNEL}.cpp
READ_CONFIG := ${SRC_KERNELS}/${READ_KERNEL}/hls_config.cfg
${READ_KERNEL}.xo: ${READ_SRC} ${READ_CONFIG}
	mkdir -p ./${READ_KERNEL}
	cd ./${READ_KERNEL} && \
	v++ -c --mode hls --config ${READ_CONFIG} && \
	mv  ${READ_KERNEL}/${READ_KERNEL}.xo ..

# ---=== Filter Low Kernel ===---
FILTER_KERNEL := filterLowNN
FILTER_SRC := ${SRC_KERNELS}/${FILTER_KERNEL}/${FILTER_KERNEL}.cpp
FILTER_CONFIG := ${SRC_KERNELS}/${FILTER_KERNEL}/hls_config.cfg
${FILTER_KERNEL}.xo: ${FILTER_SRC} ${FILTER_CONFIG}
	mkdir -p ./${FILTER_KERNEL}
	cd ./${FILTER_KERNEL} && \
	v++ -c --mode hls --config ${FILTER_CONFIG} && \
	mv  filterlownn/${FILTER_KERNEL}.xo ..

# ---=== Search Hit Kernel ===---
SEARCH_KERNEL := searchHit
SEARCH_SRC := ${SRC_KERNELS}/${SEARCH_KERNEL}/${SEARCH_KERNEL}.cpp
SEARCH_CONFIG := ${SRC_KERNELS}/${SEARCH_KERNEL}/hls_config.cfg
${SEARCH_KERNEL}.xo: ${SEARCH_SRC} ${SEARCH_CONFIG}
	mkdir -p ./${SEARCH_KERNEL}
	cd ./${SEARCH_KERNEL} && \
	v++ -c --mode hls --config ${SEARCH_CONFIG} && \
	mv  searchhit/${SEARCH_KERNEL}.xo ..



# TODO: can make csim for each kernel and csim for all kernels combined somehow...
# csim: ${RUNNER_SRC} ${RUNNER_CFG}
# 	vitis-run --mode hls --csim --config ${RUNNER_CFG} --work_dir ./kernels/runner
# ---===---===---


# ---=== XCLBIN generation ===---
DESIGN_CONFIG := ${SRC_COMMON}/design.cfg
${HLS_NAME}.xclbin: ${SEARCH_KERNEL}.xo ${FILTER_KERNEL}.xo ${READ_KERNEL}.xo ${WRITE_KERNEL}.xo ${DESIGN_CONFIG} emconfig.json
	mkdir -p ./xclbin
	cd ./xclbin && \
	v++ -l -t $(EMU_MODE) --platform $(PLATFORM) --config ${DESIGN_CONFIG} ../${SEARCH_KERNEL}.xo ../${FILTER_KERNEL}.xo ../${READ_KERNEL}.xo ../${WRITE_KERNEL}.xo -o $(HLS_NAME).xclbin && \
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

