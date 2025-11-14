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

// ac_shared_bank_array.h
// Stuart Swan, Platform Architect, Siemens EDA
// 11 April 2024
//
// ac_shared_bank_array classes support banked memory modeling in HLS.
//
// Consider the following memory in an HLS model:
//   uint32 mem[15][333];
// In the C language, mem occupies a contiguous region of memory.
// For HW implementation thru HLS,
// typically we would want 15 banks of HW RAMs, each containing 333 elements.
// The ac_shared_bank_array classes are "plug compatible" with normal C arrays, but explicitly
// make each bank separate C arrays so that HLS sees the proper HW structure and synthesizes
// more quickly and to more efficient HW.

#ifndef __AC_SHARED_BANK_ARRAY_H
#define __AC_SHARED_BANK_ARRAY_H

#include <cstddef>
#include <ac_assert.h>
#include <ac_shared_proxy.h>

#include <ac_max_pow2.h>
#include <ac_array_1D.h>

//==========================================================================

// ac_shared_bank_array_base is the base class for banked arrays,
// and typically is not directly used in user models.

template <typename B, size_t C, size_t I = 0>
class ac_shared_bank_array_base;

template <typename B, size_t I>
class ac_shared_bank_array_base<B, 1, I>
{
public:
  #pragma hls_ac_bank_mem
  B a;
public:
  B &operator[](size_t idx) { return a; }
  const B &operator[](size_t idx) const { return a; }
};

// specialization to get index checking of rightmost dimension using ac_array_1D
template <typename E, size_t D, size_t I>
class ac_shared_bank_array_base<E [D], 1, I>
{
public:
  typedef ac_array_1D<E,D> AC;
  #pragma hls_ac_bank_mem
  ac_shared<AC> a;
public:
  AC &operator[](size_t idx) { return a; }
  const AC &operator[](size_t idx) const { return a; }
};

template <typename B, size_t C, size_t I>
class ac_shared_bank_array_base
{
  static const size_t W = ac_max_pow2<C-1>::P;
  ac_shared_bank_array_base<B, W  , I> a0;
  ac_shared_bank_array_base<B, C-W, I+W> a1;
public:
  B &operator[](size_t idx) {
    AC_A_BANK_ARRAY_ASSERTION(idx < C);
    size_t aidx = idx & (W-1);
    return idx&W ? a1[aidx] : a0[aidx];
  }

  const B &operator[](size_t idx) const {
    AC_A_BANK_ARRAY_ASSERTION(idx < C);
    size_t aidx = idx & (W-1);
    return idx&W ? a1[aidx] : a0[aidx];
  }
};

// specialization to get index checking of rightmost dimension using ac_array
template <typename E, size_t D, size_t C, size_t I>
class ac_shared_bank_array_base<E [D], C, I>
{
  typedef ac_array_1D<E,D> AC;
  typedef E B[D];
  static const size_t W = ac_max_pow2<C-1>::P;
  ac_shared_bank_array_base<B, W  , I> a0;
  ac_shared_bank_array_base<B, C-W, I+W> a1;
public:
  AC &operator[](size_t idx) {
    AC_A_BANK_ARRAY_ASSERTION(idx < C);
    size_t aidx = idx & (W-1);
    return idx&W ? a1[aidx] : a0[aidx];
  }

  const AC &operator[](size_t idx) const {
    AC_A_BANK_ARRAY_ASSERTION(idx < C);
    size_t aidx = idx & (W-1);
    return idx&W ? a1[aidx] : a0[aidx];
  }
};


// ac_shared_bank_array_vary<> supports variable number of array dimensions for a banked array
//
// T can be any type: for example:  int, int [0x1000], int [0x10][0x1000]
// Most typically T would be a single dimensional C array type (e.g. int [0x1000])
// However, a wide variety of structures can be modeled, for example:
//  ac_shared_bank_array_vary<int[7][9], 5>  // 5 banks of 63 element 2D arrays
//  ac_shared_bank_array_vary<int[9], 5, 7>  // 35 banks of 9 element 1D arrays
//  ac_shared_bank_array_vary<int, 5, 7, 9>  // 315 separate registers

// variadic templates for ac_shared_bank_array_vary<> requires -std=c++11 or greater.
// TODO : add a compiler language version check here to error out if not c++11 at least.


template<typename T, size_t FirstDim, size_t... RestDims>
class ac_shared_bank_array_vary :
  public ac_shared_bank_array_base<ac_shared_bank_array_vary<T, RestDims...>, FirstDim>
{
};

template<typename T, size_t FirstDim>
class ac_shared_bank_array_vary<T, FirstDim> :
  public ac_shared_bank_array_base<T, FirstDim>
{
};

// ac_shared_bank_array_2D supports common 2D array case where FirstDim is the number of banks
// and SecondDim is the number of elements in each bank.
// T is typically a simple scalar type, e.g. uint64

template <typename T, size_t FirstDim, size_t SecondDim>
class ac_shared_bank_array_2D : public ac_shared_bank_array_base<T [SecondDim], FirstDim>
{
};

// ac_shared_bank_array_3D supports common 3D array case where FirstDim and SecondDim determine
// the number of banks and ThirdDim is the number of elements in each bank.
// T is typically a simple scalar type, e.g. uint64

template <typename T, size_t FirstDim, size_t SecondDim, size_t ThirdDim>
class ac_shared_bank_array_3D : public ac_shared_bank_array_base<ac_shared_bank_array_base<T [ThirdDim], SecondDim>, FirstDim>
{
};

#endif

