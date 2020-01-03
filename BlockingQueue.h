/*
 * Copyright 2016-2020 Grok Image Compression Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>


template<typename Data> class BlockingQueue
{
public:
	BlockingQueue() : _active(true) {}
	// deactivate and clear queue
	void deactivate() {
		std::lock_guard<std::mutex> lk(_mutex);
		_active = false;
		while (_queue.size())
			_queue.pop();
		//release all waiting threads
		_condition.notify_all();
	}
	void activate() {
		std::lock_guard<std::mutex> lk(_mutex);
		_active = true;
	}
	void push(Data const& data)	{
		std::lock_guard<std::mutex> lk(_mutex);
		if (!_active)
			return;
		_queue.push(data);
		_condition.notify_one();
	}
	bool tryPop(Data& value)	{
		std::lock_guard<std::mutex> lk(_mutex);
		return pop(value);
	}
	bool waitAndPop(Data& value)	{
		std::unique_lock<std::mutex> lk(_mutex);
		// in case of spurious wakeup, loop until predicate in lambda
		// is satisfied.
		_condition.wait(lk, [this]{ return !_active || !_queue.empty(); });
		return pop(value);
	}
private:
	bool pop(Data& value) {
		if (_queue.empty())
			return false;
		value = _queue.front();
		_queue.pop();
		return true;
	}
	std::queue<Data> _queue;
	mutable std::mutex _mutex;
	std::condition_variable _condition;
	bool _active;
};

