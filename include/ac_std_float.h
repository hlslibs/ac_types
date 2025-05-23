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
 *  Copyright 2018-2022 Siemens                                                *
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

/*  Source:         ac_std_float.h
 *  Description:    class for floating point operation handling in C++
 *  Author:         Andres Takach, Ph.D.

Overview: this header defines three classes

  ac_ieee_float<Format>
    Meant to store floats in IEEE standard binary format
    Format indicate width:
      binary16: (half float) uses short
      binary32: (float) uses int
      binary64: (double) uses array of long long with one element
      binary128: (long double in some platforms) uses array of long long with two elements
      binary256: uses array of long long with four elements

  ac::bfloat16
    Implements Google's tensorflow::bfloat16
    Stores data as "short"

  ac_std_float<W,E>
    Superset of ac_ieee_float in that any bit width and exponent width is
      allowed
    This is used by ac_ieee_float and ac::bfloat16

    Uses an ac_int<W,true> that holds the bit pattern for a standard (IEEE) style binary
    float:
         1) sign-magnitude representation, sign is MSB
         2) mantissa (significand) with implied bit for normal numbers
         3) E is not restricted to IEEE widths, another class ac_ieee_float does that

    Provides easy way to conver to/from the closest covering ac_float:
      Constructor from ac_float
        Most two negative exponents of ac_float are not representable: shift
          significand futher to the right (for now no attempt to round)
        Most negative mantissa of ac_float (in two's complement) when converted
          to sign-magnitute requires a right shift (add +1 to exponent)
          If exponent is already max, two alternatives:
            - "saturate" (store most negative number)
            - Store as -Inf  (currently this option not available)
        Exponent is offset
        Mantissa implied bit is removed from normal numbers

      Explicit convertion to_ac_float
        Ignores exceptions (Inf, NaN)
        Does inverse as above to obtain ac_float
*/

#ifndef __AC_STD_FLOAT_H
#define __AC_STD_FLOAT_H
#include <ac_float.h>
#include <cstring>
// Inclusion of cmath undefs all macros such as signbit etc that some parsers may define for C
#include <cmath>

#ifdef __SYNTHESIS__
#ifdef AC_IEEE_FLOAT_USE_BUILTIN
#undef AC_IEEE_FLOAT_USE_BUILTIN
#endif
#endif

#ifndef AC_STD_FLOAT_IEEE754_ROUND_OVERRIDE
#define AC_STD_FLOAT_IEEE754_ROUND AC_RND_CONV
#else
#define AC_STD_FLOAT_IEEE754_ROUND AC_STD_FLOAT_IEEE754_ROUND_OVERRIDE
#endif

#ifndef AC_STD_FLOAT_BFLOAT16_ROUND_OVERRIDE
#define AC_STD_FLOAT_BFLOAT16_ROUND AC_TRN_ZERO
#else
#define AC_STD_FLOAT_BFLOAT16_ROUND AC_STD_FLOAT_BFLOAT16_ROUND_OVERRIDE
#endif

#ifdef __AC_NAMESPACE
namespace __AC_NAMESPACE {
#endif

// For now make data members public since SCVerify needs it
//#ifdef __AC_MAKE_PRIVATE_DATA_PUBLIC
#if 1
#define __AC_DATA_PRIVATE public:
#else
#define __AC_DATA_PRIVATE private:
#endif

namespace ac_private {
  template<bool cond>
  struct check_rounding { enum {Only_symmetrical_roundings_or_truncations_supported}; };
  template<> struct check_rounding<false> {};

  template<ac_q_mode Q>
  void check_supported() {
    // only symmetrical roundings supported
    const bool supported = (Q==AC_RND_CONV) | (Q==AC_TRN_ZERO) | (Q==AC_RND_INF) | (Q == AC_RND_CONV_ODD);
#if __cplusplus > 199711L
    static_assert(supported, "Only symmetrical roundings/truncations supported");
#else
    (void) check_rounding<supported>::Only_symmetrical_roundings_or_truncations_supported;
#endif
  }

  template<bool cond>
  struct check_rounding2 { enum {Only_round_to_even_supported_when_using_BUILTIN}; };
  template<> struct check_rounding2<false> {};

  template<ac_q_mode Q>
  void check_supported2() {
#ifdef AC_IEEE_FLOAT_USE_BUILTIN
    const bool supported = Q==AC_RND_CONV;
#if __cplusplus > 199711L
    static_assert(supported, "Only round to even supported");
#else
    (void) check_rounding2<supported>::Only_round_to_even_supported_when_using_BUILTIN;
#endif
#endif
  }

  template<bool PosWidth, bool Constrained>
  struct LeadingSignConstrained {
    template<int MaxLShift, int W, bool S>
    static unsigned val(const ac_int<W,S> &x, bool &all_sign) {
      all_sign = x == ac_int<1,S>(x[0]);
      return 0;
    }
  };
  template<> struct LeadingSignConstrained<true,false> {
    template<int MaxLShift, int W, bool S>
    static unsigned val(const ac_int<W,S> &x, bool &all_sign) {
      return x.leading_sign(all_sign);
    }
  };
  template<> struct LeadingSignConstrained<true,true> {
    template<int MaxLShift, int W, bool S>
    static unsigned val(const ac_int<W,S> &x, bool &all_sign) {
      const int lsW = MaxLShift+S;
      ac_int<lsW,S> x_h = x >> (W-lsW);
      ac_int<W-lsW,S> x_l = x;
      unsigned ls = x_h.leading_sign(all_sign);
      all_sign &= x_l == ac_int<1,S>(x[W-1]);
      return ls;
    }
  };
#if (defined(__GNUC__) & ( (__GNUC__ == 4) & (__GNUC_MINOR__ >= 6) | (__GNUC__ > 4) ) & !defined(__EDG__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbool-compare"
#endif
  template<int MaxLShift, int W, bool S>
  unsigned leading_sign_constrained(const ac_int<W,S> &x, bool &all_sign) {
    const bool pos_width = MaxLShift > S;
    const bool constrained = MaxLShift < W-S;
    return LeadingSignConstrained<pos_width,constrained>::template val<MaxLShift>(x, all_sign);
  }
}
#if (defined(__GNUC__) & ( (__GNUC__ == 4) & (__GNUC_MINOR__ >= 6) | (__GNUC__ > 4) ) & !defined(__EDG__))
#pragma GCC diagnostic pop
#endif

namespace ac {
#ifdef __SYNTHESIS__
  #pragma hls_design ccore
  #pragma hls_ccore_type sequential
#endif
  template<int W>
  void fx_div(ac_int<W,false> const & op1, ac_int<W,false> const & op2, ac_int<W+2,false> &quotient, bool &exact) {
    ac_int<W+2,true> R = op1;
    ac_int<W,false> D = op2;
    ac_int<W+2,false> Q = 0;
    for(int i=W+1; i >= 0; i--) {
      // take MSB of N, shift it in from right to R
      ac_int<W+2,true> nextR = R - D;
      Q <<= 1;
      if (nextR[W] == 0)
        Q += 1;
      else
        nextR = R;
      R = nextR << 1;
    }
    quotient = Q;
    exact = !R;
  }

  template<int W>
  void fx_div_sim(ac_int<W,false> op1, ac_int<W,false> op2, ac_int<W+2,false> &quotient, bool &exact) {
    // need to compute extra rnd bit,
    //   +2 because we may need to shift left by 1 (mant divisor > mant dividend)
    ac_int<2*W+1,false> op1_mi = op1;
    op1_mi <<= W+1;
    // +1 bit to compute rnd bit
    quotient = (op1_mi / op2);
    exact = !(op1_mi % op2);
  }

#ifdef __SYNTHESIS__
  #pragma hls_design ccore
  #pragma hls_ccore_type sequential
#endif
  template<int W, int WR>
  bool fx_sqrt( ac_int<W,false> x, ac_int<WR,false> &sqrt) {
    // x is ac_fixed<W,2,false>, sqrt is ac_fixed<WR,1,false>
    const bool W_odd = W&1;
    const int ZW = W + W_odd;  // make it even
    ac_int<ZW,false> z = x;
    z <<= W_odd;
    // masks used only to hint synthesis on precision
    ac_int<WR+2,true> mask_d = -1;
    ac_int<WR+2,false> d = 0;
    ac_int<WR,false> r = 0;
    unsigned int z_shift = ZW-2;
    for(int i = WR-1; i >= 0; i--) {
      r <<= 1;
      mask_d <<= 2;
      d = (~mask_d & (d << 2)) | ((z >> z_shift) & 0x3 );
      ac_int<WR+2,false> t = d - (( ((ac_int<WR+1,false>)r) << 1) | 0x1);
      if( !t[WR+1] ) {  // since t is unsigned, look at MSB
        r |= 0x1;
        d = ~mask_d & t;
      }
      z <<= 2;
    }

    bool rem = (d != 0) | ((z >> 2*W) != 0);
    sqrt = r;
    return rem;
  }
}

#ifndef AC_STD_FLOAT_FX_DIV_OVERRIDE
#ifdef __SYNTHESIS__
#define AC_STD_FLOAT_FX_DIV_OVERRIDE ac::fx_div
#else
#define AC_STD_FLOAT_FX_DIV_OVERRIDE ac::fx_div_sim
#endif
#endif

template<int W, int E> class ac_std_float;

#ifdef __AC_NAMESPACE
}
#endif

#ifdef AC_STD_FLOAT_OVERRIDE_NAMESPACE
#define AC_STD_FLOAT_OVERRIDE_NS ::AC_STD_FLOAT_OVERRIDE_NAMESPACE::
namespace AC_STD_FLOAT_OVERRIDE_NAMESPACE {
#ifdef __AC_NAMESPACE
  using __AC_NAMESPACE::ac_q_mode;
  using __AC_NAMESPACE::ac_std_float;
#endif
#else
#define AC_STD_FLOAT_OVERRIDE_NS
#endif

#ifdef AC_STD_FLOAT_ADD_OVERRIDE
template<ac_q_mode QR, bool No_SubNormals, int W, int E>
ac_std_float<W,E> AC_STD_FLOAT_ADD_OVERRIDE(const ac_std_float<W,E> &op, const ac_std_float<W,E> &op2);
#endif

#ifdef AC_STD_FLOAT_MULT_OVERRIDE
template<ac_q_mode QR, bool No_SubNormals, int W, int E>
ac_std_float<W,E> AC_STD_FLOAT_MULT_OVERRIDE(const ac_std_float<W,E> &op, const ac_std_float<W,E> &op2);
#endif

#ifdef AC_STD_FLOAT_DIV_OVERRIDE
template<ac_q_mode QR, bool No_SubNormals, int W, int E>
ac_std_float<W,E> AC_STD_FLOAT_DIV_OVERRIDE(const ac_std_float<W,E> &op, const ac_std_float<W,E> &op2);
#endif

#ifdef AC_STD_FLOAT_FMA_OVERRIDE
template<ac_q_mode QR, bool No_SubNormals, int W, int E>
ac_std_float<W,E> AC_STD_FLOAT_FMA_OVERRIDE(const ac_std_float<W,E> &op, const ac_std_float<W,E> &op2, const ac_std_float<W,E> &op3);
#endif

#ifdef AC_STD_FLOAT_SQRT_OVERRIDE
template<ac_q_mode QR, bool No_SubNormals, int W, int E>
ac_std_float<W,E> AC_STD_FLOAT_SQRT_OVERRIDE(const ac_std_float<W,E> &op);
#endif

#ifdef AC_STD_FLOAT_OVERRIDE_NAMESPACE
}
#endif

#ifdef __AC_NAMESPACE
namespace __AC_NAMESPACE {
#endif

namespace ac {
  inline void copy_bits(float a, float *b) { *b = a; }
  inline void copy_bits(double a, double *b) { *b = a; }

  inline void copy_bits(short a, short *b) { *b = a; }
  inline void copy_bits(const ac_int<16,true> &a, short *b) { *b = (short) a.to_int(); }
  inline void copy_bits(short a, ac_int<16,true> *b) { *b = ac_int<16,true>(a); }
  inline void copy_bits(int a, int *b) { *b = a; }
  inline void copy_bits(const ac_int<32,true> &a, int *b) { *b = a.to_int(); }
  inline void copy_bits(int a, ac_int<32,true> *b) { *b = ac_int<32,true>(a); }
  inline void copy_bits(long long a, long long *b) { *b = a; }
  inline void copy_bits(const ac_int<64,true> &a, long long *b) { *b = a.to_int64(); }
  inline void copy_bits(long long a, ac_int<64,true> *b) { *b = ac_int<64,true>(a); }
  inline void copy_bits(const long long a[2], long long (*b)[2]) {
    (*b)[0] = a[0];
    (*b)[1] = a[1];
  }
  inline void copy_bits(const ac_int<128,true> &a, long long (*b)[2]) {
    (*b)[0] = a.to_int64();
    (*b)[1] = a.slc<64>(64).to_int64();
  }
  inline void copy_bits(const long long a[2], ac_int<128,true> *b) {
    *b = 0;
    b->set_slc(0,ac_int<64,true>(a[0]));
    b->set_slc(64,ac_int<64,true>(a[1]));
  }
  inline void copy_bits(const long long a[4], long long (*b)[4]) {
    (*b)[0] = a[0];
    (*b)[1] = a[1];
    (*b)[2] = a[2];
    (*b)[3] = a[3];
  }
  inline void copy_bits(const ac_int<256,true> &a, long long (*b)[4]) {
    (*b)[0] = a.to_int64();
    (*b)[1] = a.slc<64>(64).to_int64();
    (*b)[2] = a.slc<64>(128).to_int64();
    (*b)[3] = a.slc<64>(192).to_int64();
  }
  inline void copy_bits(const long long a[4], ac_int<256,true> *b) {
    *b = 0;
    b->set_slc(0,ac_int<64,true>(a[0]));
    b->set_slc(64,ac_int<64,true>(a[1]));
    b->set_slc(128,ac_int<64,true>(a[2]));
    b->set_slc(192,ac_int<64,true>(a[3]));
  }
  inline void copy_bits(float f, int *x);
  inline void copy_bits(double f, long long *x);
  inline void copy_bits(int x, float *f);
  inline void copy_bits(long long x, double *f);

