/*

   Copyright (c) 2006-2010, The Scripps Research Institute

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Author: Dr. Oleg Trott <ot14@columbia.edu>, 
           The Olson Lab, 
           The Scripps Research Institute

*/

#ifndef VINA_PARALLEL_PROGRESS_H
#define VINA_PARALLEL_PROGRESS_H

#include <iostream>
#include <boost/thread/mutex.hpp>

#include "incrementable.h"

// Simple progress display (replaces deprecated boost::progress_display)
class simple_progress_display {
	static constexpr unsigned long PROGRESS_BAR_WIDTH = 50;
	static constexpr double PROGRESS_BAR_WIDTH_DOUBLE = 50.0;
	
	unsigned long _count;
	unsigned long _expected_count;
	unsigned long _next_tic_count;
	unsigned int _tic;
public:
	explicit simple_progress_display(unsigned long expected_count)
		: _count(0), _expected_count(expected_count), 
		  _next_tic_count(0), _tic(0) {
		if(_expected_count > 0) {
			std::cout << "0%   10   20   30   40   50   60   70   80   90   100%" << std::endl;
			std::cout << "|----|----|----|----|----|----|----|----|----|----| " << std::flush;
		}
	}
	
	void operator++() {
		if(_expected_count == 0) return;
		if(++_count >= _next_tic_count) {
			display_tic();
		}
	}
	
	~simple_progress_display() {
		if(_expected_count > 0 && _count < _expected_count) {
			while(_count < _expected_count) {
				if(_count >= _next_tic_count) display_tic();
				++_count;
			}
		}
		if(_expected_count > 0) {
			std::cout << std::endl;
		}
	}
	
private:
	void display_tic() {
		unsigned int tics_needed = static_cast<unsigned int>(
			(static_cast<double>(_count) / _expected_count) * PROGRESS_BAR_WIDTH_DOUBLE);
		while(_tic < tics_needed) {
			std::cout << '*' << std::flush;
			_next_tic_count = static_cast<unsigned long>(
				(static_cast<double>(++_tic) / PROGRESS_BAR_WIDTH_DOUBLE) * _expected_count);
		}
	}
};

struct parallel_progress : public incrementable {
	parallel_progress() : p(NULL) {}
	void init(unsigned long n) { p = new simple_progress_display(n); }
	void operator++() {
		if(p) {
			boost::mutex::scoped_lock self_lk(self);
			++(*p);
		}
	}
	virtual ~parallel_progress() { delete p; }
private:
	boost::mutex self;
	simple_progress_display* p;
};

#endif

