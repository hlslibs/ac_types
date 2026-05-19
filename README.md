PandA/Bambu ac_types
====================

PandA/Bambu ac_types is a derived work from the original Algorithmic C (tm)
Datatypes distributed within the Open-Source High-Level Synthesis IP Libraries
by Mentor Graphics Corporation.

This work has been derived from the source code distributed at:

  https://github.com/hlslibs/ac_types

The original Algorithmic C (tm) Datatypes are licensed under the Apache License,
Version 2.0. The original copyright and license notices have been retained in
the corresponding source files and summarized in NOTICE.

PandA/Bambu modifications and additions are Copyright (C) 2018-2026
Politecnico di Milano.

PandA/Bambu ac_types, including the PandA/Bambu modifications and additions, is
licensed under the Apache License, Version 2.0, with the BAMBU exceptions stated
in LICENSE.

Specific changes to the original sources are reported in each file.
PandA/Bambu changes include:

  - Modifications to allow compilation with Clang/LLVM compilers.
  - Improved implementation to allow lowering to builtin data types on
    Clang/LLVM compilers.
  - Additional bitwidth-adjustment support for HLS bit-value and range
    analysis.
  - iv_base data structure specialization for storage of large integers to help
    Scalar Replacement Of Aggregates (SROA) optimize the generated Intermediate
    Representation (IR).
  - Forced inlining of many operators to help compiler optimizations.
  - Use of constexpr and C++14/C++17 constructs to improve compile-time
    optimizations and constant propagation.
  - Compile-time loop unrolling through modern C++ constructs.
  - Extensions to the original types API for compatibility with AMD/Xilinx AP
    Datatypes, including range operators and string conversions.
  - Additional wrapper headers for ap_int, ap_fixed, ap_shift_reg, and
    hls_stream compatibility.

The file include/utils/x_hls_utils.h includes code taken from the hls4ml
project, licensed under the Apache License, Version 2.0.
