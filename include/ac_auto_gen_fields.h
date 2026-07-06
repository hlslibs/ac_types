/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Datatypes                                          *
 *                                                                        *
 *  Software Version: 2026.2                                              *
 *                                                                        *
 *  Release Date    : Tue Jun 30 14:57:13 PDT 2026                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 2026.2.1                                            *
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

#ifndef _INCLUDED_AC_AUTO_GEN_FIELDS_H_
#define _INCLUDED_AC_AUTO_GEN_FIELDS_H_

#ifdef __clang__
#ifdef BOOST_PP_VARIADICS
#ifndef OK_BOOST_PASS
#error "For clang++ auto_gen_fields.h must be included before any other includes of boost headers"
#endif
#endif
#define BOOST_PP_VARIADICS 1
#define OK_BOOST_PASS 1
#endif

#include <boost/preprocessor/list/for_each.hpp>
#include <boost/preprocessor/tuple/to_list.hpp>
#include <ac_marshaller.h>

template <class T>
class ac_type_traits {
public:
  static constexpr bool is_array{false};
  static constexpr int  d1{0};
  using elem_type = T;

  template <bool is_marshalling, unsigned int Size, class S>
  static void Marshall(ac_marshaller<is_marshalling, Size>& m, S& A) {
    m & A ;
  }
};

template <class T, int D1>
class ac_type_traits<T[D1]>{
public:
  static constexpr bool is_array{true};
  static constexpr int  d1{D1};
  using elem_type = T;

  template <bool is_marshalling, unsigned int Size>
  static void Marshall(ac_marshaller<is_marshalling, Size>& m, elem_type A[d1]) {
    for (int i=0; i<d1; i++) {
      m & A [i];
    }
  }
};

template <class T, int D1, int D2>
class ac_type_traits<T[D1][D2]> {
public:
  static constexpr bool is_array{true};
  static constexpr int  d1{D1};
  static constexpr int  d2{D2};
  using elem_type = T;

  template <bool is_marshalling, unsigned int Size>
  static void Marshall(ac_marshaller<is_marshalling, Size>& m, elem_type A[d1][d2]) {
    for (int i1=0; i1<d1; i1++) 
      for (int i2=0; i2<d2; i2++) 
        m & A [i1][i2];
  }
};

template <class T>
class ac_calc_bit_width
{
public:
  static constexpr unsigned width = ac_wrapper<T>::width;
};

template <class T, int D1>
class ac_calc_bit_width<T[D1]>
{
public:
  static constexpr unsigned width = ac_calc_bit_width<T>::width * D1;
};


#define GEN_MARSHALL_FIELD(R, _, F) \
   ac_type_traits<decltype(F)>::Marshall(m, rhs.F); 
   //

#define GEN_MARSHALL_METHOD(FIELDS) \
template <bool is_marshalling, unsigned int Size> static void Marshall(ac_marshaller<is_marshalling, Size>& m, this_type &rhs) { \
     BOOST_PP_LIST_FOR_EACH(GEN_MARSHALL_FIELD, _, FIELDS); \
}

#define GEN_ADD_FIELD_WIDTH(R, _, F) \
   + ac_calc_bit_width<decltype(F)>::width

#define GEN_WIDTH(FIELDS) \
  static constexpr int width = 0 \
       BOOST_PP_LIST_FOR_EACH(GEN_ADD_FIELD_WIDTH, _, FIELDS); 

#define FIELD_LIST(X) BOOST_PP_TUPLE_TO_LIST(BOOST_PP_TUPLE_SIZE(X), X )

#define AUTO_GEN_FIELD_METHODS(THIS_TYPE, X) \
  using this_type = THIS_TYPE; \
  GEN_MARSHALL_METHOD(FIELD_LIST(X)) \
  GEN_WIDTH(FIELD_LIST(X))

#endif
