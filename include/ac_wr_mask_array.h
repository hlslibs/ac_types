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

// Author: Stuart Swan, Platform Architect, Siemens EDA
// 20 December 2024

// ac_wr_mask_array_1D is a 1-dimensional memory that includes "ByteEnable" support.
// The SliceWidth parameter specifies the bitwidth of each slice.
// The slices do not need to be exactly 8 bits wide.
//
// By default we do not map to a RAM that uses byte enables, but instead use a separate
// RAM per slice.
// If you want to map to a RAM that uses byte enables, set the use_be_ram parameter to true.
// When doing so, you will need to use a directive similar to below in your catapult script:
//
// directive set /dut/main/mem.mem:rsc -MAP_TO_MODULE {ram_nangate-45nm-singleport_beh.RAM num_byte_enables=4}
//
//
// Usage:
//   ac_wr_mask_array<uint32, 0x1000, 8> mem;
//   mem[0x100] = 0x123;  // write all slices at address 0x100
//   mem[0x100].mask(3) = 0xffff; // write 2 low-order bytes at address 0x100
//   uint32 val = mem[0x100];  // read all slices at address 0x100

#ifndef __AC_WR_MASK_ARRAY_1D_H
#define __AC_WR_MASK_ARRAY_1D_H

#include <mem_array.h>
#include <ac_assert.h>
#include <ac_bank_array.h>

template <typename T, int Dim1, int SliceWidth=1, bool use_be_ram=0>
class ac_wr_mask_array_1D
{
public:

  static const unsigned dim1 = Dim1;
  typedef Wrapped<T> WData_t;
  static const unsigned int WordWidth = WData_t::width;
  static const unsigned num_slices = WordWidth/SliceWidth;
  typedef ac_int<num_slices, false> WriteMask;
  static const unsigned idx_width = ac::log2_ceil<Dim1>::val;
  typedef ac_int<idx_width, false> idx_t;
  typedef sc_lv<WordWidth> Data_t;
  typedef sc_lv<SliceWidth> Slice_t;
  ac_bank_array_2D<Slice_t, num_slices, Dim1> mem;

  static_assert(WordWidth == num_slices *SliceWidth,
                "Word width must be evenly divisible by SliceWidth");

  // use this function when HLS cannot handle elem_proxy functions
  void write(unsigned idx, T val, WriteMask mask_val=~0) {
    AC_A_BANK_ARRAY_ASSERTION(idx < ac_wr_mask_array_1D::dim1);
    Data_t write_data = TypeToBits<T>(val);
    #pragma hls_unroll yes
    for (unsigned i = 0; i < ac_wr_mask_array_1D::num_slices; i++) {
      if (mask_val[i] == 1)
      { mem[i][idx] = write_data.range((i+1)*SliceWidth-1, i*SliceWidth); }
    }
  }

  template <typename slice_t>
  void write(idx_t idx, const slice_t val[num_slices], WriteMask mask_val=~0) {
    AC_A_BANK_ARRAY_ASSERTION(idx < ac_wr_mask_array_1D::dim1);
    const ac_int<32, false> slice_w = SliceWidth;
    #pragma hls_unroll yes
    for (ac_int<32, false> i=0; i < num_slices; ++i) {
      if (mask_val[i] == 1)
      { mem[i][idx] = val[i]; }
    }
  }

  // use this function when HLS cannot handle elem_proxy functions
  T read(unsigned idx) {
    AC_A_BANK_ARRAY_ASSERTION(idx < ac_wr_mask_array_1D::dim1);
    Data_t read_data = TypeToBits<NVUINTW(ac_wr_mask_array_1D::WordWidth)>(0);
    #pragma hls_unroll yes
    for (unsigned i = 0; i < ac_wr_mask_array_1D::num_slices; i++) {
      read_data.range((i+1)*SliceWidth-1, i*SliceWidth) = mem[i][idx];
    }

    CMOD_ASSERT_MSG(read_data.xor_reduce()!=sc_logic('X'), "Read data is X");
    return BitsToType<T>(read_data);
  }

  template <typename slice_t>
  void read(slice_t val[num_slices], unsigned idx) {
    AC_A_BANK_ARRAY_ASSERTION(idx < ac_wr_mask_array_1D::dim1);
    #pragma hls_unroll yes
    for (unsigned i = 0; i < ac_wr_mask_array_1D::num_slices; i++) {
      auto read_data = mem[i][idx];
      CMOD_ASSERT_MSG(read_data.xor_reduce()!=sc_logic('X'), "Read data is X");
      auto v = BitsToType<slice_t>(read_data);
      val[i] = v;
    }
  }

  struct elem_proxy {
    ac_wr_mask_array_1D &array;
    unsigned idx;
    ac_wr_mask_array_1D::WriteMask mask_val{~0};

    elem_proxy(ac_wr_mask_array_1D &_array, unsigned _idx, ac_wr_mask_array_1D::WriteMask _mask=~0)
      : array(_array), idx(_idx), mask_val(_mask) {}

    operator T () {
      AC_A_BANK_ARRAY_ASSERTION(idx < ac_wr_mask_array_1D::dim1);
      Data_t read_data = TypeToBits<NVUINTW(ac_wr_mask_array_1D::WordWidth)>(0);
      #pragma hls_unroll yes
      for (unsigned i = 0; i < ac_wr_mask_array_1D::num_slices; i++) {
        read_data.range((i+1)*SliceWidth-1, i*SliceWidth) = array.mem[i][idx];
      }

      CMOD_ASSERT_MSG(read_data.xor_reduce()!=sc_logic('X'), "Read data is X");
      return BitsToType<T>(read_data);
    }

