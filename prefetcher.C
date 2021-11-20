/*
 * File: prefetcher.C
 * Author: Omer Eren
 * Description: TODO 
 */

#include "prefetcher.h"
#include <stdio.h>
#include <stdlib.h>


Prefetcher::Prefetcher() { 
	_ready = false; 
	_last_ready_clock = 0;

	_capacity = 300;
	_clock_interval = 100;
	_load_only = true;

	_front = 0;
	_rear = 0;
	_size = 0;

	_addrs = (u_int32_t*)calloc(_capacity, sizeof(uint32_t));
	_strides = (int32_t*)calloc(_capacity, sizeof(int32_t));
	_last_clock = (u_int32_t*)calloc(_capacity, sizeof(uint32_t));
	_first_clock = (u_int32_t*)calloc(_capacity, sizeof(uint32_t));
	_lens = (u_int32_t*)calloc(_capacity, sizeof(uint32_t));
	_executed = (bool*)calloc(_capacity, sizeof(bool));
}

bool Prefetcher::hasRequest(u_int32_t cycle) { 
	int a = _front;
	if(!_ready && _size > 0 && cycle-_last_ready_clock > _clock_interval) {
		int i = _rear;
		while(i != _front) {
			if(!_executed[i]) break;
			i = i-1;
			if(i == -1) i = _capacity-1;
		}
		if(!_executed[i]) {
			int _a = i;
			printf("hasRequest is planning to execute=\n");
			printf("_rear=%d, addr=%x, stride=%d, len=%d, executed=%d, last_clock=%d, first_clock=%d\n", a, _addrs[a], _strides[a], _lens[a], _executed[a], _last_clock[a], _first_clock[a]);
			_executed[i] = true;
			_last_ready_clock = cycle;
			_ready = true;
			_nextReq.addr = _addrs[i] + _strides[i];
		}
	}
	return _ready; 
}

Request Prefetcher::getRequest(u_int32_t cycle) { 
	return _nextReq;
}

void Prefetcher::completeRequest(u_int32_t cycle) { 
	_ready = false;
}

void Prefetcher::cpuRequest(Request req) { 
	if(_load_only && !req.load)
		return;

	if(_size == _capacity-1) {
		_size--;
		_front = _front + 1;
		if(_front == _capacity) _front = 0;
	}
	int32_t max_len = -1;
	int32_t max_stride = -1;
	int32_t max_j = -1;
	int32_t i = _front;
	while(i !=_rear) {
		int32_t stride = req.addr - _addrs[i];
		u_int32_t len = 2;
		int32_t last = _addrs[i];
		int last_j = i;

		int32_t j = i - 1;
		if(j == -1) j = _capacity-1;

		int32_t bef_front = _front - 1;
		if(bef_front == -1) bef_front = _capacity-1;

		while(j != bef_front) {
			if(last - _addrs[j] == stride) {
				len++;
				last=_addrs[j];
				last_j = j;
			}

			j = j+1;
			if(j == _capacity) j=0;
		}

		if(len > max_len) {
			max_len = len;
			max_stride = stride;
			max_j = last_j;
		}
		i = i+1;
		if(i == _capacity) i = 0;
	}
	_addrs[_rear] = req.addr;
	_last_clock[_rear] = req.issuedAt;
	_executed[_rear] = false;
	if(max_len != -1) {
		_strides[_rear] = max_stride;
		_lens[_rear] = max_len;
		_first_clock[_rear] = _last_clock[max_j];
	} else {
		_strides[_rear] = 0;
		_lens[_rear] = 1;
		_first_clock[_rear] = req.issuedAt;
	}
	int a = _rear;
	printf("new stride added for cpuRequest\n");
	printf("cpuRequest = req.addr=%x, req.pc=%x, req.load=%d, req.fromCPU=%d, req.issuedAt=%d, req.HitL1=%d, req.HitL2=%d\n",req.addr, req.pc, req.load, req.fromCPU, req.issuedAt, req.HitL1, req.HitL2);
	printf("_rear=%d, addr=%x, stride=%d, len=%d, executed=%d, last_clock=%d, first_clock=%d\n", a, _addrs[a], _strides[a], _lens[a], _executed[a], _last_clock[a], _first_clock[a]);
	_size++;
	_rear++;
	if(_rear == _capacity) _rear = 0;
	return;
}
