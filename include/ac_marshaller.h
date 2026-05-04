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
 *  Copyright  Siemens                                                *
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
#pragma once

#include <ac_int.h>
#include <ac_complex.h>
#include <ac_fixed.h>
#include <ac_float.h>
#include <ac_array.h>
#include <ac_std_float.h>

#ifndef AC_MARSHALL_LIMIT
#define AC_MARSHALL_LIMIT 10000
#endif

namespace ac_private_ {
inline void internal_assert(bool cond) {
#ifndef __SYNTHESIS__
  assert(cond);
#endif
}

template<typename A, int WIDTH>
inline void type_to_ac_uint(const A &data, ac_int<WIDTH, false> &vec) {
  ac_int<WIDTH, false> bv(data);
  vec = bv;
}

template<typename A, int WIDTH>
inline void ac_uint_to_type(const ac_int<WIDTH, false> &vec, A &data) {
  data = vec;
}

template<int Twidth, int Ibits, bool Signed, ac_q_mode Qmode, ac_o_mode Omode>
inline void type_to_ac_uint(const ac_fixed<Twidth,Ibits,Signed,Qmode,Omode> &in, ac_int<Twidth,false> &out) {
  out = in.template slc<Twidth>(0);
}

template<int Twidth, int Ibits, bool Signed, ac_q_mode Qmode, ac_o_mode Omode>
inline void ac_uint_to_type(const ac_int<Twidth,false> &in, ac_fixed<Twidth,Ibits,Signed,Qmode,Omode> &data) {
  data.set_slc(0, in);
}

template<>
inline void type_to_ac_uint(const ac::bfloat16 &in, ac_int<16,false> &out) {
  type_to_ac_uint(in.data(), out);  
}

template<>
inline void ac_uint_to_type(const ac_int<16,false> &in, ac::bfloat16 &result) {
  ac_int<16,true> data(in);
  result.set_data(data);
}

template<ac_ieee_float_format Format, int Width>
inline void type_to_ac_uint(const ac_ieee_float<Format> &in, ac_int<Width,false> &out) {
  auto t = in.data_ac_int();
  static_assert(t.width == Width);
  out = t;
}

template<ac_ieee_float_format Format, int Width>
inline void ac_uint_to_type(const ac_int<Width,false> &in, ac_ieee_float<Format> &result) {
  using std_float = typename ac_ieee_float<Format>::ac_std_float_t;
  std_float v;
  v.set_data(in);
  result = v;

  ac_int<Width,false> r = result.data_ac_int();
  ac_private_::internal_assert(r == in);
}

template<int W, int E>
inline void type_to_ac_uint(const ac_std_float<W,E> &in, ac_int<W, false> &rvec) {
  rvec = in.data();
}

template<int W, int E>
inline void ac_uint_to_type(const ac_int<W, false> &in, ac_std_float<W,E> &result) {
  result.set_data(in);
}

}

// if is_marshalling == True:
//    convert type to bits
// else:
//    convert bits to type
template <bool is_type_to_bits, unsigned W>
class ac_marshaller
{
};

//    convert bits to type
template <unsigned W>
class ac_marshaller<false, W>
{
  const ac_int<W, false> &bit_vector;
  unsigned cur_idx;
  static_assert(W < AC_MARSHALL_LIMIT, "Size must be less than MARSHALL_LIMIT");
public:
  ac_marshaller(const ac_int<W, false> &bv) : bit_vector(bv), cur_idx(0) {}

  template <typename T, int FieldSize>
  void AddField(T &d) {
    ac_int<FieldSize, false> bits = bit_vector.template slc<FieldSize> (cur_idx);
    ac_private_::ac_uint_to_type(bits, d);
    cur_idx += FieldSize;
  }
};

// convert type to bits
template <unsigned W>
class ac_marshaller<true, W>
{
  ac_int<W, false> bit_vector;
  unsigned cur_idx;
  static_assert(W < AC_MARSHALL_LIMIT, "Width must be less than AC_MARSHALL_LIMIT");
public:
  ac_marshaller() : bit_vector(0), cur_idx(0) {}

