//
//  Filename         : FEdgeXDetector.h
//  Author(s)        : Stephane Grabli
//  Purpose          : Detects/flags/builds extended features edges on the
//                     WXEdge structure
//  Date of creation : 26/10/2003
//
///////////////////////////////////////////////////////////////////////////////


//
//  Copyright (C) : Please refer to the COPYRIGHT file distributed 
//   with this source distribution. 
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
///////////////////////////////////////////////////////////////////////////////


#ifndef  FEDGEXDETECTOR_H
# define FEDGEXDETECTOR_H

# include <vector>
# include "../system/FreestyleConfig.h"
# include "../geometry/Geom.h"
# include "../winged_edge/WXEdge.h"
# include "../winged_edge/Curvature.h"
# include "../system/ProgressBar.h"

using namespace Geometry;

/*! This class takes as input a WXEdge structure and fills it
 */

class LIB_VIEW_MAP_EXPORT FEdgeXDetector
{
public:

  FEdgeXDetector() {
    _pProgressBar = 0;
    _computeViewIndependant = true;
    _bbox_diagonal = 1.0;
    _meanEdgeSize = 0;
    _computeRidgesAndValleys = true;
    _computeSuggestiveContours = true;
	_computeMaterialBoundaries = true;
    _sphereRadius = 1.0;
    _orthographicProjection = false;
    _changes = false;
    _kr_derivative_epsilon = 0.0;
	_creaseAngle = 0.7; // angle of 134.43 degrees
  }
  virtual ~FEdgeXDetector() {}

  /*! Process shapes from a WingedEdge containing a list of WShapes */
  virtual void processShapes(WingedEdge&);

  // GENERAL STUFF
  virtual void preProcessShape(WXShape* iShape);
  virtual void preProcessFace(WXFace* iFace);
  virtual void computeCurvatures(WXVertex *iVertex);

  // SILHOUETTE
  virtual void processSilhouetteShape(WXShape* iShape);
  virtual void ProcessSilhouetteFace(WXFace *iFace);
  virtual void ProcessSilhouetteEdge(WXEdge *iEdge);

  // CREASE
  virtual void processCreaseShape(WXShape* iShape);
  virtual void ProcessCreaseEdge(WXEdge *iEdge);
  /*! Sets the minimum angle for detecting crease edges
   *  \param angle
   *    The angular threshold in degrees (between 0 and 180) for detecting crease
   *    edges.  An edge is considered a crease edge if the angle between two faces
   *    sharing the edge is smaller than the given threshold.
   */
  inline void setCreaseAngle(real angle) {
    if (angle < 0.0)
      angle = 0.0;
	else if (angle > 180.0)
      angle = 180.0;
    angle = cos(M_PI * (180.0 - angle) / 180.0);
    if (angle != _creaseAngle){
      _creaseAngle = angle;
      _changes = true;
    }
  }

  // BORDER
  virtual void processBorderShape(WXShape* iShape);
  virtual void ProcessBorderEdge(WXEdge *iEdge);

  // RIDGES AND VALLEYS
  virtual void processRidgesAndValleysShape(WXShape* iShape);
  virtual void ProcessRidgeFace(WXFace *iFace);

  // SUGGESTIVE CONTOURS
  virtual void processSuggestiveContourShape(WXShape* iShape);
  virtual void ProcessSuggestiveContourFace(WXFace *iFace);
  virtual void postProcessSuggestiveContourShape(WXShape* iShape);
  virtual void postProcessSuggestiveContourFace(WXFace *iFace);
  /*! Sets the minimal derivative of the radial curvature for suggestive contours
   *  \param dkr
   *    The minimal derivative of the radial curvature
   */
  inline void setSuggestiveContourKrDerivativeEpsilon(real dkr) {
    if (dkr != _kr_derivative_epsilon){
      _kr_derivative_epsilon = dkr;
      _changes = true;
    }
  }

  // MATERIAL BOUNDARY
  virtual void processMaterialBoundaryShape(WXShape* iWShape);
  virtual void ProcessMaterialBoundaryEdge(WXEdge *iEdge);

  // EVERYBODY
  virtual void buildSmoothEdges(WXShape* iShape);

  /*! Sets the current viewpoint */
  inline void setViewpoint(const Vec3r& ivp) {_Viewpoint = ivp;}
  inline void enableOrthographicProjection(bool b) {_orthographicProjection = b;}
  inline void enableRidgesAndValleysFlag(bool b) {_computeRidgesAndValleys = b;}
  inline void enableSuggestiveContours(bool b) {_computeSuggestiveContours = b;}
  inline void enableMaterialBoundaries(bool b) {_computeMaterialBoundaries = b;}
  /*! Sets the radius of the geodesic sphere around each vertex (for the curvature computation)
   *  \param r
   *    The radius of the sphere expressed as a ratio of the mean edge size
   */
  inline void setSphereRadius(real r) {
    if(r!=_sphereRadius){
      _sphereRadius = r;
      _changes=true;
    }
  }

  inline void setProgressBar(ProgressBar *iProgressBar) {_pProgressBar = iProgressBar;}

protected:

  Vec3r _Viewpoint;
  real _bbox_diagonal; // diagonal of the current processed shape bbox
  //oldtmp values
  bool _computeViewIndependant;
  real _meanK1;
  real _meanKr;
  real _minK1;
  real _minKr;
  real _maxK1;
  real _maxKr;
  unsigned _nPoints;
  real _meanEdgeSize;
  bool _orthographicProjection;

  bool _computeRidgesAndValleys;
  bool _computeSuggestiveContours;
  bool _computeMaterialBoundaries;
  real _sphereRadius; // expressed as a ratio of the mean edge size
  real _creaseAngle; // [-1, 1] compared with the inner product of face normals
  bool _changes;

  real _kr_derivative_epsilon;

  ProgressBar *_pProgressBar;
};

#endif // FEDGEDXETECTOR_H
