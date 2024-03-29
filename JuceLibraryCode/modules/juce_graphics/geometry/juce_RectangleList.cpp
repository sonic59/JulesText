/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-11 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

BEGIN_JUCE_NAMESPACE

//==============================================================================
RectangleList::RectangleList() noexcept
{
}

RectangleList::RectangleList (const Rectangle<int>& rect)
{
    if (! rect.isEmpty())
        rects.add (rect);
}

RectangleList::RectangleList (const RectangleList& other)
    : rects (other.rects)
{
}

RectangleList& RectangleList::operator= (const RectangleList& other)
{
    rects = other.rects;
    return *this;
}

#if JUCE_COMPILER_SUPPORTS_MOVE_SEMANTICS
RectangleList::RectangleList (RectangleList&& other) noexcept
    : rects (static_cast <Array <Rectangle<int> >&&> (other.rects))
{
}

RectangleList& RectangleList::operator= (RectangleList&& other) noexcept
{
    rects = static_cast <Array <Rectangle<int> >&&> (other.rects);
    return *this;
}
#endif

RectangleList::~RectangleList()
{
}

//==============================================================================
void RectangleList::clear()
{
    rects.clearQuick();
}

Rectangle<int> RectangleList::getRectangle (const int index) const noexcept
{
    if (isPositiveAndBelow (index, rects.size()))
        return rects.getReference (index);

    return Rectangle<int>();
}

bool RectangleList::isEmpty() const noexcept
{
    return rects.size() == 0;
}

//==============================================================================
RectangleList::Iterator::Iterator (const RectangleList& list) noexcept
    : current (nullptr),
      owner (list),
      index (list.rects.size())
{
}

RectangleList::Iterator::~Iterator()
{
}

bool RectangleList::Iterator::next() noexcept
{
    if (--index >= 0)
    {
        current = & (owner.rects.getReference (index));
        return true;
    }

    return false;
}


//==============================================================================
void RectangleList::add (const Rectangle<int>& rect)
{
    if (! rect.isEmpty())
    {
        if (rects.size() == 0)
        {
            rects.add (rect);
        }
        else
        {
            bool anyOverlaps = false;

            int i;
            for (i = rects.size(); --i >= 0;)
            {
                Rectangle<int>& ourRect = rects.getReference (i);

                if (rect.intersects (ourRect))
                {
                    if (rect.contains (ourRect))
                        rects.remove (i);
                    else if (! ourRect.reduceIfPartlyContainedIn (rect))
                        anyOverlaps = true;
                }
            }

            if (anyOverlaps && rects.size() > 0)
            {
                RectangleList r (rect);

                for (i = rects.size(); --i >= 0;)
                {
                    const Rectangle<int>& ourRect = rects.getReference (i);

                    if (rect.intersects (ourRect))
                    {
                        r.subtract (ourRect);

                        if (r.rects.size() == 0)
                            return;
                    }
                }

                for (i = r.getNumRectangles(); --i >= 0;)
                    rects.add (r.rects.getReference (i));
            }
            else
            {
                rects.add (rect);
            }
        }
    }
}

void RectangleList::addWithoutMerging (const Rectangle<int>& rect)
{
    if (! rect.isEmpty())
        rects.add (rect);
}

void RectangleList::add (const int x, const int y, const int w, const int h)
{
    if (rects.size() == 0)
    {
        if (w > 0 && h > 0)
            rects.add (Rectangle<int> (x, y, w, h));
    }
    else
    {
        add (Rectangle<int> (x, y, w, h));
    }
}

void RectangleList::add (const RectangleList& other)
{
    for (int i = 0; i < other.rects.size(); ++i)
        add (other.rects.getReference (i));
}

