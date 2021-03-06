/*
 * Copyright (c) 2019 Nobuyuki Umetani
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */


#ifndef DFM2_OBJFUNC_V23DTRI_h
#define DFM2_OBJFUNC_V23DTRI_h

#include "delfem2/dfm2_inline.h"
#include "delfem2/dtri2_v2dtri.h"

// ------------------------------

namespace delfem2 {

void PBD_TriStrain(double* aXYZt,
                   unsigned int nXYZ,
                   const std::vector<delfem2::CDynTri>& aETri,
                   const std::vector<CVec2d>& aVec2);

void PBD_Bend(double* aXYZt,
              unsigned int nXYZ,
              const std::vector<delfem2::CDynTri>& aETri,
              const std::vector<CVec2d>& aVec2,
              double ratio);

} // namespace delfem2

#ifdef DFM2_HEADER_ONLY
#  include "delfem2/objfdtri_objfdtri23.cpp"
#endif

#endif /* pbd_v23_h */