  inline void copy_bits(float f, ac_int<32,true> *x) {
    int x_i;
    copy_bits(f, &x_i);
    *x = x_i;
  }
  inline void copy_bits(double f, ac_int<64,true> *x) {
    long long x_i;
    copy_bits(f, &x_i);
    *x = x_i;
  }
  inline void copy_bits(const ac_int<32,true> &x, float *f) { copy_bits(x.to_int(), f); }
  inline void copy_bits(const ac_int<64,true> &x, double *f) { copy_bits(x.to_int64(), f); }
}

enum ac_ieee_float_format { binary16, binary32, binary64, binary128, binary256};

// Forward declarations for ac_ieee_float and bfloat16
template<ac_ieee_float_format Format>
class ac_ieee_float;
namespace ac {
  class bfloat16;
}

namespace ac_private {
  template<int W, int E>
  struct rt_closed_T<ac_std_float<W,E>,ac_std_float<W,E> > {
    typedef ac_std_float<W,E> type;
  };
  template<ac_ieee_float_format Format>
  struct rt_closed_T<ac_ieee_float<Format>,ac_ieee_float<Format> > {
    typedef ac_ieee_float<Format> type;
  };
  template<>
  struct rt_closed_T<ac::bfloat16,ac::bfloat16> {
    typedef ac::bfloat16 type;
  };
}

template<int W, int E>
class ac_std_float {
__AC_DATA_PRIVATE
  ac_int<W,true> d;
public:
  template<typename T>
  struct rt_T {
    typedef typename ac_private::rt_closed_T<ac_std_float,T>::type mult;
    typedef typename ac_private::rt_closed_T<ac_std_float,T>::type plus;
    typedef typename ac_private::rt_closed_T<ac_std_float,T>::type minus;
    typedef typename ac_private::rt_closed_T<ac_std_float,T>::type minus2;
    typedef typename ac_private::rt_closed_T<ac_std_float,T>::type logic;
    typedef typename ac_private::rt_closed_T<ac_std_float,T>::type div;
    typedef typename ac_private::rt_closed_T<ac_std_float,T>::type div2;
  };
  struct rt_unary {
    typedef ac_std_float neg;
    typedef ac_std_float mag_sqr;
    typedef ac_std_float mag;
  };
  static const int width = W;
  static const int e_width = E;
  static const int mant_bits = W - E - 1;
  static const int exp_bias = (1 << (E-1)) - 1;
  static const int min_exp = -exp_bias + 1;
  static const int max_exp = exp_bias;
  static const int mu_bits = mant_bits + 1;
private:
  typedef ac_int<mu_bits,false> mu_t;
  typedef ac_int<mu_bits+1,false> mu1_t;
  typedef ac_int<mu_bits+2,false> mu2_t;
  typedef ac_int<mu_bits+1,true> m_t;   // mantissa in two's complement representation
public:
  typedef ac_int<E,true> e_t;
  typedef ac_int<E,false> eu_t;
  typedef ac_float<width-e_width+1,2,e_width,AC_RND_CONV> ac_float_t;
  static ac_std_float nan() {
    ac_std_float r;
    r.d = 0;
    r.d.set_slc(mant_bits-1, ac_int<e_width+1,true>(-1));
    return r;
  }
  static ac_std_float inf() {
    ac_std_float r;
    r.d = 0;
    r.d.set_slc(mant_bits, ac_int<e_width,true>(-1));
    return r;
  }
  static ac_std_float denorm_min() {   // smallest positive non zero value (subnorm if supported)
    ac_std_float r;
    r.d = 1;
    return r;
  }
  static ac_std_float min() {   // smallest NORMAL positive non zero value
    ac_std_float r;
    r.d = 0;
    r.d[width-1-e_width] = true;
    return r;
  }
  static ac_std_float max() {   // largest pos finite value
    ac_std_float r;
    r.d = -1;
    r.d[width-1] = false;
    r.d[width-1-e_width] = false;
    return r;
  }
  static ac_std_float epsilon() {
    ac_int<e_width,true> exp = -mant_bits + exp_bias;
    ac_std_float r;
    r.d = 0;
    r.d.set_slc(mant_bits, exp);
    return r;
  }
  ac_std_float() {}
  ac_std_float(const ac_std_float &f) : d(f.d) {}
  template<int WR, ac_q_mode QR>
  ac_std_float<WR,E> convert() const {
    ac_private::check_supported<QR>();
    ac_std_float<WR,E> r;
    if(W <= WR) {
      r.d = 0;
      r.d.set_slc(WR-W, d);
    } else {
      typedef ac_std_float<WR,E> r_t;
      const int r_mant_bits = r_t::mant_bits;
      const int r_mu_bits = r_t::mu_bits;
      e_t f_e = d.template slc<E>(mant_bits);
      bool f_normal = !!f_e;
      mu_t mu = d;
      mu[mant_bits] = f_normal;
      ac_fixed<r_mu_bits+1,mu_bits+1,false,QR> r_rnd = mu;
      bool rnd_ovf = r_rnd[r_mu_bits];
      ac_int<r_mant_bits,false> m_r = r_rnd.template slc<r_mant_bits>(0);
      e_t e_r = f_e + rnd_ovf;
      r.d = m_r;
      r.d.set_slc(r_mant_bits, e_r);
      r.d[WR-1] = d[W-1];
    }
    return r;
  }

  template<int WFX, int IFX, bool SFX, ac_q_mode QFX, ac_o_mode OFX>
  ac_fixed<WFX,IFX,SFX,QFX,OFX> convert_to_ac_fixed(bool map_inf=false) const {
    static const bool need_rnd_bit = QFX != AC_TRN;
    static const bool need_rem_bits = need_rnd_bit & (QFX != AC_RND);
    static const bool need_ovf = OFX != AC_WRAP;
    static const int t_width = AC_MAX(mu_bits+1, WFX+!SFX) + need_rnd_bit + need_ovf;

    bool f_sign, f_normal, f_zero, f_inf, f_nan;
    mu_t f_mu;
    e_t f_e;
    extract(f_mu, f_e, f_sign, f_normal, f_zero, f_inf, f_nan);
    if(map_inf & f_inf) {
      ac_fixed<WFX,IFX,SFX,QFX,OFX> rv;
      if(f_sign)
        rv.template set_val<AC_VAL_MIN>();
      else
        rv.template set_val<AC_VAL_MAX>();
      return rv;
    }
    AC_ASSERT(!f_inf & !f_nan, "Expects finite float (not Nan or Inf)");
    m_t f_m = f_sign ? m_t(-f_mu) : m_t(f_mu);
    typedef ac_int<t_width,true> t_t;
    typedef ac_int<t_width+need_rem_bits,true> t2_t;
    t_t t = f_m;
    t <<= need_rnd_bit;
    static const int lsb_src = -mant_bits;
    static const int lsb_trg = IFX-WFX;
    int rshift = lsb_trg - lsb_src - (int)f_e;

    bool sticky_bit_rnd = false;
    bool rshift_neg = rshift < 0;
    if(need_rem_bits) {
      t_t shifted_out_bits = t;
      typedef ac_int< ac::template nbits< AC_MAX(lsb_trg - lsb_src - min_exp,1) >::val, false> shift_ut;
      shifted_out_bits &= ~(t_t(0).bit_complement() << (shift_ut) rshift);
      sticky_bit_rnd = !!shifted_out_bits & !rshift_neg;
    }
    bool ovf = false;
    if(need_ovf) {
      t_t shifted_out_bits = t < 0 ? t_t(~t) : t;
      // shift right by -rshift + 1
      //   +1 is OK since added extra MSB
      typedef ac_int< ac::template nbits< AC_MAX(-(lsb_trg - lsb_src - max_exp + 1),1) >::val, false> shift_ut;
      shifted_out_bits &= ~((t_t(0).bit_complement() >> 2) >> (shift_ut) ~rshift);
      ovf = !!shifted_out_bits & rshift_neg;
    }

    t >>= rshift;

    t[t_width-1] = t[t_width-1] ^ (ovf & (t[t_width-1] ^ f_sign));
    t[t_width-2] = t[t_width-2] ^ (ovf & (t[t_width-2] ^ !f_sign));
    t2_t t2 = t;
    if(need_rem_bits) {
      t2 <<= 1;
      t2[0] = t2[0] | sticky_bit_rnd;
    }

    ac_fixed<WFX,WFX+need_rnd_bit+need_rem_bits,SFX,QFX,OFX> ri = t2;
    ac_fixed<WFX,IFX,SFX,QFX,OFX> r = 0;
    r.set_slc(0,ri.template slc<WFX>(0));
    return r;
  }

  template<int WI, bool SI>
  ac_int<WI,SI> convert_to_ac_int(bool map_inf=false) const {
    return convert_to_ac_fixed<WI,WI,SI,AC_TRN_ZERO,AC_WRAP>(map_inf).to_ac_int();
  }
  int convert_to_int(bool map_inf=false) const {
    return convert_to_ac_int<32,true>(map_inf).to_int();
  }
  long long convert_to_int64(bool map_inf=false) const {
    return convert_to_ac_int<64,true>(map_inf).to_int64();
  }

  template<int W2>
  explicit ac_std_float(const ac_std_float<W2,E> &f) {
    *this = f.template convert<W,AC_RND_CONV>();
  }
  template<int WR, int ER, ac_q_mode QR>
  ac_std_float<WR,ER> convert() const {
    ac_private::check_supported<QR>();
    typedef ac_std_float<WR,ER> r_t;
    typedef typename r_t::e_t r_e_t;
    int const r_mu_bits = r_t::mu_bits;
    int const r_mant_bits = r_t::mant_bits;
    int const r_min_exp = r_t::min_exp;
    int const r_max_exp = r_t::max_exp;
    int const r_exp_bias = r_t::exp_bias;
    bool f_sign, f_normal, f_zero, f_inf, f_nan;
    mu_t f_mu;
    e_t f_e;
    r_t r;
    extract(f_mu, f_e, f_sign, f_normal, f_zero, f_inf, f_nan);
    int exp = f_e;
    ac_fixed<r_mu_bits+1, mu_bits+1,false,QR> r_rnd;
    if(ER >= E) {
      if((ER > E) & !f_normal) {
        int ls = f_mu.leading_sign();
        int max_shift_left = f_e - r_min_exp + 1;
        bool shift_exponent_limited = ls >= max_shift_left;
        int shift_l = shift_exponent_limited ? max_shift_left : ls;
        f_mu <<= shift_l;
        exp -= shift_l;
      }
      r_rnd = f_mu;
    } else {
      int shift_r = r_min_exp - f_e;
      typedef ac_fixed<r_mu_bits+1,mu_bits,false> t_t;
      t_t r_t = f_mu;
      bool sticky_bit = !!(f_mu & ~((~mu_t(0)) << mant_bits-r_mant_bits-1));
      if(shift_r > 0) {
        t_t shifted_out_bits = r_t;
        shifted_out_bits &= ~((~t_t(0)) << shift_r);
        sticky_bit |= !!shifted_out_bits;
        r_t >>= shift_r;
        exp += shift_r;
      }
      ac_fixed<r_mu_bits+2, mu_bits,false> r_t2 = r_t;
      r_t2[0] = sticky_bit;
      r_rnd = r_t2;
    }
    bool rnd_ovf = r_rnd[r_mu_bits];
    ac_int<r_mant_bits,false> r_m = r_rnd.template slc<r_mant_bits>(0);
    bool r_normal = r_rnd[r_mant_bits] | rnd_ovf;
    exp += rnd_ovf;
    bool f_conv_inf = !f_inf & (exp > r_max_exp); // treated differently than f_inf for AC_TRN_ZERO
    bool exception = f_inf | f_nan | ((QR != AC_TRN_ZERO) & f_conv_inf);
    r_e_t r_e = exception ? -1 : (f_zero | !r_normal) ? 0 : ((QR==AC_TRN_ZERO) & f_conv_inf) ? ~1 : exp + r_exp_bias;
    if(exception) {
      r_m = 0;
      r_m[r_mant_bits-1] = f_nan;
    }
    if(QR==AC_TRN_ZERO) {
      r_m |= ac_int<1,true>(-f_conv_inf);
    }
    r.d = r_m;
    r.d.set_slc(r_mant_bits, r_e);
    r.d[WR-1] = d[W-1];
    return r;
  }
  template<int W2,int E2>
  explicit ac_std_float(const ac_std_float<W2,E2> &f) {
    *this = f.template convert<W,E,AC_RND_CONV>();
  }
  template<ac_ieee_float_format Format>
  explicit ac_std_float(const ac_ieee_float<Format> &f);

  explicit ac_std_float(const ac::bfloat16 &f);

