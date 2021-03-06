/*
 * Copyright (c) 2019 Nobuyuki Umetani
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */


#include <cassert>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include "delfem2/mshtopo.h"
#include "delfem2/mats.h"
#include "delfem2/primitive.h"
#include "delfem2/iss.h"
#include "delfem2/vecxitrsol.h"
#include "delfem2/ilu_mats.h"
#include "delfem2/fem_emats.h"

// ---------------
#include <GLFW/glfw3.h>
#include "delfem2/opengl/color_glold.h"
#include "delfem2/opengl/v3q_glold.h"
#include "delfem2/opengl/funcs_glold.h"
#include "delfem2/opengl/glfw/viewer_glfw.h"

namespace dfm2 = delfem2;

// ------------------------

std::vector<unsigned int> aTet;
std::vector<double> aXYZ;
std::vector<int> aIsSurf;

std::vector<double> aVal;
std::vector<double> aVelo;
std::vector<double> aAcc;

std::vector<int> aBCFlag;

dfm2::CMatrixSparse<double> mat_A;
std::vector<double> vec_b;
delfem2::CPreconditionerILU<double>  ilu_A;

double cur_time = 0.0;
double dt_timestep = 0.01;
//double gamma_newmark = 0.6;
//double beta_newmark = 0.36;
double gamma_newmark = 1;
double beta_newmark = 0.5;

// ----------------------------

void InitializeProblem_Poisson()
{
  const int np = (int)aXYZ.size()/3;
  aVal.assign(np, 0.0);
  aBCFlag.assign(np, 0);
  for(int ip=0;ip<np;++ip){
//    const double px = aXYZ[ip*3+0];
    const double py = aXYZ[ip*3+1];
//    const double pz = aXYZ[ip*3+2];
    if( py > +0.4 ){
      aBCFlag[ip] = 1;
      aVal[ip] = 1.0;
    }
    if( py < -0.4 ){
      aBCFlag[ip] = 1;
      aVal[ip] = 0.0;
    }
  }
  //
  std::vector<unsigned int> psup_ind, psup;
  dfm2::JArray_PSuP_MeshElem(psup_ind, psup,
                             aTet.data(), aTet.size()/4, 4,
                             (int)aXYZ.size()/3);
  dfm2::JArray_Sort(psup_ind, psup);
  /*
  CJaggedArray crs;
  crs.SetEdgeOfElem(aTet, (int)aTet.size()/4, 4, (int)aXYZ.size()/3, false);
  crs.Sort();
   */
  ////
  mat_A.Initialize(np, 1, true); // diagonal
  mat_A.SetPattern(psup_ind.data(), psup_ind.size(),
                   psup.data(),     psup.size());
//  ilu_A.Initialize_ILU0(mat_A);
  ilu_A.Initialize_ILUk(mat_A, 0);
}

void SolveProblem_Poisson()
{
  const int np = (int)aXYZ.size()/3;
  const int nDoF = np;
  /////////////////////////////
  const double alpha = 1.0;
  const double source = 0.0;
  mat_A.SetZero();
  vec_b.assign(nDoF, 0.0);
  dfm2::MergeLinSys_Poission_MeshTet3D(mat_A,vec_b.data(),
                                       alpha,source,
                                       aXYZ.data(), aXYZ.size()/3,
                                       aTet.data(), aTet.size()/4,
                                       aVal.data());
  mat_A.SetFixedBC(aBCFlag.data());
  dfm2::setRHS_Zero(vec_b, aBCFlag,0);
  // ------------------------
  std::vector<double> vec_x;
  double conv_ratio = 1.0e-4;
  int iteration = 1000;
  ilu_A.SetValueILU(mat_A);
  ilu_A.DoILUDecomp();
  vec_x.resize(vec_b.size());
  Solve_PCG(vec_b.data(),vec_x.data(),
            vec_b.size(),
            conv_ratio,iteration,
            mat_A,ilu_A);
  // ------------------------------
  dfm2::XPlusAY(aVal,nDoF,aBCFlag,
                1.0,vec_x);
}


