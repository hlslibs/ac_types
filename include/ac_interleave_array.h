/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Datatypes                                          *
 *                                                                        *
 *  Software Version: 4.8                                                 *
 *                                                                        *
 *  Release Date    : Sun Jan 28 19:38:23 PST 2024                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 4.8.0                                               *
 *                                                                        *
 *  Copyright 2004-2022, Mentor Graphics Corporation,                     *
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
//  Source:         ac_interleave_array.h
*/

#ifndef __AC_INTERLEAVE_ARRAY_H__
#define __AC_INTERLEAVE_ARRAY_H__

#ifndef __cplusplus
# error C++ is required to include this header file
#endif

#include <ac_bank_array.h>

template <typename B, size_t FirstDim, size_t InterleaveFactor>
class ac_interleave_array_1D
{
  public: // required public for SCVerify
    ac_bank_array_2D<B, InterleaveFactor, FirstDim/InterleaveFactor> mem_bank;
    static const size_t W = FirstDim;

    ac_interleave_array_1D () {
      const size_t ComputedInterleaveFactor = 1UL<<BankBits(InterleaveFactor);
      static_assert(ComputedInterleaveFactor == InterleaveFactor, "Error: interleaving is only possible when interleaving factor is power of two.");
    }

    static constexpr size_t BankBits(size_t n) {
      return ((n) > 1 ? 1 + BankBits((n) >> 1) : 0);
    }

    B &operator[](size_t idx) {
#ifndef __SYNTHESIS__
      assert(idx < FirstDim);
#endif
      size_t aidx = idx & (W-1);
      size_t bank_idx = aidx&(InterleaveFactor-1); // low order address bits select the bank
      size_t block_idx = aidx>>(BankBits(InterleaveFactor));  // high order address bits select the blocks within the bank
      return mem_bank[bank_idx][block_idx];
   }

    const B &operator[](size_t idx) const {
#ifndef __SYNTHESIS__
      assert(idx < FirstDim);
#endif
      size_t aidx = idx & (W-1);
      size_t bank_idx = aidx&(InterleaveFactor-1); // low order address bits select the bank
      size_t block_idx = aidx>>(BankBits(InterleaveFactor)); // high order address bits select the blocks within the bank
      return mem_bank[bank_idx][block_idx];
    }
};

template <typename B, size_t FirstDim, size_t SecondDim, size_t InterleaveFactor>
class ac_interleave_array_2D
{
  public: // required public for SCVerify
    ac_bank_array_2D<B, InterleaveFactor, (FirstDim*SecondDim/InterleaveFactor)> mem_bank;

    ac_interleave_array_2D () {
      const size_t ComputedInterleaveFactor = 1UL<<BankBits(InterleaveFactor);
      static_assert(ComputedInterleaveFactor == InterleaveFactor, "Error: interleaving is only possible when interleaving factor is power of two.");
      //static_assert(InterleaveFactor <= SecondDim, "Error: interleaving factor cannot be higher than second dimension.");
    }

    static constexpr size_t BankBits(size_t n) {
      return ((n) > 1 ? 1 + BankBits((n) >> 1) : 0);
    }

    class SecondDimAccess{
      public: // required public for SCVerify
        size_t idxFirstDim;
        ac_interleave_array_2D & intMemObj;
      public:
        SecondDimAccess(ac_interleave_array_2D & obj, size_t idx1) : idxFirstDim(idx1), intMemObj(obj) {}

        B &operator[](size_t idx2) {
#ifndef __SYNTHESIS__
          assert(idx2 < SecondDim);
#endif
          return intMemObj.mem_bank[idx2][idxFirstDim];
        }
    };

    SecondDimAccess operator[](size_t idx1) {
#ifndef __SYNTHESIS__
      assert(idx1 < FirstDim);
#endif
      return SecondDimAccess(*this, idx1);
    }

    class ConstSecondDimAccess{
      public: // required public for SCVerify
        size_t idxFirstDim;
        const ac_interleave_array_2D & intMemObj;
      public:
        ConstSecondDimAccess(const ac_interleave_array_2D & obj, size_t idx1) : idxFirstDim(idx1), intMemObj(obj) {}

