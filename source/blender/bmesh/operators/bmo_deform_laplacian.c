/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contributor(s): Alexander Pinzon
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/bmesh/operators/bmo_deform_laplacian.c
 *  \ingroup bmesh
 *
 * Deform Laplacian.
 */
#include "MEM_guardedalloc.h"
#include "BLI_math.h"
#include "BLI_array.h"
#include "bmesh.h"
#include "ONL_opennl.h"
#include "intern/bmesh_operators_private.h" /* own include */
#include "bmo_deform_utils.h"

struct BLaplacianSystem {
	float *vweights;			/* Total sum of weights per vertice*/
	float (*delta)[3];			/* Differential Coordinates*/
	BMVert **uverts;				/* Unit vectors of projected edges onto the plane orthogonal to  n*/
	int numVerts;				/* Number of verts*/
	/* Pointers to data*/
	BMesh *bm;
	BMOperator *op;
	NLContext *context;			/* System for solve general implicit rotations*/
	NLContext *contextrot;		/* System for solve general Laplacian with rotated differential coordinates*/
	vptrSpMatrixD spLapMatrix;  /* Sparse Laplacian Matrix*/
	vptrVectorD VectorB;		/* Array to store vertex positions of handlers*/
	vptrVectorD VectorX;		/* Array to  store solution */
	vptrTripletD tripletList;	/* List of triplets in Laplacian Matrix*/
};
typedef struct BLaplacianSystem LaplacianSystem;

static void compute_mesh_laplacian();
static void delete_laplacian_system(LaplacianSystem *sys);
static void delete_void_pointer(void *data);

static float cotan_weight(float *v1, float *v2, float *v3)
{
	float a[3], b[3], c[3], clen;

	sub_v3_v3v3(a, v2, v1);
	sub_v3_v3v3(b, v3, v1);
	cross_v3_v3v3(c, a, b);

	clen = len_v3(c);
	
	if (clen < FLT_EPSILON)
		return 0.0f;

	return dot_v3v3(a, b) / clen;
}

static void delete_void_pointer(void *data)
{
	if (data) {
		MEM_freeN(data);
	}
}

static void delete_laplacian_system(LaplacianSystem *sys)
{
	if(!sys) return;
	delete_void_pointer(sys->vweights);
	delete_void_pointer(sys->delta);
	delete_void_pointer(sys->uverts);
	if (sys->context) {
		nlDeleteContext(sys->context);
	}
	if (sys->contextrot) {
		nlDeleteContext(sys->contextrot);
	}
	if (sys->spLapMatrix) {
		delete_spmatrix(sys->spLapMatrix);
	}
	if (sys->VectorB) {
		delete_vectord(sys->VectorB);
	}
	if (sys->VectorX) {
		delete_vectord(sys->VectorX);
	}
	if (sys->tripletList) {
		delete_triplet(sys->tripletList);
	}
	sys->bm = NULL;
	sys->op = NULL;
	MEM_freeN(sys);
}

static void memset_laplacian_system(LaplacianSystem *sys, int val)
{
	memset(sys->vweights,     val, sizeof(float) * sys->numVerts);
	memset(sys->delta,     val, sizeof(float) * sys->numVerts);
}

static LaplacianSystem *init_laplacian_system(int a_numVerts, int rows, int cols)
{
	LaplacianSystem *sys;
	sys = (LaplacianSystem *)MEM_callocN(sizeof(LaplacianSystem), "bmoLaplDeformSystem");
	if (!sys) {
		return NULL;
	}
	sys->numVerts = a_numVerts;

	sys->spLapMatrix = new_spmatrix(rows, cols);
	if (!sys->spLapMatrix) {
		delete_laplacian_system(sys);
		return NULL;
	}

	sys->VectorB = new_vectord(rows);
	if (!sys->VectorB) {
		delete_laplacian_system(sys);
		return NULL;
	}

	sys->VectorX = new_vectord(cols);
	if (!sys->VectorX) {
		delete_laplacian_system(sys);
		return NULL;
	}
	
	sys->tripletList = new_triplet(a_numVerts*18);
	if (!sys->tripletList) {
		delete_laplacian_system(sys);
		return NULL;
	}

	sys->vweights =  (float *)MEM_callocN(sizeof(float) * sys->numVerts, "bmoLaplDeformVweights");
	if (!sys->vweights) {
		delete_laplacian_system(sys);
		return NULL;
	}

	sys->uverts =  (BMVert **)MEM_callocN(sizeof(BMVert *) * sys->numVerts, "bmoLaplDeformuverts");
	if (!sys->uverts) {
		delete_laplacian_system(sys);
		return NULL;
	}

	sys->delta =  (float (*)[3])MEM_callocN(sizeof(float) * sys->numVerts * 3, "bmoLaplDeformDelta");
	if (!sys->delta) {
		delete_laplacian_system(sys);
		return NULL;
	}

	return sys;
}