void InitializeProblem_Diffusion()
{
  //double len = 1.1;
  const int np = (int)aXYZ.size()/3;
  aVal.assign(np, 0.0);
  aVelo.assign(np, 0.0);
  aBCFlag.assign(np, 0);
  for(int ip=0;ip<np;++ip){
//    const double px = aXYZ[ip*3+0];
    const double py = aXYZ[ip*3+1];
//    const double pz = aXYZ[ip*3+2];
    if( py > +0.4 ){
      aBCFlag[ip] = 1;
      aVal[ip] = 1.0;
    }
    if( py < -0.4 ){
      aBCFlag[ip] = 1;
      aVal[ip] = 0.0;
    }
  }
  //
  std::vector<unsigned int> psup_ind, psup;
  dfm2::JArray_PSuP_MeshElem(psup_ind, psup,
                             aTet.data(), aTet.size()/4, 4,
                             (int)aXYZ.size()/3);
  dfm2::JArray_Sort(psup_ind, psup);
  /*
  CJaggedArray crs;
  crs.SetEdgeOfElem(aTet, (int)aTet.size()/4, 4, (int)aXYZ.size()/3, false);
  crs.Sort();
   */
  ////
  mat_A.Initialize(np, 1, true);
  mat_A.SetPattern(psup_ind.data(), psup_ind.size(),
                   psup.data(),     psup.size());
  ilu_A.Initialize_ILU0(mat_A);
}

void SolveProblem_Diffusion()
{
  const int np = (int)aXYZ.size()/3;
  const int nDoF = np;
  /////////////////////////////
  const double alpha = 1.0;
  const double rho = 1.0;
  const double source = 1.0;
  mat_A.SetZero();
  vec_b.assign(nDoF, 0.0);
  dfm2::MergeLinSys_Diffusion_MeshTet3D(mat_A,vec_b.data(),
                                        alpha, rho, source,
                                        dt_timestep, gamma_newmark,
                                        aXYZ.data(), aXYZ.size()/3,
                                        aTet.data(), aTet.size()/4,
                                        aVal.data(),aVelo.data());
  mat_A.SetFixedBC(aBCFlag.data());
  dfm2::setRHS_Zero(vec_b, aBCFlag,0);
  // ------------------------
  std::vector<double> vec_x;
  double conv_ratio = 1.0e-4;
  int iteration = 1000;
  ilu_A.SetValueILU(mat_A);
  ilu_A.DoILUDecomp();
  vec_x.resize(vec_b.size());
  Solve_PCG(vec_b.data(),vec_x.data(),
            vec_b.size(),
            conv_ratio,iteration,
            mat_A,ilu_A);
  // ----------------------
  dfm2::XPlusAYBZ(aVal,nDoF,aBCFlag,
                  dt_timestep*gamma_newmark,vec_x,
                  dt_timestep,aVelo);
  dfm2::XPlusAY(aVelo,nDoF,aBCFlag,
                1.0,vec_x);
  // -----------------------
  dt_timestep = 0.03;
}


void InitializeProblem_ShellEigenPB()
{
  // set boundary condition
  const int np = (int)aXYZ.size()/3;
  const int nDoF = np*3;
  aVal.assign(nDoF, 0.0);
  aBCFlag.assign(nDoF, 0);
  for (int ip = 0; ip<np; ++ip){
    const double px = aXYZ[ip*3+0];
    const double py = aXYZ[ip*3+1];
    const double pz = aXYZ[ip*3+2];
    if (py > 0.45){
      aBCFlag[ip*3+0] = 1;
      aBCFlag[ip*3+1] = 1;
      aBCFlag[ip*3+2] = 1;
    }
  }
  //
  std::vector<unsigned int> psup_ind, psup;
  dfm2::JArray_PSuP_MeshElem(psup_ind, psup,
                             aTet.data(), aTet.size()/4, 4,
                             (int)aXYZ.size()/3);
  dfm2::JArray_Sort(psup_ind, psup);
  /*
  CJaggedArray crs;
  crs.SetEdgeOfElem(aTet, (int)aTet.size()/4, 4, np, false);
  crs.Sort();
   */
  ////
  mat_A.Initialize(np, 3, true);
  mat_A.SetPattern(psup_ind.data(), psup_ind.size(),
                   psup.data(),     psup.size());
  ilu_A.Initialize_ILU0(mat_A);
}

void SolveProblem_LinearSolid_Static()
{
  const int np = (int)aXYZ.size()/3;
  const int nDoF = np*3;
  ////////////////////////////////////////////
  double myu = 1.0;
  double lambda = 1.0;
  double rho = 1.0;
  double g[3] = {0.0, -0.5, 0.0};
  mat_A.SetZero();
  vec_b.assign(nDoF, 0.0);
  dfm2::MergeLinSys_SolidLinear_Static_MeshTet3D(mat_A, vec_b.data(),
                                                 myu, lambda, rho, g,
                                                 aXYZ.data(), aXYZ.size()/3,
                                                 aTet.data(), aTet.size()/4,
                                                 aVal.data());
  mat_A.SetFixedBC(aBCFlag.data());
  dfm2::setRHS_Zero(vec_b, aBCFlag,0);
  // --------------------------------
  std::vector<double> vec_x;
  double conv_ratio = 1.0e-4;
  int iteration = 1000;
  ilu_A.SetValueILU(mat_A);
  ilu_A.DoILUDecomp();
  vec_x.resize(vec_b.size());
  Solve_PCG(vec_b.data(),vec_x.data(),
            vec_b.size(),
            conv_ratio,iteration,
            mat_A,ilu_A);
  // ------------------------------
  dfm2::XPlusAY(aVal, nDoF, aBCFlag,
    1.0, vec_x);
}


