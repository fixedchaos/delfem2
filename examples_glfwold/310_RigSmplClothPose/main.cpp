/*
* Copyright (c) 2019 Nobuyuki Umetani
*
* This source code is licensed under the MIT license found in the
* LICENSE file in the root directory of this source tree.
*/

/**
 * @brief SMPL model
 * @details skinning
 */

#include <cstdlib>
#include <random>
#include <GLFW/glfw3.h>
#include "delfem2/garment.h"
#include "delfem2/bv.h"
#include "delfem2/mshmisc.h"
#include "delfem2/srch_v3bvhmshtopo.h"
#include "delfem2/objf_geo3.h"
#include "delfem2/objfdtri_objfdtri23.h"
#include "delfem2/rig_geo3.h"
#include "delfem2/cnpy/smpl_cnpy.h"
#include "inputs_garment.h"

#include "delfem2/opengl/funcs_glold.h"
#include "delfem2/opengl/v3q_glold.h"
#include "delfem2/opengl/color_glold.h"
#include "delfem2/opengl/caddtri_v3_glold.h"
#include "delfem2/opengl/glfw/viewer_glfw.h"

namespace dfm2 = delfem2;

// ----------------------------------
void Draw
(const std::vector<double>& aXYZ1,
 const std::vector<unsigned int>& aTri,
 const std::vector<dfm2::CRigBone>& aBone,
 const std::vector<dfm2::CTarget>& aTarget)
{
  ::glEnable(GL_LIGHTING);
  ::glEnable(GL_DEPTH_TEST);
  dfm2::opengl::DrawMeshTri3D_FaceNorm(aXYZ1.data(), aTri.data(), aTri.size()/3);
  //    dfm2::opengl::DrawMeshTri3D_FaceNorm(aXYZ0.data(), aTri.data(), aTri.size()/3);
  ::glDisable(GL_DEPTH_TEST);
  ::glDisable(GL_LIGHTING);
  ::glPointSize(10);
  ::glBegin(GL_POINTS);
  for(const auto & it : aTarget){
    const unsigned int ib = it.ib;
    ::glColor3d(1,0,0);
    dfm2::opengl::myGlVertex(aBone[ib].Pos());
  }
  ::glEnd();
  // ------
  ::glEnable(GL_DEPTH_TEST);
  ::glBegin(GL_LINES);
  ::glColor3d(1,0,0);
  for(const auto & it : aTarget){
    dfm2::CVec3d p = it.pos;
    dfm2::opengl::myGlVertex(p+10.0*dfm2::CVec3d(0,0,1));
    dfm2::opengl::myGlVertex(p-10.0*dfm2::CVec3d(0,0,1));
  }
  ::glEnd();
  /*
   ::glDisable(GL_DEPTH_TEST);
   delfem2::opengl::DrawBone(aBone,
   -1, -1,
   0.01, 1.0);
   */
  ::glEnable(GL_DEPTH_TEST);
  //    dfm2::opengl::Draw_QuaternionsCoordinateAxes(aXYZ1,aQuat1,0.02);
}