static void init_laplacian_matrix(LaplacianSystem *sys)
{
	float *v1, *v2, *v3, *v4;
	float w2, w3, w4;
	int i, j;
	bool has_4_vert;
	unsigned int idv1, idv2, idv3, idv4, idv[4];
	BMFace *f;
	BMIter fiter;
	BMIter vi;
	BMVert *vn;
	BMVert *vf[4];

	BM_ITER_MESH (f, &fiter, sys->bm, BM_FACES_OF_MESH) {
		if (BM_elem_flag_test(f, BM_ELEM_SELECT)) {

			BM_ITER_ELEM_INDEX (vn, &vi, f, BM_VERTS_OF_FACE, i) {
				vf[i] = vn;
			}
			has_4_vert = (i == 4) ? 1 : 0;
			idv1 = BM_elem_index_get(vf[0]);
			idv2 = BM_elem_index_get(vf[1]);
			idv3 = BM_elem_index_get(vf[2]);
			idv4 = has_4_vert ? BM_elem_index_get(vf[3]) : 0;

			v1 = vf[0]->co;
			v2 = vf[1]->co;
			v3 = vf[2]->co;
			v4 = has_4_vert ? vf[3]->co : 0;

			idv[0] = idv1;
			idv[1] = idv2;
			idv[2] = idv3;
			idv[3] = idv4;

			nlMakeCurrent(sys->context);
			nlRightHandSideAdd(0, idv1						, 0.0f);
			nlRightHandSideAdd(0, sys->numVerts + idv1		, 0.0f);
			nlRightHandSideAdd(0, 2*sys->numVerts + idv1	, 0.0f);

			for (j = 0; j < i; j++) {
				idv1 = idv[j];
				idv2 = idv[(j + 1) % i];
				idv3 = idv[(j + 2) % i];
				idv4 = idv[(j + 3) % i];

				v1 = vf[j]->co;
				v2 = vf[(j + 1) % i]->co;
				v3 = vf[(j + 2) % i]->co;
				v4 = has_4_vert ? vf[(j + 3) % i]->co : 0;

				if (has_4_vert) {

					w2 = (cotan_weight(v4, v1, v2) + cotan_weight(v3, v1, v2)) ;
					w3 = (cotan_weight(v2, v3, v1) + cotan_weight(v4, v1, v3)) ;
					w4 = (cotan_weight(v2, v4, v1) + cotan_weight(v3, v4, v1)) ;

					sys->delta[idv1][0] -=  v4[0] * w4;
					sys->delta[idv1][1] -=  v4[1] * w4;
					sys->delta[idv1][2] -=  v4[2] * w4;

					nlMakeCurrent(sys->context);
					nlMatrixAdd(idv1					, idv4						, -w4 );
					nlMatrixAdd(sys->numVerts + idv1	, sys->numVerts + idv4		, -w4 );
					nlMatrixAdd(sys->numVerts*2 + idv1	, sys->numVerts*2 + idv4	, -w4 );

					nlMakeCurrent(sys->contextrot);
					nlMatrixAdd(idv1					, idv4						, -w4 );

					push_back_triplet(sys->tripletList, idv1					, idv4						, -w4 );
					push_back_triplet(sys->tripletList, sys->numVerts + idv1	, sys->numVerts + idv4		, -w4 );
					push_back_triplet(sys->tripletList, sys->numVerts*2 + idv1	, sys->numVerts*2 + idv4	, -w4 );
					
					
				}
				else {
					w2 = cotan_weight(v3, v1, v2);
					w3 = cotan_weight(v2, v3, v1);
					w4 = 0.0f;
				}

				sys->vweights[idv1] += w2 + w3 + w4;

				sys->delta[idv1][0] +=  v1[0] * (w2 + w3 + w4);
				sys->delta[idv1][1] +=  v1[1] * (w2 + w3 + w4);
				sys->delta[idv1][2] +=  v1[2] * (w2 + w3 + w4);

				sys->delta[idv1][0] -=  v2[0] * w2;
				sys->delta[idv1][1] -=  v2[1] * w2;
				sys->delta[idv1][2] -=  v2[2] * w2;

				sys->delta[idv1][0] -=  v3[0] * w3;
				sys->delta[idv1][1] -=  v3[1] * w3;
				sys->delta[idv1][2] -=  v3[2] * w3;

				nlMakeCurrent(sys->context);
				nlMatrixAdd(idv1					, idv2						, -w2);
				nlMatrixAdd(sys->numVerts + idv1	, sys->numVerts + idv2		, -w2);
				nlMatrixAdd(sys->numVerts*2 + idv1	, sys->numVerts*2 + idv2	, -w2);

				nlMatrixAdd(idv1					, idv3						, -w3);
				nlMatrixAdd(sys->numVerts + idv1	, sys->numVerts + idv3		, -w3);
				nlMatrixAdd(sys->numVerts*2 + idv1	, sys->numVerts*2 + idv3	, -w3);

				nlMatrixAdd(idv1					, idv1						, w2 + w3 + w4);
				nlMatrixAdd(sys->numVerts + idv1	, sys->numVerts + idv1		, w2 + w3 + w4);
				nlMatrixAdd(sys->numVerts*2 + idv1	, sys->numVerts*2 + idv1	, w2 + w3 + w4);

				nlMakeCurrent(sys->contextrot);
				nlMatrixAdd(idv1					, idv2						, -w2);
				nlMatrixAdd(idv1					, idv3						, -w3);
				nlMatrixAdd(idv1					, idv1						, w2 + w3 + w4);

				push_back_triplet(sys->tripletList, idv1					, idv2						, -w2);
				push_back_triplet(sys->tripletList, sys->numVerts + idv1	, sys->numVerts + idv2		, -w2);
				push_back_triplet(sys->tripletList, sys->numVerts*2 + idv1	, sys->numVerts*2 + idv2	, -w2);

				push_back_triplet(sys->tripletList, idv1					, idv3						, -w3);
				push_back_triplet(sys->tripletList, sys->numVerts + idv1	, sys->numVerts + idv3		, -w3);
				push_back_triplet(sys->tripletList, sys->numVerts*2 + idv1	, sys->numVerts*2 + idv3	, -w3);

				push_back_triplet(sys->tripletList, idv1					, idv1						, w2 + w3 + w4);
				push_back_triplet(sys->tripletList, sys->numVerts + idv1	, sys->numVerts + idv1		, w2 + w3 + w4);
				push_back_triplet(sys->tripletList, sys->numVerts*2 + idv1	, sys->numVerts*2 + idv1	, w2 + w3 + w4);
				
			}
		}
	}
}

