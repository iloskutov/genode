QT_DEFINES += -DQT_NO_USING_NAMESPACE -DQT_NO_FOREACH -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT -DQT_BUILD_OPENGL_LIB -DQT_BUILDING_QT -DQT_NO_CAST_TO_ASCII -DQT_ASCII_CAST_WARNINGS -DQT_MOC_COMPAT -DQT_USE_QSTRINGBUILDER -DQT_DEPRECATED_WARNINGS -DQT_DISABLE_DEPRECATED_BEFORE=0x050000 -DQT_NO_EXCEPTIONS -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE -DQT_NO_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB

QT_INCPATH += \
  qtbase/src/opengl \
  qtbase/include \
  qtbase/include/QtOpenGL \
  qtbase/include/QtOpenGL/5.13.0 \
  qtbase/include/QtOpenGL/5.13.0/QtOpenGL \
  qtbase/include/QtWidgets/5.13.0 \
  qtbase/include/QtWidgets/5.13.0/QtWidgets \
  qtbase/include/QtGui/5.13.0 \
  qtbase/include/QtGui/5.13.0/QtGui \
  qtbase/include/QtCore/5.13.0 \
  qtbase/include/QtCore/5.13.0/QtCore \
  qtbase/include/QtWidgets \
  qtbase/include/QtGui \
  qtbase/include/QtCore \
  qtbase/mkspecs/genode-g++

QT_SOURCES += \
  qgl.cpp \
  qglcolormap.cpp \
  qglfunctions.cpp \
  qglpixelbuffer.cpp \
  qglframebufferobject.cpp \
  qglpaintdevice.cpp \
  qglbuffer.cpp \
  qglshaderprogram.cpp \
  qglgradientcache.cpp \
  qglengineshadermanager.cpp \
  qgl2pexvertexarray.cpp \
  qpaintengineex_opengl2.cpp \
  qglcustomshaderstage.cpp \
  qtextureglyphcache_gl.cpp \
  qgraphicsshadereffect.cpp \
  moc_qgl.cpp \
  moc_qgl_p.cpp \
  moc_qglshaderprogram.cpp \
  moc_qglengineshadermanager_p.cpp \
  moc_qgraphicsshadereffect_p.cpp

QT_VPATH += \
  qtbase/src/opengl \
  qtbase/src/opengl/gl2paintengineex \

# some source files need to be generated by moc from other source/header files before
# they get #included again by the original source file in the compiling stage

# source files generated from existing header files ('moc_%.cpp: %.h' rule in import-qt5.inc)
# extracted from 'compiler_moc_header_make_all' target

COMPILER_MOC_HEADER_MAKE_ALL_FILES = \
  moc_qgl.cpp \
  moc_qgl_p.cpp \
  moc_qglshaderprogram.cpp \
  moc_qglengineshadermanager_p.cpp \
  moc_qgraphicsshadereffect_p.cpp

# source files generated from existing source files ('%.moc: %.cpp' rule in import-qt5.inc)
# extracted from 'compiler_moc_source_make_all' rule

COMPILER_MOC_SOURCE_MAKE_ALL_FILES = \
 

