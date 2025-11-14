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
 *  Copyright 2004-2020 Siemens                                                *
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

#ifndef __AC_BROADCAST_H
#define __AC_BROADCAST_H

#include <ac_int.h>
#include <ac_channel.h>
#include <ac_assert.h>

#if (defined(__GNUC__) && (__cplusplus < 201103L))
#error Please use C++11 or a later standard for compilation.
#endif
#if (defined(_MSC_VER) && (_MSC_VER < 1920) && !defined(__EDG__))
#error Please use Microsoft VS 2019 or a later standard for compilation.
#endif

/*==============================================================================================

Examples:

To channels passed as individual arguments:
Each channel must be of the same type (not enforced by a C++ compiler)

  ac_channel<Data> ch0, ch1, ch2;

  ac_broadcast_all (data,       ch0, ch1, ch2);  // writes data to all channels
  ac_broadcast_mask(data, mask, ch0, ch1, ch2);  // writes data to subset of channels defined by mask
  ac_broadcast_one (data, mask, ch0, ch1, ch2);  // writes data to one channel, mask must be 1-hot encoded or 0

To channels passed as a individually typed arguments:

  ac_broadcast_typed_one(mask, data0, ch0, data1, ch1, data2, ch2);  // mask must be 1-hot encoded or 0

To array of channels locally declared or passed as reference to arrays

  ac_channel<Data> ch[3];
  void func(ac_channel<Data> (&ch)[3]);

  ac_broadcast_all (data,       ch);
  ac_broadcast_mask(data, mask, ch);
  ac_broadcast_one (data, mask, ch);

To array of channels passed as pointer to first element

  void func(ac_channel<Data> ch[3]);  // C++ cannot infer size from argument type

  ac_broadcast_ptr<Data, 3>::all (data,       ch);
  ac_broadcast_ptr<Data, 3>::mask(data, mask, ch);
  ac_broadcast_ptr<Data, 3>::one (data, mask, ch);

The following *_demux functions are aliases of the *_one functions

  ac_demux(data, mask, ch0, ch1, ch2)                       // alias of ac_broadcast_one(data, mask, ch0, ch1, ch2)
  ac_typed_demux(mask, data0, ch0, data1, ch1, data2, ch2)  // alias of ac_broadcast_typed_one(mask, data0, ch0, data1, ch1, data2, ch2)
  ac_demux(data, mask, ch)                                  // alias of ac_broadcast_one(data, mask, ch)
  ac_broadcast_ptr<Data, 3>::demux(data, mask, ch)          // alias of ac_broadcast_ptr<Data, >::one(data, mask, ch)

WARNING: Functions other than those previously listed are helper functions which should not be directly called
Calling these functions directly may result in bad logic and/or inferior QofR

  ac_broadcast_typed_mask()
  ac_broadcast_mask(data, mask)

================================================================================================*/

//==============================================================================================
//     B R O A D C A S T   T O   I N D I V I D U A L   C H A N N E L S
//==============================================================================================

template<typename Data>
void ac_broadcast_mask(const Data &data, ac_int<1, false> mask)
{
}

#pragma builtin
template<typename Data, typename Chan, typename... Chan1>
void ac_broadcast_mask(const Data &data, ac_int<sizeof...(Chan1)+1, false> mask, ac_channel<Chan> &ch0, Chan1 &...ch1)
{
  if ( mask[0] ) { ch0.write(data); }
  ac_broadcast_mask(data, mask>>1, ch1...);
}

template<typename Data, typename... Chan>
void ac_broadcast_all(const Data &data, Chan &...ch)
{
  ac_broadcast_mask(data, -1, ch...);
}

template<typename Data, typename... Chan>
void ac_broadcast_one(const Data &data, ac_int<sizeof...(Chan), false> mask, Chan &...ch)
{
  assert(!(mask&(mask-1)) && "Error: ac_broadcast_one mask is not one-hot");
  ac_broadcast_mask(data, mask, ch...);
}