  template <typename T, int FieldSize>
  void AddField(const T &d) {
    ac_int<FieldSize, false> bits;
    ac_private_::type_to_ac_uint(d, bits);
    bit_vector.set_slc(cur_idx, bits);
    cur_idx += FieldSize;
  }
  
  ac_int<W, false> GetResult() {
    return bit_vector;
  }
};

template <typename T>
class ac_wrapper
{
public:
  static constexpr int width = T::width;
  template <bool is_type_to_bits, unsigned Size>
  static void Marshall(ac_marshaller<is_type_to_bits,Size> &m, T &rhs) {
    rhs.template Marshall<is_type_to_bits, Size>(m, rhs);
  }
};

template <bool is_type_to_bits, unsigned int Size, typename T>
ac_marshaller<is_type_to_bits, Size>& operator&(ac_marshaller<is_type_to_bits,Size> &m, T &rhs) {
  ac_wrapper<typename std::remove_reference<decltype(rhs)>::type>::Marshall(m, rhs);
  return m;
}

#define AC_MARSHALL_BASIC_TYPES(Type, Signed, Sizeof) \
template <>                                           \
class ac_wrapper<Type> {                              \
 public:                                              \
  static constexpr int width = Sizeof;                \
  template <bool is_type_to_bits, unsigned int Size>  \
  static void Marshall(ac_marshaller<is_type_to_bits,Size>& m, Type& rhs) { \
     m.template AddField<Type, Sizeof>(rhs);          \
  }                                                   \
};

AC_MARSHALL_BASIC_TYPES(bool,0,1);
AC_MARSHALL_BASIC_TYPES(char,1,8);
AC_MARSHALL_BASIC_TYPES(unsigned char,0,8);
AC_MARSHALL_BASIC_TYPES(short,1,16);
AC_MARSHALL_BASIC_TYPES(unsigned short,0,16);
AC_MARSHALL_BASIC_TYPES(int,1,32);
AC_MARSHALL_BASIC_TYPES(unsigned int,0, 32);
AC_MARSHALL_BASIC_TYPES(long,1,sizeof(long)*8);
AC_MARSHALL_BASIC_TYPES(unsigned long,0,sizeof(unsigned long)*8);
AC_MARSHALL_BASIC_TYPES(long long,1,64);
AC_MARSHALL_BASIC_TYPES(unsigned long long,0,64);

template <int Width, bool Sign>
class ac_wrapper<ac_int<Width, Sign> > {
public:
  static constexpr int width = Width;
  template <bool is_type_to_bits, unsigned int Size>
  static void Marshall(ac_marshaller<is_type_to_bits,Size>& m, ac_int<Width, Sign> & rhs) {
    m.template AddField<ac_int<Width, Sign>, Width>(rhs);
  }
};

template <int Width, int IWidth, bool Sign, ac_q_mode Q, ac_o_mode O>
class ac_wrapper<ac_fixed<Width, IWidth, Sign, Q, O> > {
public:
  static constexpr int width = Width;
  template <bool is_type_to_bits, unsigned Size>
  static void Marshall(ac_marshaller<is_type_to_bits,Size>& m, ac_fixed<Width,IWidth,Sign,Q,O> &rhs) {
    m.template AddField<ac_fixed<Width,IWidth,Sign,Q,O>, width>(rhs);
  }
};

#define AC_SPECIAL_FLOAT_WRAPPER(Type, W) \
template <>                               \
class ac_wrapper<Type > {                 \
public:                                   \
 static constexpr int width = W;          \
 static_assert(Type::width == W);         \
 template <bool is_type_to_bits, unsigned int Size> \
 static void Marshall(ac_marshaller<is_type_to_bits,Size>& m, Type &rhs) { \
   m.template AddField<Type, W> (rhs);    \
 }                                        \
};

AC_SPECIAL_FLOAT_WRAPPER(ac_ieee_float<binary16>, 16);
AC_SPECIAL_FLOAT_WRAPPER(ac_ieee_float<binary32>, 32);
AC_SPECIAL_FLOAT_WRAPPER(ac_ieee_float<binary64>, 64);
AC_SPECIAL_FLOAT_WRAPPER(ac_ieee_float<binary128>, 128);
AC_SPECIAL_FLOAT_WRAPPER(ac_ieee_float<binary256>, 256);
AC_SPECIAL_FLOAT_WRAPPER(ac::bfloat16, 16);

