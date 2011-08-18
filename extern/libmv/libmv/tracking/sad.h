/****************************************************************************
**
** Copyright (c) 2011 libmv authors.
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to
** deal in the Software without restriction, including without limitation the
** rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
** sell copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
****************************************************************************/

#ifndef LIBMV_TRACKING_SAD_H_
#define LIBMV_TRACKING_SAD_H_

#ifdef __cplusplus
namespace libmv {
#endif

typedef unsigned char ubyte;
typedef unsigned int uint;

/// Affine transformation matrix in column major order.
struct mat32 {
  float data[3*2];
#ifdef __cplusplus
  inline mat32(int d=1) { for(int i=0;i<3*2;i++) data[i]=0; if(d!=0) for(int i=0;i<2;i++) m(i,i)=d; }
  inline float m(int i, int j) const { return data[j*2+i]; }
  inline float& m(int i, int j) { return data[j*2+i]; }
  inline float operator()(int i, int j) const { return m(i,j); }
  inline float& operator()(int i, int j) { return m(i,j); }
  inline operator bool() const { for (int i=0; i<3*2; i++) if(data[i]!=0) return true; return false; }
#endif
};


/*!
    Sample \a pattern from \a image.

    \a warp is the transformation to apply to \a image when sampling the \a pattern.
*/
void SamplePattern(ubyte* image, int stride, mat32 warp, ubyte* pattern, int size);

/*!
    Track \a pattern in \a image.

    This template matcher computes the
    \link http://en.wikipedia.org/wiki/Sum_of_absolute_differences Sum of Absolute Differences (SAD) \endlink
    for each integer pixel position in the search region and then iteratively
    refine subpixel position using a square search.
    A similar method is used for motion estimation in video encoders.

    \a reference is the pattern to track.
    the \a size of the pattern should be aligned to 16.
    \a image is a reference to the region to search.
    \a stride is size of \a image lines.

    On input, \a warp is the predicted affine transformation (e.g from previous frame)
    On return, \a warp is the affine transformation which best match the reference \a pattern

    \return Pearson product-moment correlation coefficient between reference and matched pattern.
            This measure of the linear dependence between the patterns
            ranges from −1 (negative correlation) to 1 (positive correlation).
            A value of 0 implies that there is no linear correlation between the variables.

    \note To track affine features:
     - Sample reference pattern using estimated (e.g previous frame) warp.
     -
    \note \a stride allow you to reference your search region instead of copying.
    \note For a 16x speedup, compile this tracker with SSE2 support.
*/
float Track(ubyte* reference, int size, ubyte* image, int stride, int width, int height, mat32* warp);

#ifdef __cplusplus
}  // namespace libmv
#endif

#endif  // LIBMV_TRACKING_SAD_H_