static void compute_implicit_rotations(LaplacianSystem * sys)
{
	BMEdge *e;
	BMIter eiter;
	BMIter viter;
	BMVert *v;
	BMVert *v2;
	BMVert ** vn = NULL;
	float minj, mjt, *qj, *qi, vj[3];
	int i, j, ln, jid;
	vptrMatrixD C, TDelta;
	BLI_array_declare(vn);

	nlMakeCurrent(sys->context);
	BM_ITER_MESH (v, &viter, sys->bm, BM_VERTS_OF_MESH) {
		i = BM_elem_index_get(v);
		
		BM_ITER_ELEM(e, &eiter, v, BM_EDGES_OF_VERT) {
			if (BM_elem_index_get(e->v1) == i) {
				BLI_array_append(vn, e->v2);
			}
			else {
				BLI_array_append(vn, e->v1);
			}
		}
		BLI_array_append(vn, v);
		ln = BLI_array_count(vn);
		minj = 1000000.0f;
		for (j = 0; j < (ln-1); j++) {
			qj = vn[j]->co;
			sub_v3_v3v3(vj, qj, v->co);
			normalize_v3(vj);
			mjt = fabs(dot_v3v3(vj, v->no));
			if (mjt < minj) {
				minj = mjt;
				sys->uverts[i] = vn[j];
				//copy_v3_v3(sys->uverts[i], vn[j]->co);
			}
		}

		C = new_matrixd( ln*3, 7);
		for (j = 0; j < ln; j++) {
			v2 = vn[j];
			set_matrixd(C, j, 0, v2->co[0]);		
			set_matrixd(C, j, 1, 0.0f);		
			set_matrixd(C, j, 2, v2->co[2]);		
			set_matrixd(C, j, 3, -v2->co[1]);
			set_matrixd(C, j, 4, 1.0f);
			set_matrixd(C, j, 5, 0.0f);
			set_matrixd(C, j, 6, 0.0f);

			set_matrixd(C, ln + j, 0, v2->co[1]);		
			set_matrixd(C, ln + j, 1, -v2->co[2]);		
			set_matrixd(C, ln + j, 2, 0.0f);		
			set_matrixd(C, ln + j, 3, v2->co[0]);
			set_matrixd(C, ln + j, 4, 0.0f);
			set_matrixd(C, ln + j, 5, 1.0f);
			set_matrixd(C, ln + j, 6, 0.0f);

			set_matrixd(C, ln*2 + j, 0, v2->co[2]);		
			set_matrixd(C, ln*2 + j, 1, v2->co[1]);		
			set_matrixd(C, ln*2 + j, 2, -v2->co[0]);		
			set_matrixd(C, ln*2 + j, 3, 0.0f);
			set_matrixd(C, ln*2 + j, 4, 0.0f);
			set_matrixd(C, ln*2 + j, 5, 0.0f);
			set_matrixd(C, ln*2 + j, 6, 1.0f);
		}
		TDelta = new_matrixd(3, ln * 3);
		compute_delta_rotations_matrixd(C, TDelta, sys->delta[i][0], sys->delta[i][1], sys->delta[i][2]);

		
		for (j = 0; j < ln; j++) {
			jid = BM_elem_index_get(vn[j]);
			nlMatrixAdd(i,						jid						, - get_matrixd(TDelta, 0, j));
			nlMatrixAdd(i,						sys->numVerts + jid		, - get_matrixd(TDelta, 0, j + ln));
			nlMatrixAdd(i,						sys->numVerts*2 + jid	, - get_matrixd(TDelta, 0, j + ln *2));
			nlMatrixAdd(i + sys->numVerts,		jid						, - get_matrixd(TDelta, 1, j));
			nlMatrixAdd(i + sys->numVerts,		sys->numVerts + jid		, - get_matrixd(TDelta, 1, j + ln));
			nlMatrixAdd(i + sys->numVerts,		sys->numVerts*2 + jid	, - get_matrixd(TDelta, 1, j + ln *2));
			nlMatrixAdd(i + sys->numVerts*2,	jid						, - get_matrixd(TDelta, 2, j));
			nlMatrixAdd(i + sys->numVerts*2,	sys->numVerts + jid		, - get_matrixd(TDelta, 2, j + ln));
			nlMatrixAdd(i + sys->numVerts*2,	sys->numVerts*2 + jid	, - get_matrixd(TDelta, 2, j + ln *2));

			push_back_triplet(sys->tripletList, i,						jid						, - get_matrixd(TDelta, 0, j));
			push_back_triplet(sys->tripletList, i,						sys->numVerts + jid		, - get_matrixd(TDelta, 0, j + ln));
			push_back_triplet(sys->tripletList, i,						sys->numVerts*2 + jid	, - get_matrixd(TDelta, 0, j + ln *2));
			push_back_triplet(sys->tripletList, i + sys->numVerts,		jid						, - get_matrixd(TDelta, 1, j));
			push_back_triplet(sys->tripletList, i + sys->numVerts,		sys->numVerts + jid		, - get_matrixd(TDelta, 1, j + ln));
			push_back_triplet(sys->tripletList, i + sys->numVerts,		sys->numVerts*2 + jid	, - get_matrixd(TDelta, 1, j + ln *2));
			push_back_triplet(sys->tripletList, i + sys->numVerts*2,	jid						, - get_matrixd(TDelta, 2, j));
			push_back_triplet(sys->tripletList, i + sys->numVerts*2,	sys->numVerts + jid		, - get_matrixd(TDelta, 2, j + ln));
			push_back_triplet(sys->tripletList, i + sys->numVerts*2,	sys->numVerts*2 + jid	, - get_matrixd(TDelta, 2, j + ln *2));

		}
		BLI_array_free(vn);
		BLI_array_empty(vn);
		vn = NULL;
		delete_matrixd(C);
		delete_matrixd(TDelta);

	}
}

