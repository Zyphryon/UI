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

#include <Zyphryon.Render/Font/Font.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the glyphs, advances and kerning of a typeface's first codepoints as plain tables.
    struct Face final
    {
        /// The codepoints the tables cover, counted from nought.
        static constexpr UInt32 kCovered = 128;

        /// The advance a codepoint the typeface draws nothing for holds, which no glyph has.
        static constexpr Real32 kNone    = -kMaximum<Real32>;

        /// The typeface, held so its address names it for as long as the tables stand.
        Retainer<ZyRender::Font>                  Font;

        /// The glyph of each covered codepoint, the fallback glyph for one it lacks, or `nullptr` for none at all.
        Sequence<ConstPtr<ZyRender::Font::Glyph>> Glyphs;

        /// The advance of each covered codepoint, in font units, the fallback glyph's for one it lacks, or \ref kNone.
        Sequence<Real32>                          Advances;

        /// The kerning between each pair of covered codepoints, in font units, a row per first and a column per second.
        Sequence<Real32>                          Kerning;

        /// \brief Constructs the tables of a typeface, asking it once for every covered codepoint and pair.
        ///
        /// \param Source The typeface, which must have finished loading.
        explicit Face(ConstRef<Retainer<ZyRender::Font>> Source);

        /// \brief Gets the glyph a codepoint is drawn with.
        ///
        /// \param Codepoint The codepoint to draw.
        /// \return The glyph, the fallback glyph for one the typeface lacks, or `nullptr` for none at all.
        ZY_INLINE ConstPtr<ZyRender::Font::Glyph> GetGlyph(UInt32 Codepoint) const
        {
            return Codepoint < kCovered ? Glyphs[Codepoint] : Font->GetGlyph(Codepoint);
        }

        /// \brief Gets how far a pair of codepoints is pulled together or pushed apart.
        ///
        /// \param First  The codepoint the pen moves over first.
        /// \param Second The codepoint that follows it.
        /// \return The kerning, in font units.
        ZY_INLINE Real32 GetKerning(UInt32 First, UInt32 Second) const
        {
            const Bool IsCovered = (First < kCovered && Second < kCovered);
            return IsCovered ? Kerning[First * kCovered + Second] : Font->GetKerning(First, Second);
        }
    };
}