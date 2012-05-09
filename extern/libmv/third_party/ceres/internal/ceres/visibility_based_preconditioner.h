// Ceres Solver - A fast non-linear least squares minimizer
// Copyright 2010, 2011, 2012 Google Inc. All rights reserved.
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
//
// Preconditioners for linear systems that arise in Structure from
// Motion problems. VisibilityBasedPreconditioner implements three
// preconditioners:
//
//  SCHUR_JACOBI
//  CLUSTER_JACOBI
//  CLUSTER_TRIDIAGONAL
//
// Detailed descriptions of these preconditions beyond what is
// documented here can be found in
//
// Bundle Adjustment in the Large
// S. Agarwal, N. Snavely, S. Seitz & R. Szeliski, ECCV 2010
// http://www.cs.washington.edu/homes/sagarwal/bal.pdf
//
// Visibility Based Preconditioning for Bundle Adjustment
// A. Kushal & S. Agarwal, submitted to CVPR 2012
// http://www.cs.washington.edu/homes/sagarwal/vbp.pdf
//
// The three preconditioners share enough code that its most efficient
// to implement them as part of the same code base.

#ifndef CERES_INTERNAL_VISIBILITY_BASED_PRECONDITIONER_H_
#define CERES_INTERNAL_VISIBILITY_BASED_PRECONDITIONER_H_

#include <set>
#include <vector>
#include <utility>
#include "ceres/collections_port.h"
#include "ceres/graph.h"
#include "ceres/linear_solver.h"
#include "ceres/linear_operator.h"
#include "ceres/suitesparse.h"
#include "ceres/internal/macros.h"
#include "ceres/internal/scoped_ptr.h"

namespace ceres {
namespace internal {

class BlockRandomAccessSparseMatrix;
class BlockSparseMatrixBase;
class CompressedRowBlockStructure;
class SchurEliminatorBase;

// This class implements three preconditioners for Structure from
// Motion/Bundle Adjustment problems. The name
// VisibilityBasedPreconditioner comes from the fact that the sparsity
// structure of the preconditioner matrix is determined by analyzing
// the visibility structure of the scene, i.e. which cameras see which
// points.
//
// Strictly speaking, SCHUR_JACOBI is not a visibility based
// preconditioner but it is an extreme case of CLUSTER_JACOBI, where
// every cluster contains exactly one camera block. Treating it as a
// special case of CLUSTER_JACOBI makes it easy to implement as part
// of the same code base with no significant loss of performance.
//
// In the following, we will only discuss CLUSTER_JACOBI and
// CLUSTER_TRIDIAGONAL.
//
// The key idea of visibility based preconditioning is to identify
// cameras that we expect have strong interactions, and then using the
// entries in the Schur complement matrix corresponding to these
// camera pairs as an approximation to the full Schur complement.
//
// CLUSTER_JACOBI identifies these camera pairs by clustering cameras,
// and considering all non-zero camera pairs within each cluster. The
// clustering in the current implementation is done using the
// Canonical Views algorithm of Simon et al. (see
// canonical_views_clustering.h). For the purposes of clustering, the
// similarity or the degree of interaction between a pair of cameras
// is measured by counting the number of points visible in both the
// cameras. Thus the name VisibilityBasedPreconditioner. Further, if we
// were to permute the parameter blocks such that all the cameras in
// the same cluster occur contiguously, the preconditioner matrix will
// be a block diagonal matrix with blocks corresponding to the
// clusters. Thus in analogy with the Jacobi preconditioner we refer
// to this as the CLUSTER_JACOBI preconditioner.
//
// CLUSTER_TRIDIAGONAL adds more mass to the CLUSTER_JACOBI
// preconditioner by considering the interaction between clusters and
// identifying strong interactions between cluster pairs. This is done
// by constructing a weighted graph on the clusters, with the weight
// on the edges connecting two clusters proportional to the number of
// 3D points visible to cameras in both the clusters. A degree-2
// maximum spanning forest is identified in this graph and the camera
// pairs contained in the edges of this forest are added to the
// preconditioner. The detailed reasoning for this construction is
// explained in the paper mentioned above.
//
// Degree-2 spanning trees and forests have the property that they
// correspond to tri-diagonal matrices. Thus there exist a permutation
// of the camera blocks under which the CLUSTER_TRIDIAGONAL
// preconditioner matrix is a block tridiagonal matrix, and thus the
// name for the preconditioner.
//
// Thread Safety: This class is NOT thread safe.
//
// Example usage:
//
//   LinearSolver::Options options;
//   options.preconditioner_type = CLUSTER_JACOBI;
//   options.num_eliminate_blocks = num_points;
//   VisibilityBasedPreconditioner preconditioner(
//      *A.block_structure(), options);
//   preconditioner.Compute(A, NULL);
//   preconditioner.RightMultiply(x, y);
//

#ifndef CERES_NO_SUITESPARSE
class VisibilityBasedPreconditioner : public LinearOperator {
 public:
  // Initialize the symbolic structure of the preconditioner. bs is
  // the block structure of the linear system to be solved. It is used
  // to determine the sparsity structure of the preconditioner matrix.
  //
  // It has the same structural requirement as other Schur complement
  // based solvers. Please see schur_eliminator.h for more details.
  //
  // LinearSolver::Options::num_eliminate_blocks should be set to the
  // number of e_blocks in the block structure.
  //
  // TODO(sameeragarwal): The use of LinearSolver::Options should
  // ultimately be replaced with Preconditioner::Options and some sort
  // of preconditioner factory along the lines of
  // LinearSolver::CreateLinearSolver. I will wait to do this till I
  // create a general purpose block Jacobi preconditioner for general
  // sparse problems along with a CGLS solver.
  VisibilityBasedPreconditioner(const CompressedRowBlockStructure& bs,
                                const LinearSolver::Options& options);
  virtual ~VisibilityBasedPreconditioner();

