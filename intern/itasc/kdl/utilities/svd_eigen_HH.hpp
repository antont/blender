// Copyright  (C)  2007  Ruben Smits <ruben dot smits at mech dot kuleuven dot be>

// Version: 1.0
// Author: Ruben Smits <ruben dot smits at mech dot kuleuven dot be>
// Maintainer: Ruben Smits <ruben dot smits at mech dot kuleuven dot be>
// URL: http://www.orocos.org/kdl

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


//Based on the svd of the KDL-0.2 library by Erwin Aertbelien
#ifndef SVD_EIGEN_HH_HPP
#define SVD_EIGEN_HH_HPP


#include <Eigen/Core>
#include <algorithm>

namespace KDL
{
    template<typename Scalar> inline Scalar PYTHAG(Scalar a,Scalar b) {
        double at,bt,ct;
        at = fabs(a);
        bt = fabs(b);
        if (at > bt ) {
            ct=bt/at;
            return Scalar(at*sqrt(1.0+ct*ct));
        } else {
            if (bt==0)
                return Scalar(0.0);
            else {
                ct=at/bt;
                return Scalar(bt*sqrt(1.0+ct*ct));
            }
        }
    }


    template<typename Scalar> inline Scalar SIGN(Scalar a,Scalar b) {
        return ((b) >= Scalar(0.0) ? fabs(a) : -fabs(a));
    }

