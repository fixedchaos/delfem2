/*
 * Copyright (c) 2020 Nobuyuki Umetani
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "delfem2/defarap.h"
#include "delfem2/mat4.h"
#include "delfem2/primitive.h"
#include <cmath>

// ----------------
#include <GLFW/glfw3.h>
#include "delfem2/opengl/funcs_glold.h"
#include "delfem2/opengl/v3q_glold.h"
#include "delfem2/opengl/glfw/viewer_glfw.h"

namespace dfm2 = delfem2;

// -------------------------------------

void SetPositionAtFixedBoundary
 (std::vector<double>& aRhs,
  unsigned int iframe,
  const std::vector<double>& aXYZ0,
  const std::vector<int>& aBCFlag)
{
  double A[16];
  {
    dfm2::Mat4_Identity(A);
    const double trans0[3] = {0, -0.8, 0};
    dfm2::Translate_Mat4Affine(A,
                               trans0);
    const double axis0[3] = {0, +2.0*sin(0.03*iframe), 1.0*sin(0.07*iframe)};
    dfm2::Rotate_Mat4AffineRodriguez(A,
                                     axis0);
    const double trans1[3] = {0.2*sin(0.03*iframe), +0.5+0.1*cos(0.05*iframe), 0};
    dfm2::Translate_Mat4Affine(A,
                               trans1);
  }
  const unsigned int np = aRhs.size()/3;
  for(unsigned int ip=0;ip<np;++ip){
    if( aBCFlag[ip*3+0] == 0 ){ continue; }
    if( aBCFlag[ip*3+0] == 1 ){
      aRhs[ip*3+0] = aXYZ0[ip*3+0];
      aRhs[ip*3+1] = aXYZ0[ip*3+1];
      aRhs[ip*3+2] = aXYZ0[ip*3+2];
    }
    if( aBCFlag[ip*3+0] == 2 ) {
      dfm2::Vec3_Mat4Vec3_AffineProjection(aRhs.data()+ip*3, A, aXYZ0.data()+ip*3);
    }
  }
}

void myGlutDisplay_Mesh
(const std::vector<double>& aXYZ0,
const std::vector<double>& aXYZ1,
const std::vector<unsigned int>& aTri)
{
  ::glLineWidth(1);
  ::glDisable(GL_LIGHTING);
  ::glColor3d(1,0,0);
  dfm2::opengl::DrawMeshTri3D_FaceNorm(aXYZ1,aTri);
  ::glColor3d(0.8,0.8,0.8);
  dfm2::opengl::DrawMeshTri3D_Edge(aXYZ0.data(),aXYZ0.size()/3,
                                   aTri.data(),aTri.size()/3);
  ::glColor3d(0,0,0);
  dfm2::opengl::DrawMeshTri3D_Edge(aXYZ1.data(),aXYZ1.size()/3,
                                   aTri.data(),aTri.size()/3);
  
}

void Draw_BCFlag(const std::vector<double>& aXYZ1,
                 const std::vector<int>& aBCFlag)
{ // draw bc as a point
  ::glPointSize(10);
  ::glBegin(GL_POINTS);
  for(unsigned int ip=0;ip<aXYZ1.size()/3;++ip){
    if( aBCFlag[ip*3+0] == 0 ){ continue; }
    if( aBCFlag[ip*3+0] == 1 ){ ::glColor3d(0,0,1); }
    if( aBCFlag[ip*3+0] == 2 ){ ::glColor3d(0,1,0); }
    ::glVertex3dv(aXYZ1.data()+ip*3);
  }
  ::glEnd();
}


// --------------------------------------------------

int main(int argc,char* argv[])
{
  dfm2::opengl::CViewer_GLFW viewer;
  viewer.Init_oldGL();
  viewer.nav.camera.view_height = 1.0;
  viewer.nav.camera.camera_rot_mode = delfem2::CCamera<double>::CAMERA_ROT_MODE::TBALL;
  delfem2::opengl::setSomeLighting();
  
  std::vector<unsigned int> aTri;
  std::vector<double> aXYZ0;
  std::vector<int> aBCFlag;
  {
    dfm2::MeshTri3D_CylinderClosed(aXYZ0, aTri,
                                   0.2, 1.6,
                                   32, 32);
    const unsigned int np = aXYZ0.size() / 3;
    aBCFlag.assign(np * 3, 0);
    for(unsigned int ip=0;ip<np;++ip) {
      double y0 = aXYZ0[ip*3+1];
      if( y0 < -0.65 ){
        aBCFlag[ip*3+0] = 1;
        aBCFlag[ip*3+1] = 1;
        aBCFlag[ip*3+2] = 1;
      }
      if( y0 > +0.65 ){
        aBCFlag[ip*3+0] = 2;
        aBCFlag[ip*3+1] = 2;
        aBCFlag[ip*3+2] = 2;
      }
    }
  }
  
  while (true){
    std::vector<double> aXYZ1 = aXYZ0;
    std::vector<double> aQuat1(aXYZ0.size()/3*4);
    for(unsigned int ip=0;ip<aXYZ0.size()/3;++ip){
      dfm2::Quat_Identity(aQuat1.data()+4*ip);
    }
    int iframe = 0;
    { // arap edge linear disponly
      dfm2::CDef_Arap def0;
      def0.Init(aXYZ0, aTri, false);
      glfwSetWindowTitle(viewer.window, "ARAP w.o. Preconditioner");
      for(;iframe<200;++iframe)
      {
        SetPositionAtFixedBoundary(aXYZ1,
                                   iframe,aXYZ0,aBCFlag);
        def0.Deform(aXYZ1,aQuat1,
                    aXYZ0,aBCFlag);
        // --------------------
        viewer.DrawBegin_oldGL();
        myGlutDisplay_Mesh(aXYZ0,aXYZ1,aTri);
        dfm2::opengl::Draw_QuaternionsCoordinateAxes(aXYZ1,aQuat1,0.04);
        Draw_BCFlag(aXYZ1,aBCFlag);
        viewer.DrawEnd_oldGL();
        if( glfwWindowShouldClose(viewer.window) ){ goto CLOSE; }
      }
    } // end linear disponly
    { // arap edge linear disponly
      dfm2::CDef_Arap def0;
      def0.Init(aXYZ0, aTri, true);
      glfwSetWindowTitle(viewer.window, "ARAP with Preconditioner");
      for(;iframe<400;++iframe)
      {
        SetPositionAtFixedBoundary(aXYZ1,
                                   iframe,aXYZ0,aBCFlag);
        def0.Deform(aXYZ1,aQuat1,
                    aXYZ0,aBCFlag);
        // --------------------
        viewer.DrawBegin_oldGL();
        myGlutDisplay_Mesh(aXYZ0,aXYZ1,aTri);
        dfm2::opengl::Draw_QuaternionsCoordinateAxes(aXYZ1,aQuat1,0.04);
        Draw_BCFlag(aXYZ1,aBCFlag);
        viewer.DrawEnd_oldGL();
        if( glfwWindowShouldClose(viewer.window) ){ goto CLOSE; }
      }
    } // end linear disponly
  }
  
CLOSE:
  glfwDestroyWindow(viewer.window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}