void RectangleList::subtract (const Rectangle<int>& rect)
{
    const int originalNumRects = rects.size();

    if (originalNumRects > 0)
    {
        const int x1 = rect.x;
        const int y1 = rect.y;
        const int x2 = x1 + rect.w;
        const int y2 = y1 + rect.h;

        for (int i = getNumRectangles(); --i >= 0;)
        {
            Rectangle<int>& r = rects.getReference (i);

            const int rx1 = r.x;
            const int ry1 = r.y;
            const int rx2 = rx1 + r.w;
            const int ry2 = ry1 + r.h;

            if (! (x2 <= rx1 || x1 >= rx2 || y2 <= ry1 || y1 >= ry2))
            {
                if (x1 > rx1 && x1 < rx2)
                {
                    if (y1 <= ry1 && y2 >= ry2 && x2 >= rx2)
                    {
                        r.w = x1 - rx1;
                    }
                    else
                    {
                        r.x = x1;
                        r.w = rx2 - x1;

                        rects.insert (i + 1, Rectangle<int> (rx1, ry1, x1 - rx1,  ry2 - ry1));
                        i += 2;
                    }
                }
                else if (x2 > rx1 && x2 < rx2)
                {
                    r.x = x2;
                    r.w = rx2 - x2;

                    if (y1 > ry1 || y2 < ry2 || x1 > rx1)
                    {
                        rects.insert (i + 1, Rectangle<int> (rx1, ry1, x2 - rx1,  ry2 - ry1));
                        i += 2;
                    }
                }
                else if (y1 > ry1 && y1 < ry2)
                {
                    if (x1 <= rx1 && x2 >= rx2 && y2 >= ry2)
                    {
                        r.h = y1 - ry1;
                    }
                    else
                    {
                        r.y = y1;
                        r.h = ry2 - y1;

                        rects.insert (i + 1, Rectangle<int> (rx1, ry1, rx2 - rx1, y1 - ry1));
                        i += 2;
                    }
                }
                else if (y2 > ry1 && y2 < ry2)
                {
                    r.y = y2;
                    r.h = ry2 - y2;

                    if (x1 > rx1 || x2 < rx2 || y1 > ry1)
                    {
                        rects.insert (i + 1, Rectangle<int> (rx1, ry1, rx2 - rx1, y2 - ry1));
                        i += 2;
                    }
                }
                else
                {
                    rects.remove (i);
                }
            }
        }
    }
}

bool RectangleList::subtract (const RectangleList& otherList)
{
    for (int i = otherList.rects.size(); --i >= 0 && rects.size() > 0;)
        subtract (otherList.rects.getReference (i));

    return rects.size() > 0;
}

bool RectangleList::clipTo (const Rectangle<int>& rect)
{
    bool notEmpty = false;

    if (rect.isEmpty())
    {
        clear();
    }
    else
    {
        for (int i = rects.size(); --i >= 0;)
        {
            Rectangle<int>& r = rects.getReference (i);

            if (! rect.intersectRectangle (r.x, r.y, r.w, r.h))
                rects.remove (i);
            else
                notEmpty = true;
        }
    }

    return notEmpty;
}

bool RectangleList::clipTo (const RectangleList& other)
{
    if (rects.size() == 0)
        return false;

    RectangleList result;

    for (int j = 0; j < rects.size(); ++j)
    {
        const Rectangle<int>& rect = rects.getReference (j);

        for (int i = other.rects.size(); --i >= 0;)
        {
            Rectangle<int> r (other.rects.getReference (i));

            if (rect.intersectRectangle (r.x, r.y, r.w, r.h))
                result.rects.add (r);
        }
    }

    swapWith (result);

    return ! isEmpty();
}

bool RectangleList::getIntersectionWith (const Rectangle<int>& rect, RectangleList& destRegion) const
{
    destRegion.clear();

    if (! rect.isEmpty())
    {
        for (int i = rects.size(); --i >= 0;)
        {
            Rectangle<int> r (rects.getReference (i));

            if (rect.intersectRectangle (r.x, r.y, r.w, r.h))
                destRegion.rects.add (r);
        }
    }

    return destRegion.rects.size() > 0;
}

void RectangleList::swapWith (RectangleList& otherList) noexcept
{
    rects.swapWithArray (otherList.rects);
}


