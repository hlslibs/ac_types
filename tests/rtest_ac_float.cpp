#include <ac_float.h>
#include <cstdio>

int main() {
  ac_float<25, 2, 8> a = -0x1.000002p-1f;
  ac_float<12, 1, 7> b = 1.0f;
  ac_float<13, 1, 7> c = -0x1.002p0f;
  ac_float<25, 2, 8> result;
  result.sub(a, b * c);
  // Exact difference: 1/2 + 2^-11 - 2^-24, with exponent -1.
  const int expected_mantissa = (1 << 23) + (1 << 13) - 1;
  int actual_mantissa = result.mantissa().slc<25>(0).to_int();
  int actual_exponent = result.exp().to_int();
  std::printf("actual=(%d,%d) expected=(%d,-1)\n",
              actual_mantissa, actual_exponent, expected_mantissa);
  return actual_mantissa == expected_mantissa && actual_exponent == -1 ? 0 : 1;
}
