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

#ifndef DGL_GEOMETRY_HPP_INCLUDED
#define DGL_GEOMETRY_HPP_INCLUDED

#include "Base.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

template<typename T>
class Point
{
public:
    Point() noexcept;
    Point(const T& x, const T& y) noexcept;
    Point(const Point<T>& pos) noexcept;

    const T& getX() const noexcept;
    const T& getY() const noexcept;

    void setX(const T& x) noexcept;
    void setY(const T& y) noexcept;
    void setPos(const T& x, const T& y) noexcept;
    void setPos(const Point<T>& pos) noexcept;

    void moveBy(const T& x, const T& y) noexcept;
    void moveBy(const Point<T>& pos) noexcept;

    Point<T>& operator=(const Point<T>& pos) noexcept;
    Point<T>& operator+=(const Point<T>& pos) noexcept;
    Point<T>& operator-=(const Point<T>& pos) noexcept;
    bool operator==(const Point<T>& pos) const noexcept;
    bool operator!=(const Point<T>& pos) const noexcept;

private:
    T fX, fY;
    template<typename> friend class Rectangle;

    DISTRHO_PREVENT_HEAP_ALLOCATION
};

// -----------------------------------------------------------------------

template<typename T>
class Size
{
public:
    Size() noexcept;
    Size(const T& width, const T& height) noexcept;
    Size(const Size<T>& size) noexcept;

    const T& getWidth() const noexcept;
    const T& getHeight() const noexcept;

    void setWidth(const T& width) noexcept;
    void setHeight(const T& height) noexcept;
    void setSize(const T& width, const T& height) noexcept;
    void setSize(const Size<T>& size) noexcept;

    void growBy(const T& multiplier) noexcept;
    void shrinkBy(const T& divider) noexcept;

    Size<T>& operator=(const Size<T>& size) noexcept;
    Size<T>& operator+=(const Size<T>& size) noexcept;
    Size<T>& operator-=(const Size<T>& size) noexcept;
    Size<T>& operator*=(const T& m) noexcept;
    Size<T>& operator/=(const T& d) noexcept;
    bool operator==(const Size<T>& size) const noexcept;
    bool operator!=(const Size<T>& size) const noexcept;

private:
    T fWidth, fHeight;
    template<typename> friend class Rectangle;

    DISTRHO_PREVENT_HEAP_ALLOCATION
};

// -----------------------------------------------------------------------

template<typename T>
class Rectangle
{
public:
    Rectangle() noexcept;
    Rectangle(const T& x, const T& y, const T& width, const T& height) noexcept;
    Rectangle(const T& x, const T& y, const Size<T>& size) noexcept;
    Rectangle(const Point<T>& pos, const T& width, const T& height) noexcept;
    Rectangle(const Point<T>& pos, const Size<T>& size) noexcept;
    Rectangle(const Rectangle<T>& rect) noexcept;

    const T& getX() const noexcept;
    const T& getY() const noexcept;
    const T& getWidth() const noexcept;
    const T& getHeight() const noexcept;

    const Point<T>& getPos() const noexcept;
    const Size<T>&  getSize() const noexcept;

    void setX(const T& x) noexcept;
    void setY(const T& y) noexcept;
    void setPos(const T& x, const T& y) noexcept;
    void setPos(const Point<T>& pos) noexcept;

    void moveBy(const T& x, const T& y) noexcept;
    void moveBy(const Point<T>& pos) noexcept;

    void setWidth(const T& width) noexcept;
    void setHeight(const T& height) noexcept;
    void setSize(const T& width, const T& height) noexcept;
    void setSize(const Size<T>& size) noexcept;

    void growBy(const T& multiplier) noexcept;
    void shrinkBy(const T& divider) noexcept;

    bool contains(const T& x, const T& y) const noexcept;
    bool contains(const Point<T>& pos) const noexcept;
    bool containsX(const T& x) const noexcept;
    bool containsY(const T& y) const noexcept;

    void draw();

    Rectangle<T>& operator=(const Rectangle<T>& rect) noexcept;
    Rectangle<T>& operator*=(const T& m) noexcept;
    Rectangle<T>& operator/=(const T& d) noexcept;
    bool operator==(const Rectangle<T>& size) const noexcept;
    bool operator!=(const Rectangle<T>& size) const noexcept;

private:
    Point<T> fPos;
    Size<T>  fSize;

    DISTRHO_PREVENT_HEAP_ALLOCATION
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_GEOMETRY_HPP_INCLUDED
