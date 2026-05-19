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
 * @file ap_shift_reg.h
 * @brief Very simple shift register model
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */

#ifndef __AP_SHIFT_REG_H
#define __AP_SHIFT_REG_H

#define __FORCE_INLINE __attribute__((always_inline)) inline

template <typename DATATYPE, unsigned N = 32>
struct ap_shift_reg
{
   ap_shift_reg() = default;
   explicit ap_shift_reg(const char*)
   {
   }
   ap_shift_reg(const ap_shift_reg<DATATYPE, N>& sr) = delete;
   ap_shift_reg& operator=(const ap_shift_reg<DATATYPE, N>& sr) = delete;
   ~ap_shift_reg() = default;
   __FORCE_INLINE DATATYPE shift(DATATYPE val, unsigned index = N - 1, bool en = true)
   {
      DATATYPE res = data[index];
      if(en)
      {
         for(auto i = N - 1; i > 0; --i)
         {
            data[i] = data[i - 1];
         }
         data[0] = val;
      }
      return res;
   }
   __FORCE_INLINE DATATYPE read(unsigned index = N - 1) const
   {
      return data[index];
   }

 private:
   DATATYPE data[N];
};
#endif