//==============================================================================
void RectangleList::consolidate()
{
    int i;
    for (i = 0; i < getNumRectangles() - 1; ++i)
    {
        Rectangle<int>& r = rects.getReference (i);
        const int rx1 = r.x;
        const int ry1 = r.y;
        const int rx2 = rx1 + r.w;
        const int ry2 = ry1 + r.h;

        for (int j = rects.size(); --j > i;)
        {
            Rectangle<int>& r2 = rects.getReference (j);
            const int jrx1 = r2.x;
            const int jry1 = r2.y;
            const int jrx2 = jrx1 + r2.w;
            const int jry2 = jry1 + r2.h;

            // if the vertical edges of any blocks are touching and their horizontals don't
            // line up, split them horizontally..
            if (jrx1 == rx2 || jrx2 == rx1)
            {
                if (jry1 > ry1 && jry1 < ry2)
                {
                    r.h = jry1 - ry1;
                    rects.add (Rectangle<int> (rx1, jry1, rx2 - rx1, ry2 - jry1));
                    i = -1;
                    break;
                }

                if (jry2 > ry1 && jry2 < ry2)
                {
                    r.h = jry2 - ry1;
                    rects.add (Rectangle<int> (rx1, jry2, rx2 - rx1, ry2 - jry2));
                    i = -1;
                    break;
                }
                else if (ry1 > jry1 && ry1 < jry2)
                {
                    r2.h = ry1 - jry1;
                    rects.add (Rectangle<int> (jrx1, ry1, jrx2 - jrx1, jry2 - ry1));
                    i = -1;
                    break;
                }
                else if (ry2 > jry1 && ry2 < jry2)
                {
                    r2.h = ry2 - jry1;
                    rects.add (Rectangle<int> (jrx1, ry2, jrx2 - jrx1, jry2 - ry2));
                    i = -1;
                    break;
                }
            }
        }
    }

    for (i = 0; i < rects.size() - 1; ++i)
    {
        Rectangle<int>& r = rects.getReference (i);

        for (int j = rects.size(); --j > i;)
        {
            if (r.enlargeIfAdjacent (rects.getReference (j)))
            {
                rects.remove (j);
                i = -1;
                break;
            }
        }
    }
}

//==============================================================================
bool RectangleList::containsPoint (const int x, const int y) const noexcept
{
    for (int i = getNumRectangles(); --i >= 0;)
        if (rects.getReference (i).contains (x, y))
            return true;

    return false;
}

bool RectangleList::containsRectangle (const Rectangle<int>& rectangleToCheck) const
{
    if (rects.size() > 1)
    {
        RectangleList r (rectangleToCheck);

        for (int i = rects.size(); --i >= 0;)
        {
            r.subtract (rects.getReference (i));

            if (r.rects.size() == 0)
                return true;
        }
    }
    else if (rects.size() > 0)
    {
        return rects.getReference (0).contains (rectangleToCheck);
    }

    return false;
}

bool RectangleList::intersectsRectangle (const Rectangle<int>& rectangleToCheck) const noexcept
{
    for (int i = rects.size(); --i >= 0;)
        if (rects.getReference (i).intersects (rectangleToCheck))
            return true;

    return false;
}

bool RectangleList::intersects (const RectangleList& other) const noexcept
{
    for (int i = rects.size(); --i >= 0;)
        if (other.intersectsRectangle (rects.getReference (i)))
            return true;

    return false;
}

Rectangle<int> RectangleList::getBounds() const noexcept
{
    if (rects.size() <= 1)
    {
        if (rects.size() == 0)
            return Rectangle<int>();
        else
            return rects.getReference (0);
    }
    else
    {
        const Rectangle<int>& r = rects.getReference (0);

        int minX = r.x;
        int minY = r.y;
        int maxX = minX + r.w;
        int maxY = minY + r.h;

        for (int i = rects.size(); --i > 0;)
        {
            const Rectangle<int>& r2 = rects.getReference (i);

            minX = jmin (minX, r2.x);
            minY = jmin (minY, r2.y);
            maxX = jmax (maxX, r2.getRight());
            maxY = jmax (maxY, r2.getBottom());
        }

        return Rectangle<int> (minX, minY, maxX - minX, maxY - minY);
    }
}

void RectangleList::offsetAll (const int dx, const int dy) noexcept
{
    for (int i = rects.size(); --i >= 0;)
    {
        Rectangle<int>& r = rects.getReference (i);

        r.x += dx;
        r.y += dy;
    }
}

//==============================================================================
Path RectangleList::toPath() const
{
    Path p;

    for (int i = rects.size(); --i >= 0;)
    {
        const Rectangle<int>& r = rects.getReference (i);

        p.addRectangle ((float) r.x,
                        (float) r.y,
                        (float) r.w,
                        (float) r.h);
    }

    return p;
}


END_JUCE_NAMESPACE
