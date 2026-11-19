// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "UI.Core/Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents an element that scrolls what it holds, cut to its box, up and down and optionally sideways.
    struct Scroll final
    {
        /// The parts a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::Measure | Dirty::Paint;

        /// The distance what it holds is scrolled down, in pixels, held within what it holds.
        Real32  Offset     = 0.0f;

        /// The height of what it holds, in pixels, as the last layout found it.
        Real32  Overflow   = 0.0f;

        /// The distance what it holds is scrolled right, in pixels, held within what it holds.
        Real32  OffsetX    = 0.0f;

        /// The width of what it holds, in pixels, as the last layout found it.
        Real32  OverflowX  = 0.0f;

        /// The room what it holds is seen through, in pixels, as the last layout found it.
        Vector2 View;

        /// The colour of the thumbs showing how far it is scrolled, where a transparent one draws none.
        Color   Thumb      = Color::White(0.3f);

        /// The width of the gutter kept free down its right side for the painted thumb, in pixels.
        Real32  Gutter     = 8.0f;

        /// `true` to keep it scrolled to the end as a log follows its newest line, `false` to leave it where it is.
        Bool    Follow     = false;

        /// `true` when what it holds keeps the width it asks for and scrolls sideways, `false` when fit to it.
        Bool    Horizontal = false;

        /// \brief Gets the farthest what it holds scrolls down.
        ///
        /// \return The distance, in pixels, nought when all it holds is seen.
        ZY_INLINE Real32 GetLimit() const
        {
            return Max(Overflow - View.GetY(), 0.0f);
        }

        /// \brief Gets the farthest what it holds scrolls right.
        ///
        /// \return The distance, in pixels, nought when all it holds is seen across.
        ZY_INLINE Real32 GetLimitX() const
        {
            return Max(OverflowX - View.GetX(), 0.0f);
        }

        /// \brief Gets the strip down the element's right edge the up and down thumb runs in and the pointer grabs.
        ///
        /// \param Frame The element's box, in pixels.
        /// \param Top   The top of the room inside its padding, in pixels.
        /// \return The strip, or an empty box when no thumb shows, as when all it holds is seen.
        ZY_INLINE Rect GetTrack(Rect Frame, Real32 Top) const
        {
            if (Thumb.GetAlpha() <= 0.0f || Overflow <= View.GetY())
            {
                return Rect();
            }
            return Rect(Frame.GetMaximumX() - Max(Gutter, 6.0f), Top, Frame.GetMaximumX(), Top + View.GetY());
        }

        /// \brief Gets the up and down thumb, as tall against its track as the room is against what it holds.
        ///
        /// \param Frame The element's box, in pixels.
        /// \param Top   The top of the room inside its padding, in pixels.
        /// \return The thumb, or an empty box when no thumb shows.
        ZY_INLINE Rect GetThumb(Rect Frame, Real32 Top) const
        {
            if (Thumb.GetAlpha() <= 0.0f || Overflow <= View.GetY())
            {
                return Rect();
            }

            const Real32 Room   = View.GetY();
            const Real32 Size   = Max(Room * Room / Overflow, 16.0f);
            const Real32 Travel = Room - Size;
            const Real32 Down   = Top + Travel * (Offset / (Overflow - Room));
            const Real32 Left   = Frame.GetMaximumX() - 6.0f;

            return Rect(Left, Down, Left + 4.0f, Down + Size);
        }
    };
}