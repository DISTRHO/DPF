 
cmake_minimum_required (VERSION 2.8)
project(dgl)

find_package(OpenGL)
find_package(Cairo)
find_package(X11)
#########GENERAL SOURCES###########
set(DGL_COMMON_SRC
    src/Application.cpp
    src/Color.cpp
    src/Geometry.cpp
    src/ImageBase.cpp
    src/Resources.cpp
    src/Window.cpp
    src/Widget.cpp
    src/WidgetPrivateData.cpp
    src/sofd/libsofd.c)

IF(${CMAKE_SYSTEM_NAME} STREQUAL "Haiku")
    set(DGL_COMMON_SRC ${DGL_COMMON_SRC}  src/pugl/pugl_haiku.cpp)
    set(DGL_COMPILE_DEFINITION ${DGL_COMPILE_DEFINITION} )
    set(DGL_LIBS ${DGL_LIBS})
ELSEIF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(DGL_COMMON_SRC ${DGL_COMMON_SRC}  src/pugl/pugl_osx.m)
    set(DGL_COMPILE_DEFINITION ${DGL_COMPILE_DEFINITION} )
    set(DGL_LIBS ${DGL_LIBS})
ELSEIF(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(DGL_COMMON_SRC ${DGL_COMMON_SRC}  src/pugl/pugl_win.cpp)
    set(DGL_COMPILE_DEFINITION ${DGL_COMPILE_DEFINITION} )
    set(DGL_LIBS ${DGL_LIBS})
ELSE()# As Fallback use Linux-X11
    set(DGL_COMMON_SRC ${DGL_COMMON_SRC}  src/pugl/pugl_x11.c)
    set(DGL_COMPILE_DEFINITION ${DGL_COMPILE_DEFINITION} )
    set(DGL_LIBS ${DGL_LIBS})
ENDIF(${CMAKE_SYSTEM_NAME} STREQUAL "Haiku")

#PID is not needed to define(as far as i think) maybe test it without.
set(DGL_COMPILE_DEFINITION ${DGL_COMPILE_DEFINITION} DONT_SET_USING_DGL_NAMESPACE PIC )
#######OPENGL DGL############
if(OpenGL_FOUND)
    message("OpenGL Found, Building libdgl-opengl")
    add_library(dgl-opengl ${DGL_COMMON_SRC}
        src/OpenGL.cpp
        src/Image.cpp
        src/ImageWidgets.cpp
        src/NanoVG.cpp)
    #HAVE_OPENGL is not needed to define(as far as i think) maybe test it without.
    target_compile_definitions(dgl-opengl PUBLIC DGL_OPENGL HAVE_OPENGL ${DGL_COMPILE_DEFINITION})
    target_include_directories(dgl-opengl PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src/ ${CMAKE_CURRENT_LIST_DIR}/)
    set_target_properties(dgl-opengl PROPERTIES POSITION_INDEPENDENT_CODE true)
    target_link_libraries(dgl-opengl PUBLIC ${OPENGL_LIBRARIES} ${X11_LIBRARIES})
endif(OpenGL_FOUND)

#######CAIRO DGL############
if(CAIRO_FOUND)
    message("Cairo Found, Building libdgl-cairo")
    add_library(dgl-cairo  ${DGL_COMMON_SRC}
        src/Cairo.cpp)
    #HAVE_CAIRO is not needed to define(as far as i think) maybe test it without.
    target_compile_definitions(dgl-cairo PUBLIC DGL_CAIRO HAVE_CAIRO ${DGL_COMPILE_DEFINITION})
    target_include_directories(dgl-cairo PUBLIC §{CMAKE_CURRENT_LIST_DIR}/src/  ${CMAKE_CURRENT_LIST_DIR}/)
    set_target_properties(dgl-cairo PROPERTIES POSITION_INDEPENDENT_CODE true)
    target_link_libraries(dgl-cairo PUBLIC ${CAIRO_LIBRARY})
endif(CAIRO_FOUND)
