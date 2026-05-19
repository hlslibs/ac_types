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
 * @file hls_stream.h
 * @brief Implementation of hls::stream object.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#ifndef __HLS_STREAM_H
#define __HLS_STREAM_H

#include "ac_channel.h"

namespace hls
{
   template <typename T, int DEPTH = 0>
   class stream : public ac_channel<T>
   {
    public:
      using element_type = T;
      using base_type = ac_channel<T>;

      stream() : base_type()
      {
      }

      stream(const char*) : base_type()
      {
      }

#if !defined(__BAMBU__) || defined(__BAMBU_SIM__)
      stream(const stream&) = default;
      stream(int init) : ac_channel<T>(init)
      {
      }
      stream(int init, T val) : ac_channel<T>(init, val)
      {
      }
      stream(std::initializer_list<T> val) : ac_channel<T>(val)
      {
      }
      stream& operator=(const stream&) = default;
#endif

      ~stream() = default;

      void operator>>(T& rdata)
      {
         this->read(rdata);
      }

      void operator<<(const T& wdata)
      {
         this->write(wdata);
      }

      bool full()
      {
         return DEPTH == 0 ? false : (DEPTH == this->size());
      }

      bool read_nb(T& head)
      {
         return this->nb_read(head);
      }

      bool write_nb(T& tail)
      {
         return this->nb_write(tail);
      }

      bool write_nb(const T& tail)
      {
         T tail_copy = tail;
         return write_nb(tail_copy);
      }

      void set_name(const char*)
      {
      }

    private:
#if defined(__BAMBU__) && !defined(__BAMBU_SIM__)
      stream(const stream&) = delete;
      stream& operator=(const stream&) = delete;
#endif
   };
} // namespace hls
#endif
