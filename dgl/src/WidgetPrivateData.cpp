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

#include "WidgetPrivateData.hpp"

#ifdef DGL_CAIRO
# include "../Cairo.hpp"
#endif
#ifdef DGL_OPENGL
# include "../OpenGL.hpp"
#endif

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

void Widget::PrivateData::display(const uint width,
                                  const uint height,
                                  const double scaling,
                                  const bool renderingSubWidget)
{
    if ((skipDisplay && ! renderingSubWidget) || size.isInvalid() || ! visible)
        return;

#ifdef DGL_OPENGL
    bool needsDisableScissor = false;

    // reset color
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    if (needsFullViewport || (absolutePos.isZero() && size == Size<uint>(width, height)))
    {
        // full viewport size
        glViewport(0,
                    -(height * scaling - height),
                    width * scaling,
                    height * scaling);
    }
    else if (needsScaling)
    {
        // limit viewport to widget bounds
        glViewport(absolutePos.getX(),
                    height - self->getHeight() - absolutePos.getY(),
                    self->getWidth(),
                    self->getHeight());
    }
    else
    {
        // only set viewport pos
        glViewport(absolutePos.getX() * scaling,
                    -std::round((height * scaling - height) + (absolutePos.getY() * scaling)),
                    std::round(width * scaling),
                    std::round(height * scaling));

        // then cut the outer bounds
        glScissor(absolutePos.getX() * scaling,
                  height - std::round((self->getHeight() + absolutePos.getY()) * scaling),
                  std::round(self->getWidth() * scaling),
                  std::round(self->getHeight() * scaling));

        glEnable(GL_SCISSOR_TEST);
        needsDisableScissor = true;
    }
#endif

#ifdef DGL_CAIRO
    cairo_t* cr = parent.getGraphicsContext().cairo;
    cairo_matrix_t matrix;
    cairo_get_matrix(cr, &matrix);
    cairo_translate(cr, absolutePos.getX(), absolutePos.getY());
    // TODO: scaling and cropping
#endif

    // display widget
    self->onDisplay();

#ifdef DGL_CAIRO
    cairo_set_matrix(cr, &matrix);
#endif

#ifdef DGL_OPENGL
    if (needsDisableScissor)
    {
        glDisable(GL_SCISSOR_TEST);
        needsDisableScissor = false;
    }
#endif

    displaySubWidgets(width, height, scaling);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
