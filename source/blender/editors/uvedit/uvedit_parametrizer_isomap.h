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
 * Contributor(s): Antony Riakiotakis
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/**
  C interface for isomap unwrapper. The unwrapper utilizes eigen for eigenvalue solving
  and this file provides the needed funtions to initialize/cleanup/solve/load the solution
*/

#ifndef UVEDIT_PARAMETRIZER_ISOMAP_H
#define UVEDIT_PARAMETRIZER_ISOMAP_H

#ifdef __cplusplus
extern "C" {
#endif

void param_isomap_new_solver(int nverts);
int param_isomap_solve(float dist_matrix[]);
void param_isomap_delete_solver(void);
void param_isomap_load_uv_solution(int index, float uv[2]);

#ifdef __cplusplus
}
#endif


#endif // UVEDIT_PARAMETRIZER_ISOMAP_H
