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

int32_t _page(int32_t addr) {
	return (addr&~((1<<(_b+_p))-1));
}

int32_t _offset(int32_t addr) {
	return ((addr-_page(addr))&~((1<<_b)-1));
}

DHB::DHB() {
	for(int i = 0; i < _dhb_capacity; i++) {
		_buffer[i].page = 0;
		_buffer[i].addr = 0;
		_buffer[i].last_predictor = 0;
		_buffer[i].used = 0;
		_buffer[i].MRU = false;
		_buffer[i].predicted = false;
		memset(_buffer[i].delta, 0, _num_delta*sizeof(int32_t));
		memset(_buffer[i].offset, 0, _num_delta*sizeof(int32_t));
	}
}

int DHB::has(int32_t addr) {
	int32_t page = _page(addr);
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

void DHB::add(int32_t ind, int32_t page, int32_t offset) {
	Entry *dhb_p = _dhb.get(dhb_ind);
	int32_t curr_delta = offset - dhb_p->offset;
	for(int i = _num_delta-1; i > 0 ; i--)
		dhb_p->delta[i] = dhb_p->delta[i-1];
	dhb_p->delta[0] = curr_delta;
	dhb_p->addr = offset; //?
	dhb_p->used++;
	dhb_p->MRU = true;
	refresh_MRU();
}

void DHB::set(int32_t ind, int32_t page, int32_t offset) {
	Entry *dhb_p = _dhb.get(dhb_ind);
	dhb_p->page = page;
	dhb_p->addr = offset;
	dhb_p->used = 1;
	dhb_p->MRU = true;
	dhb_p->last_predictor = -1;
	dhb_p->predicted = false;
	refresh_MRU();
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

bool DHB::is_hit(int32_t ind, int32_t addr) {
	int32_t offset = ((addr-_buffer[ind].page)&~((1<<_b)-1));
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
		memset(_buffer[i].delta, 0, _num_delta*sizeof(int32_t));
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

OPT::Entry* OPT::get(int32_t addr) {
	assert(_offset(addr)>=0 && _offset(addr)<(1<<(_p+_b)));
	return &(_buffer[_offset(addr)>>_b]);
}

Prefetcher::Prefetcher() { 
	_ready = false; 
	_capacity = _prefetch_capacity;

	_front = 0;
	_rear = 0;
	_size = 0;
}

bool Prefetcher::hasRequest(int32_t cycle) { 
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

Request Prefetcher::getRequest(int32_t cycle) { return _nextReq; }

void Prefetcher::completeRequest(int32_t cycle) { _ready = false; }

void Prefetcher::_add(int32_t addr) {
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

void _update_dpt(int32_t dhb_ind) {
	DHB::Entry* dhb_p = _dhb.get(dhb_ind);

}

void Prefetcher::cpuRequest(Request req) { 
	printf("cpuRequest = req.addr=%x, req.pc=%x, req.load=%d, req.fromCPU=%d, req.issuedAt=%d, req.HitL1=%d, req.HitL2=%d\n",req.addr, req.pc, req.load, req.fromCPU, req.issuedAt, req.HitL1, req.HitL2);
	// Determine PAE
	bool pae = false;
	DHB::Entry* dhb_p = NULL;

	for(int dhb_ind = 0; dhb_ind < _dhb_capacity; _dhb_ind++) {
		pae = false;
		if(_dhb.is_hit(dhb_ind, req.addr)) pae = true;
		if(!pae) continue;
		// Accurate prediction

		DHB::Entry *dhb_p = _dhb.get(dhb_ind);
		int32_t page = dhb_p->page;
		int32_t offset = ((req.addr-page)&~((1<<_b)-1));

		_dhb.add(page, offset);
		_update_dpt(dhb_ind);
		_check_prediction(dhb_ind);
	}

	if (req.HitL1) return;
	
	// L1 miss
	int dhb_ind = _dhb.has(req.addr);
	if(dhb_ind == -1) {
		// New dhb entry
		dhb_ind = _dhb.evict();
		_dhb.set(_page(req.addr), _offset(req.addr));

		// Try to predict from OPT
		OPT::Entry* opt_p = _opt.get(req.addr);
		if(opt_p->acc) {
			_pred = opt_p->pred + (int32_t)_offset(req.addr);
			_add(_pred); 
		}
		return;
	} 
	if(_dhb.is_hit(dhb_ind, req.addr)) return;
	
	DHB::Entry *dhb_p = _dhb.get(dhb_ind);
	_dhb.add(_page(req.addr), _offset(req.addr));
	_update_dpt(dhb_ind);
	_check_prediction(dhb_ind);
}
