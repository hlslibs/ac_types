/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *                Copyright (C) 2025-2026 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   A copy of the License can be found in the root directory of this repository.
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file ap_fixed.h
 * @brief Very simple wrapper to ac_types to support ap_* types.
 * This file provides the interfaces for ap_fixed objects.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#ifndef __AP_FIXED_H
#define __AP_FIXED_H
#include "ac_fixed.h"

// rounding to plus infinity
#define AP_RND ac_q_mode::AC_RND
// rounding to zero
#define AP_RND_ZERO ac_q_mode::AC_RND_ZERO
// rounding to minus infinity
#define AP_RND_MIN_INF ac_q_mode::AC_RND_MIN_INF
// rounding to infinity
#define AP_RND_INF ac_q_mode::AC_RND_INF
// convergent rounding
#define AP_RND_CONV ac_q_mode::AC_RND_CONV
// truncation
#define AP_TRN ac_q_mode::AC_TRN
// truncation to zero
#define AP_TRN_ZERO ac_q_mode::AC_TRN_ZERO

// saturation
#define AP_SAT ac_o_mode::AC_SAT
// saturation to zero
#define AP_SAT_ZERO ac_o_mode::AC_SAT_ZERO
// symmetrical saturation
#define AP_SAT_SYM ac_o_mode::AC_SAT_SYM
// wrap-around (*)
#define AP_WRAP ac_o_mode::AC_WRAP
// sign magnitude wrap-around (*)
#define AP_WRAP_SM ac_o_mode::AC_WRAP

#define ap_q_mode ac_q_mode
#define ap_o_mode ac_o_mode

template <int W, int I, ap_q_mode Q = AC_TRN, ap_o_mode O = AC_WRAP, int N = 0>
using ap_fixed = ac_fixed<W, I, true, Q, O>;

template <int W, int I, ap_q_mode Q = AC_TRN, ap_o_mode O = AC_WRAP, int N = 0>
using ap_ufixed = ac_fixed<W, I, false, Q, O>;

#include "ap_int.h"

#endif
