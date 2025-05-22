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

// ac_shared_array_1D.h
// Stuart Swan, Platform Architect, Siemens EDA
// 11 April 2024
//
// ac_shared_array_1D<> is a 1 dimensional shared array that provides
// assertion checking on index violations in both the pre-HLS model and in the post-HLS RTL
//
// usage:
//   instead of :
//       uint16 my_array[0x1000];
//   use:
//       ac_shared_array_1D<uint16, 0x1000> my_array;


#ifndef __AC_SHARED_ARRAY_1D_H
#define __AC_SHARED_ARRAY_1D_H

#include <cstddef>
#include <ac_assert.h>

#include <ac_shared_proxy.h>

template <typename T, size_t D1>
class ac_shared_array_1D
{
  ac_shared<T [D1]> data;
public:
  T &operator[](size_t idx) {
#ifndef __SYNTHESIS__
    assert(idx < D1);
#endif
    return data[idx];
  }

  const T &operator[](size_t idx) const {
#ifndef __SYNTHESIS__
    assert(idx < D1);
#endif
    return data[idx];
  }
};

#endif

