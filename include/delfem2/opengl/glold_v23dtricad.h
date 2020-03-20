/*
 * Copyright (c) 2019 Nobuyuki Umetani
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef cad_dyntri_v23_gl_h
#define cad_dyntri_v23_gl_h


#include <stdio.h>
#include <vector>

#include "delfem2/dtri.h"
#include "delfem2/vec2.h"
#include "delfem2/vec3.h"
#include "delfem2/cad2d_v2dtri.h"

// ---------------------------

namespace delfem2{
namespace opengl{


void DrawMeshDynTri_FaceNorm(const std::vector<CDynTri>& aSTri,
                             const std::vector<CVec3d>& aVec3);

void DrawMeshDynTri_FaceNorm(const std::vector<CDynTri>& aSTri,
                             const double* aXYZ);

void DrawMeshDynTri_Edge(const std::vector<CDynTri>& aSTri,
                         const std::vector<CVec3d>& aVec3);


void DrawMeshDynTri_Edge(const std::vector<CDynTri>& aSTri,
                         const std::vector<CVec2d>& aVec2);

void DrawMeshDynTri_FaceNorm(const std::vector<CDynTri>& aSTri,
                             const std::vector<CVec2d>& aVec2);

void DrawMeshDynTri3D_Edge(const std::vector<double>& aXYZ,
                           const std::vector<CDynTri>& aSTri);

// --------------------------------------------

void Draw_CCad2DEdge(const delfem2::CCad2D_EdgeGeo& edge,
                     bool is_selected,
                     int ipicked_elem);

void Draw_CCad2D(const delfem2::CCad2D& cad2d);
  
}
}

#endif /* cad_dyntri_v23_gl_hpp */
