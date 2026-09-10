/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Datatypes                                          *
 *                                                                        *
 *  Software Version: 2026.3                                              *
 *                                                                        *
 *  Release Date    : Wed Sep  2 19:47:09 PDT 2026                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 2026.3.0                                            *
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

/*
//  Source:         ac_sync.h
//  Description:    AC Channel based synchronization: 
//                  - Bool data is dummy for pre-HLS simulation
//                  - During synthesis, the channel read/write operations instantiate synchronization signals without data
//                  - Ac_start: Models one-way input synchronization signal 
//                    - for example, start of a transaction
//                    - During synthesis, the synchronization is blocking in nature
//                  - Ac_done: Models one-way output synchronization signal 
//                    - for example, end of a transaction
//                    - During synthesis, the synchronization has no back-pressure and does not block the process
//                  - Ac_sync: Derived by ac_start and ac_done and provides support for both one-way and two-way synchronization signal 
//                    - Provides non-blocking synchronization APIs for both input and output synchronization.
//                    - During synthesis, the user can choose one-way or two-way synchronization, via the interface component selection
//                    - Can model both interface and interconnect similar to a regular ac_channel
//                    - Serves as interconnect Ac_start/ac_done across hierarchies
//  Author:         Bala Sethuraman
//  Notes:
//         - When used under non-synthesis mode, virtual inheritance is necessary as flows (eg., SCVerify) and user testbench
//           can access the methods (eg., debug_size()),  in the ac_channel class.
//         - When used under synthesis mode, ac_sync available method matches builtin treatment of it
//         - In C Simulation, nb_write always returns true. So, 'else' branch based on the success of 'nb_write' is not exercisable in C simulation,
//           as the underlying buffer is unbounded in C model. But, in RTL, when mapped to two-way handshake component, both 'if' and 'else' branch 
//           are exercisable in RTL
//
*/

#ifndef __AC_SYNC_H
#define __AC_SYNC_H

#include <ac_channel.h>

#if defined(__SYNTHESIS__)
#define _VIRTUAL_INHERIT_ 
#define _INHERIT_MODE_ private
#define _PRAGMA_BUILTIN_ _Pragma("builtin")
#else
#define _VIRTUAL_INHERIT_ virtual
#define _INHERIT_MODE_ public
#define _PRAGMA_BUILTIN_ 
#endif

////////////////////////////////////////////////////////////////////////////////
// Class: ac_start
////////////////////////////////////////////////////////////////////////////////

_PRAGMA_BUILTIN_
class ac_start : _VIRTUAL_INHERIT_ _INHERIT_MODE_ ac_channel<bool>
{
public:
  typedef ac_channel<bool> Base;

  // constructor
  ac_start(): Base() { }

  template <typename ...T> 
  void sync_in(T &...t) {
    Base::read();
  }

  #ifdef __CONNECTIONS__CONNECTIONS_H__
  void bind(Connections::SyncIn  &c) { Base::bind(c); }
  #endif

private:
  // Prevent the compiler from autogenerating these.
  // This enforces that ac_start are always passed by reference.
  ac_start(const ac_start &);
  ac_start&operator=(const ac_start &);
};

////////////////////////////////////////////////////////////////////////////////
// Class: ac_done
////////////////////////////////////////////////////////////////////////////////

_PRAGMA_BUILTIN_
class ac_done : _VIRTUAL_INHERIT_ _INHERIT_MODE_ ac_channel<bool>
{
public:
  typedef ac_channel<bool> Base;

  // constructor
  ac_done(): Base() { }

  template <typename ...T> 
  void sync_out(T &...t) {
    write(true);
  }

  #ifdef __CONNECTIONS__CONNECTIONS_H__
  void bind(Connections::SyncOut &c) { Base::bind(c); }
  #endif

private:
  // Prevent the compiler from autogenerating these.
  // This enforces that ac_done are always passed by reference.
  ac_done(const ac_done &);
  ac_done&operator=(const ac_done &);

};

////////////////////////////////////////////////////////////////////////////////
// Class: ac_sync
////////////////////////////////////////////////////////////////////////////////

_PRAGMA_BUILTIN_
class ac_sync final : public ac_start, public ac_done
{
public:
  typedef ac_start Base1;
  typedef ac_done Base2;

  // constructor
  ac_sync(): Base1(), Base2() { }

#ifdef __SYNTHESIS__
  inline bool available( unsigned int cnt) {
    return true;
  }
#endif

  inline bool nb_sync_in() {
    bool rval = true;
    bool dummy_obj;
    rval = Base1::nb_read(dummy_obj); // During synthesis -- builtin treatment
    return rval;
  }

 inline bool nb_sync_out() {
    bool dummy_obj = true;
    return Base2::nb_write(dummy_obj);
  }

  #ifdef __CONNECTIONS__CONNECTIONS_H__
  void bind(Connections::SyncIn  &c) { Base1::bind(c); }
  void bind(Connections::SyncOut &c) { Base2::bind(c); }
  #endif

private:
  // Prevent the compiler from autogenerating these.
  // This enforces that ac_sync are always passed by reference.
  ac_sync(const ac_sync &);
  ac_sync &operator=(const ac_sync &);

};

////////////////////////////////////////////////////////////////////////////////
#undef _VIRTUAL_INHERIT_
#undef _INHERIT_MODE_
#undef _PRAGMA_BUILTIN_
////////////////////////////////////////////////////////////////////////////////
#endif