  template<ac_q_mode Q>
  explicit ac_std_float(const ac_float<mu_bits+1,2,E,Q> &f) {
    bool sign = f.mantissa() < 0;
    m_t m_s = f.m.template slc<mu_bits+1>(0);
    mu1_t m_u = sign ? (mu1_t) -m_s : (mu1_t) m_s;
    bool most_neg_m = m_u[mu_bits];
    bool is_max_exp = f.exp() == (1 << (E-1)) - 1;
    ac_int<E,true> e = f.exp() + exp_bias + (most_neg_m & !is_max_exp);
    mu_t m = m_u | ac_int<1,true>(most_neg_m & is_max_exp);
    m[mant_bits] = m[mant_bits] | most_neg_m;
    bool exp_dont_map = !e | e==-1;
    m >>= !e;
    m >>= 2*(e==-1);
    // exp_dont_map guarantees subnornal => e = 0
    e &= ac_int<1,true>(!exp_dont_map & !!m);
    d = m.template slc<mant_bits>(0);
    d.set_slc(mant_bits, e);
    d[W-1] = sign;
  }
  template<ac_q_mode Q, int WFX, int IFX, bool SFX, ac_q_mode QFX, ac_o_mode OFX>
  void assign_from(const ac_fixed<WFX,IFX,SFX,QFX,OFX> &fx) {
    ac_private::check_supported<Q>();
    bool sign = SFX & (fx < 0.0);
    ac_int<WFX,SFX> x = fx.template slc<WFX>(0);
    const int max_shift_left = IFX-SFX-1-min_exp;
    typedef ac_int<WFX,false> x_u_t;
    x_u_t x_u = sign ? (x_u_t) -x : (x_u_t) x;
    bool r_normal;
    bool r_inf;
    int exp = IFX-SFX-1;
    ac_int<mant_bits,false> m_r;
    if(max_shift_left >= 0) {
      bool all_sign = false;
      unsigned shift_l = ac_private::leading_sign_constrained<max_shift_left>(x,all_sign);
      x_u <<= shift_l;
      exp -= shift_l;
      // after shift of abs value, MSB should be 0 with exception of 1 followed by 0s
      bool ovf_most_neg = SFX & x_u[WFX-1];
      bool ovf = ovf_most_neg;
      if(((Q == AC_RND_CONV) | (Q == AC_RND_INF)) & (WFX-SFX > mu_bits)) {
        // Check whether rounding would trigger overflow
        mu1_t t = x_u.template slc<mu_bits+1>((WFX-SFX) - mu_bits - 1);
        ovf |= t == mu1_t(-1);
      }
      ac_fixed<mu_bits,WFX-SFX,false,Q> r_rnd;
      r_rnd = (ac_int<WFX-SFX,false>)x_u;
      m_r = r_rnd.template slc<mant_bits>(0);
      exp += ovf;
      r_normal = r_rnd[mu_bits-1] | ovf;
      bool fx_zero = all_sign & !sign;
      r_inf = (exp > max_exp) & !fx_zero;
      exp += exp_bias;
      if(Q==AC_TRN_ZERO) {
        exp = r_inf ? max_exp + exp_bias : exp;
        m_r |= ac_int<1,true>(-r_inf);  // saturate (set all bits to 1) if r_inf
        r_inf = false;
      }
    } else {
      bool sticky_bit = false;
      typedef ac_int<mu_bits+2+SFX,false> h_t;
      h_t x_bef_rnd;
      if(WFX-SFX > mu_bits+2) {
        sticky_bit = !!(x_u << mu_bits+2);
        x_bef_rnd = x_u >> ((WFX-SFX)-(mu_bits+2));
      } else {
        x_bef_rnd = x_u;
        x_bef_rnd <<= (mu_bits+2)-(WFX-SFX);
      }
      int shift_l = max_shift_left;
      unsigned shift_r_m1 = ~shift_l;
      h_t shifted_out_bits = x_bef_rnd;
      shifted_out_bits &= ~((~h_t(1)) << shift_r_m1);
      x_bef_rnd >>= shift_r_m1;
      x_bef_rnd >>= 1;
      sticky_bit |= !!shifted_out_bits;
      x_bef_rnd[0] = x_bef_rnd[0] | sticky_bit;

      ac_fixed<mu_bits,mu_bits+2,false,Q> r_rnd = x_bef_rnd;
      m_r = r_rnd.template slc<mant_bits>(0);
      bool ovf_most_neg = SFX & x_bef_rnd[mu_bits+2+SFX-1];
      r_normal = r_rnd[mu_bits-1] | ovf_most_neg;
      r_inf = false;
      exp = r_normal;
    }
    e_t e = r_inf ? -1 : r_normal ? exp : 0;
    m_r &= ac_int<1,true>(!r_inf);
    e &= ac_int<1,true>(r_normal);
    d = m_r;
    d.set_slc(mant_bits, e);
    d[W-1] = sign;
  }
  template<ac_q_mode Q, int WI, bool SI>
  void assign_from(const ac_int<WI,SI> &x) {
    this->template assign_from<Q>(ac_fixed<WI,WI,SI>(x));
  }
  template<int WFX, int IFX, bool SFX, ac_q_mode QFX, ac_o_mode OFX>
  explicit ac_std_float(const ac_fixed<WFX,IFX,SFX,QFX,OFX> &fx) {
    assign_from<AC_RND_CONV>(fx);
  }
  template<int WI, bool SI>
  explicit ac_std_float(const ac_int<WI,SI> &x) {
    assign_from<AC_RND_CONV>(x);
  }
  explicit ac_std_float(float f) {
    const int w_bits = sizeof(f)*8;
    const int m_bits = std::numeric_limits<float>::digits;
    const int e_bits = w_bits - m_bits;
    ac_int<w_bits,true> t_i;
    ac::copy_bits(f, &t_i);
    ac_std_float<w_bits,e_bits> t;
    t.set_data(t_i);
    *this = ac_std_float(t);
  }
  explicit ac_std_float(double f) {
    const int w_bits = sizeof(f)*8;
    const int m_bits = std::numeric_limits<double>::digits;
    const int e_bits = w_bits - m_bits;
    ac_int<w_bits,true> t_i;
    ac::copy_bits(f, &t_i);
    ac_std_float<w_bits,e_bits> t;
    t.set_data(t_i);
    *this = ac_std_float(t);
  }
  explicit ac_std_float(int x) {
    *this = ac_std_float(ac_fixed<32,32,true>(x));
  }
  explicit ac_std_float(long long x) {
    *this = ac_std_float(ac_fixed<64,64,true>(x));
  }
  const ac_int<W,true> &data() const { return d; }
  const ac_int<W,true> &data_ac_int() const { return d; }
  void set_data(const ac_int<W,true> &data, bool assert_on_nan=false, bool assert_on_inf=false) {
    d = data;
    if(assert_on_nan)
      AC_ASSERT(!isnan(), "Float is NaN");
    if(assert_on_inf)
      AC_ASSERT(!isinf(), "Float is Inf");
  }
  int fpclassify() const {
    ac_int<E,true> e = d.template slc<E>(mant_bits);
    if(e) {
      if(e == -1)
        return !(ac_int<mant_bits,false>)d ? FP_INFINITE : FP_NAN;
      else
        return FP_NORMAL;
    }
    else
      return !(ac_int<mant_bits,false>)d ? FP_ZERO : FP_SUBNORMAL;
  }
  bool isfinite() const {
    ac_int<E,true> e = d.template slc<E>(mant_bits);
    return e != -1;
  }
  bool isnormal() const {
    ac_int<E,true> e = d.template slc<E>(mant_bits);
    return (e | !(ac_int<mant_bits,false>)d) & (e != -1);
  }
  bool isnan() const {
    if(isfinite())
      return false;
    ac_int<mant_bits,false> m = d;
    return !!m;
  }
  bool isinf() const {
    if(isfinite())
      return false;
    ac_int<mant_bits,false> m = d;
    return !m;
  }
  const ac_float<mant_bits+2,2,E,AC_RND_CONV> to_ac_float() const {
    ac_int<E,true> e = d.template slc<E>(mant_bits);
    bool normal = !!e;
    bool sign = d[W-1];
    bool inf = e == -1;
    ac_int<mant_bits,false> m = d;
    ac_int<mant_bits+1,false> m1 = m;
    m1[mant_bits] = normal;
    ac_int<mant_bits+2,true> m_s = sign ? -m1 : (ac_int<mant_bits+2,true>) m1;
    ac_fixed<mant_bits+2,2,true> fx = 0;
    fx.set_slc(0, m_s);
    e -= exp_bias;
    // if number is subnormal, e will be MIN_EXP + 1 (10...01), but it needs to be
    //   MIN_EXP + 2  (10...010)
    e[0] = e[0] & normal;
    e[1] = e[1] | !normal;
    // normalization by at most 2 places
    bool shiftl1 = !(fx[mant_bits+1] ^ fx[mant_bits]);
    bool shiftl2 = shiftl1 & !(fx[mant_bits+1] ^ fx[mant_bits-1]);
    fx <<= shiftl1;
    fx <<= shiftl2;
    e -= shiftl1 + shiftl2;
    e = inf ? value<AC_VAL_MAX>(e) : e;
    fx = inf ? (sign ? value<AC_VAL_MIN>(fx) : value<AC_VAL_MAX>(fx)) : fx;
    return ac_float<mant_bits+2,2,E,AC_TRN>(fx, e, false);
  }
  template<ac_ieee_float_format Format>
  ac_ieee_float<Format> to_ac_ieee_float() const;
  float to_float() const {
    ac_std_float<32,8> t(*this);
    float f;
    ac::copy_bits(t.d, &f);
    return f;
  }
  double to_double() const {
    ac_std_float<64,11> t(*this);
    double f;
    ac::copy_bits(t.d, &f);
    return f;
  }
  inline static std::string type_name() {
    std::string r = "ac_std_float<";
    r += ac_int<32,true>(W).to_string(AC_DEC);
    r += ",";
    r += ac_int<32,true>(E).to_string(AC_DEC);
    r += ">";
    return r;
  }
private:
  void extract(mu_t &m, e_t &e, bool &sign, bool &normal, bool &zero, bool &inf, bool &nan, bool biased_exp=false, bool no_subnormals=false) const {
    e = d.template slc<E>(mant_bits);
    bool exception = e == -1;
    normal = !!e | no_subnormals;
    m = d;
    bool m_zero = !m.template slc<mant_bits>(0);
    zero = (!e) & (no_subnormals | m_zero);
    m[mant_bits] = !!e;
    if(!biased_exp) {
      e -= exp_bias;
      e += !normal;
    }
    sign = d[W-1];
    inf = exception & m_zero;
    nan = exception & !m_zero;
  }
public:
  static ac_std_float zero() {
    ac_std_float r;
    r.d = 0;
    return r;
  }
  static ac_std_float one() {
    ac_std_float r;
    r.d = 0;
    r.d.set_slc(mant_bits, ac_int<E,false>(exp_bias));
    return r;
  }
  template<ac_q_mode QR, bool No_SubNormals, bool Effective_Add=false>
  ac_std_float add_generic(const ac_std_float &op2) const {
    ac_private::check_supported<QR>();

    // extract exponents/mantissas/flags
    e_t op1_e, op2_e;
    bool op1_normal, op1_sign, op1_zero, op2_normal, op2_sign, op2_zero;
    bool op1_inf, op1_nan, op2_inf, op2_nan;
    mu_t op1_mu, op2_mu;
    extract(op1_mu, op1_e, op1_sign, op1_normal, op1_zero, op1_inf, op1_nan, true, No_SubNormals);
    op2.extract(op2_mu, op2_e, op2_sign, op2_normal, op2_zero, op2_inf, op2_nan, true, No_SubNormals);

    // compute abs(op1_e - op2_e)
    ac_int<E,false> op1_e_b = ac_int<E,false>(op1_e);
    ac_int<E,false> op2_e_b = ac_int<E,false>(op2_e);
    int e_dif1 = op1_e_b - op2_e_b;
    bool e1_lt_e2 = (e_dif1 < 0);
    bool e1_eq_e2 = (op1_e_b == op2_e_b);
    //unsigned e_dif = e1_lt_e2 ? e_dif2 : e_dif1;
    eu_t e_dif = op1_e_b < op2_e_b ? op2_e_b - op1_e_b : op1_e_b - op2_e_b;


    // find bigger & smaller operands
    // get rid of implicit bit to have comparison computed faster : case where this bit differ have e1_eq_e2=false so result is not taken into account
    ac_int<mant_bits,false> mant_bits1 = op1_mu;
    ac_int<mant_bits,false> mant_bits2 = op2_mu;
    bool ma1_lt_ma2 = (mant_bits1 < mant_bits2);
    bool op1_smaller = e1_lt_e2 | (e1_eq_e2 & ma1_lt_ma2);
    // align mantissa instead of correcting exp (a mux is faster than an adder)
    if (!op1_normal)
      op1_mu <<= 1;
    else if (No_SubNormals & op1_zero)
      op1_mu = 0;
    if (!op2_normal)
      op2_mu <<= 1;
    else if (No_SubNormals & op2_zero)
      op2_mu = 0;
    mu_t op_bigger = op1_smaller ? op2_mu : op1_mu;
    mu_t op_smaller = op1_smaller ? op1_mu : op2_mu;
    int exp = op1_smaller ? op2_e_b : op1_e_b;

    typedef ac_int<ac::nbits<mu_bits+1>::val,false> e_dif_sat_t;
    // saturate e_dif (if bigger than mu_bits, result will be the same as mu_bits+1 because shift will be bigger than number of bits)
    //unsigned e_dif_sat = (e_dif > mu_bits) ? mu_bits+1 : e_dif;
    e_dif_sat_t e_dif_sat = ac_fixed<ac::nbits<mu_bits+1>::val,ac::nbits<mu_bits+1>::val,false,AC_TRN,AC_SAT>(e_dif).to_ac_int();

    bool m1_eq_m2 = (op1_mu == op2_mu);
    bool op_sign = op1_smaller & op2_sign;
    op_sign |= !op1_smaller & op1_sign;
    op_sign = (op_sign & !(e1_eq_e2 & m1_eq_m2)) | (op1_sign & op2_sign);
    bool r_sign = Effective_Add ? op1_sign : op_sign;
    bool r_normal = true;
    bool r_inf = false;

    bool do_sub = !Effective_Add && (op1_sign != op2_sign);

    // compute add/sub of mantissa
    // we know result will be positive

    // int value is mu_bits+1 to keep carry-out of adder
    // decimal value is 3 bits : guard / round / sticky
    typedef ac_fixed<mu_bits+1+3,mu_bits+1,false> add_GRS_t;
    add_GRS_t res_mant;
    // perform res_mant = op_smaller / 2^e_dif
    res_mant = op_smaller;
    ac_int<mu_bits+1+2,false> sticky_bit_mask = -1;
    sticky_bit_mask <<= e_dif_sat;
    sticky_bit_mask = ~sticky_bit_mask;
    sticky_bit_mask >>= 2;
    bool sticky_bit = !!(op_smaller & sticky_bit_mask); // remove 2bits because we ignore guard and round bits
    res_mant >>= e_dif_sat;
    res_mant[0] = sticky_bit;

    // perform : res_mant = do_sub ? op_bigger - res_mant : op_bigger + res_mant
    if (do_sub)
      res_mant = ~res_mant;
    res_mant += add_GRS_t(op_bigger) + (add_GRS_t(do_sub)>>3);

    // compute left shift and res exponent
    bool all_sign = false;
    typedef ac_int<ac::nbits<mu_bits+1+3>::val,false> ls_t; 
    ls_t ls = Effective_Add ? (ls_t) !res_mant[mu_bits+3] : (ls_t) res_mant.leading_sign(all_sign);
    bool r_zero = Effective_Add ? (bool) op1_zero & op2_zero : all_sign; // we know res_mant >= 0, so all_sign means equal zero
    int resexp = exp - ls;
    bool incr = 0;
    if (resexp < 0) {
      ls = exp;
      exp = -1;
      r_normal = false;
    } else {
      exp = resexp;
      r_normal = true;
    }
    res_mant <<= ls;

    // do rounding
    typedef ac_fixed<mu_bits+1,mu_bits+2,false,QR> add_rounded_t;
    add_rounded_t res_rounded = res_mant;
    if (res_rounded[mu_bits]) { // overflow in rounding
      res_rounded >>= 1;
      incr = 1;
      r_normal |= resexp == -1;
    }
    exp += incr + 1;
    r_inf = exp > max_exp + exp_bias + 1;
    bool exp_max = eu_t(exp) == max_exp + exp_bias+1;
    r_zero |= No_SubNormals & !r_normal;

    ac_int<mant_bits,false> m_r;
    m_r = res_rounded.template slc<mant_bits>(0);

    // special case when AC_TRN_ZERO : infinity is replaced by max value
    if ((r_inf|exp_max) & (QR==AC_TRN_ZERO)) {
      exp = max_exp + exp_bias; // saturate res
      r_inf = false;
      m_r |= ac_int<1,true>(-1);  // saturate (set all bits to 1)
    }

    // compute flags and assign result
    bool r_nan = op1_nan | op2_nan | ((op1_inf & op2_inf) & (op1_sign ^ op2_sign));
    r_inf |= op1_inf | op2_inf;
    bool exception = r_nan | r_inf;
    ac_int<E,true> e_r = exp;
    if(exception | r_zero)
      e_r = ac_int<E,true>(-1)*exception;
    exception |= (exp_max & (QR!=AC_TRN_ZERO));
    if(exception | r_zero)
      m_r = ac_int<mant_bits,false>(-1)*r_nan;
    ac_int<W,true> d_r = m_r;
    d_r.set_slc(mant_bits, e_r);
    d_r[W-1] = r_sign;
    ac_std_float r;
    r.set_data(d_r);
    return r;
  }
  template<ac_q_mode QR, bool No_SubNormals, bool Effective_Add=false>
  ac_std_float add(const ac_std_float &op2) const {
#ifndef AC_STD_FLOAT_ADD_OVERRIDE
    return add_generic<QR,No_SubNormals,Effective_Add>(op2);
#else
    return AC_STD_FLOAT_OVERRIDE_NS AC_STD_FLOAT_ADD_OVERRIDE<QR,No_SubNormals>(*this, op2);
#endif
  }
  template<ac_q_mode QR, bool No_SubNormals>
  ac_std_float sub(const ac_std_float &op2) const {
    return add<QR,No_SubNormals>(-op2);
  }
  template<ac_q_mode QR, bool No_SubNormals>
  ac_std_float mult_generic(const ac_std_float &op2) const {
    ac_private::check_supported<QR>();
    e_t op1_e, op2_e;
    bool op1_normal, op1_sign, op1_zero, op2_normal, op2_sign, op2_zero;
    bool op1_inf, op1_nan, op2_inf, op2_nan;
    mu_t op1_mu, op2_mu;

    // extract exponents/mantissas/flags
    extract(op1_mu, op1_e, op1_sign, op1_normal, op1_zero, op1_inf, op1_nan, true, No_SubNormals);
    op2.extract(op2_mu, op2_e, op2_sign, op2_normal, op2_zero, op2_inf, op2_nan, true, No_SubNormals);

    if (op1_normal)
      op1_mu[mant_bits] = 1;
    if (op2_normal)
      op2_mu[mant_bits] = 1;

    bool r_sign = op1_sign ^ op2_sign;
    bool r_inf = op1_inf | op2_inf; // r_nan takes precedence later on
    bool r_zero = op1_zero | op2_zero;  // r_nan takes precedence later on
    bool r_nan = op1_nan | op2_nan | (r_inf & r_zero);

    // compute raw results exp and p
    int exp = ac_int<E,false>(op1_e) + ac_int<E,false>(op2_e) + !op1_normal + !op2_normal - exp_bias;
    typedef ac_int<2*mu_bits,false> mant_mult_t;
    mant_mult_t p = op1_mu * op2_mu;

    // sticky bits are all the bits that will not be taken into account into result (except for rounding)
    // guard bit is the bit just before. If multiplication has msb to 1 then it will be part of sticky bits
    // sticky bit is used to choose between
    // -> XXX.1000 which will round to XXX
    // -> XXX.1010 which will round to XXX+1
    //         ^^^ those 3 bits are the sticky bits

    ac_int<ac::nbits<mu_bits>::val,false> shift_left = 0;
    ac_int<ac::nbits<mu_bits>::val,false> shift_right = 0;
    bool do_shift_left = true;
    bool do_shift_left_1 = exp != 0;
    // correct exponent in case of subnormal operand
    // compute shift
    if (!No_SubNormals) {
      // if both op1 and op2 are not normal, we know result will be zero by far, using only 1 op for leading sign is enough
      // real computation would be op1_mu.leading_sign() + op2_mu.leading_sign()
      shift_left = (unsigned) (op1_normal ? op2_mu : op1_mu).leading_sign();
      int minus_exp = ~exp;// same as -exp-1
      // saturate shift_right
      //shift_right = minus_exp > ((1<<ac::nbits<mu_bits>::val)-1) ? mu_bits+1 : minus_exp;
      //shift_right = minus_exp > mu_bits ? mu_bits+1 : minus_exp;
      shift_right = ac_fixed<ac::nbits<mu_bits>::val,ac::nbits<mu_bits>::val,false,AC_TRN,AC_SAT>(minus_exp).to_ac_int();
      int exp_minus_shift_left = exp - shift_left;
      do_shift_left = exp >= 0;
      do_shift_left_1 = exp_minus_shift_left > 0;
      if (exp_minus_shift_left < 0)
        shift_left = exp; // correct shift_left as it is bigger than exp
      if (exp_minus_shift_left < 0)
        exp = 0;
      else
        exp = exp_minus_shift_left;
    }

    ac_fixed<mu_bits+2,mu_bits,false> res_bef_rnd;
    bool e_incr = false;
    // pre-compute exp+1
    int exp_plus_1 = exp + 1;
    if (do_shift_left) { // or no shift at all
      p <<= shift_left;

      mant_mult_t sticky_bit_mask = ~(mant_mult_t(-1)<<(mu_bits-2));
      mant_mult_t guard_bit_mask = mant_mult_t(1)<<(mu_bits-2);
      bool sticky_bit = !!(p & sticky_bit_mask);
      bool guard_bit = !!(p & guard_bit_mask);

      // compute rounding overflow
      mant_mult_t mant_bit_mask = (~(mant_mult_t(-1)<<(mu_bits)))<<(mu_bits-1);
      bool p_msb_all_one = (p & mant_bit_mask) == mant_bit_mask;
      bool p_msb_one = p[2*mu_bits-1];

      p_msb_all_one &= p_msb_one | !do_shift_left_1 | guard_bit;
      sticky_bit |= guard_bit & (p_msb_one | !do_shift_left_1);
      bool rnd_ovf = ((QR == AC_RND_CONV) | (QR == AC_RND_INF)) & p_msb_all_one;
      e_incr |= rnd_ovf; // overflow in rounding, we know r_rnd will be "1000...000" so no need for shift because "1000...000"[msb-1:1] == "1000...000"[msb-2:0]

      if (p_msb_one)
        e_incr = true;
      else if (do_shift_left_1)
        p <<= 1;
      res_bef_rnd = p >> (mu_bits-1);
      res_bef_rnd >>= 1;
      res_bef_rnd[0] = sticky_bit;
    } else {
      mant_mult_t sticky_bit_mask;
      // we know sticky_bit_mask lsb will be "111...111"
      // code below equivalent to sticky_bit_mask = ~((mant_mult_t(-1)<<mu_bits)<<shift_right);
      ac_int<mu_bits, false> sticky_bit_mask_msb = -1;
      sticky_bit_mask_msb <<= shift_right;
      sticky_bit_mask_msb = ~sticky_bit_mask_msb;
      sticky_bit_mask = sticky_bit_mask_msb;
      sticky_bit_mask <<= mu_bits;
      sticky_bit_mask |= ac_int<mu_bits, false>(-1);
      bool sticky_bit = !!(p & sticky_bit_mask);

      res_bef_rnd = p >> mu_bits;
      res_bef_rnd >>= 1;
      res_bef_rnd >>= shift_right;
      res_bef_rnd[0] = sticky_bit;
      // we know there will be no rounding overflow here because p is small enough
    }

    // do rounding
    ac_fixed<mu_bits+1,mu_bits+1,false,QR> r_rnd;
    r_rnd = res_bef_rnd;

    // compute flags and adjust result
    bool exp_ovf = (exp_plus_1 > max_exp + exp_bias + 1) // easy compute as max_exp+exp_bias+2 is a  power of 2
                | (e_incr & (exp == max_exp + exp_bias));

    bool r_normal = !( (!e_incr & (exp == 0)) | (exp < 0) );
    r_inf |= (exp_ovf & (QR!=AC_TRN_ZERO));
    bool zero_m = r_zero
               | (No_SubNormals & !r_normal);

    if (r_nan | r_inf)
      exp = max_exp + exp_bias + 1;
    else if (exp_ovf & (QR==AC_TRN_ZERO))
      exp = max_exp + exp_bias; // saturate res
    else if (zero_m)
      exp = 0;
    else if (e_incr)
      exp = exp_plus_1;

    ac_int<mant_bits,false> m_r = r_rnd.template slc<mant_bits>(0);
    if (r_nan | r_inf | zero_m | (exp_ovf & (QR!=AC_TRN_ZERO))) {
      m_r = 0;
      m_r[mant_bits-1] = r_nan;
    }
    else if (exp_ovf & (QR==AC_TRN_ZERO))
      m_r |= ac_int<1,true>(-1);  // saturate (set all bits to 1)
    ac_int<E,true> e_r = exp;
    ac_int<W,true> d_r = m_r;
    d_r.set_slc(mant_bits, e_r);
    d_r[W-1] = r_sign;
    ac_std_float r;
    r.set_data(d_r);
    return r;
  }
  template<ac_q_mode QR, bool No_SubNormals>
  ac_std_float mult(const ac_std_float &op2) const {
#ifndef AC_STD_FLOAT_MULT_OVERRIDE
    return mult_generic<QR,No_SubNormals>(op2);
#else
    return AC_STD_FLOAT_OVERRIDE_NS AC_STD_FLOAT_MULT_OVERRIDE<QR,No_SubNormals>(*this, op2);
#endif
  }
  template<ac_q_mode QR, bool No_SubNormals>
  ac_std_float div_generic(const ac_std_float &op2) const {
    ac_private::check_supported<QR>();
    e_t op1_e, op2_e;
    bool op1_normal, op1_sign, op1_zero, op2_normal, op2_sign, op2_zero;
    bool op1_inf, op1_nan, op2_inf, op2_nan;
    mu_t op1_mu, op2_mu;
    extract(op1_mu, op1_e, op1_sign, op1_normal, op1_zero, op1_inf, op1_nan, true, No_SubNormals);
    op2.extract(op2_mu, op2_e, op2_sign, op2_normal, op2_zero, op2_inf, op2_nan, true, No_SubNormals);
    bool r_sign = op1_sign ^ op2_sign;
    bool r_nan = op1_nan | op2_nan | (op1_zero & op2_zero) | (op1_inf & op2_inf);
    bool r_zero = op1_zero | op2_inf;
    int ls_op1 = No_SubNormals ? 0 : (unsigned) op1_mu.leading_sign();
    op1_mu <<= ls_op1;
    int ls_op2 = No_SubNormals ? 0 : (unsigned) op2_mu.leading_sign();
    op2_mu <<= ls_op2;
    int exp = ac_int<E,false>(op1_e) - ac_int<E,false>(op2_e) + !op1_normal - !op2_normal - ls_op1 + ls_op2 + exp_bias;
    ac_int<mu_bits+2,false> q0 = 0;
    bool exact = true;
    bool div_by_zero = op2_zero;
    ac_int<1,false> q_shift_amount;
    bool exp_lt_0 = (exp < 0);
    bool exp_eq_0 = (exp == 0);
    bool exp_eq_1 = ((exp>>1) == 0);
    bool exp_neq_0_or_1 = (!exp_lt_0 & !exp_eq_1);
    if (No_SubNormals) {
      q_shift_amount = !exp_eq_0;
      if (exp_lt_0) 
        r_zero = true;
      if (exp_eq_0) 
        exp = 1;
    } 
#ifdef __SYNTHESIS__
    div_by_zero = false;
#endif
    if(!div_by_zero) {
      AC_STD_FLOAT_FX_DIV_OVERRIDE(op1_mu, op2_mu, q0, exact);
    }
    ac_int<mu_bits+3,false> q = q0;
    int shift_r = min_exp + exp_bias - exp;
    bool sticky_bit = !exact;
    bool shift_l = !q[mu_bits+1]; 
    if (!No_SubNormals) {
      q <<= 1;
      q_shift_amount = 0;
      if (shift_r >= 0) {
        typedef ac_int<mu_bits+3,false> t_t;
        t_t shifted_out_bits = q;
        shifted_out_bits &= ~((~t_t(0)) << shift_r);
        sticky_bit |= !!shifted_out_bits;
        q >>= shift_r;
        exp += shift_r;
      } else {
        q_shift_amount = shift_l;
      }
    }
    ac_int<mu_bits+3,false> q1 = 0;
    if (q_shift_amount) {
      if (No_SubNormals & (exp_neq_0_or_1 & shift_l)) {
        q1.set_slc(2, q.template slc<mu_bits+1>(0));
      } else {
        q1.set_slc(1, q.template slc<mu_bits+2>(0));
      }
    } else {
      q1 = q;
    }
    q = q1; 
    q[0] = q[0] | sticky_bit;
    ac_fixed<mu_bits+1,mu_bits+4,false,QR> r_rnd = q;
    bool rnd_ovf = r_rnd[mu_bits];
    ac_int<mant_bits,false> m_r = r_rnd.template slc<mant_bits>(0);
    bool r_normal = r_rnd[mant_bits] | rnd_ovf;
    r_zero |= !r_normal & No_SubNormals;
    exp += rnd_ovf;
    exp -= shift_l;
    bool r_inf0 = op1_inf | op2_zero;  // this is not affected by rounding
    bool r_inf = ((!r_zero) & ((exp > max_exp + exp_bias + 1) | (exp == max_exp + exp_bias + 1))) | r_inf0;
    if((QR==AC_TRN_ZERO) & !r_inf0) {
      exp = r_inf ? max_exp + exp_bias : exp;
      m_r |= ac_int<1,true>(-r_inf);  // saturate (set all bits to 1) if r_inf
      r_inf = false;
    }
    bool exception = r_nan | r_inf;
    ac_int<E,true> e_r = exception ? -1 : (r_zero | !r_normal) ? 0 : exp;
    if(exception | r_zero) {
      m_r = 0;
      m_r[mant_bits-1] = r_nan;
    }
    ac_int<W,true> d_r = m_r;
    d_r.set_slc(mant_bits, e_r);
    d_r[W-1] = r_sign;
    ac_std_float r;
    r.set_data(d_r);
    return r;
  }
  template<ac_q_mode QR, bool No_SubNormals>
  ac_std_float div(const ac_std_float &op2) const {
#ifndef AC_STD_FLOAT_DIV_OVERRIDE
    return div_generic<QR,No_SubNormals>(op2);
#else
    return AC_STD_FLOAT_OVERRIDE_NS AC_STD_FLOAT_DIV_OVERRIDE<QR,No_SubNormals>(*this, op2);
#endif
  }
  template<ac_q_mode QR, bool No_SubNormals>
  ac_std_float fma_generic(const ac_std_float &op2, const ac_std_float &op3) const {
    ac_private::check_supported<QR>();
    e_t op1_e, op2_e, op3_e;
    bool op1_normal, op1_sign, op1_zero, op2_normal, op2_sign, op2_zero, op3_normal, op3_sign, op3_zero;
    bool op1_inf, op1_nan, op2_inf, op2_nan, op3_inf, op3_nan;
    mu_t op1_mu, op2_mu, op3_mu;
    extract(op1_mu, op1_e, op1_sign, op1_normal, op1_zero, op1_inf, op1_nan, true, No_SubNormals);
    op2.extract(op2_mu, op2_e, op2_sign, op2_normal, op2_zero, op2_inf, op2_nan, true, No_SubNormals);
    op3.extract(op3_mu, op3_e, op3_sign, op3_normal, op3_zero, op3_inf, op3_nan, true, No_SubNormals);
    if(No_SubNormals)
      op3_mu &= mu_t(op3_zero ? 0 : -1);
    bool mult_sign = (op1_sign ^ op2_sign) | (op1_zero & op2_inf) | (op1_inf & op1_zero);
    bool mult_nan = op1_nan | op2_nan | (op1_zero & op2_inf) | (op1_inf & op2_zero);
    bool mult_zero = op1_zero | op2_zero;  // mult_nan has precedence later on
    int mult_exp_b = ac_int<E,false>(op1_e) + ac_int<E,false>(op2_e) + !op1_normal + !op2_normal - exp_bias;
    ac_int<2*mu_bits,false> p = op1_mu * op2_mu;
    if(No_SubNormals)
      p &= ac_int<2*mu_bits,false>(mult_zero ? 0 : -1);
    bool mult_inf = op1_inf | op2_inf;

    bool diff_signs = mult_sign ^ op3_sign;
    bool toggle_r_sign = mult_sign;
    m_t op3_m = diff_signs ? m_t(-op3_mu) : m_t(op3_mu);
    unsigned op3_e_b = ac_int<E,false>(op3_e) + !op3_normal;

    int e_dif = mult_exp_b - op3_e_b;
    bool emult_lt_e3 = e_dif < 0;
    e_dif = emult_lt_e3 ? -e_dif : e_dif;

    const int extra_lsb = mu_bits -2; // (mu_bits -2) + mu_bits+6 = 2*mu_bits+4
    const int accu_size = No_SubNormals ? mu_bits+6+extra_lsb: 2*mu_bits+4; 
    AC_ASSERT(!No_SubNormals || (extra_lsb >=-3 && extra_lsb <= mu_bits -2), "The FMA accumulator has invalid size. The value of extra_lsb shoudle be in range {-3, mantissa_width-1}");
    typedef ac_int<accu_size,true> add_t;
    add_t op3_m_s = op3_m;
    add_t p_s;
    if (No_SubNormals) {
      const int shift_out = (mu_bits -1) - (extra_lsb);
      op3_m_s <<= 3 + extra_lsb;
      p_s = p >> shift_out;
      p_s <<= 3;
      p_s[2] = p[shift_out-1]; 
      const int ptr_1 = (shift_out-2 > 0) ? shift_out-2 : 0;
      p_s[1] = (shift_out-2 >= 0) ? p[ptr_1] : ac_int<1,false>(0); 
      const int s_w = (shift_out-3 > 0) ? shift_out-3 : 0;
      p_s[0] = (shift_out-3 > 0)  ? (p.template slc<s_w>(0)).or_reduce() : 0; 
    } else {
      op3_m_s <<= mu_bits+1;   // mult: ii.ffff, op3: i.ff
      p_s = p;
      p_s <<= 2;
    }
    add_t op_lshift = emult_lt_e3 ? p_s : op3_m_s;
    add_t op_no_shift = emult_lt_e3 ? op3_m_s : p_s;
    
    typedef ac_int<ac::nbits<accu_size>::val,false> e_dif_sat_t;
    e_dif_sat_t e_dif_sat = ac_fixed<ac::nbits<accu_size>::val,ac::nbits<accu_size>::val,false,AC_TRN,AC_SAT>(e_dif).to_ac_int();
    add_t shifted_out_bits = op_lshift;
    shifted_out_bits &= ~((add_t(-1)) << ((mult_zero | op3_zero) ? e_dif_sat_t(0) : e_dif_sat));
    bool sticky_bit = !!shifted_out_bits;

    op_lshift >>= ((mult_zero | op3_zero) ? e_dif_sat_t(0) : e_dif_sat);
    add_t add_r = op_lshift + op_no_shift;
    bool add_exact_zero = !add_r & !sticky_bit;
    bool r_neg = add_r[add_t::width-1];
    
    int exp = ( ((emult_lt_e3 & !op3_zero) | mult_zero) ? ( op3_e_b + 2 ) : ( mult_exp_b + 2 ) );
    
    bool all_sign;
    int ls = add_r.leading_sign(all_sign);
    // no bit growth of add_r
    int max_shift_left = exp + (- min_exp - exp_bias);
    bool shift_exponent_limited = ls >= max_shift_left;
    int shift_l = shift_exponent_limited ? max_shift_left : ls;

    typedef ac_int<ac::nbits<accu_size>::val+!No_SubNormals,!No_SubNormals> shift_l_sat_t;
    shift_l_sat_t shift_l_sat = ac_fixed<ac::nbits<accu_size>::val+!No_SubNormals,ac::nbits<accu_size>::val+!No_SubNormals,!No_SubNormals,AC_TRN,AC_SAT>(shift_l).to_ac_int();
    add_r <<= shift_l_sat;   
    add_r[0] = add_r[0] | sticky_bit;

    ac_fixed<mu_bits+1,accu_size,true,QR> r_rnd = add_r;

    const int slc_ptr = No_SubNormals ? 4+extra_lsb : mu_bits+2;
    typedef ac_int<mu_bits+1,false> t_h;
    t_h t = add_r.template slc<mu_bits+1>(slc_ptr);
    bool rnd_ovf = ((QR == AC_RND_CONV) | (QR == AC_RND_INF)) & !add_r[accu_size-1] & t == t_h(-1);
    bool r_sign = op3_inf ? op3_sign : mult_inf ? mult_sign : (r_neg ^ toggle_r_sign) & !add_exact_zero;
    ac_int<mu_bits+1,true> r_rnd_i = r_rnd.template slc<mu_bits+1>(0);
    bool r_zero = !rnd_ovf & !r_rnd_i;
    bool shift_r = rnd_ovf | (r_neg & !r_rnd_i.template slc<mu_bits>(0));
    typedef ac_int<mu_bits,false> r_un_t;
    r_un_t r_un =  r_neg ? (r_un_t) -r_rnd_i : (r_un_t) r_rnd_i;
    // get rid of implied bit, assign to ac_int
    bool r_normal = r_un[mant_bits] | shift_r;
    r_zero |= No_SubNormals & !r_normal;
    ac_int<mant_bits,false> m_r = r_un.template slc<mant_bits>(0);
    exp = (shift_exponent_limited ? min_exp + exp_bias : exp - ls) + shift_r;
    bool r_inf = (exp > max_exp + exp_bias);
    if((QR==AC_TRN_ZERO) & (r_inf)) {
      exp = max_exp + exp_bias;
      m_r |= ac_int<1,true>(-1);  // saturate (set all bits to 1) if r_inf
      r_inf = false;
    }
    bool r_nan = op3_nan | mult_nan | ((op3_inf & mult_inf) & (op3_sign ^ mult_sign));
    r_inf |= mult_inf | op3_inf;
    bool exception = r_nan | r_inf;
    ac_int<E,true> e_r = exception ? -1 : (r_zero | !r_normal) ? 0 : exp;
    if(exception | r_zero) {
      m_r = 0;
      m_r[mant_bits-1] = r_nan;
    }
    // -0 + -0 = -0 exception
    r_sign |= mult_zero & mult_sign & op3_zero & op3_sign;
    ac_int<W,true> d_r = m_r;
    d_r.set_slc(mant_bits, e_r);
    d_r[W-1] = r_sign;
    ac_std_float r;
    r.set_data(d_r);
    return r;
  }
  template<ac_q_mode QR, bool No_SubNormals>
  ac_std_float fma(const ac_std_float &op2, const ac_std_float &op3) const {
#ifndef AC_STD_FLOAT_FMA_OVERRIDE
    return fma_generic<QR,No_SubNormals>(op2,op3);
#else
    return AC_STD_FLOAT_OVERRIDE_NS AC_STD_FLOAT_FMA_OVERRIDE<QR,No_SubNormals>(*this,op2,op3);
#endif
  }
  template<ac_q_mode QR, bool No_SubNormals>
  ac_std_float sqrt_generic() const {
    ac_private::check_supported<QR>();
    const bool rnd = QR != AC_TRN_ZERO;   // need msb(rounded bits)
    const bool rbits = QR != AC_TRN_ZERO; // need bits after msb(rounded bits)
    e_t op1_e;
    bool op1_normal, op1_sign, op1_zero;
    bool op1_inf, op1_nan;
    mu_t op1_mu;
    extract(op1_mu, op1_e, op1_sign, op1_normal, op1_zero, op1_inf, op1_nan, true, No_SubNormals);
    int ls_op1 = No_SubNormals ? 0 : (unsigned) op1_mu.leading_sign();
    op1_mu <<= ls_op1;
    op1_mu[mu_bits-1] = true;  // Since it is normalized, zero is captured by op1_zero

    bool exp_odd = (op1_e  ^ !op1_normal ^ ls_op1 ^ exp_bias) & 1;

    int exp = ac_int<E,false>(op1_e) + !op1_normal - ls_op1 - exp_bias;
    exp >>= 1;   // divide by 2, truncate towards -inf

    ac_int<mu_bits+1,false> op1_mi = op1_mu;
    op1_mi <<= exp_odd;
    ac_int<mu_bits+rnd,false> sq_rt;
    bool sticky_bit = ac::fx_sqrt(op1_mi, sq_rt);
    bool r_normal = true;  // true for most practical cases on W,E
    if(mant_bits > -min_exp) {
      int exp_over = min_exp - exp;
      if(exp_over > 0) {
        if(rbits) {
          typedef ac_int<mu_bits+rnd,false> t_t;
          t_t shifted_out_bits = sq_rt;
          shifted_out_bits &= ~((~t_t(0)) << exp_over);
          sticky_bit |= !!shifted_out_bits;
        }
        sq_rt >>= exp_over;
        exp = min_exp;
        r_normal = false;
      }
    }
    // rounding should not trigger overflow (unless truncate towards +inf which is currently not supported)
    ac_fixed<mu_bits+rnd+rbits,1,false> sq_rt_rnd = 0;
    if(rbits)
      sq_rt_rnd[0] = sq_rt_rnd[0] | sticky_bit;
    sq_rt_rnd.set_slc(rbits, sq_rt);
    ac_fixed<mu_bits,1,false,QR> sq_rt_fx = sq_rt_rnd;

    ac_int<mant_bits,false> m_r = sq_rt_fx.template slc<mant_bits>(0);
    bool r_nan = op1_nan | (op1_sign & !op1_zero);
    bool r_zero = op1_zero;
    r_zero |= !r_normal & No_SubNormals;
    bool r_inf = op1_inf;
    bool exception = r_nan | r_inf;
    exp += exp_bias;
    ac_int<E,true> e_r = exception ? -1 : (r_zero | !r_normal) ? 0 : exp;
    if(exception | r_zero) {
      m_r = 0;
      m_r[mant_bits-1] = r_nan;
    }
    ac_int<W,true> d_r = m_r;
    d_r.set_slc(mant_bits, e_r);
    d_r[W-1] = op1_sign;
    ac_std_float r;
    r.set_data(d_r);
    return r;
  }
  template<ac_q_mode QR, bool No_SubNormals>
  ac_std_float sqrt() const {
#ifndef AC_STD_FLOAT_SQRT_OVERRIDE
    return sqrt_generic<QR,No_SubNormals>();
#else
    return AC_STD_FLOAT_OVERRIDE_NS AC_STD_FLOAT_SQRT_OVERRIDE<QR,No_SubNormals>(*this);
#endif
  }
  ac_std_float operator +(const ac_std_float &op2) const {
    return add<AC_STD_FLOAT_IEEE754_ROUND,false>(op2);
  }
  ac_std_float operator -(const ac_std_float &op2) const {
    return sub<AC_STD_FLOAT_IEEE754_ROUND,false>(op2);
  }
  ac_std_float operator *(const ac_std_float &op2) const {
    return mult<AC_STD_FLOAT_IEEE754_ROUND,false>(op2);
  }
  ac_std_float operator /(const ac_std_float &op2) const {
    return div<AC_STD_FLOAT_IEEE754_ROUND,false>(op2);
  }
  ac_std_float &operator +=(const ac_std_float &op2) {
    *this = operator +(op2);
    return *this;
  }
  ac_std_float &operator -=(const ac_std_float &op2) {
    *this = operator -(op2);
    return *this;
  }
  ac_std_float &operator *=(const ac_std_float &op2) {
    *this = operator *(op2);
    return *this;
  }
  ac_std_float &operator /=(const ac_std_float &op2) {
    *this = operator /(op2);
    return *this;
  }
  bool operator ==(const ac_std_float &op2) const {
    return ((d == op2.d) & !isnan()) | (operator !() & op2.operator !());
  }
  bool operator !=(const ac_std_float &op2) const {
    return !operator ==(op2);
  }
  bool magnitude_lt(const ac_std_float &op2) const {
    return ac_int<W-1,false>(d) < ac_int<W-1,false>(op2.d);
  }
  bool neg() const { return d[W-1]; }
  bool operator <(const ac_std_float &op2) const {
    return
      (operator !=(op2)) & ( (neg() & !op2.neg()) | ((!(neg() ^ op2.neg())) & (neg() ^ magnitude_lt(op2))) )
      & !isnan() & !op2.isnan();
  }
  bool operator >=(const ac_std_float &op2) const {
    return
      ((operator ==(op2)) | (!neg() & op2.neg()) | ((!(neg() ^ op2.neg())) & (!neg() ^ magnitude_lt(op2))) )
      & !isnan() & !op2.isnan();
  }
  bool operator >(const ac_std_float &op2) const {
    return
      (operator !=(op2))
      & ( (!neg() & op2.neg()) | ((!(neg() ^ op2.neg())) & (!neg() ^ magnitude_lt(op2))) )
      & !isnan() & !op2.isnan();
  }
  bool operator <=(const ac_std_float &op2) const {
    return
      ((operator == (op2)) | (neg() & !op2.neg()) | ((!neg() ^ op2.neg()) & (neg() ^ magnitude_lt(op2))) )
      & !isnan() & !op2.isnan();
  }
  bool operator !() const { return !ac_int<W-1,false>(d); }
  ac_std_float operator -() const {
    ac_std_float r(*this);
    r.d[W-1] = !d[W-1];
    return r;
  }
  ac_std_float operator +() const {
    return ac_std_float(*this);
  }
  ac_std_float abs() const {
    ac_std_float r(*this);
    r.d[W-1] = false;
    return r;
  }
  ac_std_float copysign(const ac_std_float &op2) const {
    ac_std_float r(*this);
    r.d[W-1] = op2.d[W-1];
    return r;
  }
  bool signbit() const {
    return d[W-1];
  }
  void set_signbit(bool s) {
    d[W-1] = s;
  }
  ac_std_float ceil() const {
    ac_int<E,false> e = d.template slc<E>(mant_bits);
    bool sign = d[W-1];
    if(!d.template slc<W-1>(0))
      return *this;
    if(e < exp_bias) {
      return sign ? zero() : one();
    } else {
      ac_std_float r(*this);
      int e_dif = mant_bits + exp_bias - e;
      if((e_dif < 0) | (e == ac_int<E,false>(-1)))
        return r;
      else {
        typedef ac_int<mant_bits,false> mant_t;
        mant_t m = d;
        mant_t mask = (~mant_t(0)) << e_dif;
        bool non_zero_fractional = !!(m & ~mask);
        if(!sign) {
          m |= ~mask;
          mu_t mu = m + mant_t(non_zero_fractional);
          e += mu[mant_bits];
          r.d.set_slc(mant_bits, e);
          m = mu;
        }
        m &= mask;  // truncate fractional bits
        r.d.set_slc(0, m);
        return r;
      }
    }
  }
  ac_std_float floor() const {
    ac_int<E,false> e = d.template slc<E>(mant_bits);
    bool sign = d[W-1];
    if(!d.template slc<W-1>(0))
      return *this;
    if(e < exp_bias) {
      return sign ? -one() : zero();
    } else {
      ac_std_float r(*this);
      int e_dif = mant_bits + exp_bias - e;
      if((e_dif < 0) | (e == ac_int<E,false>(-1)))
        return r;
      else {
        typedef ac_int<mant_bits,false> mant_t;
        mant_t m = d;
        mant_t mask = (~mant_t(0)) << e_dif;
        bool non_zero_fractional = !!(m & ~mask);
        if(sign) {
          m |= ~mask;
          mu_t mu = m + mant_t(non_zero_fractional);
          e += mu[mant_bits];
          r.d.set_slc(mant_bits, e);
          m = mu;
        }
        m &= mask;  // truncate fractional bits
        r.d.set_slc(0, m);
        return r;
      }
    }
  }
  ac_std_float trunc() const {
    ac_int<E,false> e = d.template slc<E>(mant_bits);
    if(e < exp_bias) {
      return zero();
    } else {
      ac_std_float r(*this);
      int e_dif = mant_bits + exp_bias - e;
      if((e_dif < 0) | (e == ac_int<E,false>(-1)))
        return r;
      else {
        typedef ac_int<mant_bits,false> mant_t;
        mant_t m = d;
        mant_t mask = (~mant_t(0)) << e_dif;
        m &= mask;  // truncate fractional bits
        r.d.set_slc(0, m);
        return r;
      }
    }
  }
  ac_std_float round() const {
    ac_int<E,false> e = d.template slc<E>(mant_bits);
    if(e < exp_bias-1) {
      return zero();
    } else {
      ac_std_float r(*this);
      int e_dif = mant_bits + exp_bias -1 - e;
      if((e_dif < 0) | (e == ac_int<E,false>(-1)))
        return r;
      else {
        typedef ac_int<mant_bits,false> mant_t;
        mant_t m = d;
        mant_t mask = (~mant_t(0)) << e_dif;
        m |= ~mask;
        mu_t mu = m + mant_t(1);
        e += mu[mant_bits];
        r.d.set_slc(mant_bits, e);
        m = mu;
        m &= mask << 1;  // truncate fractional bits
        r.d.set_slc(0, m);
        return r;
      }
    }
  }
};

template<int W, int E>
inline std::ostream& operator << (std::ostream &os, const ac_std_float<W,E> &x) {
  // for now just print the raw ac_int for it
#ifndef __SYNTHESIS__
  if ((os.flags() & std::ios::hex) != 0) {
    os << x.data().to_string(AC_HEX,false,true);
  } else if ((os.flags() & std::ios::oct) != 0) {
    os << x.data().to_string(AC_OCT,false,true);
  } else {
    if((W <= 32) & (E <= 8)) {
      os << x.to_float();
    } else if((W <= 64) & (E <= 11)) {
      os << x.to_double();
    } else {
      // operator << for decimal not yet implemented for ac_std_float with W > 64 or E > 11
      os << x.data().to_string(AC_HEX,false,true);
    }
  }
#endif
  return os;
}

namespace ac {
  // Type punning: using memcpy to avoid strict aliasing
  inline void copy_bits(float f, int *x) {
    std::memcpy(x, &f, sizeof(int));
  }
  inline void copy_bits(double f, long long *x) {
    std::memcpy(x, &f, sizeof(long long));
  }
  inline void copy_bits(int x, float *f) {
    std::memcpy(f, &x, sizeof(float));
  }
  inline void copy_bits(long long x, double *f) {
    std::memcpy(f, &x, sizeof(double));
  }

