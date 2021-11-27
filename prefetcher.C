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
#include <string.h>
#include <algorithm>

u_int32_t _page(u_int32_t addr) {
	return (addr>>(_b+_p));
}

u_int32_t _offset(u_int32_t addr) {
	return ((_page(addr)<<_p)^(addr>>_b));
}

DHB::DHB() {
	for(int i = 0; i < _dhb_capacity; i++) {
		_buffer[i].page = 0;
		_buffer[i].addr = 0;
		_buffer[i].last_predictor = 0;
		_buffer[i].used = 0;
		_buffer[i].MRU = false;
		_buffer[i].predicted = false;
		memset(_buffer[i].delta, 0, _num_delta*sizeof(u_int32_t));
		memset(_buffer[i].offset, 0, _num_delta*sizeof(u_int32_t));
	}
}

int DHB::has(u_int32_t addr) {
	u_int32_t page = _page(addr);
	for(int i = 0; i < _dhb_capacity; i++)
		if(_buffer[i].used > 0 && _buffer[i].page == page)
			return i;
	return -1;
}

int32_t DHB::evict() {
	int32_t ind = 0;
	while(ind < _dhb_capacity && _buffer[ind].MRU) ind++;
	return ind;
}

DHB::Entry* DHB::get(int32_t ind) {
	return &(_buffer[ind]);
}

void DHB::refresh_MRU() {
	bool reset = true;
	for(int i = 0; i < _dhb_capacity;i++)
		reset &= _buffer[i].MRU;
	if(!reset) return;
	for(int i = 0; i < _dhb_capacity;i++)
		_buffer[i].MRU = false;
}

bool DHB::is_hit(int32_t ind, u_int32_t addr) {
	u_int32_t offset = _offset(addr);
	for(int i = 0; i < _num_delta; i++) 
		if(_buffer[i].predicted && _buffer[ind].offset[0] == offset)
			return true;
	return false;
}

DPT::DPT() {
	for(int i = 0; i < _dpt_capacity; i++) {
		_buffer[i].pred = 0;
		_buffer[i].acc[0] = false;
		_buffer[i].acc[1] = false;
		_buffer[i].MRU = false;
		memset(_buffer[i].delta, 0, _num_delta*sizeof(u_int32_t));
	}
}

bool DPT::_delta_equal(int32_t a[_num_delta], int32_t b[_num_delta], const int32_t &depth) {
	for(int i = 0; i < depth; i++) 
		if(a[i] != b[i])
			return false;
	return true;
}

int32_t DPT::has(int32_t delta[_num_delta], int32_t depth) {
	for(int i = 0; i < _dpt_capacity; i++)
		if(_delta_equal(delta, _buffer[i].delta, depth))
			return i;
	return -1;
}

int32_t DPT::evict() {
	int32_t ind = 0;
	while(ind < _dpt_capacity && _buffer[ind].MRU) ind++;
	return ind;
}

DPT::Entry* DPT::get(int32_t ind) {
	return &(_buffer[ind]);
}

void DPT::refresh_MRU() {
	bool reset = true;
	for(int i = 0; i < _dpt_capacity; i++) reset &= _buffer[i].MRU;
	if(!reset) return;
	for(int i = 0; i < _dpt_capacity; i++) _buffer[i].MRU = false;
}

OPT::OPT() {
	for(int i = 0; i < _opt_capacity; i++) {
		_buffer[i].pred = 0;
		_buffer[i].acc = false;
	}
}

OPT::Entry* OPT::get(u_int32_t addr) {
	return &(_buffer[_offset(addr)]);
}

Prefetcher::Prefetcher() { 
	_ready = false; 
	_capacity = _prefetch_capacity;

	_front = 0;
	_rear = 0;
	_size = 0;
}

bool Prefetcher::hasRequest(u_int32_t cycle) { 
	if(_size <= 0)
		return false;
	int ind = _rear-1;
	if(ind == -1) ind = _capacity - 1;
	_nextReq.addr = _prefetch[ind];
	_rear = ind;
	_size--;
	printf("(cycle=%d)hasRequest is planning to execute=\n", cycle);
	printf("_nextReq.addr=%x\n", _nextReq.addr);
	return true;
}

Request Prefetcher::getRequest(u_int32_t cycle) { return _nextReq; }

void Prefetcher::completeRequest(u_int32_t cycle) { _ready = false; }

void Prefetcher::_add(u_int32_t addr) {
	if(_size == _capacity - 1) {
		_front = _front+1;
		if(_front == _capacity) _front = 0;
		_size--;
	}

	_prefetch[_rear] = addr;

	_size++;
	_rear++;
	if(_rear == _capacity) _rear = 0;
}