template <int W, int E>
class ac_wrapper<ac_std_float<W,E> >
{
public:
  static constexpr int width = W;
  template <bool is_type_to_bits, unsigned int Size>
  static void Marshall(ac_marshaller<is_type_to_bits,Size> &m, ac_std_float<W,E> &rhs) {
    m.template AddField<ac_std_float<W,E>, W > (rhs);
  }  
};

template <int W, int I, int E, ac_q_mode Q>
class ac_wrapper<ac_float<W,I,E,Q> >
{
public:
  using Type = ac_float<W,I,E,Q>;
  static constexpr int width = Type::width + Type::e_width;
  template <bool is_type_to_bits, unsigned int Size>
  static void Marshall(ac_marshaller<is_type_to_bits,Size> &m, ac_float<W,I,E,Q> &rhs) {
    m.template AddField<typename Type::mant_t, Type::width>(rhs.m);
    m.template AddField<typename Type::exp_t, Type::e_width>(rhs.e);
  }
};

template <class T>
class ac_wrapper<ac_complex<T>>
{
public:
  static constexpr int width = 2 * ac_wrapper<T>::width;
  template <bool is_type_to_bits, unsigned int Size>
  static void Marshall(ac_marshaller<is_type_to_bits,Size> &m, ac_complex<T> &rhs) {
    m.template AddField<T, ac_wrapper<T>::width>(rhs._r);
    m.template AddField<T, ac_wrapper<T>::width>(rhs._i);
  }
};

template<typename T>
class ac_wrapper<ac_array<T,0,0,0> >
{
public:
  static constexpr int width = ac_wrapper<T>::width;
  template <bool is_type_to_bits, unsigned int Size>
  static void Marshall(ac_marshaller<is_type_to_bits,Size> &m, ac_array<T,0,0,0> &rhs) {
    m.template AddField<T, ac_wrapper<T>::width>(rhs[0]);
  }
};

template <bool is_type_to_bits, unsigned int Size, typename T>
ac_marshaller<is_type_to_bits,Size>& operator&(ac_marshaller<is_type_to_bits,Size> &m, ac_array<T,0,0,0> &rhs)
{
  ac_wrapper< ac_array<T,0,0,0> >::Marshall(m, rhs);
  return m;
}

template<typename T, unsigned D1, unsigned D2, unsigned D3>
class ac_wrapper<ac_array<T, D1, D2, D3> >
{
public:
  using FType = ac_array<T, D2, D3>;
  using WType = ac_wrapper<FType>;  
  static constexpr int width = WType::width * D1;
  template <bool is_type_to_bits, unsigned int Size>
  static void Marshall(ac_marshaller<is_type_to_bits,Size> &m, ac_array<T, D1, D2, D3> &rhs) {
    for (unsigned i = 0; i < D1; i++) {
      m & rhs[i];
    }
  }
};

template <bool is_type_to_bits, unsigned int Size, typename T, int D1, int D2, int D3>
ac_marshaller<is_type_to_bits,Size>& operator&(ac_marshaller<is_type_to_bits,Size> &m, ac_array<T,D1,D2,D3> &rhs)
{
  ac_wrapper< ac_array<T,D1,D2,D3> >::Marshall(m, rhs);
  return m;
}

namespace ac_private_ {
template <class T>
T convert_from_ac_int(const ac_int<ac_wrapper<T>::width, false> &bv) {
  ac_marshaller<false,ac_wrapper<T>::width> marshaller(bv);
  T val;
  ac_wrapper<T>::Marshall(marshaller, val);
  return val;
};

template <class T>
ac_int<ac_wrapper<T>::width, false> convert_to_ac_int(const T &v) {
  ac_marshaller<true,ac_wrapper<T>::width> marshaller;
  ac_wrapper<T>::Marshall(marshaller, const_cast<T &> (v));
  return marshaller.GetResult();
}
}
                         