  inline void copy_bits(const ac_std_float<32,8> &x, float *f) {
    copy_bits(x.data().to_int(), f);
  }
  inline void copy_bits(const ac_std_float<64,11> &x, double *f) {
    copy_bits(x.data().to_int64(), f);
  }
}

template<ac_ieee_float_format Format>
class ac_ieee_float_base {
public:
  static const int width = 1 << ((int)Format + 4);
  // exponents are {5,8,11,15,19}, but the first three are specialized elsewhere
  static const int e_width = 11 + ((int)Format - binary64)*4; // 11, 15, 19
  static const int exp_bias = (1 << (e_width-1)) - 1;
  static const int lls = width >> 6;
  typedef long long (data_t)[lls];
  typedef ac_std_float<width,e_width> ac_std_float_t;
  typedef ac_std_float<width,e_width> helper_t;
  typedef ac_float<width-e_width+1,2,e_width,AC_RND_CONV> ac_float_t;
  data_t d;
  ac_ieee_float_base() {}
  ac_ieee_float_base(const ac_ieee_float_base &f) {
    ac::copy_bits(f.d, &d);
  }
  explicit ac_ieee_float_base(const helper_t &op) {
    ac::copy_bits(op.data(), &d);
  }
  explicit ac_ieee_float_base(double f);
protected:
  helper_t to_helper_t() const {
    ac_int<width,true> dat;
    ac::copy_bits(d, &dat);
    helper_t x;
    x.set_data(dat);
    return x;
  }
public:
  void set_data(const data_t &op) { ac::copy_bits(op, &d); }
  void set_data(const ac_int<width,true> &op) { ac::copy_bits(op, &d); }
  const data_t &data() const { return d; }
  ac_int<width,true> data_ac_int() const {
    ac_int<width,true> x;
    ac::copy_bits(d, &x);
    return x;
  }
  bool signbit() const { return d[lls-1] < 0; }
  void set_signbit(bool s) {
    ac_int<64,true> t(d[lls-1]);
    t[63] = s;
    d[lls-1] = t.to_int64();
  }
};

template<ac_ieee_float_format Format>
inline std::ostream& operator << (std::ostream &os, const ac_ieee_float_base<Format> &x) {
#ifndef __SYNTHESIS__
  if ((os.flags() & std::ios::hex) != 0) {
    os << x.data_ac_int().to_string(AC_HEX,false,true);
  } else if ((os.flags() & std::ios::oct) != 0) {
    os << x.data_ac_int().to_string(AC_OCT,false,true);
  } else {
    // operator << for decimal not yet implemented for either ieee 128 or 256
    os << x.data_ac_int().to_string(AC_HEX,false,true);
  }
#endif
  return os;
}

template<> class ac_ieee_float_base<binary16> {
public:
  static const int width = 16;
  static const int e_width = 5;
  static const int exp_bias = (1 << (e_width-1)) - 1;
  typedef ac_std_float<width,e_width> ac_std_float_t;
  typedef short data_t;
  typedef ac_std_float<width,e_width> helper_t;
  typedef ac_float<width-e_width+1,2,e_width,AC_RND_CONV> ac_float_t;
  data_t d;
  ac_ieee_float_base() {}
  ac_ieee_float_base(const ac_ieee_float_base &f) : d(f.d) {}
  explicit ac_ieee_float_base(const helper_t &op) : d(op.data()) {}
  explicit ac_ieee_float_base(float f) : d((short)ac_std_float<width,e_width>(f).data().to_int()) {}
protected:
  helper_t to_helper_t() const {
    helper_t x;
    x.set_data(d);
    return x;
  }
public:
  float to_float() const {
    ac_std_float_t t;
    t.set_data(this->data_ac_int());
    return t.to_float();
  }
#if __cplusplus > 199711L
  explicit operator float() const { return to_float(); }
#endif
  void set_data(short op) { ac::copy_bits(op, &d); }
  void set_data(const ac_int<width,true> &op) { ac::copy_bits(op, &d); }
  const data_t &data() const { return d; }
  ac_int<width,true> data_ac_int() const {
    ac_int<width,true> x;
    ac::copy_bits(d, &x);
    return x;
  }
  bool signbit() const { return d < 0; }
  void set_signbit(bool s) {
    ac_int<width,true> t(d);
    t[width-1] = s;
    d = t;
  }
};

inline std::ostream& operator << (std::ostream &os, const ac_ieee_float_base<binary16> &x) {
#ifndef __SYNTHESIS__
  if ((os.flags() & std::ios::hex) != 0) {
    os << x.data_ac_int().to_string(AC_HEX,false,true);
  } else if ((os.flags() & std::ios::oct) != 0) {
    os << x.data_ac_int().to_string(AC_OCT,false,true);
  } else {
    os << x.to_float();
  }
#endif
  return os;
}

struct float_helper {
  float d;
  float_helper() {}
  float_helper(float f) { d = f; }
  float_helper(const float_helper &f) { d = f.d; }
  float_helper(const float_helper &f, bool no_subnormals) {
    d = no_subnormals & (f.fpclassify() == FP_SUBNORMAL) ? std::signbit(f.d) ? -0.0 : 0.0 : f.d;
  }
  float_helper(const ac_std_float<32,8> &f) { set_data(f.data().to_int()); }
  template<ac_q_mode Q>
  float_helper(const ac_float<25,2,8,Q> &f) : d(f.to_float()) {}
  const float &data() const { return d; }
  void set_data(int data) { ac::copy_bits(data, &d); }
  void set_data(float data) { d = data; }
  operator float() const { return d; }
  float to_float() const { return d; }
  int fpclassify() const { return std::fpclassify(d); }
  bool isfinite() const { return std::isfinite(d); }
  bool isnormal() const { return std::isnormal(d); }
  bool isinf() const { return std::isinf(d); }
  bool isnan() const { return std::isnan(d); }
  static float nan() { return ac_std_float<32,8>::nan().to_float(); }
  static float inf() { return ac_std_float<32,8>::inf().to_float(); }
  static float denorm_min() { return ac_std_float<32,8>::denorm_min().to_float(); }
  static float min() { return ac_std_float<32,8>::min().to_float(); }
  static float max() { return ac_std_float<32,8>::max().to_float(); }
  static float epsilon() { return ac_std_float<32,8>::epsilon().to_float(); }
  template<ac_q_mode QR, bool No_SubNormals>
  float_helper add(const float_helper &op2) const {
    ac_private::check_supported2<QR>();
    return float_helper( float_helper(*this, No_SubNormals) + float_helper(op2, No_SubNormals), No_SubNormals);
  }
  template<ac_q_mode QR, bool No_SubNormals>
  float_helper sub(const float_helper &op2) const {
    ac_private::check_supported2<QR>();
    return float_helper( float_helper(*this, No_SubNormals) - float_helper(op2, No_SubNormals), No_SubNormals);
  }
  template<ac_q_mode QR, bool No_SubNormals>
  float_helper mult(const float_helper &op2) const {
    ac_private::check_supported2<QR>();
    return float_helper( float_helper(*this, No_SubNormals) * float_helper(op2, No_SubNormals), No_SubNormals);
  }
  template<ac_q_mode QR, bool No_SubNormals>
  float_helper div(const float_helper &op2) const {
    ac_private::check_supported2<QR>();
    return float_helper( float_helper(*this, No_SubNormals) / float_helper(op2, No_SubNormals), No_SubNormals);
  }
  template<ac_q_mode QR, bool No_SubNormals>
  float_helper fma(const float_helper &op2, const float_helper &op3) const {
    ac_private::check_supported2<QR>();
    return float_helper( ::fmaf(float_helper(*this, No_SubNormals), float_helper(op2, No_SubNormals), float_helper(op3, No_SubNormals)), No_SubNormals);
  }
  template<ac_q_mode QR, bool No_SubNormals>
  float_helper sqrt() const {
    ac_private::check_supported2<QR>();
    return float_helper( ::sqrtf(float_helper(*this, No_SubNormals)), No_SubNormals);
  }
  float_helper ceil() const { return float_helper(std::ceil(d)); }
  float_helper floor() const { return float_helper(std::floor(d)); }
  float_helper trunc() const { return float_helper(::truncf(d)); }
  float_helper round() const { return float_helper(::roundf(d)); }
  bool operator !() const { return !d; }
};

template<> class ac_ieee_float_base<binary32> {
public:
  static const int width = 32;
  static const int e_width = 8;
  static const int exp_bias = (1 << (e_width-1)) - 1;
  typedef ac_std_float<width,e_width> ac_std_float_t;
#ifdef AC_IEEE_FLOAT_USE_BUILTIN
  typedef float data_t;
  typedef float_helper helper_t;
#else
  typedef int data_t;
  typedef ac_std_float<width,e_width> helper_t;
#endif
  typedef ac_float<width-e_width+1,2,e_width,AC_RND_CONV> ac_float_t;
  data_t d;
  ac_ieee_float_base() {}
  ac_ieee_float_base(const ac_ieee_float_base &f) : d(f.d) {}
  explicit ac_ieee_float_base(const helper_t &op) : d(op.data()) {}
  explicit ac_ieee_float_base(float f) { ac::copy_bits(f, &d); }
protected:
  helper_t to_helper_t() const {
    helper_t x;
    x.set_data(d);
    return x;
  }
public:
#if __cplusplus > 199711L
  explicit operator float() const {
    float f;
    ac::copy_bits(d, &f);
    return f;
  }
#endif
  float to_float() const {
    float f;
    ac::copy_bits(d, &f);
    return f;
  }
  void set_data(int op) { ac::copy_bits(op, &d); }
  void set_data(float op) { ac::copy_bits(op, &d); }
  void set_data(const ac_int<width,true> &op) { ac::copy_bits(op, &d); }
  const data_t &data() const { return d; }
  ac_int<width,true> data_ac_int() const {
    ac_int<width,true> x;
    ac::copy_bits(d, &x);
    return x;
  }
  bool signbit() const {
    int x; ac::copy_bits(d, &x);
    return x < 0;
  }
  void set_signbit(bool s) {
    ac_int<width,true> t;
    ac::copy_bits(d, &t);
    t[width-1] = s;
    ac::copy_bits(t, &d);
  }
};

inline std::ostream& operator << (std::ostream &os, const ac_ieee_float_base<binary32> &x) {
#ifndef __SYNTHESIS__
  if ((os.flags() & std::ios::hex) != 0) {
    os << x.data_ac_int().to_string(AC_HEX,false,true);
  } else if ((os.flags() & std::ios::oct) != 0) {
    os << x.data_ac_int().to_string(AC_OCT,false,true);
  } else {
    os << x.to_float();
  }
#endif
  return os;
}

struct double_helper {
  double d;
  double_helper() {}
  double_helper(double f) { d = f; }
  double_helper(const float_helper &f) { d = f.d; }
  double_helper(const double_helper &f, bool no_subnormals) {
    d = no_subnormals & (f.fpclassify() == FP_SUBNORMAL) ? std::signbit(f.d) ? -0.0 : 0.0 : f.d;
  }
  double_helper(const ac_std_float<64,11> &f) { set_data(f.data().to_int64()); }
  template<ac_q_mode Q>
  double_helper(const ac_float<54,2,11,Q> &f) : d(f.to_double()) {}
  const double &data() const { return d; }
  void set_data(long long data) {
    ac::copy_bits(data, &d);
  }
  void set_data(double data) { d = data; }
  operator double() const { return d; }
  double to_double() const { return d; }
  int fpclassify() const { return std::fpclassify(d); }
  bool isfinite() const { return std::isfinite(d); }
  bool isnormal() const { return std::isnormal(d); }
  bool isinf() const { return std::isinf(d); }
  bool isnan() const { return std::isnan(d); }
  static double nan() { return ac_std_float<64,11>::nan().to_double(); }
  static double inf() { return ac_std_float<64,11>::inf().to_double(); }
  static double denorm_min() { return ac_std_float<64,11>::denorm_min().to_double(); }
  static double min() { return ac_std_float<64,11>::min().to_double(); }
  static double max() { return ac_std_float<64,11>::max().to_double(); }
  static double epsilon() { return ac_std_float<64,11>::epsilon().to_double(); }
  template<ac_q_mode QR, bool No_SubNormals>
  double_helper add(const double_helper &op2) const {
    ac_private::check_supported2<QR>();
    return double_helper( double_helper(*this, No_SubNormals) + double_helper(op2, No_SubNormals), No_SubNormals);
  }
  template<ac_q_mode QR, bool No_SubNormals>
  double_helper sub(const double_helper &op2) const {
    ac_private::check_supported2<QR>();
    return double_helper( double_helper(*this, No_SubNormals) - double_helper(op2, No_SubNormals), No_SubNormals);
  }
  template<ac_q_mode QR, bool No_SubNormals>
  double_helper mult(const double_helper &op2) const {
    ac_private::check_supported2<QR>();
    return double_helper( double_helper(*this, No_SubNormals) * double_helper(op2, No_SubNormals), No_SubNormals);
  }
  template<ac_q_mode QR, bool No_SubNormals>
  double_helper div(const double_helper &op2) const {
    ac_private::check_supported2<QR>();
    return double_helper( double_helper(*this, No_SubNormals) / double_helper(op2, No_SubNormals), No_SubNormals);
  }
  template<ac_q_mode QR, bool No_SubNormals>
  double_helper fma(const double_helper &op2, const double_helper &op3) const {
    ac_private::check_supported2<QR>();
    return double_helper( ::fma((double) double_helper(*this, No_SubNormals), (double) double_helper(op2, No_SubNormals), (double) double_helper(op3, No_SubNormals)), No_SubNormals);
  }
  template<ac_q_mode QR, bool No_SubNormals>
  double_helper sqrt() const {
    ac_private::check_supported2<QR>();
    return double_helper( ::sqrt((double) double_helper(*this, No_SubNormals)), No_SubNormals);
  }
  double_helper ceil() const { return double_helper(std::ceil(d)); }
  double_helper floor() const { return double_helper(std::floor(d)); }
  double_helper trunc() const { return double_helper(::trunc(d)); }
  double_helper round() const { return double_helper(::round(d)); }
  bool operator !() const { return !d; }
};

template<> class ac_ieee_float_base<binary64> {
public:
  static const int width = 64;
  static const int e_width = 11;
  static const int exp_bias = (1 << (e_width-1)) - 1;
  typedef ac_std_float<width,e_width> ac_std_float_t;
#ifdef AC_IEEE_FLOAT_USE_BUILTIN
  typedef double data_t;
  typedef double_helper helper_t;
#else
  typedef long long data_t;
  typedef ac_std_float<width,e_width> helper_t;
#endif
  typedef ac_float<width-e_width+1,2,e_width,AC_RND_CONV> ac_float_t;
  data_t d;
  ac_ieee_float_base() {}
  ac_ieee_float_base(const ac_ieee_float_base &f) : d(f.d) {}
  explicit ac_ieee_float_base(const helper_t &op) : d(op.data()) {}
  explicit ac_ieee_float_base(double f) { ac::copy_bits(f, &d); }
protected:
  helper_t to_helper_t() const {
    helper_t x;
    x.set_data(d);
    return x;
  }
public:
#if __cplusplus > 199711L
  explicit operator double() const {
    double f;
    ac::copy_bits(d, &f);
    return f;
  }
#endif
  double to_double() const {
    double f;
    ac::copy_bits(d, &f);
    return f;
  }
  void set_data(long long op) { ac::copy_bits(op, &d); }
  void set_data(double op) { ac::copy_bits(op, &d); }
  void set_data(const ac_int<width,true> &op) { ac::copy_bits(op, &d); }
  const data_t &data() const { return d; }
  ac_int<width,true> data_ac_int() const {
    ac_int<width,true> x;
    ac::copy_bits(d, &x);
    return x;
  }
  bool signbit() const {
    long long x; ac::copy_bits(d, &x);
    return x < 0;
  }
  void set_signbit(bool s) {
    ac_int<width,true> t;
    ac::copy_bits(d, &t);
    t[width-1] = s;
    ac::copy_bits(t, &d);
  }
};

inline std::ostream& operator << (std::ostream &os, const ac_ieee_float_base<binary64> &x) {
#ifndef __SYNTHESIS__
  if ((os.flags() & std::ios::hex) != 0) {
    os << x.data_ac_int().to_string(AC_HEX,false,true);
  } else if ((os.flags() & std::ios::oct) != 0) {
    os << x.data_ac_int().to_string(AC_OCT,false,true);
  } else {
    os << x.to_double();
  }
#endif
  return os;
}

namespace ac_private {
  template<ac_ieee_float_format Format, typename T2>
  struct ac_ieee_float_constructor {};
  template<> struct ac_ieee_float_constructor<binary16,float> {
    typedef int type_explicit;
  };
  template<> struct ac_ieee_float_constructor<binary16,double> {
    typedef int type_explicit;
  };
  template<> struct ac_ieee_float_constructor<binary32,float> {
    typedef int type;
  };
  template<> struct ac_ieee_float_constructor<binary32,double> {
    typedef int type_explicit;
  };
  template<> struct ac_ieee_float_constructor<binary64,float> {
    typedef int type_explicit;
  };
  template<> struct ac_ieee_float_constructor<binary64,double> {
    typedef int type;
  };
  template<> struct ac_ieee_float_constructor<binary128,float> {
    typedef int type_explicit;
  };
  template<> struct ac_ieee_float_constructor<binary128,double> {
    typedef int type_explicit;
  };
  template<> struct ac_ieee_float_constructor<binary256,float> {
    typedef int type_explicit;
  };
  template<> struct ac_ieee_float_constructor<binary256,double> {
    typedef int type_explicit;
  };
}

template<ac_ieee_float_format Format>
class ac_ieee_float : public ac_ieee_float_base<Format> {
public:
  typedef ac_ieee_float_base<Format> Base;
  template<typename T>
  struct rt_T {
    typedef typename ac_private::rt_closed_T<ac_ieee_float,T>::type mult;
    typedef typename ac_private::rt_closed_T<ac_ieee_float,T>::type plus;
    typedef typename ac_private::rt_closed_T<ac_ieee_float,T>::type minus;
    typedef typename ac_private::rt_closed_T<ac_ieee_float,T>::type minus2;
    typedef typename ac_private::rt_closed_T<ac_ieee_float,T>::type logic;
    typedef typename ac_private::rt_closed_T<ac_ieee_float,T>::type div;
    typedef typename ac_private::rt_closed_T<ac_ieee_float,T>::type div2;
  };
  struct rt_unary {
    typedef ac_ieee_float neg;
    typedef ac_ieee_float mag_sqr;
    typedef ac_ieee_float mag;
  };
  static const int width = Base::width;
  static const int e_width = Base::e_width;
  static const int exp_bias = (1 << (e_width-1)) - 1;
  static const int lls = width >> 6;
  typedef typename Base::data_t data_t;
  typedef typename Base::helper_t helper_t;
  typedef typename Base::ac_float_t ac_float_t;
  typedef ac_std_float<width,e_width> ac_std_float_t;
public:
  static ac_ieee_float nan() { return ac_ieee_float(helper_t::nan()); }
  static ac_ieee_float inf() { return ac_ieee_float(helper_t::inf()); }
  static ac_ieee_float denorm_min() { return ac_ieee_float(helper_t::denorm_min()); }
  static ac_ieee_float min() { return ac_ieee_float(helper_t::min()); }
  static ac_ieee_float max() { return ac_ieee_float(helper_t::max()); }
  static ac_ieee_float epsilon() { return ac_ieee_float(helper_t::epsilon()); }
  static ac_ieee_float zero() { return ac_ieee_float(ac_std_float_t::zero()); }
  static ac_ieee_float one() { return ac_ieee_float(ac_std_float_t::one()); }
  ac_ieee_float() {}
private:
  ac_ieee_float(const Base &f) : Base(f) {}
public:
  ac_ieee_float(const ac_std_float<width,e_width> &f) : Base(f) {}
  ac_ieee_float(const ac_ieee_float &f) : Base(f) {}
  template<ac_ieee_float_format Format2>
  explicit ac_ieee_float(const ac_ieee_float<Format2> &f) : Base(ac_std_float_t(f.to_ac_std_float())) {}
  template<int W, int E>
  explicit ac_ieee_float(const ac_std_float<W,E> &f) : Base(ac_std_float_t(f)) {}
  explicit ac_ieee_float(const ac::bfloat16 &f);
  explicit ac_ieee_float(const ac_float_t &f) : Base(ac_std_float_t(f)) {}
  template<int WFX, int IFX, bool SFX, ac_q_mode QFX, ac_o_mode OFX>
  explicit ac_ieee_float(const ac_fixed<WFX,IFX,SFX,QFX,OFX> &fx) : Base(ac_std_float_t(fx)) {}
  template<int WI, bool SI>
  explicit ac_ieee_float(const ac_int<WI,SI> &x) : Base(ac_std_float_t(x)) {}
  template<ac_q_mode Q>
  explicit ac_ieee_float(const ac_float<width-e_width+1,2,e_width,Q> &f) : Base(ac_std_float_t(f)) {}
  template<ac_ieee_float_format Format2>
  ac_ieee_float<Format2> to_ac_ieee_float() const { return ac_ieee_float<Format2>(*this); }
  const ac_float_t to_ac_float() const {
    return to_ac_std_float().to_ac_float();
  }
  const ac_std_float<width,e_width> to_ac_std_float() const {
    ac_std_float<width,e_width> r;
    r.set_data(data_ac_int());
    return r;
  }
  template<int WFX, int IFX, bool SFX, ac_q_mode QFX, ac_o_mode OFX>
  ac_fixed<WFX,IFX,SFX,QFX,OFX> convert_to_ac_fixed(bool map_inf=false) const {
    return to_ac_std_float().template convert_to_ac_fixed<WFX,IFX,SFX,QFX,OFX>(map_inf);
  }
  template<int WI, bool SI>
  ac_int<WI,SI> convert_to_ac_int(bool map_inf=false) const {
    return to_ac_std_float().template convert_to_ac_int<WI,SI>(map_inf);
  }
  int convert_to_int(bool map_inf=false) const {
    return to_ac_std_float().convert_to_int(map_inf);
  }
  long long convert_to_int64(bool map_inf=false) const {
    return to_ac_std_float().convert_to_int64(map_inf);
  }
  void set_data(const data_t &data) {
    Base::set_data(data);
  }
  const ac_int<width,true> data_ac_int() const { return Base::data_ac_int(); }
  const data_t &data() const { return Base::d; }
  template<typename T>
  ac_ieee_float(const T &f, typename ac_private::template ac_ieee_float_constructor<Format,T>::type d = 0) : Base(ac_std_float_t(f)) {}
  template<typename T>
  explicit ac_ieee_float(const T &f, typename ac_private::template ac_ieee_float_constructor<Format,T>::type_explicit d = 0) : Base(ac_std_float_t(f)) {}
  explicit ac_ieee_float(int x) {
    *this = ac_ieee_float(ac_fixed<32,32,true>(x));
  }
  explicit ac_ieee_float(long long x) {
    *this = ac_ieee_float(ac_fixed<64,64,true>(x));
  }
  inline static std::string type_name() {
    std::string r = "ac_ieee_float<binary";
    const char *format[] = {"16", "32", "64", "128", "256"};
    r += format[(int)Format];
    r += ">";
    return r;
  }
  int fpclassify() const { return Base::to_helper_t().fpclassify(); }
  bool isfinite() const { return Base::to_helper_t().isfinite(); }
  bool isnormal() const { return Base::to_helper_t().isnormal(); }
  bool isinf() const { return Base::to_helper_t().isinf(); }
  bool isnan() const { return Base::to_helper_t().isnan(); }

