/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Datatypes                                          *
 *                                                                        *
 *  Software Version: 5.1                                                 *
 *                                                                        *
 *  Release Date    : Tue May 13 15:28:19 PDT 2025                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 5.1.1                                               *
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

// ac_bank_array.h
// Stuart Swan, Platform Architect, Siemens EDA
// 11 April 2024
//
// ac_bank_array classes support banked memory modeling in HLS.
//
// Consider the following memory in an HLS model:
//   uint32 mem[15][333];
// In the C language, mem occupies a contiguous region of memory.
// For HW implementation thru HLS,
// typically we would want 15 banks of HW RAMs, each containing 333 elements.
// The ac_bank_array classes are "plug compatible" with normal C arrays, but explicitly
// make each bank separate C arrays so that HLS sees the proper HW structure and synthesizes
// more quickly and to more efficient HW.

#ifndef __AC_BANK_ARRAY_H
#define __AC_BANK_ARRAY_H

#ifndef __cplusplus
# error C++ is required to include this header file
#endif
#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

// for safety, check for C-style macros defined that interfere with template parameters
#if (defined(W) || defined(B) || defined(C) || defined(T) || defined(FirstDim) || defined(SecondDim) || defined(ThirdDim) || defined(RestDims))
#error One or more of the following is defined: (W, B, C, T, FirstDim, SecondDim, ThirdDim, RestDims) which conflicts with their 
#error usage as template parameters. DO NOT use defines before including third party header files.
#endif

#include <cstddef>
#include <ac_assert.h>

#include <ac_max_pow2.h>
#include <ac_array_1D.h>

//==========================================================================

//==========================================================================

// ac_bank_array_base is the base class for banked arrays,
// and typically is not directly used in user models.

template <typename B, size_t C>
class ac_bank_array_base;

template <typename B>
class ac_bank_array_base<B, 1>
{
public: // required public for SCVerify
  #pragma hls_ac_bank_mem
  B a;
public:
  B &operator[](size_t idx) { return a; }
  const B &operator[](size_t idx) const { return a; }
};

// specialization to get index checking of rightmost dimension using ac_array_1D
template <typename E, size_t D>
class ac_bank_array_base<E [D], 1>
{
public: // required public for SCVerify
  typedef ac_array_1D<E,D> AC;
  #pragma hls_ac_bank_mem
  AC a;
public:
  AC &operator[](size_t idx) { return a; }
  const AC &operator[](size_t idx) const { return a; }
};

template <typename B, size_t C>
class ac_bank_array_base
{
  static const size_t W = ac_max_pow2<C-1>::P;
public: // required public for SCVerify
  ac_bank_array_base<B, W  > a0;
  ac_bank_array_base<B, C-W> a1;
public:
  B &operator[](size_t idx) {
#ifndef __SYNTHESIS__
    assert(idx < C);
#endif
    size_t aidx = idx & (W-1);
    return idx&W ? a1[aidx] : a0[aidx];
  }

  const B &operator[](size_t idx) const {
#ifndef __SYNTHESIS__
    assert(idx < C);
#endif
    size_t aidx = idx & (W-1);
    return idx&W ? a1[aidx] : a0[aidx];
  }
};

// specialization to get index checking of rightmost dimension using ac_array_1D
template <typename E, size_t D, size_t C>
class ac_bank_array_base<E [D], C>
{
  public:
  typedef ac_array_1D<E,D> AC;
  typedef E B[D];
  static const size_t W = ac_max_pow2<C-1>::P;
  ac_bank_array_base<B, W  > a0;
  ac_bank_array_base<B, C-W> a1;
  AC &operator[](size_t idx) {
#ifndef __SYNTHESIS__
    assert(idx < C);
#endif
    size_t aidx = idx & (W-1);
    return idx&W ? a1[aidx] : a0[aidx];
  }

  const AC &operator[](size_t idx) const {
#ifndef __SYNTHESIS__
    assert(idx < C);
#endif
    size_t aidx = idx & (W-1);
    return idx&W ? a1[aidx] : a0[aidx];
  }
};


// ac_bank_array_vary<> supports variable number of array dimensions for a banked array
//
// T can be any type: for example:  int, int [0x1000], int [0x10][0x1000]
// Most typically T would be a single dimensional C array type (e.g. int [0x1000])
// However, a wide variety of structures can be modeled, for example:
//  ac_bank_array_vary<int[7][9], 5>  // 5 banks of 63 element 2D arrays
//  ac_bank_array_vary<int[9], 5, 7>  // 35 banks of 9 element 1D arrays
//  ac_bank_array_vary<int, 5, 7, 9>  // 315 separate registers

template<typename T, size_t FirstDim, size_t... RestDims>
class ac_bank_array_vary :
  public ac_bank_array_base<ac_bank_array_vary<T, RestDims...>, FirstDim>
{
};

template<typename T, size_t FirstDim>
class ac_bank_array_vary<T, FirstDim> :
  public ac_bank_array_base<T, FirstDim>
{
};

// ac_bank_array_2D supports common 2D array case where FirstDim is the number of banks
// and SecondDim is the number of elements in each bank.
// T is typically a simple scalar type, e.g. uint64

template <typename T, size_t FirstDim, size_t SecondDim>
class ac_bank_array_2D : public ac_bank_array_base<T [SecondDim], FirstDim>
{
};

// ac_bank_array_3D supports common 3D array case where FirstDim and SecondDim determine
// the number of banks and ThirdDim is the number of elements in each bank.
// T is typically a simple scalar type, e.g. uint64

template <typename T, size_t FirstDim, size_t SecondDim, size_t ThirdDim>
class ac_bank_array_3D : public ac_bank_array_base<ac_bank_array_base<T [ThirdDim], SecondDim>, FirstDim>
{
};

#endif

