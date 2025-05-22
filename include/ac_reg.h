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
 *  Copyright 2022 Siemens                                                *
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