template<typename Data, typename... Chan>
void ac_demux(const Data &data, ac_int<sizeof...(Chan), false> mask, Chan &...ch)
{
  ac_broadcast_one(data, mask, ch...);
}

//===================================================================================================
// B R O A D C A S T   T O   I N D I V I D U A L L Y   T Y P E D   C H A N N E L S
// only writing to one channel at each call is supported
//===================================================================================================

inline void ac_broadcast_typed_mask(ac_int<1, false> mask)
{
}

template<typename Data0, typename Chan0, typename... DataChan>
void ac_broadcast_typed_mask(ac_int<sizeof...(DataChan)/2+1, false> mask, const Data0 &data0, ac_channel<Chan0> &ch0, DataChan &...ch1)
{
  if ( mask[0] ) { ch0.write(data0); }
  ac_broadcast_typed_mask(mask>>1, ch1...);
}

#pragma builtin
template<typename... DataChan>
void ac_broadcast_typed_one(ac_int<sizeof...(DataChan)/2, false> mask, DataChan &...ch)
{
  assert(!(mask&(mask-1)) && "Error: ac_broadcast_one mask is not one-hot");
  ac_broadcast_typed_mask(mask, ch...);
}

#pragma builtin
template<typename... DataChan>
void ac_typed_demux(ac_int<sizeof...(DataChan)/2, false> mask, DataChan &...ch)
{
  ac_broadcast_typed_one(mask, ch...);
}

//===================================================================================================
//     B R O A D C A S T   T O   A R R A Y   O F   C H A N N E L S
//===================================================================================================

template<typename Data, unsigned int N>
struct ac_broadcast_ptr;

template<typename Data>
struct ac_broadcast_ptr<Data, 0> {
  template<typename... Chan>
  static void mask(const Data &data, ac_int<sizeof...(Chan), false> mask, ac_channel<Data> *cp, Chan &...ch) {
    ac_broadcast_mask(data, mask, ch...);
  }
};

template<typename Data, unsigned int N>
struct ac_broadcast_ptr {
  template<typename... Chan>
  static void mask(const Data &data, ac_int<N+sizeof...(Chan), false> mask, ac_channel<Data> *cp, Chan &...ch) {
    ac_broadcast_ptr<Data, N-1>::mask(data, mask, cp, cp[N-1], ch...);
  }

  static void all(const Data &data, ac_channel<Data> *cp) {
    mask(data, -1, cp);
  }

  static void one(const Data &data, ac_int<N, false> mask1, ac_channel<Data> *cp) {
    assert(!(mask1&(mask1-1)) && "Error: ac_broadcast_one mask is not one-hot");
    mask(data, mask1, cp);
  }

  static void demux(const Data &data, ac_int<N, false> mask1, ac_channel<Data> *cp) {
    one(data, mask1, cp);
  }
};

//===================================================================================================
//        B R O A D C A S T   T O   C H A N N E L A R R A Y
//===================================================================================================

template<typename Data, typename Num, typename Chan, int N>
void ac_broadcast_mask(const Data &data, Num mask, ac_channel<Chan> (&ch)[N])
{
  ac_broadcast_ptr<Chan, N>::mask(data, mask, ch);
};

template<typename Data, typename Chan, int N>
void ac_broadcast_all(const Data &data, ac_channel<Chan> (&ch)[N])
{
  ac_broadcast_ptr<Chan, N>::all(data, ch);
};

template<typename Data, typename Num, typename Chan, int N>
void ac_broadcast_one(const Data &data, Num mask, ac_channel<Chan> (&ch)[N])
{
  ac_broadcast_ptr<Chan, N>::one(data, mask, ch);
};

template<typename Data, typename Num, typename Chan, int N>
void ac_demux(const Data &data, Num mask, ac_channel<Chan> (&ch)[N])
{
  ac_broadcast_ptr<Chan, N>::one(data, mask, ch);
};

#endif

