# qmake project file for app bundle test

TARGET = nekobi-ui

# -------------------------------------------------------

CONFIG   = app_bundle warn_on
TEMPLATE = app
VERSION  = 1.0.0

# -------------------------------------------------------

INCLUDEPATH = ../../dgl
LIBS        = ../../libdgl.a -framework OpenGL -framework Cocoa
SOURCES     = ../nekobi-ui.cpp

# -------------------------------------------------------
