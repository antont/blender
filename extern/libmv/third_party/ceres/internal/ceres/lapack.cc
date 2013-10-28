// Ceres Solver - A fast non-linear least squares minimizer
// Copyright 2013 Google Inc. All rights reserved.
// http://code.google.com/p/ceres-solver/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of Google Inc. nor the names of its contributors may be
//   used to endorse or promote products derived from this software without
//   specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Author: sameeragarwal@google.com (Sameer Agarwal)

#include "ceres/lapack.h"
#include "glog/logging.h"

// C interface to the LAPACK Cholesky factorization and triangular solve.
extern "C" void dpotrf_(char* uplo,
                       int* n,
                       double* a,
                       int* lda,
                       int* info);

extern "C" void dpotrs_(char* uplo,
                        int* n,
                        int* nrhs,
                        double* a,
                        int* lda,
                        double* b,
                        int* ldb,
                        int* info);

extern "C" void dgels_(char* uplo,
                       int* m,
                       int* n,
                       int* nrhs,
                       double* a,
                       int* lda,
                       double* b,
                       int* ldb,
                       double* work,
                       int* lwork,
                       int* info);


namespace ceres {
namespace internal {

int LAPACK::SolveInPlaceUsingCholesky(int num_rows,
                                      const double* in_lhs,
                                      double* rhs_and_solution) {
#ifdef CERES_NO_LAPACK
  LOG(FATAL) << "Ceres was built without a BLAS library.";
  return -1;
#else
  char uplo = 'L';
  int n = num_rows;
  int info = 0;
  int nrhs = 1;
  double* lhs = const_cast<double*>(in_lhs);

  dpotrf_(&uplo, &n, lhs, &n, &info);
  if (info != 0) {
    LOG(INFO) << "Cholesky factorization (dpotrf) failed: " << info;
    return info;
  }

  dpotrs_(&uplo, &n, &nrhs, lhs, &n, rhs_and_solution, &n, &info);
  if (info != 0) {
    LOG(INFO) << "Triangular solve (dpotrs) failed: " << info;
  }

  return info;
#endif
};

int LAPACK::EstimateWorkSizeForQR(int num_rows, int num_cols) {
#ifdef CERES_NO_LAPACK
  LOG(FATAL) << "Ceres was built without a LAPACK library.";
  return -1;
#else
  char trans = 'N';
  int nrhs = 1;
  int lwork = -1;
  double work;
  int info = 0;
  dgels_(&trans,
         &num_rows,
         &num_cols,
         &nrhs,
         NULL,
         &num_rows,
         NULL,
         &num_rows,
         &work,
         &lwork,
         &info);

  CHECK_EQ(info, 0);
  return work;
#endif
}

int LAPACK::SolveUsingQR(int num_rows,
                         int num_cols,
                         const double* in_lhs,
                         int work_size,
                         double* work,
                         double* rhs_and_solution) {
#ifdef CERES_NO_LAPACK
  LOG(FATAL) << "Ceres was built without a LAPACK library.";
  return -1;
#else
  char trans = 'N';
  int m = num_rows;
  int n = num_cols;
  int nrhs = 1;
  int lda = num_rows;
  int ldb = num_rows;
  int info = 0;
  double* lhs = const_cast<double*>(in_lhs);

  dgels_(&trans,
         &m,
         &n,
         &nrhs,
         lhs,
         &lda,
         rhs_and_solution,
         &ldb,
         work,
         &work_size,
         &info);

  return info;
#endif
}

}  // namespace internal
}  // namespace ceres