  template<ac_q_mode QR, bool No_SubNormals, bool Effective_Add=false>
  ac_ieee_float add(const ac_ieee_float &op2) const {
    #ifdef AC_IEEE_FLOAT_USE_BUILTIN
    return ac_ieee_float(Base(Base::to_helper_t().template add<QR,No_SubNormals>(op2.Base::to_helper_t())));
    #else
    return ac_ieee_float(Base(Base::to_helper_t().template add<QR,No_SubNormals,Effective_Add>(op2.Base::to_helper_t())));
    #endif
  }
  template<ac_q_mode QR, bool No_SubNormals>
  ac_ieee_float sub(const ac_ieee_float &op2) const {
    return ac_ieee_float(Base(Base::to_helper_t().template sub<QR,No_SubNormals>(op2.Base::to_helper_t())));
  }
  template<ac_q_mode QR, bool No_SubNormals>
  ac_ieee_float mult(const ac_ieee_float &op2) const {
    return ac_ieee_float(Base(Base::to_helper_t().template mult<QR,No_SubNormals>(op2.Base::to_helper_t())));
  }
  template<ac_q_mode QR, bool No_SubNormals>
  ac_ieee_float div(const ac_ieee_float &op2) const {
    return ac_ieee_float(Base(Base::to_helper_t().template div<QR,No_SubNormals>(op2.Base::to_helper_t())));
  }
  template<ac_q_mode QR, bool No_SubNormals>
  ac_ieee_float fma(const ac_ieee_float &op2, const ac_ieee_float &op3) const {
    return ac_ieee_float(Base(Base::to_helper_t().template fma<QR,No_SubNormals>(op2.Base::to_helper_t(), op3.Base::to_helper_t())));
  }
  template<ac_q_mode QR, bool No_SubNormals>
  ac_ieee_float sqrt() const {
    return ac_ieee_float(Base(Base::to_helper_t().template sqrt<QR,No_SubNormals>()));
  }

