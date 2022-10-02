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

#include "common/span.h"

// Call pointer to member function.
// See <https://isocpp.org/wiki/faq/pointers-to-members>
#define CALL_MEMBER_FN(object, fn) ((object).*(fn))

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
Common::Span<T> spanOf(Common::Array<T>& array) {
	return Common::Span<T>(array.data(), array.size());
}

template<class T>
Common::Span<const T> spanOf(const Common::Array<T>& array) {
	return Common::Span<const T>(array.data(), array.size());
}

} // End of namespace Funhouse

#endif