void InitializeProblem_LinearSolid_Dynamic()
{
  const double len = 1.1;
  // set boundary condition
  const int np = (int)aXYZ.size()/3;
  const int nDoF = np*3;
  ////
  aVal.assign(nDoF, 0.0);
  aVelo.assign(nDoF, 0.0);
  aAcc.assign(nDoF, 0.0);
  aBCFlag.assign(nDoF, 0);
  for(int ip=0;ip<np;++ip){
    const double px = aXYZ[ip*3+0];
    const double py = aXYZ[ip*3+1];
    const double pz = aXYZ[ip*3+2];
    if (py > 0.45){
      aBCFlag[ip*3+0] = 1;
      aBCFlag[ip*3+1] = 1;
      aBCFlag[ip*3+2] = 1;
    }
  }
  //
  std::vector<unsigned int> psup_ind, psup;
  dfm2::JArray_PSuP_MeshElem(psup_ind, psup,
                             aTet.data(), aTet.size()/4, 4,
                             (int)aXYZ.size()/3);
  dfm2::JArray_Sort(psup_ind, psup);
  /*
  CJaggedArray crs;
  crs.SetEdgeOfElem(aTet, (int)aTet.size()/4, 4, (int)aXYZ.size()/3, false);
  crs.Sort();
   */
  //
  mat_A.Initialize(np, 3, true);
  mat_A.SetPattern(psup_ind.data(), psup_ind.size(),
                   psup.data(),     psup.size());
  ilu_A.Initialize_ILU0(mat_A);
  //
  dt_timestep = 0.03;
}

void SolveProblem_LinearSolid_Dynamic()
{
  const int np = (int)aXYZ.size()/3;
  const int nDoF = np*3;
  // --------
  double myu = 1.0;
  double lambda = 1.0;
  double rho = 1.0;
  const double g[3] = {0.0, -0.3, 0.0};
  mat_A.SetZero();
  vec_b.assign(nDoF, 0.0);
  dfm2::MergeLinSys_SolidLinear_NewmarkBeta_MeshTet3D(mat_A,vec_b.data(),
                                                      myu,lambda,rho,g,
                                                      dt_timestep,gamma_newmark,beta_newmark,
                                                      aXYZ.data(), aXYZ.size()/3,
                                                      aTet.data(), aTet.size()/4,
                                                      aVal.data(),aVelo.data(),aAcc.data());
  mat_A.SetFixedBC(aBCFlag.data());
  dfm2::setRHS_Zero(vec_b, aBCFlag,0);
  // ----------------------
  std::vector<double> vec_x;
  double conv_ratio = 1.0e-4;
  int iteration = 1000;
  ilu_A.SetValueILU(mat_A);
  ilu_A.DoILUDecomp();
  vec_x.resize(vec_b.size());
  Solve_PCG(vec_b.data(),vec_x.data(),
            vec_b.size(),
            conv_ratio,iteration,
            mat_A,ilu_A);
  // -----------------------
  dfm2::XPlusAYBZCW(aVal, nDoF, aBCFlag,
                    dt_timestep,aVelo,
                    0.5*dt_timestep*dt_timestep,aAcc,
                    dt_timestep*dt_timestep*beta_newmark,vec_x);
  dfm2::XPlusAYBZ(aVelo,nDoF, aBCFlag,
                  dt_timestep*gamma_newmark,vec_x,
                  dt_timestep,aAcc);
  dfm2::XPlusAY(aAcc, nDoF, aBCFlag,
                1.0, vec_x);
}


