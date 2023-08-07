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
 *  Copyright 2004-2020, Mentor Graphics Corporation,                     *
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
//  Source:         ac_sync.h
*/

#ifndef __AC_SYNC_H
#define __AC_SYNC_H

#include <ac_channel.h>

////////////////////////////////////////////////////////////////////////////////
// Class: ac_sync
////////////////////////////////////////////////////////////////////////////////

#if defined(__SYNTHESIS__)
#pragma builtin
#define INHERIT_MODE private
#else
#define INHERIT_MODE public
#endif
class ac_sync : INHERIT_MODE ac_channel<bool>
{
public:
  typedef ac_channel<bool> Base;

  // constructor
  ac_sync(): Base() { }

template <typename ...T> 
void sync_in(T &...t) {
    Base::read();
}

template <typename ...T> 
void sync_out(T &...t) {
    Base::write(true);
}

  inline bool nb_sync_in() {
    bool rval = true;
    bool dummy_obj;
    rval = Base::nb_read(dummy_obj); // During synthesis -- builtin treatment
    return rval;
  }

  #if 0
  inline bool nb_sync_out();
  #else
  // C simulation always returns true -- So, 'else' branch based on the
  // successs of 'nb_write' is not exercisable in C simulation, as the
  // underlying buffer is unbounded in C model.
  // But, in RTL, when mapped to two-way handshake component, both 'if' and
  // 'else' branch are exercisable in RTL
  inline bool nb_sync_out() {
    bool rval = true;
    rval = Base::nb_write(rval);
    return rval;
  }
  #endif

  inline bool available( unsigned int cnt) {
    return Base::available(cnt);
  }

  #ifdef __CONNECTIONS__CONNECTIONS_H__
  void bind(Connections::SyncIn  &c)  { Base::bind(c); }
  void bind(Connections::SyncOut &c)  { Base::bind(c); }
  #endif

private:
  // Prevent the compiler from autogenerating these.
  // This enforces that ac_sync are always passed by reference.
  ac_sync(const ac_sync &);
  ac_sync &operator=(const ac_sync &);
};

#endif

