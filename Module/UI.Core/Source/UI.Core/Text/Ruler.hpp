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

#include "Block.hpp"
#include "Face.hpp"
#include "UI.Core/Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Provides the measures text is edited and cut short by, each one placing glyphs as the painter does.
    class Ruler final
    {
    public:

        /// \brief Gets where a line of text starts across a box, as it is aligned.
        ///
        /// \param Alignment The alignment of the text.
        /// \param Inner     The box, inside its padding, in pixels.
        /// \param Width     The width of the line, in pixels.
        /// \return The left edge of the line, in pixels.
        static Real32 Lead(Align Alignment, Rect Inner, Real32 Width);

        /// \brief Measures how far the pen moves over a line of text, kerning included.
        ///
        /// \param Source The tables of the typeface.
        /// \param Line   The line, holding no line break.
        /// \param Size   The size the text is set at, in pixels.
        /// \return The distance, in pixels.
        static Real32 Measure(ConstRef<Face> Source, Text Line, Real32 Size);

        /// \brief Finds the place in a line of text a caret standing nearest a distance along it stands at.
        ///
        /// \param Source   The tables of the typeface.
        /// \param Line     The line, holding no line break.
        /// \param Size     The size the text is set at, in pixels.
        /// \param Distance The distance from the start of the line, in pixels.
        /// \return The byte the caret stands before, the size of the line past its last character.
        static UInt Locate(ConstRef<Face> Source, Text Line, Real32 Size, Real32 Distance);

        /// \brief Lays text out, breaking it onto a new line at the last space before it would overrun a width.
        ///
        /// \param Source  The tables of the typeface the text is set in.
        /// \param Content The text to lay out.
        /// \param Size    The size the text is set at, in pixels.
        /// \param Width   The width no line may overrun, in pixels, or nought to break only where the text does.
        /// \param Lines   The receiver of the text with a line break wherever a line was broken.
        /// \param Widths  The receiver of the width of each line, in pixels.
        /// \param Natural The receiver of the widest line's reach in font units, or the largest real if a line broke.
        /// \return The width of the widest line and the height of every line together, in pixels.
        static Vector2 Wrap(ConstRef<Face>        Source,
                            Text                  Content,
                            Real32                Size,
                            Real32                Width,
                            Ref<Str>              Lines,
                            Ref<Sequence<Real32>> Widths,
                            Ref<Real32>           Natural);

        /// \brief Cuts every laid out line reaching past a width short, ending it with an ellipsis or three dots.
        ///
        /// \param Source The tables of the typeface.
        /// \param Size   The size the text is set at, in pixels.
        /// \param Width  The width no line may reach past, in pixels.
        /// \param Lines  The laid out lines, each one ending in a line break but the last, cut where they overrun.
        /// \param Widths The width of each line, in pixels, made that of the line as cut.
        /// \return `true` if any line was cut, `false` otherwise.
        static Bool Ellipsize(ConstRef<Face>        Source,
                              Real32                Size,
                              Real32                Width,
                              Ref<Str>              Lines,
                              Ref<Sequence<Real32>> Widths);

        /// \brief Breaks a text into the lines it stands on screen in, as the layout breaks a caption.
        ///
        /// \param Content The text to break.
        /// \param Width   The width no line may overrun, in pixels, or nought to break only where the text does.
        /// \param Output  The block, whose typeface and size are given, receiving where each line starts and its width.
        static void Break(Text Content, Real32 Width, Ref<Block> Output);

        /// \brief Gets the line a place in a text stands on.
        ///
        /// \param Shape  The text, broken into lines.
        /// \param Offset The place, as a byte of the text.
        /// \return The line, counted from nought.
        static UInt32 GetLine(ConstRef<Block> Shape, UInt Offset);

        /// \brief Gets the last place a caret stands on a line, before the break ending it or after the last character.
        ///
        /// \param Shape   The text, broken into lines.
        /// \param Content The text the block was broken from.
        /// \param Line    The line, counted from nought.
        /// \return The place, as a byte of the text.
        static UInt GetEnd(ConstRef<Block> Shape, Text Content, UInt32 Line);

        /// \brief Gets where a caret standing at a place in a text is drawn.
        ///
        /// \param Shape   The text, broken into lines.
        /// \param Content The text the block was broken from.
        /// \param Offset  The place, as a byte of the text.
        /// \return The top of the caret, at the left of the character it stands before, in pixels.
        static Vector2 Place(ConstRef<Block> Shape, Text Content, UInt Offset);

        /// \brief Finds the place in a text nearest a point, on the line under it or the nearest one above or below.
        ///
        /// \param Shape   The text, broken into lines.
        /// \param Content The text the block was broken from.
        /// \param Point   The point to find the nearest place to, in pixels.
        /// \return The place, as a byte of the text.
        static UInt Find(ConstRef<Block> Shape, Text Content, Vector2 Point);

    private:

        /// \brief Gets how far a codepoint moves the pen, before any kerning.
        ///
        /// \param Source    The tables of the typeface.
        /// \param Codepoint The codepoint the pen moves over.
        /// \return The advance, in font units, the fallback glyph's for one the typeface lacks, or \ref Face::kNone.
        static Real32 Advance(ConstRef<Face> Source, UInt32 Codepoint);

        /// \brief Breaks text onto new lines, as \ref Wrap does.
        ///
        /// \tparam Copied  The flag, `true` to turn only spaces of a copy into breaks, `false` to write all codepoints.
        /// \param Source  The tables of the typeface.
        /// \param Content The text to lay out.
        /// \param Size    The size the text is set at, in pixels.
        /// \param Limit   The width no line may overrun, in font units.
        /// \param Lines   The receiver of the text with line breaks, holding a copy of the text when \p Copied.
        /// \param Widths  The receiver of the width of each line, in pixels.
        /// \param Natural The receiver of the widest line's reach in font units, or the largest real if a line broke.
        /// \return `true` if the walk ran to the end, `false` otherwise.
        template<Bool Copied>
        static Bool Walk(ConstRef<Face>        Source,
                         Text                  Content,
                         Real32                Size,
                         Real32                Limit,
                         Ref<Str>              Lines,
                         Ref<Sequence<Real32>> Widths,
                         Ref<Real32>           Natural);
    };
}