// --------------------
int main()
{
  const double dt = 0.01;
  const double gravity[3] = {0.0, -0.1, 0.0};
  const double contact_clearance = 0.001;
  const double rad_explore = 0.1;
  const double mesher_edge_length = 0.02;
  const double bend_stiffness_ratio = 0.01;

  // -----------------------------
  // below: input data
  std::vector<dfm2::CDynTri> aETri_Cloth;
  std::vector<dfm2::CVec2d> aVec2_Cloth;
  std::vector<double> aXYZ_Cloth; // deformed vertex positions
  std::vector<unsigned int> aLine_Cloth;
  dfm2::CMesher_Cad2D mesher;
  {
    std::string name_cad_in_test_input;
    double scale_adjust = 0.0;
    std::vector<unsigned int> aIESeam;
    double mesher_edge_length0;
    std::vector<dfm2::CRigidTrans_2DTo3D> aRT23;
    // -------
    //    Inputs_SmplTshirt(name_cad_in_test_input,
    Inputs_SmplLtshirt(name_cad_in_test_input,
                       scale_adjust,
                       aIESeam,
                       mesher_edge_length0,
                       aRT23);
    std::string path_svg = std::string(PATH_INPUT_DIR)+"/"+name_cad_in_test_input;
    std::cout << "open svg: " << path_svg << std::endl;
    delfem2::CCad2D cad;
    dfm2::ReadSVG_Cad2D(cad, path_svg, 0.001*scale_adjust);
    // -------
    dfm2::MeshingPattern(aETri_Cloth,aVec2_Cloth,aXYZ_Cloth,aLine_Cloth,mesher,
                         aRT23,cad,aIESeam,mesher_edge_length);
  }
  std::vector<double> aXYZt_Cloth = aXYZ_Cloth;
  std::vector<double> aUVW_Cloth(aXYZ_Cloth.size(), 0.0);
  const std::vector<int> aBCFlag_Cloth(aXYZ_Cloth.size()/3, 0);
  std::vector<dfm2::CInfoNearest<double>> aInfoNearest_Cloth;
  
  // ----------
  std::vector<double> aXYZ0_Body;
  std::vector<double> aW_Body;
  std::vector<dfm2::CRigBone> aBone;
  std::vector<unsigned int> aTri_Body;
  {
    std::vector<int> aIndBoneParent;
    std::vector<double> aJntRgrs;
    dfm2::cnpy::LoadSmpl(aXYZ0_Body,
                         aW_Body,
                         aTri_Body,
                         aIndBoneParent,
                         aJntRgrs,
                         std::string(PATH_INPUT_DIR)+"/smpl_model_f.npz");
    dfm2::Smpl2Rig(aBone,
                   aIndBoneParent, aXYZ0_Body, aJntRgrs);
    
  }
  
  std::vector<double> aXYZ1_Body = aXYZ0_Body;
  { // initalize pose
    dfm2::UpdateBoneRotTrans(aBone);
    dfm2::Skinning_LBS(aXYZ1_Body,
                       aXYZ0_Body, aBone, aW_Body);
  }
  std::vector< dfm2::CQuatd > aQuatTarget;
  {
    std::ifstream fin(std::string(PATH_OUTPUT_DIR)+"/pose.txt");
    for(unsigned int ib=0;ib<aBone.size();++ib){
      double d0,d1,d2,d3;
      fin >> d0 >> d1 >> d2 >> d3;
      auto q = dfm2::CQuatd(d0,d1,d2,d3);
      q.SetSmallerRotation();
      aQuatTarget.push_back( q );
    }
    {
      dfm2::CVec3d posTarget;
      double d0,d1,d2;
      fin >> d0 >> d1 >> d2;
      posTarget = dfm2::CVec3d(d0,d1,d2);
    }
  }
  std::vector<double> aNorm_Body(aXYZ1_Body.size());
  delfem2::Normal_MeshTri3D(aNorm_Body.data(),
                            aXYZ1_Body.data(), aXYZ1_Body.size()/3,
                            aTri_Body.data(), aTri_Body.size()/3);
  dfm2::CBVH_MeshTri3D<dfm2::CBV3d_Sphere,double> bvh_Body;
  bvh_Body.Init(aXYZ1_Body.data(), aXYZ1_Body.size()/3,
                aTri_Body.data(), aTri_Body.size()/3,
                contact_clearance);

   // -----------
  dfm2::opengl::CViewer_GLFW viewer;
  viewer.Init_oldGL();
  viewer.nav.camera.camera_rot_mode = dfm2::CCamera<double>::CAMERA_ROT_MODE::YTOP;
  viewer.nav.camera.view_height = 1.0;
  dfm2::opengl::setSomeLighting();

  unsigned int iframe = 0;
  while (true)
  {
    const unsigned int iframe0 = 300;
    const unsigned int iframe1 = 1500;
    iframe++ ;
    std::cout << iframe << std::endl;
    if( iframe < iframe0 ){
      dfm2::StepTime_PbdClothSim(
          aXYZ_Cloth, aXYZt_Cloth, aUVW_Cloth, aInfoNearest_Cloth,
          aBCFlag_Cloth, aETri_Cloth, aVec2_Cloth, aLine_Cloth,
          //
          aXYZ1_Body, aTri_Body,aNorm_Body, bvh_Body,
          dt,gravity,contact_clearance,rad_explore,bend_stiffness_ratio);
    }
    else if( iframe < iframe1 ){
      double r = (double)(iframe-iframe0)/(iframe1-iframe0);
      if( r > 1 ){ r = 1; }
      for(unsigned int ib=0;ib<aBone.size();++ib){
        dfm2::CQuatd q = dfm2::SphericalLinearInterp( dfm2::CQuatd::Identity(), aQuatTarget[ib], r);
        q.SetNormalized();
        q.CopyTo(aBone[ib].quatRelativeRot);
      }
      dfm2::UpdateBoneRotTrans(aBone);
      Skinning_LBS(aXYZ1_Body,
                   aXYZ0_Body, aBone, aW_Body);
      if( iframe % 100 == 0 ){
        bvh_Body.Init(aXYZ1_Body.data(), aXYZ1_Body.size()/3,
                      aTri_Body.data(), aTri_Body.size()/3,
                      contact_clearance);
      }
      bvh_Body.UpdateGeometry(aXYZ1_Body.data(), aXYZ1_Body.size()/3,
                              aTri_Body.data(), aTri_Body.size()/3,
                              contact_clearance);
      delfem2::Normal_MeshTri3D(aNorm_Body.data(),
                                aXYZ1_Body.data(), aXYZ1_Body.size()/3,
                                aTri_Body.data(), aTri_Body.size()/3);
      dfm2::StepTime_PbdClothSim(
          aXYZ_Cloth, aXYZt_Cloth, aUVW_Cloth,aInfoNearest_Cloth,
          aBCFlag_Cloth, aETri_Cloth, aVec2_Cloth, aLine_Cloth,
          //
          aXYZ1_Body, aTri_Body,aNorm_Body, bvh_Body,
          dt,gravity,contact_clearance,rad_explore,bend_stiffness_ratio);
    }
    else{
      dfm2::StepTime_PbdClothSim(
          aXYZ_Cloth, aXYZt_Cloth, aUVW_Cloth,aInfoNearest_Cloth,
          aBCFlag_Cloth, aETri_Cloth, aVec2_Cloth, aLine_Cloth,
          //
          aXYZ1_Body, aTri_Body,aNorm_Body, bvh_Body,
          dt,gravity,contact_clearance,rad_explore,bend_stiffness_ratio);
    }

    // -------------------
    viewer.DrawBegin_oldGL();
    {
      ::glEnable(GL_LIGHTING);
      dfm2::opengl::myGlColorDiffuse( dfm2::CColor::Gray(0.8f) );
      //    delfem2::opengl::DrawMeshTri3D_Edge(aXYZ_Contact.data(), aXYZ_Contact.size()/3,
      //                                        aTri_Contact.data(), aTri_Contact.size()/3);
      // draw body
      ::glEnable(GL_LIGHTING);
      dfm2::opengl::myGlColorDiffuse( dfm2::CColor::Gray(0.8f) );
      delfem2::opengl::DrawMeshTri3D_FaceNorm(aXYZ1_Body.data(),
                                              aTri_Body.data(), aTri_Body.size()/3);
      // draw cloth
      ::glDisable(GL_LIGHTING);
      ::glColor3d(0,0,0);
      //    delfem2::opengl::DrawMeshDynTri3D_Edge(aXYZ, aETri);
      delfem2::opengl::DrawMeshDynTri3D_Edge(aXYZ_Cloth, aETri_Cloth);
      // draw cloth
      ::glEnable(GL_LIGHTING);
      dfm2::opengl::myGlColorDiffuse( dfm2::CColor::Red() );
      delfem2::opengl::DrawMeshDynTri_FaceNorm(aETri_Cloth, aXYZ_Cloth.data());
    }
    glfwSwapBuffers(viewer.window);
    glfwPollEvents();
    if( glfwWindowShouldClose(viewer.window) ){ goto EXIT; }
  }
EXIT:
  glfwDestroyWindow(viewer.window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}
