/*
 *
 * File: prefetcher.h
 * Author: Sat Garcia (sat@cs)
 * Description: Header file for prefetcher implementation
 *
 */

#ifndef PREFETCHER_H
#define PREFETCHER_H

#include <sys/types.h>
#include "mem-sim.h"

class Prefetcher {
  private:
	bool _ready;
	Request _nextReq;

	u_int32_t *_tag;
	u_int32_t *_addr;
	int32_t *_stride;
	int32_t *_state; // This really uses 2 bits only!
	
	u_int32_t *_buffer;

	int32_t _front;
	int32_t _rear;
	int32_t _size;

	int32_t _capacity;
	int32_t _num_bits;

	void _add(u_int32_t addr);
  public:
	Prefetcher();

	// should return true if a request is ready for this cycle
	bool hasRequest(u_int32_t cycle);

	// request a desired address be brought in
	Request getRequest(u_int32_t cycle);

	// this function is called whenever the last prefetcher request was successfully sent to the L2
	void completeRequest(u_int32_t cycle);

	/*
	 * This function is called whenever the CPU references memory.
	 * Note that only the addr, pc, load, issuedAt, and HitL1 should be considered valid data
	 */
	void cpuRequest(Request req); 
};

#endif
