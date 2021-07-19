/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef FUNHOUSE_UTIL_H
#define FUNHOUSE_UTIL_H

#include "common/endian.h"
#include "common/queue.h"
#include "common/span.h"
#include "common/textconsole.h"

namespace Funhouse {

template<class T>
class ScopedArray {
public:
	struct Movable
	{
		friend class ScopedArray;
	public:
		Movable() : data(nullptr), size(0) { }
	private:
		T* data;
		uint size;
	};

	explicit ScopedArray(Movable o = Movable())
		: _internal(o)
	{ }

	~ScopedArray() {
		delete[] _internal.data;
		_internal.data = nullptr;
	}

	operator bool() const {
		return _internal.data;
	}

	uint size() const {
		return _internal.size;
	}

	void reset(Movable o = Movable()) {
		if (o.data != _internal.data) {
			delete[] _internal.data;
			_internal = o;
		}
	}

	void alloc(uint arraySize) {
		delete[] _internal.data;
		_internal.data = nullptr;
		_internal.size = arraySize;
		if (arraySize > 0) {
			_internal.data = new T[arraySize];
		}
	}

	T& operator[](uint idx) {
		assert(idx < _internal.size);
		return _internal.data[idx];
	}

	const T& operator[](uint idx) const {
		assert(idx < _internal.size);
		return _internal.data[idx];
	}

	Movable release() {
		Movable result = _internal;
		_internal = Movable();
		return result;
	}

	Movable clone() const {
		Movable result;
		result.data = new T[_internal.size];
		result.size = _internal.size;

		for (uint i = 0; i < _internal.size; ++i) {
			result.data[i] = _internal.data[i];
		}

		return result;
	}

	Common::Span<const byte> span() const {
		return Common::Span<const byte>(_internal.data, _internal.size);
	}

private:
	// Prevent accidentally copying a ScopedArray. To clone a ScopedArray, use
	// the clone method.
	// XXX: Class is made noncopyable here (instead of inheriting from
	// Common::NonCopyable) because VS2015 emits very unhelpful error messages.
	ScopedArray(const ScopedArray&);
	ScopedArray& operator=(const ScopedArray&);

	Movable _internal;
};

template<class T>
class ScopedArrayQueue
{
public:
	ScopedArrayQueue() { }
	~ScopedArrayQueue() {
		// Correctly delete all contents
		while (!_queue.empty()) {
			ScopedArray<T> item(_queue.pop());
			item.reset();
		}
	}

	bool empty() const {
		return _queue.empty();
	}

	void clear() {
		_queue.clear();
	}

	void push(typename ScopedArray<T>::Movable item) {
		_queue.push(item);
	}

	typename ScopedArray<T>::Movable pop() {
		return _queue.pop();
	}

private:
	ScopedArrayQueue(const ScopedArrayQueue&);
	ScopedArrayQueue& operator=(const ScopedArrayQueue&);

	Common::Queue<typename ScopedArray<T>::Movable> _queue;
};

} // End of namespace Funhouse

#endif
