#pragma once

#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_vector.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"

#include "projectDefines.h"

typedef ap_axiu<HIT_SIZE,0,0,0> stream_t;
typedef ap_axiu<3,0,0,0> done_t;



enum COMPARISON{BOTH, TRKA, TRKB};