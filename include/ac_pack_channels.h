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
 *  Copyright 2025 Siemens                                                *
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

#ifndef __AC_PACK_CHANNELS_H
#define __AC_PACK_CHANNELS_H

#include <ac_channel.h>
#include <ac_bank_array.h>
#include <ac_int.h>

// ac_pack_channels provides packaging for channels into an array-like indexable container
// base can be any channel data type: for example: int, uint8, ac_int<32>
// size specifies the number of channels in the array
//
// Examples:
//  ac_pack_channels<int, 5>     // 5 channels of int get packed
//  ac_pack_channels<uint8, 8>   // 8 channels of uint8 get packed
//
// Usage with aggregate initialization:
//  ac_pack_channels<uint8, 4> in{in0, in1, in2, in3};
//  where in0, in1, in2, in3 are ac_channel of uint8

#ifdef __AC_NAMESPACE
namespace __AC_NAMESPACE {
#endif

namespace ac {

  template <typename base, size_t size>
  class ac_pack_channels {
    public:
      ac_bank_array_base<ac_channel<base>&, size> chan;

      ac_pack_channels() = default;

      ac_channel<base>& operator[](size_t idx) {
        return chan[idx];
      }

      const ac_channel<base>& operator[](size_t idx) const {
        return chan[idx];
      }

      // Utility functions for channel packing

      // Check if all channels have data available
      inline bool all_available(unsigned int k = 1) {
        #pragma hls_unroll yes
        for(size_t i = 0; i < size; i++) {
          if(!chan[i].available(k)) return false;
        }
        return true;
      }

      // Check if any channel has data available
      inline bool any_available(unsigned int k = 1) {
        #pragma hls_unroll yes
        for(size_t i = 0; i < size; i++) {
          if(chan[i].available(k)) return true;
        }
        return false;
      }

      // Read from all channels into array
      inline void read_all(base (&data)[size]) {
        #pragma hls_unroll yes
        for(size_t i = 0; i < size; i++) {
          data[i] = chan[i].read();
        }
      }

      // Write data from array to each channel
      inline void write_all(const base (&data)[size]) {
        #pragma hls_unroll yes
        for(size_t i = 0; i < size; i++) {
          chan[i].write(data[i]);
        }
      }

      // Transfer data between packed channels
      inline void transfer(ac_pack_channels& dst) {
        #pragma hls_unroll yes
        for(size_t i = 0; i < size; i++) {
          dst[i].write(chan[i].read());
        }
      }
  };

  // Template aliases for common channel array sizes
  template <typename base>
  using ac_pack_channels_2 = ac_pack_channels<base, 2>;

  template <typename base>
  using ac_pack_channels_3 = ac_pack_channels<base, 3>;

  template <typename base>
  using ac_pack_channels_4 = ac_pack_channels<base, 4>;

  template <typename base>
  using ac_pack_channels_8 = ac_pack_channels<base, 8>;

  template <typename base>
  using ac_pack_channels_16 = ac_pack_channels<base, 16>;

  template <typename base>
  using ac_pack_channels_32 = ac_pack_channels<base, 32>;

} // namespace ac

using ac::ac_pack_channels;
using ac::ac_pack_channels_2;
using ac::ac_pack_channels_3;
using ac::ac_pack_channels_4;
using ac::ac_pack_channels_8;
using ac::ac_pack_channels_16;
using ac::ac_pack_channels_32;

#ifdef __AC_NAMESPACE
} // namespace __AC_NAMESPACE
#endif


#endif // __AC_PACK_CHANNELS_H