void InitializeProblem_Stokes_Static()
{
  // set boundary condition
  const unsigned int np = aXYZ.size()/3;
  const unsigned int nDoF = np*4;
  //
  aVal.assign(np*4, 0.0);
  aBCFlag.assign(nDoF, 0);
  assert(aIsSurf.size() == np);
  for(unsigned int ip=0;ip<np;++ip){
//    const double px = aXYZ[ip*3+0];
    const double py = aXYZ[ip*3+1];
//    const double pz = aXYZ[ip*3+2];
    if( aIsSurf[ip] == 1 ){
      aBCFlag[ip*4+0] = 1;
      aBCFlag[ip*4+1] = 1;
      aBCFlag[ip*4+2] = 1;
    }
    if( py > +0.45 ){
      aVal[ip*4+0] = 10;
    }
    /*
    if( py > +0.45 ){
      aBCFlag[ip*4+0] = 1;
      aBCFlag[ip*4+1] = 1;
      aBCFlag[ip*4+2] = 1;
      aVal[ip*4+1] = 10;
    }
     */
  }
  /*
  CJaggedArray crs;
  crs.SetEdgeOfElem(aTet, (int)aTet.size()/4, 4, (int)aXYZ.size()/3, false);
  crs.Sort();
   */
  std::vector<unsigned int> psup_ind, psup;
  dfm2::JArray_PSuP_MeshElem(psup_ind, psup,
                             aTet.data(), aTet.size()/4, 4,
                             (int)aXYZ.size()/3);
  dfm2::JArray_Sort(psup_ind, psup);
  //
  mat_A.Initialize(np, 4, true);
  mat_A.SetPattern(psup_ind.data(), psup_ind.size(),
                   psup.data(),     psup.size());
  ilu_A.Initialize_ILU0(mat_A);
  //
}


void SolveProblem_Stokes_Static()
{
  const unsigned int np = aXYZ.size()/3;
  const unsigned int nDoF = np*4;
  // -------------
  double myu = 1.0;
  double rho = 1.0;
  double g_x = 0.0;
  double g_y = -0.0;
  double g_z = -0.0;
  mat_A.SetZero();
  vec_b.assign(nDoF, 0.0);
  dfm2::MergeLinSys_Stokes3D_Static(mat_A,vec_b,
                                    myu,rho,g_x,g_y,g_z,
                                    aXYZ,aTet,
                                    aVal,aVelo);
  mat_A.SetFixedBC(aBCFlag.data());
  dfm2::setRHS_Zero(vec_b, aBCFlag,0);
  // ----------------------
  std::vector<double> vec_x;
  double conv_ratio = 1.0e-4;
  int iteration = 1000;
  ilu_A.SetValueILU(mat_A);
  ilu_A.DoILUDecomp();
  vec_x.resize(vec_b.size());
  Solve_PCG(vec_b.data(),vec_x.data(),
            vec_b.size(),
            conv_ratio,iteration,
            mat_A,ilu_A);
  // ----------------------
  dfm2::XPlusAY(aVal, nDoF, aBCFlag, 1.0, vec_x);
}

void InitializeProblem_Stokes_Dynamic()
{
  // set boundary condition
  const unsigned int np = aXYZ.size()/3;
  const unsigned int nDoF = np*4;
  aVal.assign(nDoF, 0.0);
  aVelo.assign(nDoF, 0.0);
  aBCFlag.assign(nDoF, 0);
  assert(aIsSurf.size() == np);
  for(unsigned int ip=0;ip<np;++ip){
//    const double px = aXYZ[ip*3+0];
    const double py = aXYZ[ip*3+1];
//    const double pz = aXYZ[ip*3+2];
    if( aIsSurf[ip] == 1 ){
      aBCFlag[ip*4+0] = 1;
      aBCFlag[ip*4+1] = 1;
      aBCFlag[ip*4+2] = 1;
    }
    if( py > +0.45 ){
      aVal[ip*4+0] = 10;
    }
    /*
     if( py > +0.45 ){
     aBCFlag[ip*4+0] = 1;
     aBCFlag[ip*4+1] = 1;
     aBCFlag[ip*4+2] = 1;
     aVal[ip*4+1] = 10;
     }
     */
  }
  //////
  /*
  CJaggedArray crs;
  crs.SetEdgeOfElem(aTet, (int)aTet.size()/4, 4, (int)aXYZ.size()/3, false);
  crs.Sort();
   */
  std::vector<unsigned int> psup_ind, psup;
  dfm2::JArray_PSuP_MeshElem(psup_ind, psup,
                             aTet.data(), aTet.size()/4, 4,
                             (int)aXYZ.size()/3);
  dfm2::JArray_Sort(psup_ind, psup);
  //
  mat_A.Initialize(np, 4, true);
  mat_A.SetPattern(psup_ind.data(), psup_ind.size(),
                   psup.data(),     psup.size());
  ilu_A.Initialize_ILU0(mat_A);
  //
}