void Prefetcher::cpuRequest(Request req) { 
	printf("cpuRequest = req.addr=%x, req.pc=%x, req.load=%d, req.fromCPU=%d, req.issuedAt=%d, req.HitL1=%d, req.HitL2=%d\n",req.addr, req.pc, req.load, req.fromCPU, req.issuedAt, req.HitL1, req.HitL2);
	// Determine PAE
	bool pae = false;
	DHB::Entry* dhb_p = NULL;
	if (!req.HitL1) pae = true;

	int dhb_ind = _dhb.has(req.addr);
	if(dhb_ind != -1 && _dhb.is_hit(dhb_ind, req.addr)) pae = true;
	if(!pae) return;

	if(dhb_ind != -1) { 
		dhb_p = _dhb.get(dhb_ind);

		int32_t curr_delta = (int32_t)_offset(req.addr)-(int32_t)(dhb_p->addr);
		for(int i = _num_delta-1; i > 0 ; i--)
			dhb_p->delta[i] = dhb_p->delta[i-1];
		dhb_p->delta[0] = curr_delta;

		dhb_p->page = _page(req.addr);
		dhb_p->addr = _offset(req.addr);
		dhb_p->used++;
		dhb_p->MRU = true;
		_dhb.refresh_MRU();

	} else {
		dhb_ind = _dhb.evict();
		dhb_p = _dhb.get(dhb_ind);
		dhb_p->page = _page(req.addr);
		dhb_p->addr = _offset(req.addr);
		dhb_p->used = 1;
		dhb_p->MRU = true;
		dhb_p->last_predictor = -1;
		dhb_p->predicted = false;
		_dhb.refresh_MRU();
	}

	OPT::Entry* opt_p = _opt.get(req.addr);
	if(dhb_p->used == 2) {
		if((opt_p->pred) == (dhb_p->delta[0]))
			opt_p->acc = true;
		else {
			if(!opt_p->acc) opt_p->pred = dhb_p->delta[0];
			opt_p->acc = false;
		}
	}
	if(dhb_p->used >= 2) { // TODO: Neden last predictor fieldi gerekmedi huh ? 
		int depth = std::min(_dpt_depth-1, dhb_p->used-2);
		int32_t actual = dhb_p->delta[0], prev_delta[_num_delta];
		for(int i = 0; i < _dpt_depth-1; i++) {
			prev_delta[i] = dhb_p->delta[i+1];
		}
		for(int i = depth; i >= 0; i--) {
			int32_t dpt_ind = _dpt[i].has(prev_delta, i+1);
			DPT::Entry* dpt_p = NULL;
			if(dpt_ind != -1) {
				dpt_p = _dpt[i].get(dpt_ind);
				if(dpt_p->pred == actual) {
					if(!dpt_p->acc[0]) dpt_p->acc[0] = true;
					else if(!dpt_p->acc[1]) {
						dpt_p->acc[1] = true;
						dpt_p->acc[0] = false;
					}
				} else {
					if(dpt_p->acc[0]) dpt_p->acc[0] = false;
					else if(dpt_p->acc[1]) {
						dpt_p->acc[1] = false;
						dpt_p->acc[0] = true;
					} else {
						dpt_p->pred = actual;
					}
				}
			} else {
				dpt_ind = _dpt[i].evict();
				dpt_p = _dpt[i].get(dpt_ind);
				dpt_p->pred = actual;
				dpt_p->acc[0] = false;
				dpt_p->acc[1] = false;
				for(int j = 0; j < _dpt_depth-1; j++)
					dpt_p->delta[j] = prev_delta[j];
			}
			dpt_p->MRU = true;
			_dpt[i].refresh_MRU();
		}
	}

	u_int32_t _pred;
	if(dhb_p->used == 1 /*&& opt_p->acc*/) {
		_ready = true;
		_pred = opt_p->pred + (int32_t)(dhb_p->addr);
	}
	if(dhb_p->used > 1) { 
		int depth = std::min(_dpt_depth-1, dhb_p->used-2);
		for(int i = depth; i >= 0; i--) {
			int32_t dpt_ind = _dpt[i].has(dhb_p->delta, i+1);
			if(dpt_ind == -1) continue;
			DPT::Entry* dpt_p = _dpt[i].get(dpt_ind);
			/* if(dpt_p->acc[1] == false && dpt_p->acc[0] == false) continue; */
			_ready = true;
			_pred = (int32_t)(dhb_p->addr) + dpt_p->pred;
			dhb_p->last_predictor = i;
			break;
		}
	}

	if(_ready) {
		_add((((dhb_p->page)<<_p)|_pred)<<_b);
		for(int i = _num_delta-1; i > 0; i--)
			dhb_p->offset[i] = dhb_p->offset[i-1];
		dhb_p->offset[0] = _pred;
		dhb_p->predicted = true;
		_ready = false;
	}
}
