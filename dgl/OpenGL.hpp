/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2019 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DGL_OPENGL_HPP_INCLUDED
#define DGL_OPENGL_HPP_INCLUDED

#include "ImageBase.hpp"

// -----------------------------------------------------------------------
// Fix OpenGL includes for Windows, based on glfw code (part 1)

#undef DGL_CALLBACK_DEFINED
#undef DGL_WINGDIAPI_DEFINED

#ifdef DISTRHO_OS_WINDOWS

#ifndef APIENTRY
# define APIENTRY __stdcall
#endif // APIENTRY

/* We need WINGDIAPI defined */
#ifndef WINGDIAPI
# if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__POCC__)
#  define WINGDIAPI __declspec(dllimport)
# elif defined(__LCC__)
#  define WINGDIAPI __stdcall
# else
#  define WINGDIAPI extern
# endif
# define DGL_WINGDIAPI_DEFINED
#endif // WINGDIAPI

/* Some <GL/glu.h> files also need CALLBACK defined */
#ifndef CALLBACK
# if defined(_MSC_VER)
#  if (defined(_M_MRX000) || defined(_M_IX86) || defined(_M_ALPHA) || defined(_M_PPC)) && !defined(MIDL_PASS)
#   define CALLBACK __stdcall
#  else
#   define CALLBACK
#  endif
# else
#  define CALLBACK __stdcall
# endif
# define DGL_CALLBACK_DEFINED
#endif // CALLBACK

/* Most GL/glu.h variants on Windows need wchar_t */
#include <cstddef>

#endif // DISTRHO_OS_WINDOWS

// -----------------------------------------------------------------------
// OpenGL includes

#ifdef DISTRHO_OS_MAC
# include <OpenGL/gl.h>
#else
# ifndef DISTRHO_OS_WINDOWS
#  define GL_GLEXT_PROTOTYPES
# endif
# include <GL/gl.h>
# include <GL/glext.h>
#endif

// -----------------------------------------------------------------------
// Missing OpenGL defines

#if defined(GL_BGR_EXT) && !defined(GL_BGR)
# define GL_BGR GL_BGR_EXT
#endif

#if defined(GL_BGRA_EXT) && !defined(GL_BGRA)
# define GL_BGRA GL_BGRA_EXT
#endif

#ifndef GL_CLAMP_TO_BORDER
# define GL_CLAMP_TO_BORDER 0x812D
#endif

// -----------------------------------------------------------------------
// Fix OpenGL includes for Windows, based on glfw code (part 2)

#ifdef DGL_CALLBACK_DEFINED
# undef CALLBACK
# undef DGL_CALLBACK_DEFINED
#endif

#ifdef DGL_WINGDIAPI_DEFINED
# undef WINGDIAPI
# undef DGL_WINGDIAPI_DEFINED
#endif

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

/**
   OpenGL Graphics context.
 */
struct OpenGLGraphicsContext : GraphicsContext
{
};

// -----------------------------------------------------------------------

/**
   OpenGL Image class.

   This is an Image class that handles raw image data in pixels.
   You can init the image data on the contructor or later on by calling loadFromMemory().

   To generate raw data useful for this class see the utils/png2rgba.py script.
   Be careful when using a PNG without alpha channel, for those the format is 'GL_BGR'
   instead of the default 'GL_BGRA'.

   Images are drawn on screen via 2D textures.
 */
class OpenGLImage : public ImageBase
{
public:
   /**
      Constructor for a null Image.
    */
    OpenGLImage();

   /**
      Constructor using raw image data.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    OpenGLImage(const char* const rawData,
                const uint width,
                const uint height,
                const GLenum format = GL_BGRA,
                const GLenum type = GL_UNSIGNED_BYTE);

   /**
      Constructor using raw image data.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    OpenGLImage(const char* const rawData,
                const Size<uint>& size,
                const GLenum format = GL_BGRA,
                const GLenum type = GL_UNSIGNED_BYTE);

   /**
      Constructor using another image data.
    */
    OpenGLImage(const OpenGLImage& image);

   /**
      Destructor.
    */
    ~OpenGLImage() override;

   /**
      Load image data from memory.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    void loadFromMemory(const char* const rawData,
                        const uint width,
                        const uint height,
                        const GLenum format = GL_BGRA,
                        const GLenum type = GL_UNSIGNED_BYTE) noexcept;

   /**
      Load image data from memory.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    void loadFromMemory(const char* const rawData,
                        const Size<uint>& size,
                        const GLenum format = GL_BGRA,
                        const GLenum type = GL_UNSIGNED_BYTE) noexcept;

   /**
      TODO document this.
    */
    void setup();

   /**
      TODO document this.
    */
    void cleanup();

   /**
      Get the image format.
    */
    GLenum getFormat() const noexcept;

   /**
      Get the image type.
    */
    GLenum getType() const noexcept;

   /**
      Draw this image at position @a pos using the graphics context @a context.
    */
    void drawAt(const GraphicsContext& context, const Point<int>& pos) override;

   /**
      TODO document this.
    */
    OpenGLImage& operator=(const OpenGLImage& image) noexcept;

   /**
      Draw this image at (0, 0) point using the current OpenGL context.
    */
    // TODO mark as deprecated
    void draw();

   /**
      Draw this image at (x, y) point using the current OpenGL context.
    */
    // TODO mark as deprecated
    void drawAt(const int x, const int y);

   /**
      Draw this image at position @a pos using the current OpenGL context.
    */
    // TODO mark as deprecated
    void drawAt(const Point<int>& pos);

protected:
   /** @internal */
//     void _drawAt(const Point<int>& pos) override;

private:
    GLenum fFormat;
    GLenum fType;
    GLuint fTextureId;
    bool setupCalled;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif
