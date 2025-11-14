/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Datatypes                                          *
 *                                                                        *
 *  Software Version: 2025.4                                              *
 *                                                                        *
 *  Release Date    : Tue Nov 11 17:37:52 PST 2025                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 2025.4.0                                            *
 *                                                                        *
 *  Copyright 2024 Siemens                                                *
 *                                                                        *
 *                                                                        *
 *                                                                        *
 **************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
 *                                                                        *
 *      http://www.apache.org/licenses/LICENSE-2.0                        *
 *                                                                        *
 *  Unless required by applicable law or agreed to in writing, software   *
 *  distributed under the License is distributed on an "AS IS" BASIS,     *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or       *
 *  implied.                                                              *
 *  See the License for the specific language governing permissions and   *
 *  limitations under the License.                                        *
 **************************************************************************
 *                                                                        *
 *  The most recent version of this package is available at github.       *
 *                                                                        *
 *************************************************************************/

// ac_array_subrange.h
// Stuart Swan, Platform Architect, Siemens EDA
// 11 Feb 2025
//
// ac_array_subrange allows subranges to be constructed from other ac*array classes.
// These subranges can be used in HLS models as if they were self-contained arrays,
// and in particular pre-HLS and post-HLS assertions are created to check that all indexes used
// to access the array are in the specified subrange.
//
// This class works in both the Catapult SystemC flow and the C++ flow.

#ifndef __AC_ARRAY_SUBRANGE_H
#define __AC_ARRAY_SUBRANGE_H

#include <cstddef>
#include <ac_assert.h>

#include <ac_wr_mask_array.h>

//==========================================================================

template <class B>
class ac_array_subrange
{
public:
  B &array;
  size_t offset; // offset of this subrange wrt base array
  size_t size;   // size of this subrange
  typedef decltype(array[0]) T;

  ac_array_subrange(B &_array, size_t _offset, size_t _size)
    : array(_array), offset(_offset), size(_size)
  {}

  T &operator[](size_t idx) {
    AC_A_BANK_ARRAY_ASSERTION(idx < size);
    return array[idx + offset];
  }

  const T &operator[](size_t idx) const {
    AC_A_BANK_ARRAY_ASSERTION(idx < size);
    return array[idx + offset];
  }
};

template <typename T, int Dim1, int SliceWidth, bool use_be_ram>
class ac_array_subrange<ac_wr_mask_array_1D<T, Dim1, SliceWidth, use_be_ram>>
{
public:
  typedef ac_wr_mask_array_1D<T, Dim1, SliceWidth, use_be_ram> array_t;
  typedef typename array_t::WriteMask mask_t;
  array_t &array;
  size_t offset; // offset of this subrange wrt base array
  size_t size;   // size of this subrange

  ac_array_subrange(array_t &_array, size_t _offset, size_t _size)
    : array(_array), offset(_offset), size(_size)
  {}

  void write(unsigned idx, T val, mask_t mask_val=~0) {
    AC_A_BANK_ARRAY_ASSERTION(idx < size);
    array.write(idx + offset, val, mask_val);
  }

  T read(unsigned idx) {
    AC_A_BANK_ARRAY_ASSERTION(idx < size);
    return array.read(idx + offset);
  }
};


// This create function is used to enable template argument deduction
template <class B>
ac_array_subrange<B> create_ac_array_subrange(B &array, size_t offset, size_t size)
{
  return ac_array_subrange<B>(array, offset, size);
}

#endif

