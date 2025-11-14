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
 *  Copyright 2019 Siemens                                                *
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
//  Source:         ac_shared.h
//  Description:    Catapult built-in class to declare intent to share an
//                  an object between different threads/processes.
//                  It can be used as a replacement of its base type.
//  Author:         Peter Gutberlet
*/

#ifndef __AC_SHARED_H
#define __AC_SHARED_H

#include <ac_int.h>

template <typename T>
class ac_shared
{
  #if defined(CCS_SCVERIFY) || defined(__SYNTHESIS__)
public:
  #endif
  T      d;

public:

  ac_shared() {}
  ac_shared(const ac_shared &s) : d(s.d) {}
  ac_shared(const T &t) : d(t) {}
  template <typename T2> ac_shared(const T2 &t) : d(t) {}

  operator const T &() const   { return d; }
  operator       T &()         { return d; }

  const ac_shared &operator =(const ac_shared &s) { d = s.d; return *this; }
  const T &operator =(const T &t) { d = t; return d; }
  template <typename T2>
  const T &operator =(const T2 &t) { d = t; return d; }
};

template <typename T>
class ac_shared_d : public T
{
public:
  ac_shared_d() {}
  ac_shared_d(const ac_shared_d &s) : T(s) {}
  ac_shared_d(const T &t) : T(t) {}
  template <typename T2> ac_shared_d(const T2 &t) : T(t) {}

  const ac_shared_d &operator =(const ac_shared_d &s) { T::operator =(s); return *this; }
  const T &operator =(const T &t) { T::operator =(t); return *this; }
  template <typename T2>
  const T &operator =(const T2 &t) { T::operator =(t); return *this; }
};

#ifdef __AC_NAMESPACE
namespace __AC_NAMESPACE
{
#endif

  namespace ac
  {
    template <ac_special_val V, typename T>
    inline bool init_array(ac_shared<T> &a, int n) { return init_array<V> (&*a, n); }
  }
  #ifdef __AC_NAMESPACE
}
  #endif


#endif