  ac_ieee_float operator +(const ac_ieee_float &op2) const {
    return ac_ieee_float(Base(Base::to_helper_t() + op2.Base::to_helper_t()));
  }
  ac_ieee_float operator -(const ac_ieee_float &op2) const {
    return ac_ieee_float(Base(Base::to_helper_t() - op2.Base::to_helper_t()));
  }
  ac_ieee_float operator *(const ac_ieee_float &op2) const {
    return ac_ieee_float(Base(Base::to_helper_t() * op2.Base::to_helper_t()));
  }
  ac_ieee_float operator /(const ac_ieee_float &op2) const {
    return ac_ieee_float(Base(Base::to_helper_t() / op2.Base::to_helper_t()));
  }

  ac_ieee_float &operator +=(const ac_ieee_float &op2) {
    return *this = operator +(op2);
  }
  ac_ieee_float &operator -=(const ac_ieee_float &op2) {
    return *this = operator -(op2);
  }
  ac_ieee_float &operator *=(const ac_ieee_float &op2) {
    return *this = operator *(op2);
  }
  ac_ieee_float &operator /=(const ac_ieee_float &op2) {
    return *this = operator /(op2);
  }

  bool operator ==(const ac_ieee_float &op2) const {
    return Base::to_helper_t() == op2.Base::to_helper_t();
  }
  bool operator !=(const ac_ieee_float &op2) const {
    return Base::to_helper_t() != op2.Base::to_helper_t();
  }
  bool operator <(const ac_ieee_float &op2) const {
    return Base::to_helper_t() < op2.Base::to_helper_t();
  }
  bool operator >=(const ac_ieee_float &op2) const {
    return Base::to_helper_t() >= op2.Base::to_helper_t();
  }
  bool operator >(const ac_ieee_float &op2) const {
    return Base::to_helper_t() > op2.Base::to_helper_t();
  }
  bool operator <=(const ac_ieee_float &op2) const {
    return Base::to_helper_t() <= op2.Base::to_helper_t();
  }

