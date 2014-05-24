/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#include "../NanoWidget.hpp"

// -----------------------------------------------------------------------

#define NANOVG_GL2_IMPLEMENTATION
#include "nanovg/nanovg_gl.h"

#if defined(NANOVG_GL2)
# define nvgCreateGL nvgCreateGL2
# define nvgDeleteGL nvgDeleteGL2
#elif defined(NANOVG_GL3)
# define nvgCreateGL nvgCreateGL3
# define nvgDeleteGL nvgDeleteGL3
#elif defined(NANOVG_GLES2)
# define nvgCreateGL nvgCreateGLES2
# define nvgDeleteGL nvgDeleteGLES2
#elif defined(NANOVG_GLES3)
# define nvgCreateGL nvgCreateGLES3
# define nvgDeleteGL nvgDeleteGLES3
#endif

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Conversions

NanoWidget::Color::Color() noexcept
    : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}

NanoWidget::Color::Color(const NVGcolor& c) noexcept
    : r(c.r), g(c.g), b(c.b), a(c.a) {}

NanoWidget::Color::operator NVGcolor() const noexcept
{
    NVGcolor nc = { r, g, b, a };
    return nc;
}

NanoWidget::Paint::Paint() noexcept
    : radius(0.0f), feather(0.0f), innerColor(), outerColor(), imageId(0), repeat(REPEAT_NONE)
{
    std::memset(xform, 0, sizeof(float)*6);
    std::memset(extent, 0, sizeof(float)*2);
}

NanoWidget::Paint::Paint(const NVGpaint& p) noexcept
    : radius(p.radius), feather(p.feather), innerColor(p.innerColor), outerColor(p.outerColor), imageId(p.image), repeat(static_cast<PatternRepeat>(p.repeat))
{
    std::memcpy(xform, p.xform, sizeof(float)*6);
    std::memcpy(extent, p.extent, sizeof(float)*2);
}

NanoWidget::Paint::operator NVGpaint() const noexcept
{
    NVGpaint p;
    p.radius = radius;
    p.feather = feather;
    p.innerColor = innerColor;
    p.outerColor = outerColor;
    p.image = imageId;
    p.repeat = repeat;
    std::memcpy(p.xform, xform, sizeof(float)*6);
    std::memcpy(p.extent, extent, sizeof(float)*2);
    return p;
}

// -----------------------------------------------------------------------
// NanoImage

static NVGcontext* sLastContext = nullptr;

NanoImage::NanoImage(const char* filename)
    : fContext(sLastContext),
      fImageId((fContext != nullptr) ? nvgCreateImage(fContext, filename) : 0) {}

NanoImage::NanoImage(uchar* data, int ndata)
    : fContext(sLastContext),
      fImageId((fContext != nullptr) ? nvgCreateImageMem(fContext, data, ndata) : 0) {}

NanoImage::NanoImage(int w, int h, const uchar* data)
    : fContext(sLastContext),
      fImageId((fContext != nullptr) ? nvgCreateImageRGBA(fContext, w, h, data) : 0) {}

NanoImage::~NanoImage()
{
    if (fContext != nullptr && fImageId != 0)
        nvgDeleteImage(fContext, fImageId);
}

Size<int> NanoImage::getSize() const
{
    int w=0, h=0;

    if (fContext != nullptr && fImageId != 0)
        nvgImageSize(fContext, fImageId, &w, &h);

    return Size<int>(w, h);
}

void NanoImage::updateImage(const uchar* data)
{
    if (fContext != nullptr && fImageId != 0)
        nvgUpdateImage(fContext, fImageId, data);
}

// -----------------------------------------------------------------------
// NanoWidget

NanoWidget::NanoWidget(Window& parent)
    : Widget(parent),
      fContext(nvgCreateGL(512, 512, NVG_ANTIALIAS))
{
    DISTRHO_SAFE_ASSERT_RETURN(fContext != nullptr,);
}

NanoWidget::~NanoWidget()
{
    if (fContext == nullptr)
        return;

    nvgDeleteGL(fContext);
}

// -----------------------------------------------------------------------

void NanoWidget::beginFrame(Alpha alpha)
{
    nvgBeginFrame(fContext, getWidth(), getHeight(), 1.0f, static_cast<NVGalpha>(alpha));
}

void NanoWidget::endFrame()
{
    nvgEndFrame(fContext);
}

// -----------------------------------------------------------------------
// Color utils

NanoWidget::Color NanoWidget::RGB(uchar r, uchar g, uchar b)
{
    return nvgRGB(r, g, b);
}

NanoWidget::Color NanoWidget::RGBf(float r, float g, float b)
{
    return nvgRGBf(r, g, b);
}

NanoWidget::Color NanoWidget::RGBA(uchar r, uchar g, uchar b, uchar a)
{
    return nvgRGBA(r, g, b, a);
}

NanoWidget::Color NanoWidget::RGBAf(float r, float g, float b, float a)
{
    return nvgRGBAf(r, g, b, a);
}

NanoWidget::Color NanoWidget::lerpRGBA(const Color& c0, const Color& c1, float u)
{
    return nvgLerpRGBA(c0, c1, u);
}