    /**
     * svd calculation of boost ublas matrices
     *
     * @param A matrix<double>(mxn)
     * @param U matrix<double>(mxn)
     * @param S vector<double> n
     * @param V matrix<double>(nxn)
     * @param tmp vector<double> n
     * @param maxiter defaults to 150
     *
     * @return -2 if maxiter exceeded, 0 otherwise
     */
	template<typename MatrixA, typename MatrixUV, typename VectorS> 
	int svd_eigen_HH(
		const Eigen::MatrixBase<MatrixA>&		A,
		Eigen::MatrixBase<MatrixUV>&			U,
		Eigen::MatrixBase<VectorS>&				S,
		Eigen::MatrixBase<MatrixUV>&			V,
		Eigen::MatrixBase<VectorS>&				tmp,
		int maxiter=150)
	{
        //get the rows/columns of the matrix
        const int rows = A.rows();
        const int cols = A.cols();
        
        U = A;
        
        int i(-1),its(-1),j(-1),jj(-1),k(-1),nm=0;
        int ppi(0);
        bool flag;
        e_scalar maxarg1,maxarg2,anorm(0),c(0),f(0),h(0),s(0),scale(0),x(0),y(0),z(0),g(0);
        
        g=scale=anorm=e_scalar(0.0);
        
        /* Householder reduction to bidiagonal form. */
        for (i=0;i<cols;i++) {
            ppi=i+1;
            tmp(i)=scale*g;
            g=s=scale=e_scalar(0.0); 
            if (i<rows) {
                // compute the sum of the i-th column, starting from the i-th row
                for (k=i;k<rows;k++) scale += fabs(U(k,i));
                if (scale!=0) {
                    // multiply the i-th column by 1.0/scale, start from the i-th element
                    // sum of squares of column i, start from the i-th element
                    for (k=i;k<rows;k++) {
                        U(k,i) /= scale;
                        s += U(k,i)*U(k,i);
                    }
                    f=U(i,i);  // f is the diag elem
                    g = -SIGN(e_scalar(sqrt(s)),f);
                    h=f*g-s;
                    U(i,i)=f-g;
                    for (j=ppi;j<cols;j++) {
                        // dot product of columns i and j, starting from the i-th row
                        for (s=0.0,k=i;k<rows;k++) s += U(k,i)*U(k,j);
                        f=s/h;
                        // copy the scaled i-th column into the j-th column
                        for (k=i;k<rows;k++) U(k,j) += f*U(k,i);
                    }
                    for (k=i;k<rows;k++) U(k,i) *= scale;
                }
            }
            // save singular value
            S(i)=scale*g;
            g=s=scale=e_scalar(0.0);
            if ((i <rows) && (i+1 != cols)) {
                // sum of row i, start from columns i+1
                for (k=ppi;k<cols;k++) scale += fabs(U(i,k));
                if (scale!=0) {
                    for (k=ppi;k<cols;k++) {
                        U(i,k) /= scale;
                        s += U(i,k)*U(i,k);
                    }
                    f=U(i,ppi);
                    g = -SIGN(e_scalar(sqrt(s)),f);
                    h=f*g-s;
                    U(i,ppi)=f-g;
                    for (k=ppi;k<cols;k++) tmp(k)=U(i,k)/h;
                    for (j=ppi;j<rows;j++) {
                        for (s=0.0,k=ppi;k<cols;k++) s += U(j,k)*U(i,k);
                        for (k=ppi;k<cols;k++) U(j,k) += s*tmp(k);
                    }
                    for (k=ppi;k<cols;k++) U(i,k) *= scale;
                }
            }
            maxarg1=anorm;
            maxarg2=(fabs(S(i))+fabs(tmp(i)));
            anorm = maxarg1 > maxarg2 ?	maxarg1 : maxarg2;		
        }
        /* Accumulation of right-hand transformations. */
        for (i=cols-1;i>=0;i--) {
            if (i<cols-1) {
                if (g) {
                    for (j=ppi;j<cols;j++) V(j,i)=(U(i,j)/U(i,ppi))/g;
                    for (j=ppi;j<cols;j++) {
                        for (s=0.0,k=ppi;k<cols;k++) s += U(i,k)*V(k,j);
                        for (k=ppi;k<cols;k++) V(k,j) += s*V(k,i);
                    }
                }
                for (j=ppi;j<cols;j++) V(i,j)=V(j,i)=0.0;
            }
            V(i,i)=1.0;
            g=tmp(i);
            ppi=i;
        }
        /* Accumulation of left-hand transformations. */
        for (i=cols-1<rows-1 ? cols-1:rows-1;i>=0;i--) {
            ppi=i+1;
            g=S(i);
            for (j=ppi;j<cols;j++) U(i,j)=0.0;
            if (g) {
                g=e_scalar(1.0)/g;
                for (j=ppi;j<cols;j++) {
                    for (s=0.0,k=ppi;k<rows;k++) s += U(k,i)*U(k,j);
                    f=(s/U(i,i))*g;
                    for (k=i;k<rows;k++) U(k,j) += f*U(k,i);
                }
                for (j=i;j<rows;j++) U(j,i) *= g;
            } else {
                for (j=i;j<rows;j++) U(j,i)=0.0;
            }
            ++U(i,i);
        }
        
        /* Diagonalization of the bidiagonal form. */
        for (k=cols-1;k>=0;k--) { /* Loop over singular values. */
            for (its=1;its<=maxiter;its++) {  /* Loop over allowed iterations. */
                flag=true;
                for (ppi=k;ppi>=0;ppi--) {  /* Test for splitting. */
                    nm=ppi-1;             /* Note that tmp(1) is always zero. */
                    if ((fabs(tmp(ppi))+anorm) == anorm) {
                        flag=false;
                        break;
                    }
                    if ((fabs(S(nm)+anorm) == anorm)) break;
                }
                if (flag) {
                    c=e_scalar(0.0);           /* Cancellation of tmp(l), if l>1: */
                    s=e_scalar(1.);
                    for (i=ppi;i<=k;i++) {
                        f=s*tmp(i);
                        tmp(i)=c*tmp(i);
                        if ((fabs(f)+anorm) == anorm) break;
                        g=S(i);
                        h=PYTHAG(f,g);
                        S(i)=h;
                        h=e_scalar(1.0)/h;
                        c=g*h;
                        s=(-f*h);
                        for (j=0;j<rows;j++) {
                            y=U(j,nm);
                            z=U(j,i);
                            U(j,nm)=y*c+z*s;
                            U(j,i)=z*c-y*s;
                        }
                    }
                }
                z=S(k);
                
                if (ppi == k) {       /* Convergence. */
                    if (z < e_scalar(0.0)) {   /* Singular value is made nonnegative. */
                        S(k) = -z;
                        for (j=0;j<cols;j++) V(j,k)=-V(j,k);
                    }
                    break;
                }
                
                x=S(ppi);            /* Shift from bottom 2-by-2 minor: */
                nm=k-1;
                y=S(nm);
                g=tmp(nm);
                h=tmp(k);
                f=((y-z)*(y+z)+(g-h)*(g+h))/(e_scalar(2.0)*h*y);
                
                g=PYTHAG(f,e_scalar(1.0));
                f=((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;
                
                /* Next QR transformation: */
                c=s=1.0;
                for (j=ppi;j<=nm;j++) {
                    i=j+1;
                    g=tmp(i);
                    y=S(i);
                    h=s*g;
                    g=c*g;
                    z=PYTHAG(f,h);
                    tmp(j)=z;
                    c=f/z;
                    s=h/z;
                    f=x*c+g*s;
                    g=g*c-x*s;
                    h=y*s;
                    y=y*c;
                    for (jj=0;jj<cols;jj++) {
                        x=V(jj,j);
                        z=V(jj,i);
                        V(jj,j)=x*c+z*s;
                        V(jj,i)=z*c-x*s;
                    }
                    z=PYTHAG(f,h);
                    S(j)=z;
                    if (z) {
                        z=e_scalar(1.0)/z;
                        c=f*z;
                        s=h*z;
                    }
                    f=(c*g)+(s*y);
                    x=(c*y)-(s*g);
                    for (jj=0;jj<rows;jj++) {
                        y=U(jj,j);
                        z=U(jj,i);
                        U(jj,j)=y*c+z*s;
                        U(jj,i)=z*c-y*s;
                    }
                }
                tmp(ppi)=0.0;
                tmp(k)=f;
                S(k)=x;
            }
        }

        //Sort eigen values:
        for (i=0; i<cols; i++){
            
            double S_max = S(i);
            int i_max = i;
            for (j=i+1; j<cols; j++){
                double Sj = S(j);
                if (Sj > S_max){
                    S_max = Sj;
                    i_max = j;
                }
            }
            if (i_max != i){
                /* swap eigenvalues */
                e_scalar tmp = S(i);
                S(i)=S(i_max);
                S(i_max)=tmp;
                
                /* swap eigenvectors */
                U.col(i).swap(U.col(i_max));
                V.col(i).swap(V.col(i_max));
            }
        }
        
        
        if (its == maxiter) 
            return (-2);
        else 
            return (0);
    }

}
#endif