/*
	for i=1:n
        j = nbr_i(i);
        pi = xyz(i, :);
        ni = normal(:, i)';
        pj = xyz(j, :);
        uij = pj -pi;
        uij = uij - (dot(uij,ni))*ni;
        uij = uij/max(norm(uij), 0.00001);
        e2 = cross(ni, uij);
        deltai = delta(i,:);
        alpha = dot(ni,deltai);
        beta = dot(uij,deltai);
        gamma = dot(e2,deltai);
        
        pi = xyz_prime_nonh(i,:);
        ni = normal_deform(:, i)';
        pj = xyz_prime_nonh(j,:);
        uij = pj -pi;
        uij = uij - (dot(uij,ni))*ni;
        uij = uij/max(norm(uij), 0.000001);
        e2 = cross(ni, uij);  
        
        new_d = alpha*ni + beta*uij + gamma*e2;
        new_delta(i,:) = new_d;
    end
		*/
void rotate_differential_coordinates(LaplacianSystem *sys) 
{
	BMFace *f;
	BMVert *v, *v2;
	BMIter fiter;
	BMIter viter, viter2;
	float alpha, beta, gamma,
		pj[3], ni[3], di[3], rdelta[3],
		uij[3], dun[3], e2[3], pi[3], fni[3], vn[4][3];
	int i, j, *vin, lvin, num_fni;

	BLI_array_declare(vin);

	BM_ITER_MESH (v, &viter, sys->bm, BM_VERTS_OF_MESH) {
		i = BM_elem_index_get(v);
		copy_v3_v3(pi, v->co);
		copy_v3_v3(ni, v->no);
		copy_v3_v3(pj, sys->uverts[i]->co);
		sub_v3_v3v3(uij, pj, pi);
		mul_v3_v3fl(dun, ni, dot_v3v3(uij, ni));
		sub_v3_v3(uij, dun);
		normalize_v3(uij);
		cross_v3_v3v3(e2, ni, uij);
		copy_v3_v3(di, sys->delta[i]);
		alpha = dot_v3v3(ni, di);
        beta = dot_v3v3(uij, di);
        gamma = dot_v3v3(e2, di);

		pi[0] = nlGetVariable(0, i);
		pi[1] = nlGetVariable(0, i + sys->numVerts);
		pi[2] = nlGetVariable(0, i + sys->numVerts*2);
		ni[0] = 0.0f;	ni[1] = 0.0f;	ni[2] = 0.0f;
		num_fni = 0;
		nlMakeCurrent(sys->context);
		BM_ITER_ELEM_INDEX(f, &fiter, v, BM_FACES_OF_VERT, num_fni) {
			BM_ITER_ELEM(v2, &viter2, f, BM_VERTS_OF_FACE) {
				BLI_array_append(vin, BM_elem_index_get(v2));
			}
			lvin = BLI_array_count(vin);
			
			for (j=0; j<lvin; j++ ) {
				vn[j][0] = nlGetVariable(0, vin[j]);
				vn[j][1] = nlGetVariable(0, vin[j] + sys->numVerts);
				vn[j][2] = nlGetVariable(0, vin[j] + sys->numVerts*2);
				if (j == BM_elem_index_get(sys->uverts[i])) {
					copy_v3_v3(pj, vn[j]);
				}
			}
			if (lvin == 3) {
				normal_tri_v3(fni, vn[0], vn[1], vn[2]);
			} 
			else if(lvin == 4) {
				normal_quad_v3(fni, vn[0], vn[1], vn[2], vn[3]);
			} 
			add_v3_v3(ni, fni);
			BLI_array_free(vin);
			BLI_array_empty(vin);
			vin = NULL;
		}
		if (num_fni>0) mul_v3_fl(ni, 1.0f/num_fni);
		sub_v3_v3v3(uij, pj, pi);
		mul_v3_v3fl(dun, ni, dot_v3v3(uij, ni));
		sub_v3_v3(uij, dun);
		normalize_v3(uij);
		cross_v3_v3v3(e2, ni, uij);

		nlMakeCurrent(sys->contextrot);
		nlRightHandSideSet(0, i, alpha*ni[0] + beta*uij[0] + gamma*e2[0]);
		nlRightHandSideSet(1, i, alpha*ni[1] + beta*uij[1] + gamma*e2[1]);
		nlRightHandSideSet(2, i, alpha*ni[2] + beta*uij[2] + gamma*e2[2]);
	}
	
}

