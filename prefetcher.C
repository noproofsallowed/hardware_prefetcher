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

	_capacity = 750;
	_clock_interval = 10; 
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
	_is_dupe = (bool*)calloc(_capacity, sizeof(bool));
}

bool Prefetcher::shouldExecute(int32_t i) {
	if (_lens[i] < 4) return false;
	if (_is_dupe[i]) return false;
	double clock_avg = ((double)_last_clock[i]-(double)_first_clock[i])/(double)_lens[i];
	printf("i=%d, first_clock=%d, last_clock=%d, clock_avg=%.4lf", i, _first_clock[i], _last_clock[i], clock_avg);
	return true;
}

bool Prefetcher::hasRequest(u_int32_t cycle) { 
	// TODO: dupelari temizle sondaki burda
	/* while(_rear != _front) { */
	/* 	int bef_rear = _rear - 1; */
	/* 	if(bef_rear == -1) bef_rear = _capacity-1; */
	/* 	if(!is_dup */
	/* } */

	int a = _front;
	if(!_ready && _size > 0 && cycle-_last_ready_clock > _clock_interval) {
#ifdef DEBUG
		printf("trying to find a suitable candidate...\n");
		printf("_front=%d, _rear=%d, _size=%d\n", _front, _rear, _size);
#endif
		int i = _rear;
		while(i != _front) {
			if(!_executed[i] && shouldExecute(i)) break;
			i = i-1;
			if(i == -1) i = _capacity-1;
		}

		if(!_executed[i]) {
			_executed[i] = true;
			_last_ready_clock = cycle;
			_ready = true;
			_nextReq.addr = _addrs[i] + _strides[i];

			printf("(cycle=%d)hasRequest is planning to execute=\n", cycle);
			printf("_nextReq.addr=%x\n", _nextReq.addr);
			printf("i=%d, addr=%x, stride=%d, len=%d, executed=%d, last_clock=%d, first_clock=%d\n", i, _addrs[i], _strides[i], _lens[i], _executed[i], _last_clock[i], _first_clock[i]);
#ifdef DEBUG
			i = _rear;
			int bef_front = _front - 1;
			if(bef_front == -1) bef_front = _capacity - 1;
			while(i != bef_front) {
				i = i-1;
				if(i == -1) i = _capacity-1;
				printf("i=%d, addr=%x, stride=%d, len=%d, executed=%d, last_clock=%d, first_clock=%d, is_dupe=%d\n", i, _addrs[i], _strides[i], _lens[i], _executed[i], _last_clock[i], _first_clock[i], _is_dupe[i]);
			}
#endif
		} else {
#ifdef DEBUG
			printf("hasRequest is not planning to execute anything :(\n");
#endif
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
	printf("cpuRequest = req.addr=%x, req.pc=%x, req.load=%d, req.fromCPU=%d, req.issuedAt=%d, req.HitL1=%d, req.HitL2=%d\n",req.addr, req.pc, req.load, req.fromCPU, req.issuedAt, req.HitL1, req.HitL2);
	req.addr = ((req.addr >> 4) << 4);
	printf("block req.addr=%x\n", req.addr);
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
		int32_t stride = (int32_t)req.addr - _addrs[i];
		int32_t len = 2;
		int32_t last = _addrs[i];
		int last_j = i;

		int32_t j = i-1;
		if(j == -1) j = _capacity-1;

		int32_t bef_front = _front - 1;
		if(bef_front == -1) bef_front = _capacity-1;

		while(j != bef_front) {
			if(last - _addrs[j] == stride) {
				len++;
				last=_addrs[j];
				last_j = j;
			}

			j = j-1;
			if(j == -1) j = _capacity-1;
		}

		/* printf("len=%d, max_len=%d\n", len, max_len); */
		/* if((stride == 0 || stride >= 4) && len > max_len) { */
		if(stride != 0 && len > 4 && (len > max_len || (len==max_len && abs(max_stride) > abs(stride)))) {
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
	_is_dupe[_rear] = false;
	if(max_len != -1) {
		_strides[_rear] = max_stride;
		_lens[_rear] = max_len;
		_first_clock[_rear] = _last_clock[max_j];
	} else {
		_strides[_rear] = 16;
		_lens[_rear] = 4;
		_first_clock[_rear] = req.issuedAt;
	}
#ifdef DEBUG
	printf("new stride added for cpuRequest\n");
	printf("_rear=%d, addr=%x, stride=%d, len=%d, executed=%d, last_clock=%d, first_clock=%d\n", _rear, _addrs[_rear], _strides[_rear], _lens[_rear], _executed[_rear], _last_clock[_rear], _first_clock[_rear]);
#endif

	// Deduping requests
	// TODO:implement 
	int32_t bef_front = _front - 1;
	if(bef_front == -1) bef_front = _capacity-1;
	i = _rear;
	while(i != bef_front) {
		int j = i;
		while(j != bef_front) {
			if(i!=j && !_is_dupe[j] && (int32_t)_addrs[i]-(int32_t)_addrs[j]==_strides[i]) {
#ifdef DEBUG
				printf("j=%d is a continuing dupe of i=%d, _is_dupe is marked true!\n", j, i);
#endif
				_is_dupe[j] = true;
			}
			if(i!=j && !_is_dupe[i] && _addrs[i] == _addrs[j] && _strides[i] == _strides[j]) {
#ifdef DEBUG
				printf("i=%d is an exact dupe of j=%d, _is_dupe is marked true!\n", i, j);
#endif
				_is_dupe[i] = true;
			}

			j = j-1;
			if(j == -1) j = _capacity-1;
		}

		i = i-1;
		if(i == -1) i = _capacity-1;
	}
	_size++;
	_rear++;
	if(_rear == _capacity) _rear = 0;
	return;
}
