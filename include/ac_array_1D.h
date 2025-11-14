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

// ac_array_1D.h
// Stuart Swan, Platform Architect, Siemens EDA
// 11 April 2024
//
// ac_array_1D<> is a 1 dimensional array that provides
// assertion checking on index violations in both the pre-HLS model and in the post-HLS RTL
//
// usage:
//   instead of :
//       uint16 my_array[0x1000];
//   use:
//       ac_array_1D<uint16, 0x1000> my_array;
//
// To enable synthesizable assertions for out-of-bounds checks, compile -DAC_BANK_ARRAY_ASSERT.
// To enable plain C++ assertions (not synthesized into the RTL) no additional flags required.
// To disable all asserts (S/W and H/W), compile -DAC_BANK_ARRAY_ASSERT_NOOP.


#ifndef __AC_ARRAY_1D_H
#define __AC_ARRAY_1D_H

#include <cstddef>
#include <ac_assert.h>

#if defined(AC_BANK_ARRAY_ASSERT)
  #define AC_A_BANK_ARRAY_ASSERTION AC_ASSERTION
#else
  #if defined(AC_BANK_ARRAY_ASSERT_NOOP)
    #define AC_A_BANK_ARRAY_ASSERTION
  #else
    #define AC_A_BANK_ARRAY_ASSERTION assert
  #endif
#endif

template <typename T, size_t D1>
class ac_array_1D
{
  public: // required public for SCVerify
  T data[D1];

  T &operator[](size_t idx) {
    AC_A_BANK_ARRAY_ASSERTION(idx < D1);
    return data[idx];
  }

  const T &operator[](size_t idx) const {
    AC_A_BANK_ARRAY_ASSERTION(idx < D1);
    return data[idx];
  }
};

#endif

