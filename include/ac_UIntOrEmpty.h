/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Datatypes                                          *
 *                                                                        *
 *  Software Version: 2026.1                                              *
 *                                                                        *
 *  Release Date    : Wed Mar 11 20:32:09 PDT 2026                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 2026.1.1                                            *
 *                                                                        *
 *  Copyright 2026 Siemens                                                *
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

/* Derived from NVIDIA MatchLib UIntOrEmpty.h */
/* This is an experimental header and should not be used in production designs */

/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License")
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

///////////////////
// Provides an ac_int field that can also be of width = 0. This can be
// useful for defining structs where, based on template parameter values,
// some optional fields can be specified as width=0 so that they do not 
// appear in the final hardware.
//
// Example:
//    #include <ac_int.h>
//    #include <ac_channel.h>
//    #include <ac_UIntOrEmpty.h>
//    
//    template <int DATA_WIDTH, int ID_WIDTH>
//    struct ch_payload {
//      ac_int<DATA_WIDTH,false>               data; // Data is always required
//      typename ac::UIntOrEmpty<ID_WIDTH>::T  id;   // Transaction ID is optional
//    };
//    
//    #include <mc_scverify.h>
//    
//    #pragma hls_design top
//    void CCS_BLOCK(dut_design)(
//      ac_channel<ch_payload<32,8> > &din_w_id,  // a 32+8 bit channel
//      ac_channel<ch_payload<32,8> > &dout_w_id,
//      ac_channel<ch_payload<32,0> > &din,       // a 32 bit channel
//      ac_channel<ch_payload<32,0> > &dout)
//    {
//      #ifndef __SYNTHESIS__
//      while (din_w_id.available(1) && din.available(1))
//      #endif
//      {
//        dout_w_id.write(din_w_id.read());
//        dout.write(din.read());
//      }
//    }
///////////////////

#ifndef __AC_UINTOREMPTY_H
#define __AC_UINTOREMPTY_H

#include <ac_int.h>
#include <stdint.h>
#include <iostream>
#include <ac_assert.h>

#ifdef __AC_NAMESPACE
namespace __AC_NAMESPACE {
#endif

namespace ac {

/**
 * \brief The EmptyField class is used for rudimentary support for members of a struct that can be configured to have zero width.
 */
struct EmptyField {
  template <typename T>
  EmptyField operator=(T const &) {
#ifndef __SYNTHESIS__
	 // If an assignment actually occurs during runtime you've done something wrong
    AC_ASSERT(true,"EmptyField should never be assigned or accessed");
#endif
    return EmptyField();
  }
  uint64_t to_uint64() { return 0; }
//  const sc_bit operator[](std::size_t idx) const { return static_cast<sc_bit>(0); }
  bool and_reduce() { return 0; }
  bool or_reduce() { return 0; }
};

/* Operator << for EmptyField. */
inline std::ostream &operator<<(std::ostream &os, const EmptyField &empty) {
  (void)empty; // silence compiler warning about unused parameter
  return os << "EMPTYFIELD";
}

/* Operator != for EmptyField and int. */
inline bool operator!=(const EmptyField lhs, const int rhs) {
  (void)lhs; // silence compiler warning about unused parameter
  (void)rhs; // silence compiler warning about unused parameter
  return true; // EmptyField never equals anything
}

/**
 * \brief A class to determine whether to instantiate an ac_int or an EmptyField.
 */
template <bool, int W>
class UIntOrEmptywCheck;

/**
 * \brief Template specialization to instantiate an ac_int if the width of the UIntOrEmpty is greater than 0.
 */
template <int W>
class UIntOrEmptywCheck<true, W> {
 public:
  typedef ac_int<W,false> T;
};

/**
 * \brief Template specialization to instantiate an EmptyField if the width of the UIntOrEmpty is 0.
 */
template <int W>
class UIntOrEmptywCheck<false, W> {
 public:
  typedef EmptyField T;
};

/**
 * \brief The UIntOrEmpty class is used to define a bitvector that can have a bitwidth of 0.
 *
 * \tparam W     The width of the bitfield (can be 0).
 *
 * \par Overview
 * UIntOrEmpty allows fields to be elided entirely from a struct in the case that they are not used.
 * If W=0, an EmptyField is instantiated instead of an ac_int.
 * EmptyField is ignored by the Marshaller and overloads some common operators so that
 * code using it does not need to be special-cased for zero-width parameterizations.
 */
template <int W>
class UIntOrEmpty {
 public:
  typedef typename UIntOrEmptywCheck<(W > 0), W>::T T;
};

};

#ifdef __AC_NAMESPACE
}
#endif
 
//////////////////

#endif

