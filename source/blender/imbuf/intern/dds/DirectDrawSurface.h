// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#pragma once
#ifndef NV_IMAGE_DIRECTDRAWSURFACE_H
#define NV_IMAGE_DIRECTDRAWSURFACE_H

#include "nvimage.h"

namespace nv
{
    class Image;
    class Stream;
    struct ColorBlock;

    extern const uint FOURCC_NVTT;
    extern const uint FOURCC_DDS;
    extern const uint FOURCC_DXT1;
    extern const uint FOURCC_DXT2;
    extern const uint FOURCC_DXT3;
    extern const uint FOURCC_DXT4;
    extern const uint FOURCC_DXT5;
    extern const uint FOURCC_RXGB;
    extern const uint FOURCC_ATI1;
    extern const uint FOURCC_ATI2;

    extern const uint D3DFMT_G16R16;
    extern const uint D3DFMT_A16B16G16R16;
    extern const uint D3DFMT_R16F;
    extern const uint D3DFMT_R32F;
    extern const uint D3DFMT_G16R16F;
    extern const uint D3DFMT_G32R32F;
    extern const uint D3DFMT_A16B16G16R16F;
    extern const uint D3DFMT_A32B32G32R32F;

    extern uint findD3D9Format(uint bitcount, uint rmask, uint gmask, uint bmask, uint amask);

    struct NVIMAGE_CLASS DDSPixelFormat
    {
        uint size;
        uint flags;
        uint fourcc;
        uint bitcount;
        uint rmask;
        uint gmask;
        uint bmask;
        uint amask;
    };

    struct NVIMAGE_CLASS DDSCaps
    {
        uint caps1;
        uint caps2;
        uint caps3;
        uint caps4;
    };

    /// DDS file header for DX10.
    struct NVIMAGE_CLASS DDSHeader10
    {
        uint dxgiFormat;
        uint resourceDimension;
        uint miscFlag;
        uint arraySize;
        uint reserved;
    };

    /// DDS file header.
    struct NVIMAGE_CLASS DDSHeader
    {
        uint fourcc;
        uint size;
        uint flags;
        uint height;
        uint width;
        uint pitch;
        uint depth;
        uint mipmapcount;
        uint reserved[11];
        DDSPixelFormat pf;
        DDSCaps caps;
        uint notused;
        DDSHeader10 header10;


        // Helper methods.
        DDSHeader();

        void setWidth(uint w);
        void setHeight(uint h);
        void setDepth(uint d);
        void setMipmapCount(uint count);
        void setTexture2D();
        void setTexture3D();
        void setTextureCube();
        void setLinearSize(uint size);
        void setPitch(uint pitch);
        void setFourCC(uint8 c0, uint8 c1, uint8 c2, uint8 c3);
        void setFormatCode(uint code);
        void setSwizzleCode(uint8 c0, uint8 c1, uint8 c2, uint8 c3);
        void setPixelFormat(uint bitcount, uint rmask, uint gmask, uint bmask, uint amask);
        void setDX10Format(uint format);
        void setNormalFlag(bool b);
        void setSrgbFlag(bool b);
        void setHasAlphaFlag(bool b);
        void setUserVersion(int version);

        void swapBytes();

        bool hasDX10Header() const;
        uint signature() const;
        uint toolVersion() const;
        uint userVersion() const;
        bool isNormalMap() const;
        bool isSrgb() const;
        bool hasAlpha() const;
        uint d3d9Format() const;
    };

    NVIMAGE_API Stream & operator<< (Stream & s, DDSHeader & header);


    /// DirectDraw Surface. (DDS)
    class NVIMAGE_CLASS DirectDrawSurface
    {
    public:
        DirectDrawSurface();
        DirectDrawSurface(const char * file);
        DirectDrawSurface(Stream * stream);
        ~DirectDrawSurface();

        bool load(const char * filename);
        bool load(Stream * stream);

        bool isValid() const;
        bool isSupported() const;

        bool hasAlpha() const;

        uint mipmapCount() const;
        uint width() const;
        uint height() const;
        uint depth() const;
        bool isTexture1D() const;
        bool isTexture2D() const;
        bool isTexture3D() const;
        bool isTextureCube() const;

        void setNormalFlag(bool b);
        void setHasAlphaFlag(bool b);
        void setUserVersion(int version);

        void mipmap(Image * img, uint f, uint m);
        //	void mipmap(FloatImage * img, uint f, uint m);
        void * readData(uint * sizePtr);

        void printInfo() const;

    private:

        uint blockSize() const;
        uint faceSize() const;
        uint mipmapSize(uint m) const;

        uint offset(uint f, uint m);

        void readLinearImage(Image * img);
        void readBlockImage(Image * img);
        void readBlock(ColorBlock * rgba);


    private:
        Stream * stream;
        DDSHeader header;
        DDSHeader10 header10;
    };

} // nv namespace

#endif // NV_IMAGE_DIRECTDRAWSURFACE_H
