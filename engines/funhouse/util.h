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

/// Generate a sequence of shuffled numbers. Optionally, a deviance factor can be specified,
/// and a list of numbers to exclude can be given.
/// If an exclusion list is passed in, this function first generates a shuffled sequence
/// that excludes the specified numbers. If there is space remaining in the
/// output buffer, the excluded numbers are shuffled and placed in the rest of the
/// output buffer. If there is still space remaining in the output buffer,
/// the function continues generating shuffled sequences (ignoring exclusions) until no more
/// space remains.
void makeShuffledSequence(int count, Common::Span<int> out, int deviance = 0, int previous = -1, Common::Span<bool> placed = {});

template<class T>
class ScopedArray {
public:
	ScopedArray() = default;

	ScopedArray(std::nullptr_t) {
	}

	ScopedArray(const ScopedArray &) = delete;
	ScopedArray &operator=(const ScopedArray &) = delete;

	ScopedArray(ScopedArray&& other) {
		*this = std::move(other); // Use move assignment operator
	}

	ScopedArray& operator=(ScopedArray&& other) {
		std::swap(_data, other._data);
		std::swap(_size, other._size);
		return *this;
	}

	~ScopedArray() {
		reset();
	}

	operator bool() const {
		return _data;
	}

	uint size() const {
		return _size;
	}

	void reset() {
		delete[] _data;
		_data = nullptr;
		_size = 0;
	}

	void alloc(uint arraySize) {
		delete[] _data;
		_data = nullptr;
		_size = arraySize;
		if (arraySize > 0) {
			_data = new T[arraySize];
		}
	}

	T& operator[](uint idx) {
		assert(idx < _size);
		return _data[idx];
	}

	const T& operator[](uint idx) const {
		assert(idx < _size);
		return _data[idx];
	}

	ScopedArray clone() const {
		ScopedArray result;
		result.alloc(_size);
		for (uint i = 0; i < _size; ++i) {
			result._data[i] = _data[i];
		}
		return result;
	}

	T* begin() { return &_data[0]; }
	T* end() { return &_data[_size]; }
	const T *begin() const { return &_data[0]; }
	const T *end() const { return &_data[_size]; }

	Common::Span<T> span() {
		return Common::Span<T>(_data, _size);
	}

	Common::Span<const T> span() const {
		return Common::Span<const T>(_data, _size);
	}

private:
	T *_data = nullptr;
	uint _size = 0;
};

} // End of namespace Funhouse

#endif
