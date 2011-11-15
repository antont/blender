# turn everything OFF except for python which defaults to ON
# and is needed for the UI
#
# Example usage:
#   cmake -C../blender/build_files/cmake/config/blender_lite.cmake  ../blender
#

set(WITH_INSTALL_PORTABLE    ON  CACHE FORCE BOOL)

set(WITH_BUILDINFO           OFF CACHE FORCE BOOL)
set(WITH_BUILTIN_GLEW        OFF CACHE FORCE BOOL)
set(WITH_BULLET              OFF CACHE FORCE BOOL)
set(WITH_CODEC_FFMPEG        OFF CACHE FORCE BOOL)
set(WITH_CODEC_SNDFILE       OFF CACHE FORCE BOOL)
set(WITH_CYCLES              OFF CACHE FORCE BOOL)
set(WITH_FFTW3               OFF CACHE FORCE BOOL)
set(WITH_LIBMV               OFF CACHE FORCE BOOL)
set(WITH_GAMEENGINE          OFF CACHE FORCE BOOL)
set(WITH_IK_ITASC            OFF CACHE FORCE BOOL)
set(WITH_IMAGE_CINEON        OFF CACHE FORCE BOOL)
set(WITH_IMAGE_DDS           OFF CACHE FORCE BOOL)
set(WITH_IMAGE_FRAMESERVER   OFF CACHE FORCE BOOL)
set(WITH_IMAGE_HDR           OFF CACHE FORCE BOOL)
set(WITH_IMAGE_OPENEXR       OFF CACHE FORCE BOOL)
set(WITH_IMAGE_OPENJPEG      OFF CACHE FORCE BOOL)
set(WITH_IMAGE_REDCODE       OFF CACHE FORCE BOOL)
set(WITH_IMAGE_TIFF          OFF CACHE FORCE BOOL)
set(WITH_INPUT_NDOF          OFF CACHE FORCE BOOL)
set(WITH_INTERNATIONAL       OFF CACHE FORCE BOOL)
set(WITH_JACK                OFF CACHE FORCE BOOL)
set(WITH_LZMA                OFF CACHE FORCE BOOL)
set(WITH_LZO                 OFF CACHE FORCE BOOL)
set(WITH_MOD_BOOLEAN         OFF CACHE FORCE BOOL)
set(WITH_MOD_DECIMATE        OFF CACHE FORCE BOOL)
set(WITH_MOD_FLUID           OFF CACHE FORCE BOOL)
set(WITH_MOD_SMOKE           OFF CACHE FORCE BOOL)
set(WITH_MOD_OCEANSIM        OFF CACHE FORCE BOOL)
set(WITH_AUDASPACE           OFF CACHE FORCE BOOL)
set(WITH_OPENAL              OFF CACHE FORCE BOOL)
set(WITH_OPENCOLLADA         OFF CACHE FORCE BOOL)
set(WITH_OPENMP              OFF CACHE FORCE BOOL)
set(WITH_PYTHON_INSTALL      OFF CACHE FORCE BOOL)
set(WITH_RAYOPTIMIZATION     OFF CACHE FORCE BOOL)
set(WITH_SDL                 OFF CACHE FORCE BOOL)
set(WITH_X11_XINPUT          OFF CACHE FORCE BOOL)