//-----------------------------------------------------------------------------------------
template <typename T, class V = void>
struct ac_bv_conv {
  static constexpr int width = ac_wrapper<T>::width;
  static ac_int<width, false> type_to_bv(const T &obj) {
    ac_private_::internal_assert(ac_private_::convert_to_ac_int(obj) == ac_private_::convert_to_ac_int(bv_to_type(ac_private_::convert_to_ac_int(obj))));
    return ac_private_::convert_to_ac_int(obj);
  }

  static T bv_to_type(const ac_int<width, false> &acd) {
    return ac_private_::convert_from_ac_int<T> (acd);
  }
};

template <int WIDTH, bool Sign>
class ac_bv_conv<ac_int<WIDTH, Sign>, void > {
public:
  static constexpr int width = WIDTH;
  static ac_int<width, false> type_to_bv(const ac_int<width, Sign> &obj) {
    ac_private_::internal_assert(ac_private_::convert_to_ac_int(obj) == ac_private_::convert_to_ac_int(bv_to_type(ac_private_::convert_to_ac_int(obj))));
    return obj;
  }

  static ac_int<width, Sign> bv_to_type(const ac_int<width, false> &bv) {
    return ac_int<width, Sign> (bv);
  }
};

template <int Width, int IWidth, bool Sign, ac_q_mode Q, ac_o_mode O>
class ac_bv_conv<ac_fixed<Width, IWidth, Sign, Q, O> , void> {
public:
  static constexpr int width = Width;
  using T = ac_fixed<Width, IWidth, Sign, Q, O>;

  static ac_int<width, false> type_to_bv(const T &obj) {
    ac_private_::internal_assert(ac_private_::convert_to_ac_int(obj) == ac_private_::convert_to_ac_int(bv_to_type(ac_private_::convert_to_ac_int(obj))));
    return ac_private_::convert_to_ac_int(obj);
  }

  static T bv_to_type(const ac_int<width, false> &acd) {
    return ac_private_::convert_from_ac_int<T> (acd);
  }
};

template <class T>
class ac_bv_conv<ac_complex<T>, void> {
public:
  static constexpr int width = 2 * ac_bv_conv<T, void>::width;

  static ac_int<width, false> type_to_bv(const ac_complex<T> &obj) {
    ac_private_::internal_assert(ac_private_::convert_to_ac_int(obj) == ac_private_::convert_to_ac_int(bv_to_type(ac_private_::convert_to_ac_int(obj))));
    return ac_private_::convert_to_ac_int(obj);
  }
    
  static ac_complex<T> bv_to_type(const ac_int<width, false> &acd) {
    return ac_private_::convert_from_ac_int<ac_complex<T>>(acd);
  }
};

template <int W, int I, int E, ac_q_mode Q>
class ac_bv_conv<ac_float<W,I,E,Q> , void> {
public:
  using Type = ac_float<W,I,E,Q>;
  static constexpr int width = Type::width + Type::e_width;

  static ac_int<width, false> type_to_bv(const Type &obj) {
    ac_private_::internal_assert(ac_private_::convert_to_ac_int(obj) == ac_private_::convert_to_ac_int(bv_to_type(ac_private_::convert_to_ac_int(obj))));
    ac_int<width, false> ret = ac_private_::convert_to_ac_int(obj);
    return ret;
  }
  
  static Type bv_to_type(const ac_int<width, false> &acd) {
    return ac_private_::convert_from_ac_int<Type>(acd);
  }  
};

template <int W, int E>
class ac_bv_conv<ac_std_float<W,E> , void> {
public:
  using Type = ac_std_float<W,E>;
  static constexpr int width = W;

  static ac_int<width, false> type_to_bv(const Type &obj) {
    ac_private_::internal_assert(ac_private_::convert_to_ac_int(obj) == ac_private_::convert_to_ac_int(bv_to_type(ac_private_::convert_to_ac_int(obj))));
    return ac_private_::convert_to_ac_int(obj);
  }
  
  static Type bv_to_type(const ac_int<width, false> &acd) {
    return ac_private_::convert_from_ac_int<Type>(acd);
  }
};

