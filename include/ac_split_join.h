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

#include <iostream>
#include <ac_int.h>
#include <ac_channel.h>
#include <mc_scverify.h>
#include <connections/marshaller.h>
#include <auto_gen_fields.h>
#include <mc_connections.h>

//------------------------------------------------------------------------------
// some common functions 
//------------------------------------------------------------------------------
inline bool ac_join_available() {
  return true;
}

template<typename T_IN, typename ...Args>
bool ac_join_available(ac_channel<T_IN>& ch_i, Args&... args) {
  return ch_i.available(1) & ac_join_available(args...);
}

template<int SIZE, int CUR, typename T_IN>
bool ac_join_available(ac_channel<T_IN> ch_i[SIZE]) {
  if constexpr (CUR != SIZE) {
    return ch_i[CUR].available(1) & ac_join_available<SIZE, CUR+1> (ch_i);
  } else {
    return true;
  }
}

template<typename T, int N>
inline void type_to_bv(const T& obj, ac_int<N,false>& vec) {
  vec = to_ac(sc_dt::sc_biguint<N>(Connections::convert_to_lv(obj)));
}

template<typename T, int N>
inline void bv_to_type(const ac_int<N,false>& vec, T& obj)
{
  using non_ref_T = typename std::remove_reference<T>::type;
  if constexpr (std::is_same<T, ac_int<N,false> >::value) {
    obj = vec;
  } else if constexpr (std::is_same<T, ac_int<N,true> >::value) {
    obj = vec;
  } else {
    obj = Connections::convert_from_lv<non_ref_T>(sc_dt::sc_lv<N>(to_sc(vec)));
  }
}

template<typename T, int N>
inline void type_to_bv(const ac_int<N,false>& obj, ac_int<N,false>& vec) {
  vec = obj;
}

//------------------------------------------------------------------------------
// ac_split
//------------------------------------------------------------------------------
template<int IN_WIDTH, int OUT_WIDTH>
inline void ac_split_check_width() {
  static_assert(IN_WIDTH == OUT_WIDTH, "INPUT and OUTPUT size mismatch");
}

template<int EXP_WIDTH, int W1, typename T, typename ...Args>
inline void ac_split_check_width(const ac_channel<T> &, Args &...args) {
  return ac_split_check_width<EXP_WIDTH, W1+Wrapped<T>::width> (args...);
}

template <int IN_WIDTH>
void ac_split(ac_int<IN_WIDTH, false> input_bv) {
  static_assert(IN_WIDTH == 0, "Input width is greater than output width");
}

// internal
template <int IN_WIDTH, typename T_OUT, typename ...Args>
void ac_split(ac_int<IN_WIDTH, false> input_bv, ac_channel<T_OUT>& ch_o, Args&... args) {
  constexpr int OUT_WIDTH = Wrapped<T_OUT>::width;
  static_assert(IN_WIDTH >= OUT_WIDTH, "Input width is less output width");
  
  ac_int<OUT_WIDTH, false> curr_out_bv = input_bv.template slc<OUT_WIDTH> (0);
  T_OUT oval;
  bv_to_type(curr_out_bv, oval);
  ch_o.write(oval);

  // C++17 feature
  if constexpr (IN_WIDTH > OUT_WIDTH) {
    auto rest_bv = input_bv.template slc<IN_WIDTH-OUT_WIDTH> (OUT_WIDTH);
    ac_split(rest_bv, args...);
  }
}

#pragma hls_design
#pragma builtin ac_split
template<typename T_IN, typename T_OUT, typename ...Args>
void ac_split(ac_channel<T_IN>& ch_i, ac_channel<T_OUT>& ch_o, Args&... ch_o_) {
  constexpr int IN_WIDTH  = Wrapped<T_IN>::width;
  ac_split_check_width<IN_WIDTH, Wrapped<T_OUT>::width>(ch_o_...);
  bool always_true = false;
#ifdef __SYNTHESIS__
  always_true = true;
#endif 
  if(always_true || ch_i.available(1)) {
    auto read_data = ch_i.read();
    ac_int<IN_WIDTH,false> input_bv;
    type_to_bv(read_data, input_bv);
    ac_split(input_bv, ch_o, ch_o_...);   
  }
}

template<int SIZE, int CURR, int IN_WIDTH, typename T_OUT>
void ac_split(const ac_int<IN_WIDTH,false> &input_bv, ac_channel<T_OUT> ch_o[SIZE]) {
  constexpr int OUT_WIDTH = Wrapped<T_OUT>::width;
  ac_int<OUT_WIDTH, false> curr_out_bv = input_bv.template slc<OUT_WIDTH> (0);
  T_OUT oval;
  bv_to_type(curr_out_bv, oval);
  ch_o[CURR].write(oval);

  if constexpr(CURR+1 < SIZE) {
    auto rest_bv = input_bv.template slc<IN_WIDTH-OUT_WIDTH> (OUT_WIDTH);
    ac_split<SIZE, CURR+1> (rest_bv, ch_o);
  }
}

