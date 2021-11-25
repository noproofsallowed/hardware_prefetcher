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
#include <stdlib.h>

Prefetcher::Prefetcher() { 
	_ready = false; 
	
	_capacity = 2000;

	_front = 0;
	_rear = 0;
	_size = 0;

	_addr = (u_int32_t*)calloc(_capacity, sizeof(uint32_t));
	_executed = (bool*)calloc(_capacity, sizeof(bool));
	/* _tag = (bool*)calloc(_capacity, sizeof(bool)); */
}

bool Prefetcher::hasRequest(u_int32_t cycle) { 
	int32_t i = _rear-1;
	if(i == -1) i = _capacity-1;
	
	int32_t bef_front = _front-1;
	if(bef_front == -1) bef_front = _capacity-1;

	while(i != bef_front) {
		if(!_executed[i]) break;
		i--;
		if(i == -1) i = _capacity-1;
	}

	if(i != bef_front) {
		_ready = true;
		_nextReq.addr = _addr[i];
		_executed[i] = true;
		printf("(cycle=%d)hasRequest is planning to execute=\n", cycle);
		printf("_nextReq.addr=%x\n", _nextReq.addr);
		printf("i=%d, addr=%x, executed=%d\n", i, _addr[i], _executed[i]);
	}

	return _ready; 
}

Request Prefetcher::getRequest(u_int32_t cycle) { 
	return _nextReq; 
}

void Prefetcher::completeRequest(u_int32_t cycle) { 
	_ready = false; 
}

void Prefetcher::add(u_int32_t addr) {
	if(_size == _capacity-1) {
		_size--;
		_front = _front + 1;
		if(_front == _capacity) _front = 0;
	}

	_addr[_rear] = addr;
	_executed[_rear] = false;

	_size++;
	_rear++;
	if(_rear == _capacity) _rear = 0;
}

void Prefetcher::cpuRequest(Request req) { 
	u_int32_t block = ((req.addr >> 4) << 4);
	printf("cpuRequest = req.addr=%x, block=%x, req.pc=%x, req.load=%d, req.fromCPU=%d, req.issuedAt=%d, req.HitL1=%d, req.HitL2=%d\n",req.addr, block, req.pc, req.load, req.fromCPU, req.issuedAt, req.HitL1, req.HitL2);
	if(req.HitL1) {
		int32_t i = _front;
		while(i != _rear) {
			/* if(block == _addr[i] && _executed[i]) add(block+16); */
			if(block == _addr[i] /* && _executed[i] */) {
				_addr[i] += 16;
				_executed[i] = false;
			}
			i++;
			if(i == _capacity) i = 0;
		}

		return;
	}
	add(block+16);
	return;
}