NanoWidget::Color NanoWidget::HSL(float h, float s, float l)
{
    return nvgHSL(h, s, l);
}

NanoWidget::Color NanoWidget::HSLA(float h, float s, float l, uchar a)
{
    return nvgHSLA(h, s, l, a);
}

// -----------------------------------------------------------------------
// State Handling

void NanoWidget::save()
{
    nvgSave(fContext);
}

void NanoWidget::restore()
{
    nvgRestore(fContext);
}

void NanoWidget::reset()
{
    nvgReset(fContext);
}

// -----------------------------------------------------------------------
// Render styles

void NanoWidget::strokeColor(const Color& color)
{
    nvgStrokeColor(fContext, color);
}

void NanoWidget::strokePaint(const Paint& paint)
{
    nvgStrokePaint(fContext, paint);
}

void NanoWidget::fillColor(const Color& color)
{
    nvgFillColor(fContext, color);
}

void NanoWidget::fillPaint(const Paint& paint)
{
    nvgFillPaint(fContext, paint);
}

void NanoWidget::miterLimit(float limit)
{
    nvgMiterLimit(fContext, limit);
}

void NanoWidget::strokeWidth(float size)
{
    nvgStrokeWidth(fContext, size);
}

void NanoWidget::lineCap(NanoWidget::LineCap cap)
{
    nvgLineCap(fContext, cap);
}

void NanoWidget::lineJoin(NanoWidget::LineCap join)
{
    nvgLineJoin(fContext, join);
}

// -----------------------------------------------------------------------
// Transforms

void NanoWidget::resetTransform()
{
    nvgResetTransform(fContext);
}

void NanoWidget::transform(float a, float b, float c, float d, float e, float f)
{
    nvgTransform(fContext, a, b, c, d, e, f);
}

void NanoWidget::translate(float x, float y)
{
    nvgTranslate(fContext, x, y);
}

void NanoWidget::rotate(float angle)
{
    nvgRotate(fContext, angle);
}

void NanoWidget::skewX(float angle)
{
    nvgSkewX(fContext, angle);
}

void NanoWidget::skewY(float angle)
{
    nvgSkewY(fContext, angle);
}

void NanoWidget::scale(float x, float y)
{
    nvgScale(fContext, x, y);
}

void NanoWidget::currentTransform(float xform[6])
{
    nvgCurrentTransform(fContext, xform);
}

void NanoWidget::transformIdentity(float dst[6])
{
    nvgTransformIdentity(dst);
}

void NanoWidget::transformTranslate(float dst[6], float tx, float ty)
{
    nvgTransformTranslate(dst, tx, ty);
}

void NanoWidget::transformScale(float dst[6], float sx, float sy)
{
    nvgTransformScale(dst, sx, sy);
}

void NanoWidget::transformRotate(float dst[6], float a)
{
    nvgTransformRotate(dst, a);
}

void NanoWidget::transformSkewX(float dst[6], float a)
{
    nvgTransformSkewX(dst, a);
}

void NanoWidget::transformSkewY(float dst[6], float a)
{
    nvgTransformSkewY(dst, a);
}

void NanoWidget::transformMultiply(float dst[6], const float src[6])
{
    nvgTransformMultiply(dst, src);
}

void NanoWidget::transformPremultiply(float dst[6], const float src[6])
{
    nvgTransformPremultiply(dst, src);
}

int NanoWidget::transformInverse(float dst[6], const float src[6])
{
    return nvgTransformInverse(dst, src);
}

void NanoWidget::transformPoint(float& dstx, float& dsty, const float xform[6], float srcx, float srcy)
{
    nvgTransformPoint(&dstx, &dsty, xform, srcx, srcy);
}

float NanoWidget::degToRad(float deg)
{
    return nvgDegToRad(deg);
}

float NanoWidget::radToDeg(float rad)
{
    return nvgRadToDeg(rad);
}

// -----------------------------------------------------------------------
// Images

NanoImage NanoWidget::createImage(const char* filename)
{
    sLastContext = fContext;
    return NanoImage(filename);
}

NanoImage NanoWidget::createImageMem(uchar* data, int ndata)
{
    sLastContext = fContext;
    return NanoImage(data, ndata);
}

NanoImage NanoWidget::createImageRGBA(int w, int h, const uchar* data)
{
    sLastContext = fContext;
    return NanoImage(w, h, data);
}

// -----------------------------------------------------------------------
// Paints

NanoWidget::Paint NanoWidget::linearGradient(float sx, float sy, float ex, float ey, const NanoWidget::Color& icol, const NanoWidget::Color& ocol)
{
    return nvgLinearGradient(fContext, sx, sy, ex, ey, icol, ocol);
}

NanoWidget::Paint NanoWidget::boxGradient(float x, float y, float w, float h, float r, float f, const NanoWidget::Color& icol, const NanoWidget::Color& ocol)
{
    return nvgBoxGradient(fContext, x, y, w, h, r, f, icol, ocol);
}

NanoWidget::Paint NanoWidget::radialGradient(float cx, float cy, float inr, float outr, const NanoWidget::Color& icol, const NanoWidget::Color& ocol)
{
    return nvgRadialGradient(fContext, cx, cy, inr, outr, icol, ocol);
}

