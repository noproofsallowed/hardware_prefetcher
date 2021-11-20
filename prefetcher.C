/*
 *
 * File: prefetcher.C
 * Author: Sat Garcia (sat@cs)
 * Description: This simple prefetcher waits until there is a D-cache miss then 
 * requests location (addr + 16), where addr is the address that just missed 
 * in the cache.
 *
 */

#include "prefetcher.h"
#include <stdio.h>

Prefetcher::Prefetcher() { _ready = false; _n = 0; _m = 0; _a = 0;}

bool Prefetcher::hasRequest(u_int32_t cycle) { 
	/* printf("hasRequest\n"); */
	/* return false; */
	return _ready; 
}

Request Prefetcher::getRequest(u_int32_t cycle) { 
	/* printf("getRequest\n"); */
	/* printf("%d\n", _n); */
	_nextReq.addr = _last_addrs[_a++];
	_m--;
	return _nextReq; 
}

void Prefetcher::completeRequest(u_int32_t cycle) { 
	/* printf("completeRequest\n"); */
	if(_n == _a || _m == 0)
		_ready = false; 
}

void Prefetcher::cpuRequest(Request req) { 
	/* if(req.issuedAt%100 == 0) */
	/* printf("%d\n",req.issuedAt); */
	/* printf("cpuRequest\n"); */
	if(req.load == false)
		return;
	_m++;
	int tag = req.addr>>4;
	int block = req.addr%16;
	if(!req.HitL1) {
		printf("----------");
		_last_addrs[_n++] = req.addr+32;
		_m += 1;
	} 
	printf("tag:%#05x block: %d\n", tag, block);
	_last_addrs[_n++] = req.addr+16;
	if(!_ready && _m > 0 && _n > 0) {
		_ready = true;
	}
}