void bmo_deform_laplacian_vert_exec(BMesh *bm, BMOperator *op)
{
	int vid;
	BMOIter siter;
	BMVert *v;
	LaplacianSystem *sys;

	if (bm->totface == 0) return;
	sys = init_laplacian_system( bm->totvert, bm->totvert*3 + 40*3, bm->totvert * 3 );
	if (!sys) return;
	sys->bm = bm;
	sys->op = op;
	memset_laplacian_system(sys, 0);
	BM_mesh_elem_index_ensure(bm, BM_VERT);

	nlNewContext();
	sys->context = nlGetCurrent();
	nlNewContext();
	sys->contextrot = nlGetCurrent();

	nlMakeCurrent(sys->context);
	nlSolverParameteri(NL_NB_VARIABLES, bm->totvert*3);
	nlSolverParameteri(NL_SYMMETRIC, NL_FALSE);
	nlSolverParameteri(NL_LEAST_SQUARES, NL_TRUE);
	nlSolverParameteri(NL_NB_ROWS, bm->totvert*3 + 40*3);
	nlSolverParameteri(NL_NB_RIGHT_HAND_SIDES, 1);

	nlMakeCurrent(sys->contextrot);
	nlSolverParameteri(NL_NB_VARIABLES, bm->totvert);
	nlSolverParameteri(NL_SYMMETRIC, NL_FALSE);
	nlSolverParameteri(NL_LEAST_SQUARES, NL_TRUE);
	nlSolverParameteri(NL_NB_ROWS, bm->totvert + 40);
	nlSolverParameteri(NL_NB_RIGHT_HAND_SIDES, 3);

	nlMakeCurrent(sys->context);
	nlBegin(NL_SYSTEM);
	nlBegin(NL_MATRIX);

	nlMakeCurrent(sys->contextrot);
	nlBegin(NL_SYSTEM);
	nlBegin(NL_MATRIX);
	
	init_laplacian_matrix(sys);
	compute_implicit_rotations(sys);

	/* Block code only for testing*/
	BMO_ITER (v, &siter, sys->op->slots_in, "verts", BM_VERT) {
		vid = BM_elem_index_get(v);
		if (vid < 10) {
			nlMakeCurrent(sys->context);
			nlRightHandSideAdd(0, bm->totvert * 3 + vid * 3	 	, v->co[0]);
			nlRightHandSideAdd(0, bm->totvert * 3 + vid * 3 + 1	, v->co[1]);
			nlRightHandSideAdd(0, bm->totvert * 3 + vid * 3 + 2	, v->co[2]);

			nlMatrixAdd(bm->totvert * 3 + vid * 3		, vid					, 1.0f);
			nlMatrixAdd(bm->totvert * 3	+ vid * 3 + 1   , bm->totvert + vid		, 1.0f);
			nlMatrixAdd(bm->totvert * 3	+ vid * 3 + 2	, 2*bm->totvert + vid	, 1.0f);

			nlMakeCurrent(sys->contextrot);
			nlRightHandSideAdd(0, bm->totvert + vid 	 	, v->co[0]);
			nlRightHandSideAdd(1, bm->totvert + vid 		, v->co[1]);
			nlRightHandSideAdd(2, bm->totvert + vid 		, v->co[2]);

			nlMatrixAdd(bm->totvert + vid 		, vid					, 1.0f);

			set_vectord(sys->VectorB, bm->totvert * 3 + vid * 3	 	, v->co[0]);
			set_vectord(sys->VectorB, bm->totvert * 3 + vid * 3 + 1	, v->co[1]);
			set_vectord(sys->VectorB, bm->totvert * 3 + vid * 3 + 2	, v->co[2]);

			push_back_triplet(sys->tripletList, bm->totvert * 3 + vid * 3		, vid					, 1.0f);
			push_back_triplet(sys->tripletList, bm->totvert * 3	+ vid * 3 + 1   , bm->totvert + vid		, 1.0f);
			push_back_triplet(sys->tripletList, bm->totvert * 3	+ vid * 3 + 2	, 2*bm->totvert + vid	, 1.0f);
			

		} else if (vid <= 39) {
			nlMakeCurrent(sys->context);
			nlRightHandSideAdd(0, bm->totvert * 3 + vid * 3	 	, v->co[0]*2.0f+2.0f);
			nlRightHandSideAdd(0, bm->totvert * 3 + vid * 3 + 1	, v->co[1]*2.0f+2.0f);
			nlRightHandSideAdd(0, bm->totvert * 3 + vid * 3 + 2	, v->co[2]);

			nlMatrixAdd(bm->totvert * 3 + vid * 3		, vid					, 1.0f);
			nlMatrixAdd(bm->totvert * 3	+ vid * 3 + 1   , bm->totvert + vid		, 1.0f);
			nlMatrixAdd(bm->totvert * 3	+ vid * 3 + 2	, 2*bm->totvert + vid	, 1.0f);

			nlMakeCurrent(sys->contextrot);
			nlRightHandSideAdd(0, bm->totvert + vid 	 	, v->co[0]*2.0f+2.0f);
			nlRightHandSideAdd(1, bm->totvert + vid			, v->co[1]*2.0f+2.0f);
			nlRightHandSideAdd(2, bm->totvert + vid			, v->co[2]);

			nlMatrixAdd(bm->totvert + vid 		, vid					, 1.0f);

			set_vectord(sys->VectorB, bm->totvert * 3 + vid * 3	 	, v->co[0]+2.0f);
			set_vectord(sys->VectorB, bm->totvert * 3 + vid * 3 + 1	, v->co[1]+2.0f);
			set_vectord(sys->VectorB, bm->totvert * 3 + vid * 3 + 2	, v->co[2]);

			push_back_triplet(sys->tripletList, bm->totvert * 3 + vid * 3		, vid					, 1.0f);
			push_back_triplet(sys->tripletList, bm->totvert * 3	+ vid * 3 + 1   , bm->totvert + vid		, 1.0f);
			push_back_triplet(sys->tripletList, bm->totvert * 3	+ vid * 3 + 2	, 2*bm->totvert + vid	, 1.0f);
		}
	}


	nlMakeCurrent(sys->context);
	nlEnd(NL_MATRIX);
	nlEnd(NL_SYSTEM);

	/* Solve system with Eigen3*/
	//set_spmatrix_from_triplets(sys->spLapMatrix, sys->tripletList);
	//solve_system(sys->spLapMatrix, sys->VectorB, sys->VectorX);

	if (nlSolveAdvanced(NULL, NL_TRUE) ) {
		rotate_differential_coordinates(sys);
		nlMakeCurrent(sys->contextrot);
		nlEnd(NL_MATRIX);
		nlEnd(NL_SYSTEM);
		if (nlSolveAdvanced(NULL, NL_TRUE) ) {
			BMO_ITER (v, &siter, sys->op->slots_in, "verts", BM_VERT) {
				vid = BM_elem_index_get(v);
				/* Solve system with Eigen3*/
				//v->co[0] = get_vectord(sys->VectorX, vid);
				//v->co[1] = get_vectord(sys->VectorX, sys->numVerts + vid);
				//v->co[2] = get_vectord(sys->VectorX, 2*sys->numVerts + vid);
				v->co[0] = nlGetVariable(0, vid);
				v->co[1] = nlGetVariable(1, vid);
				v->co[2] = nlGetVariable(2, vid);
			}
		}
	}

	delete_laplacian_system(sys);
}