  bool operator !() const { return !Base::to_helper_t(); }
  ac_ieee_float operator -() const {
    ac_ieee_float r(*this);
    r.set_signbit(!this->signbit());
    return r;
  }
  ac_ieee_float operator +() const {
    return ac_ieee_float(*this);
  }
  ac_ieee_float abs() const {
    ac_ieee_float r(*this);
    r.set_signbit(false);
    return r;
  }
  ac_ieee_float copysign(const ac_ieee_float &op2) const {
    ac_ieee_float r(*this);
    r.set_signbit(op2.signbit());
    return r;
  }
  bool signbit() const { return Base::signbit(); }
  ac_ieee_float add(const ac_ieee_float &op1, const ac_ieee_float &op2) {
    return *this = op1 + op2;
  }
  ac_ieee_float ceil() const {
    return ac_ieee_float(Base(Base::to_helper_t().ceil()));
  }
  ac_ieee_float floor() const {
    return ac_ieee_float(Base(Base::to_helper_t().floor()));
  }
  ac_ieee_float trunc() const {
    return ac_ieee_float(Base(Base::to_helper_t().trunc()));
  }
  ac_ieee_float round() const {
    return ac_ieee_float(Base(Base::to_helper_t().round()));
  }
  ac_ieee_float sub(const ac_ieee_float &op1, const ac_ieee_float &op2) {
    return *this = op1 - op2;
  }
  ac_ieee_float mult(const ac_ieee_float &op1, const ac_ieee_float &op2) {
    return *this = op1 * op2;
  }
  ac_ieee_float div(const ac_ieee_float &op1, const ac_ieee_float &op2) {
    return *this = op1 / op2;
  }
};

template<ac_ieee_float_format Format>
inline std::ostream& operator << (std::ostream &os, const ac_ieee_float<Format> &x) {
  os << (const ac_ieee_float_base<Format>&) x;
  return os;
}

template<int W, int E>
template<ac_ieee_float_format Format>
ac_ieee_float<Format> ac_std_float<W,E>::to_ac_ieee_float() const { return ac_ieee_float<Format>(*this); }

namespace ac {
class bfloat16 {
public:
  template<typename T>
  struct rt_T {
    typedef typename ac_private::rt_closed_T<bfloat16,T>::type mult;
    typedef typename ac_private::rt_closed_T<bfloat16,T>::type plus;
    typedef typename ac_private::rt_closed_T<bfloat16,T>::type minus;
    typedef typename ac_private::rt_closed_T<bfloat16,T>::type minus2;
    typedef typename ac_private::rt_closed_T<bfloat16,T>::type logic;
    typedef typename ac_private::rt_closed_T<bfloat16,T>::type div;
    typedef typename ac_private::rt_closed_T<bfloat16,T>::type div2;
  };
  struct rt_unary {
    typedef bfloat16 neg;
    typedef bfloat16 mag_sqr;
    typedef bfloat16 mag;
  };
  static const int width = 16;
  static const int e_width = 8;
  static const int exp_bias = (1 << (e_width-1)) - 1;
  static bfloat16 nan() { return bfloat16(helper_t::nan()); }
  static bfloat16 inf() { return bfloat16(helper_t::inf()); }
  static bfloat16 denorm_min() { return bfloat16(helper_t::denorm_min()); }
  static bfloat16 min() { return bfloat16(helper_t::min()); }
  static bfloat16 max() { return bfloat16(helper_t::max()); }
  static bfloat16 epsilon() { return bfloat16(helper_t::epsilon()); }
  static bfloat16 zero() { return bfloat16(ac_std_float_t::zero()); }
  static bfloat16 one() { return bfloat16(ac_std_float_t::one()); }
  typedef ac_std_float<width,e_width> helper_t;
  typedef short data_t;
  typedef ac_float<width-e_width+1,2,e_width,AC_RND_CONV> ac_float_t;
  typedef ac_std_float<width,e_width> ac_std_float_t;
  data_t d;
  bfloat16() {}
  bfloat16(const bfloat16 &f) : d(f.d) {}
  bfloat16(const ac_std_float_t &op) : d(op.data()) {}
  bfloat16(float f) { int x; ac::copy_bits(f, &x); d = (short) (x >> 16); }
  template<int W2>
  explicit bfloat16(const ac_std_float<W2,e_width> &f) {
    *this = f.template convert<width,AC_TRN_ZERO>();
  }
  template<int W2,int E2>
  explicit bfloat16(const ac_std_float<W2,E2> &f) {
    *this = f.template convert<width,e_width,AC_TRN_ZERO>();
  }
  template<ac_ieee_float_format Format>
  explicit bfloat16(const ac_ieee_float<Format> &f) {
    *this = f.to_ac_std_float().template convert<width,e_width,AC_TRN_ZERO>();
  }
  template<int WFX, int IFX, bool SFX, ac_q_mode QFX, ac_o_mode OFX>
  explicit bfloat16(const ac_fixed<WFX,IFX,SFX,QFX,OFX> &fx) {
    ac_std_float_t x;
    x.assign_from<AC_TRN_ZERO>(fx);
    *this = x;
  }
  template<int WI, bool SI>
  explicit bfloat16(const ac_int<WI,SI> &xi) {
    ac_std_float_t x;
    x.assign_from<AC_TRN_ZERO>(xi);
    *this = x;
  }
private:
  const helper_t to_helper_t() const {
    helper_t x;
    x.set_data(d);
    return x;
  }
public:
  const ac_std_float_t to_ac_std_float() const {
    ac_std_float_t x;
    x.set_data(d);
    return x;
  }
  template<ac_ieee_float_format Format>
  ac_ieee_float<Format> to_ac_ieee_float() const { return ac_ieee_float<Format>(*this); }
  const ac_float_t to_ac_float() const {
    return ac_std_float_t().to_ac_float();
  }
  template<int WFX, int IFX, bool SFX, ac_q_mode QFX, ac_o_mode OFX>
  ac_fixed<WFX,IFX,SFX,QFX,OFX> convert_to_ac_fixed(bool map_inf=false) const {
    return to_ac_std_float().template convert_to_ac_fixed<WFX,IFX,SFX,QFX,OFX>(map_inf);
  }
  template<int WI, bool SI>
  ac_int<WI,SI> convert_to_ac_int(bool map_inf=false) const {
    return to_ac_std_float().template convert_to_ac_int<WI,SI>(map_inf);
  }
  int convert_to_int(bool map_inf=false) const {
    return to_ac_std_float().convert_to_int(map_inf);
  }
  long long convert_to_int64(bool map_inf=false) const {
    return to_ac_std_float().convert_to_int64(map_inf);
  }
  float to_float() const {
    return to_ac_std_float().to_float();
  }
  double to_double() const {
    return to_ac_std_float().to_double();
  }
  // operator is efficient since E is identical and mantissa is longer
#if __cplusplus > 199711L
  explicit operator float() const { return to_float(); }
#endif
  inline static std::string type_name() {
    std::string r = "ac::bfloat16";
    return r;
  }
  int fpclassify() const { return to_helper_t().fpclassify(); }
  bool isfinite() const { return to_helper_t().isfinite(); }
  bool isnormal() const { return to_helper_t().isnormal(); }
  bool isinf() const { return to_helper_t().isinf(); }
  bool isnan() const { return to_helper_t().isnan(); }
  void set_data(short op) { ac::copy_bits(op, &d); }
  void set_data(const ac_int<width,true> &op) { ac::copy_bits(op, &d); }
  const data_t &data() const { return d; }
  ac_int<16,true> data_ac_int() const { return ac_int<16,true>(d); }

