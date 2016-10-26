/************************************************************************
 *
 * Copyright (c) 2013 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Intel Corporation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***************************************************************************/
#ifndef _MEMINIT_UTILS_H_
#define _MEMINIT_UTILS_H_

// General Definitions:
#ifdef QUICKSIM
#define SAMPLE_SIZE     4   // reduce number of training samples in simulation env
#else
#define SAMPLE_SIZE     6   // must be odd number
#endif

#define EARLY_DB    (0x12)  // must be less than this number to enable early deadband
#define LATE_DB     (0x34)  // must be greater than this number to enable late deadband
#define CHX_REGS    (11*4)
#define FULL_CLK      128
#define HALF_CLK       64
#define QRTR_CLK       32



#define MCEIL(num,den) ((uint8_t)((num+den-1)/den))
#define MMAX(a,b)      ((a)>(b)?(a):(b))
#define MCOUNT(a)      (sizeof(a)/sizeof(*a))

typedef enum ALGOS_enum {
  eRCVN = 0,
  eWDQS,
  eWDQx,
  eRDQS,
  eVREF,
  eWCMD,
  eWCTL,
  eWCLK,
  eMAX_ALGOS,
} ALGOs_t; 


// Prototypes:
void set_rcvn(uint8_t channel, uint8_t rank, uint8_t byte_lane, uint32_t pi_count);
void set_rdqs(uint8_t channel, uint8_t rank, uint8_t byte_lane, uint32_t pi_count);
void set_wdqs(uint8_t channel, uint8_t rank, uint8_t byte_lane, uint32_t pi_count);
void set_wdq(uint8_t channel, uint8_t rank, uint8_t byte_lane, uint32_t pi_count);
void set_wcmd(uint8_t channel, uint32_t pi_count);
void set_wclk(uint8_t channel, uint8_t grp, uint32_t pi_count);
void set_wctl(uint8_t channel, uint8_t rank, uint32_t pi_count);
void set_vref(uint8_t channel, uint8_t byte_lane, uint32_t setting);
uint32_t get_rcvn(uint8_t channel, uint8_t rank, uint8_t byte_lane);
uint32_t get_rdqs(uint8_t channel, uint8_t rank, uint8_t byte_lane);
uint32_t get_wdqs(uint8_t channel, uint8_t rank, uint8_t byte_lane);
uint32_t get_wdq(uint8_t channel, uint8_t rank, uint8_t byte_lane);
uint32_t get_wcmd(uint8_t channel);
uint32_t get_wclk(uint8_t channel, uint8_t group);
uint32_t get_wctl(uint8_t channel, uint8_t rank);
uint32_t get_vref(uint8_t channel, uint8_t byte_lane);

void clear_pointers(void);
void enable_cache(void);
void disable_cache(void);
void find_rising_edge(MRCParams_t *mrc_params, uint32_t delay[], uint8_t channel, uint8_t rank, bool rcvn);
uint32_t sample_dqs(MRCParams_t *mrc_params, uint8_t channel, uint8_t rank, bool rcvn);
uint32_t get_addr(MRCParams_t *mrc_params, uint8_t channel, uint8_t rank);
uint32_t byte_lane_mask(MRCParams_t *mrc_params);

uint64_t read_tsc(void);
uint32_t get_tsc_freq(void);
void delay_n(uint32_t nanoseconds);
void delay_u(uint32_t microseconds);
void delay_m(uint32_t milliseconds);
void delay_s(uint32_t seconds);

void post_code(uint8_t major, uint8_t minor);
void training_message(uint8_t channel, uint8_t rank, uint8_t byte_lane);
void print_timings(MRCParams_t *mrc_params);

void enable_scrambling(MRCParams_t *mrc_params);
void store_timings(MRCParams_t *mrc_params); 
void restore_timings(MRCParams_t *mrc_params);
void default_timings(MRCParams_t *mrc_params);

#ifndef SIM
void *memset(void *d, int c, size_t n);
void *memcpy(void *d, const void *s, size_t n);
#endif

#endif // _MEMINIT_UTILS_H_
