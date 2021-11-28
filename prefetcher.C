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
		_buffer[i].num_preds = 0;
		_buffer[i].MRU = false;
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
	Entry *dhb_p = get(ind);

	/* printf("#BEFORE dhb_p(%d) = page=%x, addr=%x, (num_preds=%d), last_predictor=%d, used=%d, MRU=%d, delta=[%d, %d, %d, %d, %d], offset=[%x, %x, %x, %x, %x]\n", ind, dhb_p->page, dhb_p->addr, dhb_p->num_preds, dhb_p->last_predictor, dhb_p->used, dhb_p->MRU, dhb_p->delta[0], dhb_p->delta[1], dhb_p->delta[2], dhb_p->delta[3], dhb_p->delta[4], dhb_p->offset[0], dhb_p->offset[1], dhb_p->offset[2], dhb_p->offset[3], dhb_p->offset[4]); */

	int32_t curr_delta = offset - dhb_p->addr;
	for(int i = _num_delta-1; i > 0 ; i--)
		dhb_p->delta[i] = dhb_p->delta[i-1];
	dhb_p->delta[0] = curr_delta;
	dhb_p->addr = offset; //?
	dhb_p->used++;
	dhb_p->MRU = true;
	refresh_MRU();

	/* printf("#AFTER dhb_p(%d) = page=%x, addr=%x, (num_preds=%d), last_predictor=%d, used=%d, MRU=%d, delta=[%d, %d, %d, %d, %d], offset=[%x, %x, %x, %x, %x]\n", ind, dhb_p->page, dhb_p->addr, dhb_p->num_preds, dhb_p->last_predictor, dhb_p->used, dhb_p->MRU, dhb_p->delta[0], dhb_p->delta[1], dhb_p->delta[2], dhb_p->delta[3], dhb_p->delta[4], dhb_p->offset[0], dhb_p->offset[1], dhb_p->offset[2], dhb_p->offset[3], dhb_p->offset[4]); */
}