void SolveProblem_Stokes_Dynamic()
{
  const unsigned int np = aXYZ.size()/3;
  const unsigned int nDoF = np*4;
  double myu = 1;
  double rho = 100.0;
  double g_x = 0.0;
  double g_y = -0.0;
  double g_z = -0.0;
  mat_A.SetZero();
  vec_b.assign(nDoF, 0.0);
  dfm2::MergeLinSys_Stokes3D_Dynamic(mat_A,vec_b,
                                     myu,rho,g_x,g_y,g_z,
                                     dt_timestep,gamma_newmark,
                                     aXYZ,aTet,
                                     aVal,aVelo);
  mat_A.SetFixedBC(aBCFlag.data());
  dfm2::setRHS_Zero(vec_b, aBCFlag,0);
  // ------------------------
  std::vector<double> vec_x;
  double conv_ratio = 1.0e-4;
  int iteration = 1000;
  ilu_A.SetValueILU(mat_A);
  ilu_A.DoILUDecomp();
  vec_x.resize(vec_b.size());
  Solve_PCG(vec_b.data(),vec_x.data(),
            vec_b.size(),
            conv_ratio,iteration,
            mat_A,ilu_A);
  // -----------------------
  dfm2::XPlusAYBZ(aVal,nDoF,aBCFlag,
            dt_timestep*gamma_newmark,vec_x,
            dt_timestep,aVelo);
  dfm2::XPlusAY(aVelo,nDoF,aBCFlag,
          1.0,vec_x);
}


void InitializeProblem_NavierStokes_Dynamic()
{
  // set boundary condition
  const unsigned int np = aXYZ.size()/3;
  const unsigned int nDoF = np*4;
  // ----------
  aVal.assign(np*4, 0.0);
  aVelo.assign(np*4, 0.0);
  aBCFlag.assign(nDoF, 0);
  assert(aIsSurf.size() == np);
  for(unsigned int ip=0;ip<np;++ip){
    const double px = aXYZ[ip*3+0];
    const double py = aXYZ[ip*3+1];
    const double pz = aXYZ[ip*3+2];
    if( aIsSurf[ip] != 1 ){ continue; }
    aBCFlag[ip*4+0] = 1;
    aBCFlag[ip*4+1] = 1;
    aBCFlag[ip*4+2] = 1;
    int ishape = 0;
    if( ishape == 0 || ishape == 1 ){
      if( py > +0.48 ){
        aVal[ip*4+0] = 10;
      }
    }
    if( ishape == 2 ){
      if( fabs(pz) < 0.3-0.03 || fabs(px) < 0.3-0.03 ){
        if( py > +0.48 ){ aVal[ip*4+1] = 1; }
        if( py < -0.48 ){ 
          aBCFlag[ip*4+1] = 0;
//          aVal[ip*4+1] = 1; 
        }
      }
    }
  }
  //
  /*
  CJaggedArray crs;
  crs.SetEdgeOfElem(aTet, (int)aTet.size()/4, 4, (int)aXYZ.size()/3, false);
  crs.Sort();
   */
  std::vector<unsigned int> psup_ind, psup;
  dfm2::JArray_PSuP_MeshElem(psup_ind, psup,
                             aTet.data(), aTet.size()/4, 4,
                             (int)aXYZ.size()/3);
  dfm2::JArray_Sort(psup_ind, psup);
  //
  mat_A.Initialize(np, 4, true);
  mat_A.SetPattern(psup_ind.data(), psup_ind.size(),
                   psup.data(),     psup.size());
//  ilu_A.Initialize_ILU0(mat_A);
  ilu_A.Initialize_ILUk(mat_A, 0);
  //
}


void SolveProblem_NavierStokes_Dynamic()
{
  const unsigned int np = aXYZ.size()/3;
  const unsigned int nDoF = np*4;
  //
  double myu = 1;
  double rho = 1000.0;
  double g_x = 0.0;
  double g_y = -0.0;
  double g_z = -0.0;
  mat_A.SetZero();
  vec_b.assign(nDoF, 0.0);
  dfm2::MergeLinSys_NavierStokes3D_Dynamic(mat_A,vec_b,
                                           myu,rho,g_x,g_y,g_z,
                                           dt_timestep,gamma_newmark,
                                           aXYZ,aTet,
                                           aVal,aVelo);
  mat_A.SetFixedBC(aBCFlag.data());
  dfm2::setRHS_Zero(vec_b, aBCFlag,0);
  // --------------------------------------
  std::vector<double> vec_x;
//  SolveLinSys_PCG(mat_A,vec_b,vec_x,ilu_A,aBCFlag);
  double conv_ratio = 1.0e-5;
  int iteration = 1000;
  ilu_A.SetValueILU(mat_A);
  ilu_A.DoILUDecomp();
  vec_x.resize(vec_b.size());
  Solve_PBiCGStab(vec_b.data(),vec_x.data(),
                  conv_ratio,iteration,mat_A,ilu_A);
//  Solve_BiCGStab(mat_A,vec_b,vec_x,ilu_A, conv_ratio,iteration);
  // ----------------------------------------
  dfm2::XPlusAYBZ(aVal,nDoF,aBCFlag,
                  dt_timestep*gamma_newmark,vec_x,
                  dt_timestep,aVelo);
  dfm2::XPlusAY(aVelo,nDoF,aBCFlag,
                1.0,vec_x);
  /////
  dt_timestep = 0.0025;
}