  // Compute the numerical value of the preconditioner for the linear
  // system:
  //
  //  |   A   | x = |b|
  //  |diag(D)|     |0|
  //
  // for some vector b. It is important that the matrix A have the
  // same block structure as the one used to construct this object.
  //
  // D can be NULL, in which case its interpreted as a diagonal matrix
  // of size zero.
  bool Compute(const BlockSparseMatrixBase& A,
               const double* D);

  // LinearOperator interface. Since the operator is symmetric,
  // LeftMultiply and num_cols are just calls to RightMultiply and
  // num_rows respectively. Compute() must be called before
  // RightMultiply can be called.
  virtual void RightMultiply(const double* x, double* y) const;
  virtual void LeftMultiply(const double* x, double* y) const {
    RightMultiply(x, y);
  }
  virtual int num_rows() const;
  virtual int num_cols() const { return num_rows(); }

  friend class VisibilityBasedPreconditionerTest;
 private:
  void ComputeSchurJacobiSparsity(const CompressedRowBlockStructure& bs);
  void ComputeClusterJacobiSparsity(const CompressedRowBlockStructure& bs);
  void ComputeClusterTridiagonalSparsity(const CompressedRowBlockStructure& bs);
  void InitStorage(const CompressedRowBlockStructure& bs);
  void InitEliminator(const CompressedRowBlockStructure& bs);
  bool Factorize();
  void ScaleOffDiagonalCells();

  void ClusterCameras(const vector< set<int> >& visibility);
  void FlattenMembershipMap(const HashMap<int, int>& membership_map,
                            vector<int>* membership_vector) const;
  void ComputeClusterVisibility(const vector<set<int> >& visibility,
                                vector<set<int> >* cluster_visibility) const;
  Graph<int>* CreateClusterGraph(const vector<set<int> >& visibility) const;
  void ForestToClusterPairs(const Graph<int>& forest,
                            HashSet<pair<int, int> >* cluster_pairs) const;
  void ComputeBlockPairsInPreconditioner(const CompressedRowBlockStructure& bs);
  bool IsBlockPairInPreconditioner(int block1, int block2) const;
  bool IsBlockPairOffDiagonal(int block1, int block2) const;

  LinearSolver::Options options_;

  // Number of parameter blocks in the schur complement.
  int num_blocks_;
  int num_clusters_;

  // Sizes of the blocks in the schur complement.
  vector<int> block_size_;

  // Mapping from cameras to clusters.
  vector<int> cluster_membership_;

  // Non-zero camera pairs from the schur complement matrix that are
  // present in the preconditioner, sorted by row (first element of
  // each pair), then column (second).
  set<pair<int, int> > block_pairs_;

  // Set of cluster pairs (including self pairs (i,i)) in the
  // preconditioner.
  HashSet<pair<int, int> > cluster_pairs_;
  scoped_ptr<SchurEliminatorBase> eliminator_;

  // Preconditioner matrix.
  scoped_ptr<BlockRandomAccessSparseMatrix> m_;

  // RightMultiply is a const method for LinearOperators. It is
  // implemented using CHOLMOD's sparse triangular matrix solve
  // function. This however requires non-const access to the
  // SuiteSparse context object, even though it does not result in any
  // of the state of the preconditioner being modified.
  SuiteSparse ss_;

  // Symbolic and numeric factorization of the preconditioner.
  cholmod_factor* factor_;

  // Temporary vector used by RightMultiply.
  cholmod_dense* tmp_rhs_;
  DISALLOW_COPY_AND_ASSIGN(VisibilityBasedPreconditioner);
};
#else  // SuiteSparse
// If SuiteSparse is not compiled in, the preconditioner is not
// available.
class VisibilityBasedPreconditioner : public LinearOperator {
 public:
  VisibilityBasedPreconditioner(const CompressedRowBlockStructure& bs,
                                const LinearSolver::Options& options) {
    LOG(FATAL) << "Visibility based preconditioning is not available. Please "
        "build Ceres with SuiteSparse.";
  }
  virtual ~VisibilityBasedPreconditioner() {}
  virtual void RightMultiply(const double* x, double* y) const {}
  virtual void LeftMultiply(const double* x, double* y) const {}
  virtual int num_rows() const { return -1; }
  virtual int num_cols() const { return -1; }
  bool Compute(const BlockSparseMatrixBase& A, const double* D) {
    return false;
  }
};
#endif  // CERES_NO_SUITESPARSE

}  // namespace internal
}  // namespace ceres

#endif  // CERES_INTERNAL_VISIBILITY_BASED_PRECONDITIONER_H_