void DHB::set(int32_t ind, int32_t page, int32_t offset) {
	Entry *dhb_p = get(ind);
	dhb_p->page = page;
	dhb_p->addr = offset;
	dhb_p->used = 1;
	dhb_p->MRU = true;
	dhb_p->num_preds = 0;
	dhb_p->last_predictor = -1;
	refresh_MRU();
	/* printf("#AFTER dhb_p(%d) = page=%x, addr=%x, (num_preds=%d), last_predictor=%d, used=%d, MRU=%d, delta=[%d, %d, %d, %d, %d], offset=[%x, %x, %x, %x, %x]\n", ind, dhb_p->page, dhb_p->addr, dhb_p->num_preds, dhb_p->last_predictor, dhb_p->used, dhb_p->MRU, dhb_p->delta[0], dhb_p->delta[1], dhb_p->delta[2], dhb_p->delta[3], dhb_p->delta[4], dhb_p->offset[0], dhb_p->offset[1], dhb_p->offset[2], dhb_p->offset[3], dhb_p->offset[4]); */
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
		/* if(_buffer[ind].num_preds>0 && _buffer[ind].offset[0] == offset) */
		if(_buffer[ind].last_predictor != -1 && _buffer[ind].offset[0] == offset)
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

OPT::Entry* OPT::get(int32_t ind) {
	/* assert(_offset(addr)>=0 && _offset(addr)<(1<<(_p+_b))); */
	/* return &(_buffer[_offset(addr)>>_b]); */
	return &(_buffer[ind]);
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

void Prefetcher::_add(int32_t dhb_ind, int32_t addr, bool should_add) {
	if(should_add) { 
		assert(dhb_ind>=0 && dhb_ind<_dhb_capacity);
		DHB::Entry *dhb_p = _dhb.get(dhb_ind);
		for(int i = _num_delta-1; i > 0; i--)
			dhb_p->offset[i] = dhb_p->offset[i-1];
		dhb_p->offset[0] = addr-(dhb_p->page);
		/* dhb_p->num_preds++; */
	}

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

void Prefetcher::_update_dpt(int32_t dhb_ind) {
	DHB::Entry* dhb_p = _dhb.get(dhb_ind);
	if(dhb_p->used < 2) return;
	int depth = std::min(_dpt_depth-1, dhb_p->used-2);
	int32_t actual = dhb_p->delta[0], prev_delta[_num_delta];
	for(int i = 0; i < _dpt_depth - 1; i++) {
		prev_delta[i] = dhb_p->delta[i+1];
	}
	for(int i = depth; i >= 0; i--) {
		int32_t dpt_ind = _dpt[i].has(prev_delta, i+1);
		DPT::Entry* dpt_p = NULL;
		if(dpt_ind != -1) {
			dpt_p = _dpt[i].get(dpt_ind);
			/* printf("#BEFORE dpt_p(%d/%d): pred=%d, acc[0]=%d, acc[1]=%d, MRU=%d, delta={%d, %d, %d, %d}\n", i, dpt_ind, dpt_p->pred, dpt_p->acc[0], dpt_p->acc[1], dpt_p->MRU, dpt_p->delta[0], dpt_p->delta[1], dpt_p->delta[2], dpt_p->delta[3]); */
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
			/* printf("#BEFORE dpt_p(%d/%d): pred=%d, acc[0]=%d, acc[1]=%d, MRU=%d, delta={%d, %d, %d, %d}\n", i, dpt_ind, dpt_p->pred, dpt_p->acc[0], dpt_p->acc[1], dpt_p->MRU, dpt_p->delta[0], dpt_p->delta[1], dpt_p->delta[2], dpt_p->delta[2]); */
			dpt_p->pred = actual;
			dpt_p->acc[0] = false;
			dpt_p->acc[1] = false;
			for(int j = 0; j <= i; j++)
				dpt_p->delta[j] = prev_delta[j];
		}
		dpt_p->MRU = true;
		_dpt[i].refresh_MRU();
		/* printf("#AFTER dpt_p(%d/%d): pred=%d, acc[0]=%d, acc[1]=%d, MRU=%d, delta={%d, %d, %d, %d}\n", i, dpt_ind, dpt_p->pred, dpt_p->acc[0], dpt_p->acc[1], dpt_p->MRU, dpt_p->delta[0], dpt_p->delta[1], dpt_p->delta[2], dpt_p->delta[2]); */
	}
}

void Prefetcher::_update_opt(int32_t dhb_ind) {
	DHB::Entry* dhb_p = _dhb.get(dhb_ind);
	/* return &(_buffer[_offset(addr)>>_b]); */
	if(dhb_p->used != 2) return;
	int32_t offset = dhb_p->addr - dhb_p->delta[0];
	OPT::Entry* opt_p = _opt.get(offset>>_b);
	if((opt_p->pred) == (dhb_p->delta[0])) {
		printf("#OPTUSED HIT, ISTAKEN=%d!\n", opt_p->acc);
		opt_p->acc = true;
		return;
	}
	printf("#OPTUSED MISS, ISTAKEN=%d!\n", opt_p->acc);
	if(!opt_p->acc) opt_p->pred = dhb_p->delta[0];
	opt_p->acc = false;
}

void Prefetcher::_check_prediction(int32_t dhb_ind, bool load, bool success) {
	DHB::Entry* dhb_p = _dhb.get(dhb_ind);
	if(dhb_p->used < 2) return;
	int depth = std::min(_dpt_depth-1, dhb_p->used-2);
	if(!success) {
		for(int i = depth; i >= 0; i--) {
			int32_t delta[_num_delta];
			memcpy(delta, dhb_p->delta, sizeof(delta));
			int32_t dpt_ind = _dpt[i].has(delta, i+1);
			if(dpt_ind == -1) continue;
			int32_t inds[_degree];
			int32_t preds[_degree];
			int32_t num_preds = 0;
			int32_t _pred = (int32_t)(dhb_p->page) + (int32_t)(dhb_p->addr);
			for(int j = 0; j < _degree; j++ ) {
				DPT::Entry* dpt_p = _dpt[i].get(dpt_ind);
				/* if(dpt_p->acc[1] == false && dpt_p->acc[0] == false) continue; */
				/* if(dpt_p->pred == 0) { */
				/* 	printf("#ZEROPRED IS MADE\n"); */
				/* 	printf("load=%d\n", load); */
				/* 	#<{(| if(load) break; |)}># */
				/* } */
				_pred += dpt_p->pred;
				printf("_pred=%x\n",_pred);
				inds[j] = dhb_ind;
				preds[j] = _pred;
				printf("#USED dpt_p(%d/%d): pred=%d, acc[0]=%d, acc[1]=%d, MRU=%d, delta={%d, %d, %d, %d}\n", i, dpt_ind, dpt_p->pred, dpt_p->acc[0], dpt_p->acc[1], dpt_p->MRU, dpt_p->delta[0], dpt_p->delta[1], dpt_p->delta[2], dpt_p->delta[2]);
				dhb_p->last_predictor = i;

				for(int k = 0; k < _num_delta-1; k++)
					delta[k+1] = delta[k];
				delta[0] = dpt_p->pred;
				dpt_ind = _dpt[i].has(delta, i+1);
				num_preds++;
				if(dpt_ind == -1) break;
			}
			dhb_p->num_preds = num_preds;
			for(int j = num_preds-1; j >= 0; j--)
				_add(inds[j], preds[j], true);
			break;
		}
	} else {
		int32_t delta[_num_delta];
		int j = 0;
		printf("BURAYI KULLANIYOM\n");
		for(int i = dhb_p->num_preds-1; i>0 && j < _num_delta ; i--, j++)
			delta[j] = dhb_p->offset[i]-dhb_p->offset[i-1];
		for(int i = 0; i<_num_delta && j < _num_delta; i++, j++)
			delta[j] = dhb_p->delta[i];
		/* memcpy(delta, dhb_p->delta, sizeof(delta)); */
		for(int i = depth; i >= 0; i--) {
			int32_t dpt_ind = _dpt[i].has(delta, i+1);
			if(dpt_ind == -1) continue;
			DPT::Entry* dpt_p = _dpt[i].get(dpt_ind);
			/* if(dpt_p->acc[1] == false && dpt_p->acc[0] == false) continue; */
			/* if(dpt_p->pred == 0) { */
			/* 	printf("#ZEROPRED IS MADE\n"); */
			/* 	printf("load=%d\n", load); */
			/* 	#<{(| if(load) break; |)}># */
			/* } */
			int32_t _pred = (int32_t)(dhb_p->page) + (int32_t)(dhb_p->offset[dhb_p->num_preds-1]) + dpt_p->pred;
			printf("_pred=%x\n",_pred);
			_add(dhb_ind, _pred, false);
			assert(dhb_ind>=0 && dhb_ind<_dhb_capacity);
			DHB::Entry *dhb_p = _dhb.get(dhb_ind);
			for(int i = 0; i < dhb_p->num_preds-1; i++)
				dhb_p->offset[i] = dhb_p->offset[i+1];
			dhb_p->offset[dhb_p->num_preds-1] = _pred-(dhb_p->page);

			/* printf("#USED dpt_p(%d/%d): pred=%d, acc[0]=%d, acc[1]=%d, MRU=%d, delta={%d, %d, %d, %d}\n", i, dpt_ind, dpt_p->pred, dpt_p->acc[0], dpt_p->acc[1], dpt_p->MRU, dpt_p->delta[0], dpt_p->delta[1], dpt_p->delta[2], dpt_p->delta[3]); */
			dhb_p->last_predictor = i;
			/* printf("for bitti\n"); */
			break;
		}
	}
}

void Prefetcher::cpuRequest(Request req) { 
	printf("cpuRequest = req.addr=%x, req.pc=%x, req.load=%d, req.fromCPU=%d, req.issuedAt=%d, req.HitL1=%d, req.HitL2=%d\n",req.addr, req.pc, req.load, req.fromCPU, req.issuedAt, req.HitL1, req.HitL2);
	// Determine PAE
	/* if(!req.load) return; */
	if(!req.load && !req.HitL1) return;
	bool pae = false;
	DHB::Entry* dhb_p = NULL;
	bool success = false;

	for(int dhb_ind = 0; dhb_ind < _dhb_capacity; dhb_ind++) {
		pae = false;
		if(_dhb.is_hit(dhb_ind, req.addr)) pae = true;
		if(!pae) continue;
		// Accurate prediction

		dhb_p = _dhb.get(dhb_ind);
		int32_t page = dhb_p->page;
		if(page == _page(req.addr)) success = true;
		int32_t offset = ((req.addr-page)&~((1<<_b)-1));

		_dhb.add(dhb_ind, page, offset);
		_update_opt(dhb_ind);
		_update_dpt(dhb_ind);
		// Succesfull prediction, only fetch one
		printf("I AM SUCCESFULL\n");
		_check_prediction(dhb_ind, req.load, true);
	}

	if (req.HitL1) return;
	
	// L1 miss
	int dhb_ind = _dhb.has(req.addr);
	if(dhb_ind == -1) {
		// New dhb entry
		dhb_ind = _dhb.evict();
		_dhb.set(dhb_ind, _page(req.addr), _offset(req.addr));

		// Try to predict from OPT
		OPT::Entry* opt_p = _opt.get(_offset(req.addr)>>_b);
		/* if(opt_p->acc) { */
			int32_t _pred = opt_p->pred + req.addr;
			printf("#OPTUSED _offset(req.addr)=%x, opt_p->pred=%d, _pred=%x\n", _offset(req.addr), opt_p->pred, _pred);
			_add(dhb_ind, _pred, true); 
		/* } */
		return;
	} 
	if(success) return;
	printf("I AM UNSUCCESFULL\n");
	
	dhb_p = _dhb.get(dhb_ind);
	_dhb.add(dhb_ind, _page(req.addr), _offset(req.addr));
	_update_opt(dhb_ind);
	_update_dpt(dhb_ind);
	// Unsuccesfull prediction predict degree
	_check_prediction(dhb_ind, req.load, false);
}