    void operator=(const T &val) {
      AC_A_BANK_ARRAY_ASSERTION(idx < ac_wr_mask_array_1D::dim1);
      Data_t write_data = TypeToBits<T>(val);
      #pragma hls_unroll yes
      for (unsigned i = 0; i < ac_wr_mask_array_1D::num_slices; i++) {
        if (mask_val[i] == 1)
        { array.mem[i][idx] = write_data.range((i+1)*SliceWidth-1, i*SliceWidth); }
      }
    }

    elem_proxy mask(ac_wr_mask_array_1D::WriteMask _mask) {
      return elem_proxy(array, idx, _mask);
    }
  };

  elem_proxy operator[](unsigned idx) { return elem_proxy(*this, idx); }
  const elem_proxy operator[](unsigned idx) const { return elem_proxy(*this, idx); }
};


// Specialization for mapping to a true byte_enable RAM
// When using,
// ** User must use -MAP_TO_MODULE and num_byte_enables in Catapult tcl file **

template <typename T, int Dim1, int SliceWidth>
class ac_wr_mask_array_1D<T, Dim1, SliceWidth, true>
{
public:

  static const unsigned dim1 = Dim1;
  typedef Wrapped<T> WData_t;
  static const unsigned int WordWidth = WData_t::width;
  static const unsigned num_slices = WordWidth/SliceWidth;
  static const unsigned idx_width = ac::log2_ceil<Dim1>::val;
  typedef ac_int<idx_width, false> idx_t;
  typedef ac_int<num_slices, false> WriteMask;
  typedef T data_t;

  T mem[Dim1];  // ** User must use -MAP_TO_MODULE and num_byte_enables in Catapult tcl file **

  ac_wr_mask_array_1D() {
    #ifndef __SYNTHESIS__
    std::cout << "ac_wr_mask_array_1D: Note: In Catapult tcl script need to set num_byte_enables to " <<
              num_slices << "\n";
    #endif
  }

  static_assert(WordWidth == num_slices *SliceWidth,
                "Word width must be evenly divisible by SliceWidth");

  // use this function when HLS cannot handle elem_proxy functions
  void write(idx_t idx, data_t val, WriteMask mask_val=~0) {
    AC_A_BANK_ARRAY_ASSERTION(idx < ac_wr_mask_array_1D::dim1);
    const ac_int<32, false> slice_w = SliceWidth;
    #pragma hls_unroll yes
    for (ac_int<32, false> i=0; i < num_slices; ++i) {
      if (mask_val[i] == 1)
      { mem[idx].set_slc(i * slice_w, val.template slc<SliceWidth>( i * slice_w)); }
    }
  }

  template <typename slice_t>
  void write(idx_t idx, const slice_t val[num_slices], WriteMask mask_val=~0) {
    AC_A_BANK_ARRAY_ASSERTION(idx < ac_wr_mask_array_1D::dim1);
    const ac_int<32, false> slice_w = SliceWidth;
    #pragma hls_unroll yes
    for (ac_int<32, false> i=0; i < num_slices; ++i) {
      if (mask_val[i] == 1)
      { mem[idx].set_slc(i * slice_w, val[i]); }
    }
  }

  // use this function when HLS cannot handle elem_proxy functions
  T read(idx_t idx) {
    AC_A_BANK_ARRAY_ASSERTION(idx < ac_wr_mask_array_1D::dim1);
    return mem[idx];
  }

  template <typename slice_t>
  void read(slice_t val[num_slices], unsigned idx) {
    AC_A_BANK_ARRAY_ASSERTION(idx < ac_wr_mask_array_1D::dim1);
    #pragma hls_unroll yes
    for (unsigned i = 0; i < ac_wr_mask_array_1D::num_slices; i++) {
      auto read_data = mem[idx];
      val[i] = read_data.range((i+1)*SliceWidth-1, i*SliceWidth);
    }
  }

  struct elem_proxy {
    ac_wr_mask_array_1D &array;
    ac_wr_mask_array_1D::idx_t idx;
    ac_wr_mask_array_1D::WriteMask mask_val{~0};

    elem_proxy(ac_wr_mask_array_1D &_array, unsigned _idx, ac_wr_mask_array_1D::WriteMask _mask=~0)
      : array(_array), idx(_idx), mask_val(_mask) {}

    operator T () {
      AC_A_BANK_ARRAY_ASSERTION(idx < ac_wr_mask_array_1D::dim1);
      return array.mem[idx];
    }

    void operator=(const ac_wr_mask_array_1D::data_t &val) {
      AC_A_BANK_ARRAY_ASSERTION(idx < ac_wr_mask_array_1D::dim1);
      const ac_int<32, false> slice_w = SliceWidth;
      #pragma hls_unroll yes
      for (ac_int<32, false> i=0; i < num_slices; ++i) {
        if (mask_val[i] == 1)
        { array.mem[idx].set_slc(i * slice_w, val.template slc<SliceWidth>( i * slice_w)); }
      }
    }

    elem_proxy mask(ac_wr_mask_array_1D::WriteMask _mask) {
      return elem_proxy(array, idx, _mask);
    }
  };

  elem_proxy operator[](unsigned idx) { return elem_proxy(*this, idx); }
  const elem_proxy operator[](unsigned idx) const { return elem_proxy(*this, idx); }
};

#endif

