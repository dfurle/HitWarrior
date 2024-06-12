
# name of .xcl file
HLS_NAME := SearchKernel
# xilinx platform to be used
PLATFORM := xilinx_u250_gen3x16_xdma_4_1_202210_1
# emulation mode (sw_emu, hw_emu, hw)
EMU_MODE := sw_emu

########

WORK := $(realpath ../)
LDFLAGS := -L${XILINX_XRT}/lib
LDFLAGS += -lxilinxopencl -lxrt_core -lxrt++ -lxrt_coreutil
# LDFLAGS += -pthread   #doesnt seem to be needed, but if anything breaks try uncomment this first!
DESIGN_CONFIG := ${WORK}/design.cfg
# HLS_CONFIG := ${WORK}/runner/hls_config.cfg
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

WRITE_KERNEL := write_data
WRITE_SRC := ${WORK}/kernel_${WRITE_KERNEL}/${WRITE_KERNEL}.cpp
WRITE_INCLUDE := -I${WORK}
WRITE_CONFIG := ${WORK}/kernel_${WRITE_KERNEL}/hls_config.cfg
${WRITE_KERNEL}.xo: ${WRITE_SRC} ${WRITE_CONFIG}
	mkdir -p ./${WRITE_KERNEL}
	cd ./${WRITE_KERNEL} && \
	v++ -c --mode hls --config ${WRITE_CONFIG} && \
	mv  ${WRITE_KERNEL}/${WRITE_KERNEL}.xo ..
	
READ_KERNEL := read_data
READ_SRC := ${WORK}/kernel_${READ_KERNEL}/${READ_KERNEL}.cpp
READ_INCLUDE := -I${WORK}
READ_CONFIG := ${WORK}/kernel_${READ_KERNEL}/hls_config.cfg
${READ_KERNEL}.xo: ${READ_SRC} ${READ_CONFIG}
	mkdir -p ./${READ_KERNEL}
	cd ./${READ_KERNEL} && \
	v++ -c --mode hls --config ${READ_CONFIG} && \
	mv  ${READ_KERNEL}/${READ_KERNEL}.xo ..

FILTER_KERNEL := filterLowNN
FILTER_SRC := ${WORK}/kernel_${FILTER_KERNEL}/${FILTER_KERNEL}.cpp
FILTER_INCLUDE := -I${WORK}
FILTER_CONFIG := ${WORK}/kernel_${FILTER_KERNEL}/hls_config.cfg
${FILTER_KERNEL}.xo: ${FILTER_SRC} ${FILTER_CONFIG}
	mkdir -p ./${FILTER_KERNEL}
	cd ./${FILTER_KERNEL} && \
	v++ -c --mode hls --config ${FILTER_CONFIG} && \
	mv  filterlownn/${FILTER_KERNEL}.xo ..

SEARCH_KERNEL := searchHit
SEARCH_SRC := ${WORK}/kernel_${SEARCH_KERNEL}/${SEARCH_KERNEL}.cpp
SEARCH_INCLUDE := -I${WORK}
SEARCH_CONFIG := ${WORK}/kernel_${SEARCH_KERNEL}/hls_config.cfg
${SEARCH_KERNEL}.xo: ${SEARCH_SRC} ${SEARCH_CONFIG}
	mkdir -p ./${SEARCH_KERNEL}
	cd ./${SEARCH_KERNEL} && \
	v++ -c --mode hls --config ${SEARCH_CONFIG} && \
	mv  searchhit/${SEARCH_KERNEL}.xo ..


# csim: ${RUNNER_SRC} ${HLS_CONFIG}
# 	vitis-run --mode hls --csim --config ${HLS_CONFIG} --work_dir ./kernels/runner

# v++ -l -t $(EMU_MODE) --config ${WORK}/runner/config.cfg ../${RUNNER_KERNEL}.xo -o $(HLS_NAME).xclbin

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