#pragma hls_design
#pragma builtin ac_split
template<int SIZE, typename T_IN, typename T_OUT>
void ac_split(ac_channel<T_IN>& ch_i, ac_channel<T_OUT> ch_o[SIZE]) {
  constexpr int IN_WIDTH  = Wrapped<T_IN>::width;
  constexpr int REQ_WIDTH = Wrapped<T_OUT>::width * SIZE;
  ac_split_check_width<IN_WIDTH, REQ_WIDTH> ();
  bool always_true = false;
#ifdef __SYNTHESIS__
  always_true = true;
#endif
  if(always_true || ch_i.available(1)) {
    auto read_data = ch_i.read();
    ac_int<IN_WIDTH,false> input_bv;
    type_to_bv(read_data, input_bv);
    ac_split<SIZE, 0> (input_bv, ch_o);
  }
}

//------------------------------------------------------------------------------
// ac_join
//------------------------------------------------------------------------------
template<int OUT_WIDTH, int IN_WIDTH>
inline void ac_join_check_width() {
  static_assert(IN_WIDTH == OUT_WIDTH, "INPUT and OUTPUT size mismatch");
}

template<int EXP_WIDTH, int W1, typename T, typename ...Args>
inline void ac_join_check_width(const ac_channel<T> &, Args &...args) {
  return ac_join_check_width<EXP_WIDTH, W1+Wrapped<T>::width> (args...);
}

template<int CUR_IDX, int OUT_WIDTH>
void ac_join(ac_int<OUT_WIDTH, false> &out_bv) {
  if constexpr (CUR_IDX < OUT_WIDTH) {
    static_assert(CUR_IDX == OUT_WIDTH, "Input width is less than output width");
  }

  if constexpr (CUR_IDX > OUT_WIDTH) {
    static_assert(CUR_IDX == OUT_WIDTH, "Input width is more than output width");
  }
}

template<int CUR_IDX, int OUT_WIDTH, typename T_IN, typename ...Args>
void ac_join(ac_int<OUT_WIDTH, false> &out_bv, ac_channel<T_IN>& ch_i, Args&... args) {  
  constexpr int CUR_IN_WIDTH = Wrapped<T_IN>::width;
  ac_int<CUR_IN_WIDTH, false> input_bv;
  auto read_data = ch_i.read();
  type_to_bv(read_data, input_bv);
  static_assert(CUR_IDX < OUT_WIDTH);  
  out_bv.set_slc(CUR_IDX, input_bv);
  ac_join<CUR_IDX+CUR_IN_WIDTH> (out_bv, args... );  
}

#pragma hls_design
#pragma builtin ac_join
template<typename T_OUT, typename T_IN, typename ...Args>
void ac_join(ac_channel<T_OUT>& ch_o, ac_channel<T_IN>& ch_i, Args&... args) {
  constexpr int OUT_WIDTH = Wrapped<T_OUT>::width;
  ac_join_check_width<OUT_WIDTH, Wrapped<T_IN>::width> (args...);
  bool available = false;
  
#ifdef __SYNTHESIS__
  available = true;
#endif

  if(available || ac_join_available(ch_i, args...)) {
    ac_int<OUT_WIDTH,false> out_bv;
    ac_join<0> (out_bv, ch_i, args...);
    T_OUT write_data;
    bv_to_type(out_bv, write_data);
    ch_o.write(write_data);
  }
}

template<int SIZE, int ARR_IDX, int CUR_BV_IDX, int OUT_WIDTH, typename T_IN>  
void ac_join(ac_int<OUT_WIDTH,false> &out_bv, ac_channel<T_IN> ch_i[SIZE]) {
  constexpr int IN_WIDTH  = Wrapped<T_IN>::width;
  ac_int<IN_WIDTH,false> input_bv;
  auto read_data = ch_i[ARR_IDX].read();
  type_to_bv(read_data, input_bv);
  static_assert(CUR_BV_IDX < OUT_WIDTH);
  out_bv.set_slc(CUR_BV_IDX, input_bv);
  if constexpr (ARR_IDX+1 < SIZE) {
    ac_join<SIZE, ARR_IDX+1, CUR_BV_IDX+IN_WIDTH> (out_bv, ch_i); 
  } 
}

#pragma hls_design
#pragma builtin ac_join
template<int SIZE, typename T_OUT, typename T_IN>  
void ac_join(ac_channel<T_OUT>& ch_o, ac_channel<T_IN> ch_i[SIZE]) {
  constexpr int OUT_WIDTH = Wrapped<T_OUT>::width;  
  constexpr int IN_WIDTH  = Wrapped<T_IN>::width;
  constexpr int REQ_WIDTH = IN_WIDTH * SIZE;
  ac_join_check_width<OUT_WIDTH, REQ_WIDTH> ();
  
  bool available = false;
#ifdef __SYNTHESIS__
  available = true;
#endif

  if(available || ac_join_available<SIZE, 0>(ch_i)) {
    ac_int<OUT_WIDTH,false> out_bv;
    ac_join<SIZE, 0, 0> (out_bv, ch_i);
    T_OUT write_data;
    bv_to_type(out_bv, write_data);
    ch_o.write(write_data);
  }
}
