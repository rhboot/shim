DEPENDS = "python-numpy v4l-utils libav libtool swig swig-native python jpeg bzip2 zlib libpng tiff glib-2.0"

EXTRA_OECMAKE = "-DPYTHON_NUMPY_INCLUDE_DIR:PATH=${STAGING_LIBDIR}/${PYTHON_DIR}/site-packages/numpy/core/include \
                 -DBUILD_PYTHON_SUPPORT=ON \
                 -DWITH_FFMPEG=ON \
                 -DWITH_GSTREAMER=OFF \
                 -DWITH_V4L=ON \
                 -DCMAKE_SKIP_RPATH=ON \
                 ${@bb.utils.contains("TARGET_CC_ARCH", "-msse3", "-DENABLE_SSE=1 -DENABLE_SSE2=1 -DENABLE_SSE3=1 -DENABLE_SSSE3=1", "", d)} \
"

