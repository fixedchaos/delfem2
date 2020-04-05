/*
 * Copyright (c) 2019 Nobuyuki Umetani
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef funcs_glcolorvec3_hpp
#define funcs_glcolorvec3_hpp

#include "delfem2/vec3.h"

#include "delfem2/opengl/color_glold.h"

#include <stdio.h>

void DrawQuad_ScalarQ1
(const delfem2::CVec3d& p0,
 const delfem2::CVec3d& p1,
 const delfem2::CVec3d& p2,
 const delfem2::CVec3d& p3,
 double v0, double v1, double v2, double v3,
 const std::vector<std::pair<double, delfem2::CColor> >& colorMap);

#endif /* funcs_glcolorvec3_hpp */
