/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Datatypes                                          *
 *                                                                        *
 *  Software Version: 4.7                                                 *
 *                                                                        *
 *  Release Date    : Tue Jul 18 16:56:08 PDT 2023                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 4.7.0                                               *
 *                                                                        *
 *  Copyright 2022, Mentor Graphics Corporation,                     *
 *                                                                        *
 *  All Rights Reserved.                                                  *
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

/*
//  Source:         ac_reg.h
*/

#ifndef __AC_REG_H
#define __AC_REG_H

#ifdef __AC_NAMESPACE
namespace __AC_NAMESPACE {
#endif

namespace ac {
  template <typename Dtype>
  inline Dtype reg(Dtype data)
  {
    // Causes a register to be inserted during syntheiss where this function is called
    return data;
  }
}

#ifdef __AC_NAMESPACE
}
#endif

#endif
