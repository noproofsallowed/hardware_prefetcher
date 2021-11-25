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
#include <math.h>

Prefetcher::Prefetcher() { 
	_ready = false; 

	_num_bits = 10;

	_tag = (u_int32_t*)calloc(1<<_num_bits, sizeof(u_int32_t));
	_addr = (u_int32_t*)calloc(1<<_num_bits, sizeof(u_int32_t));
	_stride = (int32_t*)calloc(1<<_num_bits, sizeof(int32_t));
	_state = (int32_t*)calloc(1<<_num_bits, sizeof(int32_t));

	_capacity = 10;

	_front = 0;
	_rear = 0;
	_size = 0;

	_buffer = (u_int32_t*)calloc(_capacity, sizeof(u_int32_t));
}

bool Prefetcher::hasRequest(u_int32_t cycle) { 
	if(_size > 0) {
		int curr = _rear - 1;
		if(curr == -1) curr = _capacity-1;

		_ready = true;
		_nextReq.addr = _buffer[curr];
		_size--;
		_rear--;
		printf("(cycle=%d)hasRequest is planning to execute=\n", cycle);
		printf("_nextReq.addr=%x\n", _nextReq.addr);
		if(_rear == -1) _rear = _capacity-1;
	}
	return _ready; 
}

Request Prefetcher::getRequest(u_int32_t cycle) { 
	return _nextReq; 
}

void Prefetcher::completeRequest(u_int32_t cycle) { 
	_ready = false; 
}

u_int32_t get_lastn(u_int32_t addr, int n) {
	return (addr&((1<<n)-1));
}

void Prefetcher::_add(u_int32_t addr) {
	if(_size == _capacity-1) {
		_size--;
		_front++;
		if(_front == _capacity) _front = 0;
	}

	_buffer[_rear] = addr;
	
	_size++;
	_rear++;
	if(_rear == _capacity) _rear = 0;
}

void Prefetcher::cpuRequest(Request req) { 
	printf("cpuRequest = req.addr=%x, req.pc=%x, req.load=%d, req.fromCPU=%d, req.issuedAt=%d, req.HitL1=%d, req.HitL2=%d\n",req.addr, req.pc, req.load, req.fromCPU, req.issuedAt, req.HitL1, req.HitL2);
	if(!_ready && !req.HitL1) {
		_ready = false;
	}

	int ind = get_lastn(req.pc>>2, _num_bits);
	int tag = (req.pc>>14);
	u_int32_t block = ((req.addr>>3)<<3);
/* #ifdef DEBUG */
	printf("ind=%d, tag=%x, block=%x, _tag[ind]=%x, _addr[ind]=%x, _stride[ind]=%d, _state[ind]=%d\n", ind, tag, block, _tag[ind], _addr[ind], _stride[ind], _state[ind]); 
/* #endif */
	u_int32_t next;

	if(tag != _tag[ind]) {
		_tag[ind] = tag;
		_addr[ind] = block;
		_stride[ind] = 0;
		_state[ind] = 0;
	}
	else if(tag == _tag[ind] && block != _addr[ind]) {
		if(_state[ind] == 0) {
			_state[ind] = 2;
			_stride[ind] = (int32_t)block-(int32_t)_addr[ind];
			_addr[ind] = block;
		} else if(_state[ind] == 1){
			next = (int32_t)_addr[ind]+_stride[ind];
			if(next == req.addr) {
				_state[ind] = 1;
				_addr[ind] = next;
			} else {
				_state[ind] = 0;
				_addr[ind] = block;
				_stride[ind] = 0;
			}
		} else if(_state[ind] == 2) {
			next = (int32_t)_addr[ind]+_stride[ind];
			if(next == req.addr) {
				_state[ind] = 1;
				_addr[ind] = next;
			} else {
				_state[ind] = 2;
				_stride[ind] = (int32_t)block-(int32_t)_addr[ind];
				_addr[ind] = block;
			}
		} 

		if((_state[ind] == 2 || _state[ind] == 1 ) && _stride[ind] != 0) {
			_add(_addr[ind]+_stride[ind]);
			printf("_add is called\n");
		} else {
			printf("nothing added to prefetch buffer\n");
		}
	}
/* #ifdef DEBUG */
	printf("ind=%d, tag=%x, block=%x, _tag[ind]=%x, _addr[ind]=%x, _stride[ind]=%d, _state[ind]=%d\n", ind, tag, block, _tag[ind], _addr[ind], _stride[ind], _state[ind]); 
/* #endif */
}