class CInSphere : public dfm2::CInput_IsosurfaceStuffing
{
public:
  virtual double SignedDistance(double x, double y, double z) const {
    double n[3];
    return sdf.Projection(n,
                          x, y, z);
  }
  virtual void Level(int& ilevel_vol, int& ilevel_srf, int& nlayer, double& sdf,
                     double px, double py, double pz) const
  {
    sdf = this->SignedDistance(px, py, pz);
    ilevel_vol = -1;
    ilevel_srf = 2;
    nlayer = 3;
  }

public:
  delfem2::CSphere<double> sdf;
};

class CInBox : public dfm2::CInput_IsosurfaceStuffing
{
public:
  virtual double SignedDistance(double x, double y, double z) const override
  {
    double n[3];
    return sdf.Projection(n,
                          x, y, z);
  }
public:
  delfem2::CBox<double> sdf;
};

void SetMesh(int ishape)
{
  ::glMatrixMode(GL_MODELVIEW);
  
  if(ishape==0){
    const double rad = 0.5;
    CInSphere sphere;
    sphere.sdf.is_out_ = true;
    sphere.sdf.radius_ = rad;
    double cent[3] = {0,0,0};
    IsoSurfaceStuffing(aXYZ, aTet, aIsSurf, sphere, 0.30, rad*2.1, cent);
  }
  else if( ishape == 1 ){
    const double hwx = 0.5;
    const double hwy = 0.5;
    const double hwz = 0.5;
    CInBox box;
    box.sdf.hwx = hwx;
    box.sdf.hwy = hwy;
    box.sdf.hwz = hwz;
    double cent[3] = {0,0,0};
    dfm2::IsoSurfaceStuffing(aXYZ, aTet, aIsSurf, box, 0.2, 1.1, cent);
  }
  else if( ishape == 2 ){
    class CCavSphere : public dfm2::CInput_IsosurfaceStuffing
    {
    public:
      CCavSphere(){
        const double hwx = 0.3;
        const double hwy = 0.5;
        const double hwz = 0.3;
        box.sdf.hwx = hwx;
        box.sdf.hwy = hwy;
        box.sdf.hwz = hwz;
        ////
        const double rad = 0.1;
        sphere.sdf.radius_ = rad;
        sphere.sdf.is_out_ = true;
      }
      virtual double SignedDistance(double x, double y, double z ) const override {
        double dist0 = -sphere.SignedDistance(x, y, z);
        double cx = 0.0;
        double cy = 0.0;
        double cz = 0.0;
        double dist1 = box.SignedDistance(x-cx, y-cy, z-cz);
        return (dist0<dist1) ? dist0 : dist1;
      }
    public:
      CInBox box;
      CInSphere sphere;
    } cav_sphere;
    double cent[3] = {0,0,0};
    dfm2::IsoSurfaceStuffing(aXYZ, aTet, aIsSurf, cav_sphere, 0.05, 1.1, cent);
  }
}

/*
void Solve(bool is_next)
{
  cur_time = 0.0;
  if( is_next ){ iphysics = (iphysics+1)%7; }
  if( iphysics == 0 ){
    InitializeProblem_Poisson();
    SolveProblem_Poisson();
  }
  else if( iphysics == 1 ){
    InitializeProblem_Diffusion();
    SolveProblem_Diffusion();
  }
  else if( iphysics == 2 ){
    InitializeProblem_ShellEigenPB();
    SolveProblem_LinearSolid_Static();
  }
  else if( iphysics == 3 ){
    InitializeProblem_LinearSolid_Dynamic();
    SolveProblem_LinearSolid_Dynamic();
  }
  else if( iphysics == 4 ){
    InitializeProblem_Stokes_Static();
    SolveProblem_Stokes_Static();
  }
  else if( iphysics == 5 ){
    InitializeProblem_Stokes_Dynamic();
    SolveProblem_Stokes_Dynamic();
  }
  else if( iphysics == 6 ){
    InitializeProblem_NavierStokes_Dynamic();
    SolveProblem_NavierStokes_Dynamic();
  }
  std::cout<<"node number:" << aXYZ.size()/3<<std::endl;
}
 */