NanoWidget::Paint NanoWidget::imagePattern(float ox, float oy, float ex, float ey, float angle, const NanoImage& image, NanoWidget::PatternRepeat repeat)
{
    return nvgImagePattern(fContext, ox, oy, ex, ey, angle, image.fImageId, repeat);
}

// -----------------------------------------------------------------------
// Scissoring

void NanoWidget::scissor(float x, float y, float w, float h)
{
    nvgScissor(fContext, x, y, w, h);
}

void NanoWidget::resetScissor()
{
    nvgResetScissor(fContext);
}

// -----------------------------------------------------------------------
// Paths

void NanoWidget::beginPath()
{
    nvgBeginPath(fContext);
}

void NanoWidget::moveTo(float x, float y)
{
    nvgMoveTo(fContext, x, y);
}

void NanoWidget::lineTo(float x, float y)
{
    nvgLineTo(fContext, x, y);
}

void NanoWidget::bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y)
{
    nvgBezierTo(fContext, c1x, c1y, c2x, c2y, x, y);
}

void NanoWidget::arcTo(float x1, float y1, float x2, float y2, float radius)
{
    nvgArcTo(fContext, x1, y1, x2, y2, radius);
}

void NanoWidget::closePath()
{
    nvgClosePath(fContext);
}

void NanoWidget::pathWinding(NanoWidget::Winding dir)
{
    nvgPathWinding(fContext, dir);
}

void NanoWidget::arc(float cx, float cy, float r, float a0, float a1, NanoWidget::Winding dir)
{
    nvgArc(fContext, cx, cy, r, a0, a1, dir);
}

void NanoWidget::rect(float x, float y, float w, float h)
{
    nvgRect(fContext, x, y, w, h);
}

void NanoWidget::roundedRect(float x, float y, float w, float h, float r)
{
    nvgRoundedRect(fContext, x, y, w, h, r);
}

void NanoWidget::ellipse(float cx, float cy, float rx, float ry)
{
    nvgEllipse(fContext, cx, cy, rx, ry);
}

void NanoWidget::circle(float cx, float cy, float r)
{
    nvgCircle(fContext, cx, cy, r);
}

void NanoWidget::fill()
{
    nvgFill(fContext);
}

void NanoWidget::stroke()
{
    nvgStroke(fContext);
}

// -----------------------------------------------------------------------
// Text

NanoWidget::FontId NanoWidget::createFont(const char* name, const char* filename)
{
    return nvgCreateFont(fContext, name, filename);
}

NanoWidget::FontId NanoWidget::createFontMem(const char* name, uchar* data, int ndata, bool freeData)
{
    return nvgCreateFontMem(fContext, name, data, ndata, freeData);
}

NanoWidget::FontId NanoWidget::findFont(const char* name)
{
    return nvgFindFont(fContext, name);
}

void NanoWidget::fontSize(float size)
{
    nvgFontSize(fContext, size);
}

void NanoWidget::fontBlur(float blur)
{
    nvgFontBlur(fContext, blur);
}

void NanoWidget::textLetterSpacing(float spacing)
{
    nvgTextLetterSpacing(fContext, spacing);
}

void NanoWidget::textLineHeight(float lineHeight)
{
    nvgTextLineHeight(fContext, lineHeight);
}

void NanoWidget::textAlign(NanoWidget::Align align)
{
    nvgTextAlign(fContext, align);
}

void NanoWidget::fontFaceId(FontId font)
{
    nvgFontFaceId(fContext, font);
}

void NanoWidget::fontFace(const char* font)
{
    nvgFontFace(fContext, font);
}

float NanoWidget::text(float x, float y, const char* string, const char* end)
{
    return nvgText(fContext, x, y, string, end);
}

void NanoWidget::textBox(float x, float y, float breakRowWidth, const char* string, const char* end)
{
    nvgTextBox(fContext, x, y, breakRowWidth, string, end);
}

float NanoWidget::textBounds(float x, float y, const char* string, const char* end, float* bounds)
{
    return nvgTextBounds(fContext, x, y, string, end, bounds);
}

void NanoWidget::textBoxBounds(float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds)
{
    nvgTextBoxBounds(fContext, x, y, breakRowWidth, string, end, bounds);
}

int NanoWidget::textGlyphPositions(float x, float y, const char* string, const char* end, NanoWidget::GlyphPosition* positions, int maxPositions)
{
    return nvgTextGlyphPositions(fContext, x, y, string, end, (NVGglyphPosition*)positions, maxPositions);
}

void NanoWidget::textMetrics(float* ascender, float* descender, float* lineh)
{
    nvgTextMetrics(fContext, ascender, descender, lineh);
}

int NanoWidget::textBreakLines(const char* string, const char* end, float breakRowWidth, NanoWidget::TextRow* rows, int maxRows)
{
    return nvgTextBreakLines(fContext, string, end, breakRowWidth, (NVGtextRow*)rows, maxRows);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

extern "C" {
#include "nanovg/nanovg.c"
}

// -----------------------------------------------------------------------
