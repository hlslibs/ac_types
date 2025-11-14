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
 *  Copyright  Siemens                                                *
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
#pragma once

#include <ac_int.h>
#include <ac_channel.h>
#include <ac_bank_array.h>

#pragma hls_design
#pragma builtin 
template<
         int NUM_IN,
         int NUM_OUT,
         typename T,
         typename ...Args >
void ac_xbar(ac_int<NUM_IN, false> sel_i[NUM_OUT], ac_channel<T> &chan, Args&...args) {
  constexpr auto size_args = sizeof...(args);
  static_assert((NUM_IN+NUM_OUT-1) == size_args, "Unexpected number of arguments");

  bool available = true;
#ifndef __SYNTHESIS__
  auto check_zero_or_one_hot = [&]() -> void {
    for(int i = 0; i < NUM_OUT; ++i) {
      const auto &sel = sel_i[i];
      if(sel & (sel-1)) {
        std::stringstream ss;
        ss<<"sel_i["<<i<<"]="<<sel.to_string(AC_HEX, false, true)<<" must be one-hot or zero ";
        AC_ASSERT(false, ss.str().c_str());
      }
    } 
  };

  check_zero_or_one_hot();
  available = false;     
#endif

  ac_bank_array_base< ac_channel<T> &, NUM_IN+NUM_OUT> bank { chan, args... };
  for(int j = 0; j < NUM_IN; ++j) {
    bool selected = false;
    for(int k = 0; k < NUM_OUT; ++k) {
      if(sel_i[k][j]) {
        selected = true;
      }
    }

    if( available || bank[j].available(1) ) {      
      if(selected) {
        const T &data = bank[j].read();
        for(int k = 0; k < NUM_OUT; ++k) {
          if(sel_i[k][j]) {
            bank[k+NUM_IN].write(data);
          }
        }
      }      
    }
  }
}