// ---------------------------------------------

static void myGlVertex3d
(unsigned int ixyz,
 const std::vector<double>& aXYZ1 )
{
  ::glVertex3d(aXYZ1[ixyz*3+0], aXYZ1[ixyz*3+1], aXYZ1[ixyz*3+2] );
}

//static void myGlVertex3d(const CVec3& v){
//  ::glVertex3d(v.x, v.y, v.z);
//}

void myGlutDisplay(int iphysics)
{
//	::glClearColor(0.2f, 0.7f, 0.7f ,1.0f);
	::glClearColor(1.0f, 1.0f, 1.0f ,1.0f);
	::glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	::glEnable(GL_DEPTH_TEST);

	::glEnable(GL_POLYGON_OFFSET_FILL );
  ::glPolygonOffset( 3.1f, 2.0f );

//  glEnable(GL_BLEND);
//  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  GLboolean is_lighting = glIsEnabled(GL_LIGHTING);
  {
    float color[4] = {200.0/256.0, 200.0/256.0, 200.0/256.0,1.0f};
    ::glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,color);
    ::glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,color);
//    glShadeModel(GL_SMOOTH);
    glShadeModel(GL_FLAT);
 }
  ::glDisable(GL_LIGHTING);
  
  if (iphysics==0 || iphysics==1){
    glShadeModel(GL_SMOOTH);
    ::glDisable(GL_LIGHTING);
    ::glColor3d(0,0,0);
    delfem2::opengl::DrawMeshTet3D_Edge(
        aXYZ.data(),aXYZ.size()/3,
        aTet.data(),aTet.size()/4);
    {
      std::vector< std::pair<double,delfem2::CColor> > colorMap;
      delfem2::ColorMap_BlueGrayRed(colorMap, 0, 1.0);
      delfem2::opengl::DrawMeshTet3D_ScalarP1(aXYZ.data(), aXYZ.size()/3,
                                              aTet.data(), aTet.size()/4,
                                              aVal.data(),
                                              colorMap);
    }
  }
  if (iphysics==2||iphysics==3){
    ::glColor3d(0,0,0);
    delfem2::opengl::DrawMeshTet3D_EdgeDisp(aXYZ.data(), aTet.data(), aTet.size()/4, aVal.data(), 1.0);
    ::glEnable(GL_LIGHTING);
    {
      float color[4] = {180.0/256.0, 180.0/256.0, 130.0/256.0,1.0f};
      ::glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,color);
      ::glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,color);
      glShadeModel(GL_FLAT);
    }
    delfem2::opengl::DrawMeshTet3D_FaceNormDisp(aXYZ.data(), aXYZ.size()/3,
                                       aTet.data(), aTet.size()/4,
                                       aVal.data());
  }
  if( iphysics == 4 || iphysics == 5 || iphysics == 6 ){
    ::glEnable(GL_LIGHTING);
    {
      float color[4] = {256.0/256.0, 0.0/256.0, 0.0/256.0,1.0f};
      ::glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,color);
      ::glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,color);
      //    glShadeModel(GL_SMOOTH);
      glShadeModel(GL_FLAT);
    }
    for(unsigned int ip=0;ip<aXYZ.size()/3;++ip){
      const dfm2::CVec3d p(aXYZ[ip*3+0],aXYZ[ip*3+1],aXYZ[ip*3+2]);
      const dfm2::CVec3d v(aVal[ip*4+0],aVal[ip*4+1],aVal[ip*4+2]);
      double a = 0.1;
      delfem2::opengl::DrawArrow(p, a*v);
    }
  }

  
  if( is_lighting ){ ::glEnable(GL_LIGHTING); }
  else{              ::glDisable(GL_LIGHTING); }

  ::glColor3d(0,0,0);
}

/*
void myGlutKeyboard(unsigned char Key, int x, int y)
{
  switch (Key)
  {
  case 'q':
  case 'Q':
  case '\033':
    exit(0);
  case 'a':
    {
      is_animation = !is_animation;
      break;
    }
  case 'd':
    {
      ishape = (ishape+1)%3;
      SetMesh(ishape);
      Solve(false);
      break;
    }
  case ' ':
      Solve(true);
      break;
  }

//	::glMatrixMode(GL_PROJECTION);
//	::glLoadIdentity();
//	Com::View::SetProjectionTransform(camera);
//	::glutPostRedisplay();
}
 */


