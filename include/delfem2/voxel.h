#ifndef VOXEL_H
#define VOXEL_H

#include <vector>

// TODO: remove dependency to vec3
#include "vec3.h"

int Adj_Grid
(int ivox_picked, int iface_picked,
 int ndivx, int ndivy, int ndivz);

void GetQuad_VoxelGrid
(std::vector<double>& aXYZ, std::vector<int>& aQuad,
 int ndivx, int ndivy, int ndivz,
 int iorgx, int iorgy, int iorgz,
 const std::vector<int>& aIsVox);

bool IsInclude_AABB(const int aabb[8], int igvx, int igvy, int igvz);
void Add_AABB(int aabb[8], int ivx, int ivy, int ivz);

////////////////////////////////////////////////////////////////////

class CCubeGrid
{
public:
  CCubeGrid(){
    this->ivx = 0;
    this->ivy = 0;
    this->ivz = 0;
    is_active = true;
  }
  CCubeGrid(int i, int j, int k){
    this->ivx = i;
    this->ivy = j;
    this->ivz = k;
    is_active = true;
  }
  bool operator<(const CCubeGrid& rhs) const
  {
    if( this->ivx != rhs.ivx ){ return this->ivx < rhs.ivx; }
    if( this->ivy != rhs.ivy ){ return this->ivy < rhs.ivy; }
    if( this->ivz != rhs.ivz ){ return this->ivz < rhs.ivz; }
    return false;
  }
public:
  int ivx, ivy, ivz;
  bool is_active;
};

void Draw_CubeGrid(bool is_picked, int iface_picked,
                   double elen, const CVector3& org,
                   const CCubeGrid& cube);
void Pick_CubeGrid(int& icube_pic, int& iface_pic,
                   const CVector3& src_pic, const CVector3& dir_pic,
                   double elen,
                   const CVector3& org,
                   const std::vector<CCubeGrid>& aCube);
void Adj_CubeGrid(int& ivx, int& ivy, int& ivz,
                  int ivox, int iface,
                  std::vector<CCubeGrid>& aCube);
void Add_CubeGrid(std::vector<CCubeGrid>& aVox,
                  int ivx1, int ivy1, int ivz1);
void Del_CubeGrid(std::vector<CCubeGrid>& aCube,
                  int i1, int j1, int k1);
void AABB_CubeGrid(int aabb[6],
                   const std::vector<CCubeGrid>& aCube);

////////////////////////////////////////////////////////////////////////////////////


class CVoxelGrid
{
public:
  CVoxelGrid(){
    ndivx = ndivy = ndivz = 0;
    iorgx = iorgy = iorgz = 0;
    aIsVox.clear();
  }
  void Init_AABB(const int aabb[6]){
    ndivx = aabb[1]-aabb[0];
    ndivy = aabb[3]-aabb[2];
    ndivz = aabb[5]-aabb[4];
    iorgx = aabb[0];
    iorgy = aabb[2];
    iorgz = aabb[4];
    const int nvoxel = ndivx*ndivy*ndivz;
    aIsVox.assign(nvoxel,0);
  }
  void AABB(int aabb[8]) const {
    bool is_initial = true;
    aabb[0] = +1;
    aabb[1] = -1;
    for(int igvx=0;igvx<ndivx;++igvx){
      for(int igvy=0;igvy<ndivy;++igvy){
        for(int igvz=0;igvz<ndivz;++igvz){
          const int ivoxel = igvx*(ndivy*ndivz)+igvy*ndivz+igvz;
          if( aIsVox [ivoxel] ==0 ){ continue; }
          Add_AABB(aabb, igvx+iorgx, igvy+iorgy, igvz+iorgz);
        }
      }
    }
  }
  bool IsInclude(int ivx, int ivy, int ivz){
    int igvx = ivx-iorgx;
    int igvy = ivy-iorgy;
    int igvz = ivz-iorgz;
    if( igvx<0 || igvx>ndivx ){ return false; }
    if( igvy<0 || igvy>ndivy ){ return false; }
    if( igvz<0 || igvz>ndivz ){ return false; }
    return true;
  }
  void Add(int ivx, int ivy, int ivz){
    if( this->IsInclude(ivx, ivy, ivz) ){
      Set(ivx,ivy,ivz,1);
    }
    else{
      int aabb[8]; this->AABB(aabb);
      Add_AABB(aabb,ivx,ivy,ivz);
      CVoxelGrid vg0 = (*this);
      this->Init_AABB(aabb);
      for(int igvx0=0;igvx0<vg0.ndivx;++igvx0){
      for(int igvy0=0;igvy0<vg0.ndivy;++igvy0){
      for(int igvz0=0;igvz0<vg0.ndivz;++igvz0){
        const int ivoxel0 = igvx0*(vg0.ndivy*vg0.ndivz)+igvy0*vg0.ndivz+igvz0;
        if( vg0.aIsVox[ivoxel0] == 0 ) continue;
        int ivx = igvx0+vg0.iorgx;
        int ivy = igvy0+vg0.iorgy;
        int ivz = igvz0+vg0.iorgz;
        assert( IsInclude(ivx,ivy,ivz) );
        Set(ivx,ivy,ivz,1);
      }
      }
      }
    }
  }
  void Set(int ivx, int ivy, int ivz, int isVox){
    if( !this->IsInclude(ivx,ivy,ivz) ){ return; }
    const int igvx = ivx-iorgx;
    const int igvy = ivy-iorgy;
    const int igvz = ivz-iorgz;
    const int ivoxel = igvx*(ndivy*ndivz)+igvy*ndivz+igvz;
    aIsVox[ivoxel] = isVox;
  }
  void GetQuad(std::vector<double>& aXYZ, std::vector<int>& aQuad){
    GetQuad_VoxelGrid(aXYZ, aQuad,
                      ndivx, ndivy, ndivz,
                      iorgx, iorgy, iorgz,
                      aIsVox);
  }
public:
  int ndivx, ndivy, ndivz;
  int iorgx, iorgy, iorgz;
  std::vector<int> aIsVox;
};

#endif
