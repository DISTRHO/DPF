/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
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

#include "../OpenGL.hpp"
#include "WidgetPrivateData.hpp"
#include "WindowPrivateData.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Line

template<typename T>
void Line<T>::draw()
{
    DISTRHO_SAFE_ASSERT_RETURN(fPosStart != fPosEnd,);

    glBegin(GL_LINES);

    {
        glVertex2d(fPosStart.fX, fPosStart.fY);
        glVertex2d(fPosEnd.fX, fPosEnd.fY);
    }

    glEnd();
}

// -----------------------------------------------------------------------
// Circle

template<typename T>
void Circle<T>::_draw(const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(fNumSegments >= 3 && fSize > 0.0f,);

    double t, x = fSize, y = 0.0;

    glBegin(outline ? GL_LINE_LOOP : GL_POLYGON);

    for (uint i=0; i<fNumSegments; ++i)
    {
        glVertex2d(x + fPos.fX, y + fPos.fY);

        t = x;
        x = fCos * x - fSin * y;
        y = fSin * t + fCos * y;
    }

    glEnd();
}

// -----------------------------------------------------------------------
// Triangle

template<typename T>
void Triangle<T>::_draw(const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(fPos1 != fPos2 && fPos1 != fPos3,);

    glBegin(outline ? GL_LINE_LOOP : GL_TRIANGLES);

    {
        glVertex2d(fPos1.fX, fPos1.fY);
        glVertex2d(fPos2.fX, fPos2.fY);
        glVertex2d(fPos3.fX, fPos3.fY);
    }

    glEnd();
}

// -----------------------------------------------------------------------
// Rectangle

template<typename T>
void Rectangle<T>::_draw(const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(fSize.isValid(),);

    glBegin(outline ? GL_LINE_LOOP : GL_QUADS);

    {
        glTexCoord2f(0.0f, 0.0f);
        glVertex2d(fPos.fX, fPos.fY);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2d(fPos.fX+fSize.fWidth, fPos.fY);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2d(fPos.fX+fSize.fWidth, fPos.fY+fSize.fHeight);

        glTexCoord2f(0.0f, 1.0f);
        glVertex2d(fPos.fX, fPos.fY+fSize.fHeight);
    }

    glEnd();
}

// -----------------------------------------------------------------------

void Widget::PrivateData::display(const uint width,
                                  const uint height,
                                  const double scaling,
                                  const bool renderingSubWidget)
{
    if (/*(skipDisplay && ! renderingSubWidget) ||*/ size.isInvalid() || ! visible)
        return;

//     bool needsDisableScissor = false;

    // reset color
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

//     if (needsFullViewport || (absolutePos.isZero() && size == Size<uint>(width, height)))
    {
        // full viewport size
        glViewport(0,
                    -(height * scaling - height),
                    width * scaling,
                    height * scaling);
    }
//     else if (needsScaling)
//     {
//         // limit viewport to widget bounds
//         glViewport(absolutePos.getX(),
//                     height - self->getHeight() - absolutePos.getY(),
//                     self->getWidth(),
//                     self->getHeight());
//     }
//     else
//     {
//         // only set viewport pos
//         glViewport(absolutePos.getX() * scaling,
//                     -std::round((height * scaling - height) + (absolutePos.getY() * scaling)),
//                     std::round(width * scaling),
//                     std::round(height * scaling));
//
//         // then cut the outer bounds
//         glScissor(absolutePos.getX() * scaling,
//                   height - std::round((self->getHeight() + absolutePos.getY()) * scaling),
//                   std::round(self->getWidth() * scaling),
//                   std::round(self->getHeight() * scaling));
//
//         glEnable(GL_SCISSOR_TEST);
//         needsDisableScissor = true;
//     }

    // display widget
    self->onDisplay();

//     if (needsDisableScissor)
//     {
//         glDisable(GL_SCISSOR_TEST);
//         needsDisableScissor = false;
//     }

    displaySubWidgets(width, height, scaling);
}

// -----------------------------------------------------------------------

const GraphicsContext& Window::PrivateData::getGraphicsContext() const noexcept
{
    return (const GraphicsContext&)graphicsContext;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