/*
void myGlutIdle(int iphysics){
  if( is_animation ){
    bool is_dynamic = false;
    if( iphysics == 1){
      is_dynamic = true;
      SolveProblem_Diffusion();
    }
    if( iphysics == 3){
      is_dynamic = true;
      SolveProblem_LinearSolid_Dynamic();
    }
    if( iphysics == 5){
      is_dynamic = true;
      SolveProblem_Stokes_Dynamic();
    }
    if( iphysics == 6){
      is_dynamic = true;
      SolveProblem_NavierStokes_Dynamic();
    }
    if( is_dynamic ){
      cur_time += dt_timestep;
      std::cout << "current time: " << cur_time << std::endl;
    }
  }
//  ::glutPostRedisplay();
}
 */


int main(int argc,char* argv[])
{
  dfm2::opengl::CViewer_GLFW viewer;
  viewer.Init_oldGL();
  viewer.nav.camera.view_height = 1.5;
  viewer.nav.camera.camera_rot_mode = delfem2::CCamera<double>::CAMERA_ROT_MODE::TBALL;
  delfem2::opengl::setSomeLighting();

  while (true){
    SetMesh(0);
    int iframe = 0;
    // ---------------------------
    glfwSetWindowTitle(viewer.window, "Poisson");
    InitializeProblem_Poisson();
    SolveProblem_Poisson();
    for(;iframe<50;++iframe){ // poisson
      viewer.DrawBegin_oldGL();
      myGlutDisplay(0);
      viewer.DrawEnd_oldGL();
      if( glfwWindowShouldClose(viewer.window) ){ goto CLOSE; }
    }
    // ---------------------------
    glfwSetWindowTitle(viewer.window, "Diffusion");
    InitializeProblem_Diffusion();
    SolveProblem_Diffusion();
    for(;iframe<100;++iframe){
      SolveProblem_Diffusion();
      viewer.DrawBegin_oldGL();
      myGlutDisplay(1);
      viewer.DrawEnd_oldGL();
      if( glfwWindowShouldClose(viewer.window) ){ goto CLOSE; }
    }
    // ---------------------------
    glfwSetWindowTitle(viewer.window, "SolidLinearStatic");
    InitializeProblem_ShellEigenPB();
    SolveProblem_LinearSolid_Static();
    for(;iframe<150;++iframe){
      viewer.DrawBegin_oldGL();
      myGlutDisplay(2);
      viewer.DrawEnd_oldGL();
      if( glfwWindowShouldClose(viewer.window) ){ goto CLOSE; }
    }
    // ---------------------------
    glfwSetWindowTitle(viewer.window, "SolidLinearDynamic");
    InitializeProblem_LinearSolid_Dynamic();
    for(;iframe<200;++iframe){
      SolveProblem_LinearSolid_Dynamic();
      viewer.DrawBegin_oldGL();
      myGlutDisplay(3);
      viewer.DrawEnd_oldGL();
      if( glfwWindowShouldClose(viewer.window) ){ goto CLOSE; }
    }
    // ---------------------------
    glfwSetWindowTitle(viewer.window, "StokesStatic");
    InitializeProblem_Stokes_Static();
    SolveProblem_Stokes_Static();
    for(;iframe<250;++iframe){
      viewer.DrawBegin_oldGL();
      myGlutDisplay(4);
      viewer.DrawEnd_oldGL();
      if( glfwWindowShouldClose(viewer.window) ){ goto CLOSE; }
    }
    // ---------------------------
    glfwSetWindowTitle(viewer.window, "StokesDynamic");
    InitializeProblem_Stokes_Dynamic();
    for(;iframe<300;++iframe){
      SolveProblem_Stokes_Dynamic();
      viewer.DrawBegin_oldGL();
      myGlutDisplay(5);
      viewer.DrawEnd_oldGL();
      if( glfwWindowShouldClose(viewer.window) ){ goto CLOSE; }
    }
    // ---------------------------
    glfwSetWindowTitle(viewer.window, "NavierStokesDynamic");
    InitializeProblem_NavierStokes_Dynamic();
    for(;iframe<350;++iframe){
      SolveProblem_NavierStokes_Dynamic();
      viewer.DrawBegin_oldGL();
      myGlutDisplay(6);
      viewer.DrawEnd_oldGL();
      if( glfwWindowShouldClose(viewer.window) ){ goto CLOSE; }
    }
  }
  
CLOSE:
  glfwDestroyWindow(viewer.window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}