  // mirroed most constructors in tensorflow implementation (except template version)
  //   tensorflow uses static_cast<float>
  //   this implementation goes through ac_std_float so there is no dependency on rounding mode
//  template <class T>
//  explicit bfloat16(const T& val) { *this = bfloat16(static_cast<float>(val)); }
  explicit bfloat16(unsigned short val) {
    ac_std_float_t t;
    t.assign_from<AC_TRN_ZERO>( ac_int<16,false>(val) );
    *this = t;
  }
  explicit bfloat16(int val) {
    ac_std_float_t t;
    t.assign_from<AC_TRN_ZERO>( ac_int<32,true>(val) );
    *this = t;
  }
  explicit bfloat16(unsigned int val) {
    ac_std_float_t t;
    t.assign_from<AC_TRN_ZERO>( ac_int<32,false>(val) );
    *this = t;
  }
  explicit bfloat16(long val) {
    const int long_w = ac_private::long_w;
    ac_std_float_t t;
    t.assign_from<AC_TRN_ZERO>( ac_int<long_w,false>(val) );
    *this = t;
  }
  explicit bfloat16(long long val) {
    ac_std_float_t t;
    t.assign_from<AC_TRN_ZERO>( ac_int<64,false>(val) );
    *this = t;
  }
  explicit bfloat16(double val) { *this = bfloat16(ac_ieee_float<binary64>(val)); }

  template<ac_q_mode QR, bool No_SubNormals, bool Effective_Add=false>
  bfloat16 add(const bfloat16 &op2) const {
    return bfloat16(to_helper_t().add<QR,No_SubNormals,Effective_Add>(op2.to_helper_t()));
  }
  template<ac_q_mode QR, bool No_SubNormals>
  bfloat16 sub(const bfloat16 &op2) const {
    return bfloat16(to_helper_t().sub<QR,No_SubNormals>(op2.to_helper_t()));
  }
  template<ac_q_mode QR, bool No_SubNormals>
  bfloat16 mult(const bfloat16 &op2) const {
    return bfloat16(to_helper_t().mult<QR,No_SubNormals>(op2.to_helper_t()));
  }
  template<ac_q_mode QR, bool No_SubNormals>
  bfloat16 div(const bfloat16 &op2) const {
    return bfloat16(to_helper_t().div<QR,No_SubNormals>(op2.to_helper_t()));
  }
  template<ac_q_mode QR, bool No_SubNormals>
  bfloat16 fma(const bfloat16 &op2, const bfloat16 &op3) const {
    return bfloat16(to_helper_t().fma<QR,No_SubNormals>(op2.to_helper_t(), op3.to_helper_t()));
  }
  template<ac_q_mode QR, bool No_SubNormals>
  bfloat16 sqrt() const {
    return bfloat16(to_helper_t().sqrt<QR,No_SubNormals>());
  }

  bfloat16 operator +(const bfloat16 &op2) const {
    return bfloat16(to_helper_t().add<AC_STD_FLOAT_BFLOAT16_ROUND,false>(op2.to_helper_t()));
  }
  bfloat16 operator -(const bfloat16 &op2) const {
    return bfloat16(to_helper_t().sub<AC_STD_FLOAT_BFLOAT16_ROUND,false>(op2.to_helper_t()));
  }
  bfloat16 operator *(const bfloat16 &op2) const {
    return bfloat16(to_helper_t().mult<AC_STD_FLOAT_BFLOAT16_ROUND,false>(op2.to_helper_t()));
  }
  bfloat16 operator /(const bfloat16 &op2) const {
    return bfloat16(to_helper_t().div<AC_STD_FLOAT_BFLOAT16_ROUND,false>(op2.to_helper_t()));
  }
  bfloat16 &operator +=(const bfloat16 &op2) {
    return *this = operator +(op2);
  }
  bfloat16 &operator -=(const bfloat16 &op2) {
    return *this = operator -(op2);
  }
  bfloat16 &operator *=(const bfloat16 &op2) {
    return *this = operator *(op2);
  }
  bfloat16 &operator /=(const bfloat16 &op2) {
    return *this = operator /(op2);
  }

  bool operator ==(const bfloat16 &op2) const {
    return to_helper_t() == op2.to_helper_t();
  }
  bool operator !=(const bfloat16 &op2) const {
    return to_helper_t() != op2.to_helper_t();
  }
  bool operator <(const bfloat16 &op2) const {
    return to_helper_t() < op2.to_helper_t();
  }
  bool operator >=(const bfloat16 &op2) const {
    return to_helper_t() >= op2.to_helper_t();
  }
  bool operator >(const bfloat16 &op2) const {
    return to_helper_t() > op2.to_helper_t();
  }
  bool operator <=(const bfloat16 &op2) const {
    return to_helper_t() <= op2.to_helper_t();
  }

  bool operator !() const { return !(d & 0x7fff); }
  bfloat16 operator -() const {
    bfloat16 r(*this);
    r.set_signbit(!this->signbit());
    return r;
  }
  bfloat16 operator +() const {
    return bfloat16(*this);
  }
  bfloat16 abs() const {
    bfloat16 r(*this);
    r.set_signbit(false);
    return r;
  }
  bfloat16 copysign(const bfloat16 &op2) const {
    bfloat16 r(*this);
    r.set_signbit(op2.signbit());
    return r;
  }
  bool signbit() const { return d < 0; }
  void set_signbit(bool s) {
    ac_int<width,true> t(d);
    t[width-1] = s;
    d = t;
  }
  bfloat16 ceil() const { return to_helper_t().ceil(); }
  bfloat16 floor() const { return to_helper_t().floor(); }
  bfloat16 trunc() const { return to_helper_t().trunc(); }
  bfloat16 round() const { return to_helper_t().round(); }
};

inline std::ostream& operator << (std::ostream &os, const ac::bfloat16 &x) {
#ifndef __SYNTHESIS__
  if ((os.flags() & std::ios::hex) != 0) {
    os << x.data_ac_int().to_string(AC_HEX,false,true);
  } else if ((os.flags() & std::ios::oct) != 0) {
    os << x.data_ac_int().to_string(AC_OCT,false,true);
  } else {
    os << x.to_float();
  }
#endif
  return os;
}

}

template<int W, int E>
template<ac_ieee_float_format Format>
inline ac_std_float<W,E>::ac_std_float(const ac_ieee_float<Format> &f) {
  *this = ac_std_float(f.to_ac_std_float());
}

template<int W, int E>
inline ac_std_float<W,E>::ac_std_float(const ac::bfloat16 &f) {
  *this = ac_std_float(f.to_ac_std_float());
}

template<ac_ieee_float_format Format>
inline ac_ieee_float<Format>::ac_ieee_float(const ac::bfloat16 &f) {
  *this = ac_ieee_float(f.to_ac_std_float());
}

typedef ac_ieee_float<binary16> ac_ieee_float16;
typedef ac_ieee_float<binary32> ac_ieee_float32;
typedef ac_ieee_float<binary64> ac_ieee_float64;
typedef ac_ieee_float<binary128> ac_ieee_float128;
typedef ac_ieee_float<binary256> ac_ieee_float256;


#ifdef __AC_NAMESPACE
}
#endif

// Global functions for ac_ieee_float
namespace std {
#ifdef __AC_NAMESPACE
using namespace __AC_NAMESPACE;
#endif
template<ac_ieee_float_format Format>
inline ac_ieee_float<Format> abs(const ac_ieee_float<Format> &x) { return x.abs(); }
template<ac_ieee_float_format Format>
inline ac_ieee_float<Format> fabs(const ac_ieee_float<Format> &x) { return x.abs(); }

template<ac_ieee_float_format Format>
inline ac_ieee_float<Format> copysign(const ac_ieee_float<Format> &x, const ac_ieee_float<Format> &y) { return x.copysign(y); }

template<ac_ieee_float_format Format>
inline int fpclassify(const ac_ieee_float<Format> &x) { return x.fpclassify(); }
template<ac_ieee_float_format Format>
inline bool isfinite(const ac_ieee_float<Format> &x) { return x.isfinite(); }
template<ac_ieee_float_format Format>
inline bool isnormal(const ac_ieee_float<Format> &x) { return x.isnormal(); }
template<ac_ieee_float_format Format>
inline bool isinf(const ac_ieee_float<Format> &x) { return x.isinf(); }
template<ac_ieee_float_format Format>
inline bool isnan(const ac_ieee_float<Format> &x) { return x.isnan(); }

// Don't do "long double" versions since they are 80-bits, it is an extended presicion
// TODO: fmod, fmodf, fmodl
// TODO: fmod, remainder, remquo, fma, fmax, fmin, fdim
// remainder(x,y),  x - n*y, where n = x/y rounded to the nearest integer (RND_CONV)
// remquo(x,y, int *quo),  returns same as remainder, unclear what quo is, also Nan, inf etc
// fmax, fmin:  if one number is Nan, the other is returned
// fdim(x,y) returns max(x-y,0), if x or y is NaN, a NaN is returned, if result overflows, HUGE_VAL is returned
// TODO: ceil, floor, trunc, round, lround, nearbyint, rint, lrint, llround, llrint
// if x is +0, -0, NaN or Inf, x is returned
//   ceil(x), floor(x), trunc(x)
//   round(x) : RND_INF
//   nearbyint: depends on rounding mode
//   rint, same as nearbyint, but may raise inexaxt exception (FE_INEXACT)
// TODO: frexp, ldexp, modf, nextafter, nexttoward, copysign
// modf(x, *iptr), modff   break into integral (*iptr) and fractional (returned) values,
// Don't cause exception: isgreater, isgreaterequal, isless, islessequal, islessgreater, isunordered
//  isunordered: x or y is NaN
template<ac_ieee_float_format Format>
inline bool signbit(const ac_ieee_float<Format> &x) { return x.signbit(); }

// Global functions for bfloat16
inline bool signbit(const ac::bfloat16 &x) { return x.signbit(); }

inline int fpclassify(const ac::bfloat16 &x) { return x.fpclassify(); }
inline bool isfinite(const ac::bfloat16 &x) { return x.isfinite(); }
inline bool isnormal(const ac::bfloat16 &x) { return x.isnormal(); }
inline bool isinf(const ac::bfloat16 &x) { return x.isinf(); }
inline bool isnan(const ac::bfloat16 &x) { return x.isnan(); }
}

#undef __AC_DATA_PRIVATE
#undef AC_STD_FLOAT_FX_DIV_OVERRIDE

#endif