        const B &operator[](size_t idx2) const {
#ifndef __SYNTHESIS__
          assert(idx2 < SecondDim);
#endif
          return intMemObj.mem_bank[idx2][idxFirstDim];
        }
    };

    ConstSecondDimAccess operator[](size_t idx1) const {
#ifndef __SYNTHESIS__
      assert(idx1 < FirstDim);
#endif
      return ConstSecondDimAccess(*this, idx1);
    }
};

template <typename B, size_t FirstDim, size_t SecondDim, size_t ThirdDim, size_t InterleaveFactor>
class ac_interleave_array_3D
{
  public: // required public for SCVerify
    // the following is valid only when InterleaveFactor == ThirdDim
    // consider when it is not equal later
    ac_bank_array_2D<B [SecondDim], InterleaveFactor, FirstDim> mem_bank;

  public:
    ac_interleave_array_3D () {
      const size_t ComputedInterleaveFactor = 1UL<<BankBits(InterleaveFactor);
      static_assert(ComputedInterleaveFactor == InterleaveFactor, "Error: interleaving is only possible when interleaving factor is power of two.");
      static_assert(InterleaveFactor == ThirdDim, "Error: interleaving factor should be equal to the third dimension. Currently, only the last dimension is interleaved.");
    }

    static constexpr size_t BankBits(size_t n) {
      return ((n) > 1 ? 1 + BankBits((n) >> 1) : 0);
    }

    class ThirdDimAccess{
      public: // required public for SCVerify
        size_t idxFirstDim;
        size_t idxSecondDim;
        ac_interleave_array_3D & intMemObj;

      public:
        ThirdDimAccess(ac_interleave_array_3D & obj, size_t idx1, size_t idx2) : idxFirstDim(idx1), idxSecondDim(idx2), intMemObj(obj) {}

        B &operator[](size_t idx3) {
#ifndef __SYNTHESIS__
          assert(idx3 < ThirdDim);
#endif
          return intMemObj.mem_bank[idx3][idxFirstDim][idxSecondDim];
        }
    };

    class SecondDimAccess{
      public: // required public for SCVerify
        size_t idxFirstDim;
        ac_interleave_array_3D & intMemObj;

      public:
        SecondDimAccess(ac_interleave_array_3D & obj, size_t idx1) : idxFirstDim(idx1), intMemObj(obj) {}

        ThirdDimAccess operator[](size_t idx2) {
#ifndef __SYNTHESIS__
          assert(idx2 < SecondDim);
#endif
          return ThirdDimAccess(intMemObj, idxFirstDim, idx2);
        }
    };

    SecondDimAccess operator[](size_t idx1) {
#ifndef __SYNTHESIS__
      assert(idx1 < FirstDim);
#endif
      return SecondDimAccess(*this, idx1);
    }

    class ConstThirdDimAccess{
      public: // required public for SCVerify
        size_t idxFirstDim;
        size_t idxSecondDim;
        const ac_interleave_array_3D & intMemObj;

      public:
        ConstThirdDimAccess(const ac_interleave_array_3D & obj, size_t idx1, size_t idx2) : idxFirstDim(idx1), idxSecondDim(idx2), intMemObj(obj) {}

        const B &operator[](size_t idx3) const {
#ifndef __SYNTHESIS__
          assert(idx3 < ThirdDim);
#endif
          return intMemObj.mem_bank[idx3][idxFirstDim][idxSecondDim];
        }
    };

    class ConstSecondDimAccess{
      public: // required public for SCVerify
        size_t idxFirstDim;
        const ac_interleave_array_3D & intMemObj;

      public:
        ConstSecondDimAccess(const ac_interleave_array_3D & obj, size_t idx1) : idxFirstDim(idx1), intMemObj(obj) {}

        ConstThirdDimAccess operator[](size_t idx2) const {
#ifndef __SYNTHESIS__
          assert(idx2 < SecondDim);
#endif
          return ConstThirdDimAccess(intMemObj, idxFirstDim, idx2);
        }
    };

    ConstSecondDimAccess operator[](size_t idx1) const {
#ifndef __SYNTHESIS__
      assert(idx1 < FirstDim);
#endif
      return ConstSecondDimAccess(*this, idx1);
    }
};

#endif

