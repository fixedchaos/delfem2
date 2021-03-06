#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "delfem2/cuda/cu_matvec.h"

namespace dfm2 = delfem2;

int main() {
  const unsigned int n = 64 * 16 + 1;
  float h_A[n];
  float h_B[n];
  for (int i = 0; i < n; ++i) {
    h_A[i] = (float) i/100.0;
    h_B[i] = (float) i/100.0;
  }

  // ------------------------------------------------

  clock_t time0 = clock();

  float h_res = dfm2::cuda::cuda_Dot(h_A,h_B,n);


  printf("gpu sum %.2f \n", h_res);

  clock_t time1 = clock();

  printf("on gpu it takes %.2f sec \n", (double) (time1 - time0) / CLOCKS_PER_SEC);

  {
    float sum = 0;
    for (int i = 0; i < n; ++i) {
      sum += h_A[i] * h_B[i];
    }
    printf("cpu sum %.2f \n", h_res);
  }
}