#define AC_BV_SPECIAL_FLOATS(TYPE, WIDTH)                   \
template <>                                                 \
class ac_bv_conv<TYPE , void> {                             \
public:                                                     \
  static constexpr int width = WIDTH;                       \
  static ac_int<width, false> type_to_bv(const TYPE &in) {  \
    ac_private_::internal_assert(ac_private_::convert_to_ac_int(in) == ac_private_::convert_to_ac_int(bv_to_type(ac_private_::convert_to_ac_int(in)))); \
    return ac_private_::convert_to_ac_int(in);              \
  }                                                         \
  static TYPE bv_to_type(const ac_int<width, false> &acd) { \
    return ac_private_::convert_from_ac_int<TYPE>(acd);     \
  }                                                         \
};

AC_BV_SPECIAL_FLOATS(ac_ieee_float<binary16>, 16);
AC_BV_SPECIAL_FLOATS(ac_ieee_float<binary32>, 32);
AC_BV_SPECIAL_FLOATS(ac_ieee_float<binary64>, 64);
AC_BV_SPECIAL_FLOATS(ac_ieee_float<binary128>, 128);
AC_BV_SPECIAL_FLOATS(ac_ieee_float<binary256>, 256);
AC_BV_SPECIAL_FLOATS(ac::bfloat16, 16);

#define AC_BV_BASIC_TYPES(Type, Signed, Sizeof)             \
template <>                                                 \
class ac_bv_conv<Type, void> {                              \
public:                                                     \
  static constexpr int width = Sizeof;                      \
  static ac_int<width, false> type_to_bv(const Type &rhs) { \
    ac_int<width, false> bv(rhs);                           \
    return bv;                                              \
  }                                                         \
                                                            \
  static Type bv_to_type(const ac_int<width, false> &acd) { \
    return acd;                                             \
  }                                                         \
};

AC_BV_BASIC_TYPES(bool,0,1);
AC_BV_BASIC_TYPES(char,1,8);
AC_BV_BASIC_TYPES(unsigned char,0,8);
AC_BV_BASIC_TYPES(short,1,16);
AC_BV_BASIC_TYPES(unsigned short,0,16);
AC_BV_BASIC_TYPES(int,1,32);
AC_BV_BASIC_TYPES(unsigned int,0, 32);
AC_BV_BASIC_TYPES(long,1,sizeof(long)*8);
AC_BV_BASIC_TYPES(unsigned long,0,sizeof(unsigned long)*8);
AC_BV_BASIC_TYPES(long long,1,64);
AC_BV_BASIC_TYPES(unsigned long long,0,64);

template<typename T>
class ac_bv_conv<ac_array<T,0,0,0> , void>
{
public:
  static constexpr int width = ac_bv_conv<T, void>::width;  
  using Type = ac_array<T,0,0,0>;

  static ac_int<width, false> type_to_bv(const Type &obj) {
    ac_private_::internal_assert(ac_private_::convert_to_ac_int(obj) == ac_private_::convert_to_ac_int(bv_to_type(ac_private_::convert_to_ac_int(obj))));
    return ac_private_::convert_to_ac_int(obj);
  }
  
  static Type bv_to_type(const ac_int<width, false> &acd) {
    return ac_private_::convert_from_ac_int<Type>(acd);
  }
};

template<typename T, unsigned D1, unsigned D2, unsigned D3>
class ac_bv_conv<ac_array<T, D1, D2, D3>, void>
{
public:
  using FType = ac_array<T, D2, D3>;
  using WType = ac_bv_conv<FType, void>;  
  static constexpr int width = WType::width * D1;
  using Type = ac_array<T, D1, D2, D3>;

  static ac_int<width, false> type_to_bv(const Type &obj) {
    ac_private_::internal_assert(ac_private_::convert_to_ac_int(obj) == ac_private_::convert_to_ac_int(bv_to_type(ac_private_::convert_to_ac_int(obj))));
    return ac_private_::convert_to_ac_int(obj);
  }

  static Type bv_to_type(const ac_int<width, false> &acd) {
    return ac_private_::convert_from_ac_int<Type>(acd);
  }
};
