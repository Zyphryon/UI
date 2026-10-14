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
#include <Zyphryon.Render/Font/Font.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the text an element shows, and how it was last laid out.
    struct Caption final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::Measure | Dirty::Paint;

        /// The text shown, as written before it is broken into lines.
        Str                                 Content;

        /// The typeface the text is set in.
        Retainer<ZyRender::Font>            Font;

        /// The text as it was laid out, broken into lines where it overran its box.
        Str                                 Lines;

        /// The width of each laid out line, in pixels.
        Sequence<Real32>                    Widths;

        /// The glyphs of the laid out lines as the font placed them, in font units from the start of each line.
        Sequence<ZyRender::Font::Placement> Glyphs;

        /// The count of glyphs each laid out line holds in \ref Glyphs, in order.
        Sequence<UInt32>                    Runs;

        /// The digest of the lines and the typeface \ref Glyphs was last made for.
        UInt64                              Stamp        = 0;

        /// The digest of the text, the typeface and the size \ref Lines was last broken for.
        UInt64                              Measured     = 0;

        /// The size of the laid out text, in pixels.
        Vector2                             Extent;

        /// How far that copy is pushed, in pixels.
        Vector2                             ShadowOffset = Vector2(1.0f, 1.0f);

        /// The colour the text is drawn in.
        Color                               Tint         = Color::White();

        /// The colour the text is outlined in.
        Color                               Stroke       = Color::Transparent();

        /// The colour of the copy of the text drawn under it, where a transparent one draws none.
        Color                               Shadow       = Color::Transparent();

        /// The size the text is set at, in pixels.
        Real32                              Size         = 16.0f;

        /// The thickness of the outline around the text, in pixels, where nought draws none.
        Real32                              StrokeWidth  = 0.0f;

        /// The width \ref Lines was last broken at, in font units, the largest real when nothing broke it.
        Real32                              Limit        = -1.0f;

        /// How far the widest line reached, in font units, when nothing broke at \ref Limit, or the largest real.
        Real32                              Natural      = kMaximum<Real32>;

        /// Where each line sits along the width of the box.
        Align                               Alignment    = Align::Start;

        /// `true` when the text breaks onto a new line where it would overrun its box.
        Bool                                Breaks       = true;

        /// `true` when \ref Glyphs holds the lines as they are laid out now, `false` once they change.
        Bool                                Shaped       = false;

        /// What a line too long for the box does, which only cuts a caption that does not break.
        UI::Overflow                        Overflow     = UI::Overflow::Clip;

        /// \brief Checks whether breaking the text at another width gives the lines it was last broken into.
        ///
        /// \param Width The width the text would be broken at, in pixels, where nought breaks nothing.
        /// \return `true` if the lines would come out the same, `false` otherwise.
        ZY_INLINE Bool Holds(Real32 Width) const
        {
            const Real32 Wanted = (Width > 0.0f && Size > 0.0f) ? Width / Size : kMaximum<Real32>;
            return Wanted == Limit || (Natural <= Wanted && Natural <= Limit);
        }
    };
}