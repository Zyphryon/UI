// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Ruler.hpp"
#include "Block.hpp"
#include "Face.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ZY_INLINE Real32 Ruler::Advance(ConstRef<Face> Source, UInt32 Codepoint)
    {
        if (Codepoint < Face::kCovered)
        {
            return Source.Advances[Codepoint];
        }

        const ConstPtr<ZyRender::Font::Glyph> Glyph = Source.Font->GetGlyph(Codepoint);
        return Glyph ? Glyph->Advance : Face::kNone;
    }
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Ruler::Lead(Align Alignment, Rect Inner, Real32 Width)
    {
        switch (Alignment)
        {
        case Align::Center:
            return Inner.GetMinimumX() + (Inner.GetWidth() - Width) * 0.5f;
        case Align::End:
            return Inner.GetMaximumX() - Width;
        default:
            return Inner.GetMinimumX();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Ruler::Measure(ConstRef<Face> Source, Text Line, Real32 Size)
    {
        Real32 Pen      = 0.0f;
        UInt32 Previous = 0;

        // The pen moves as the painter places glyphs, kerned against the glyph before.
        for (UInt Cursor = 0; Cursor < Line.GetSize();)
        {
            const UInt32 Codepoint = Unicode::Decode(Line, Cursor);
            const Real32 Step      = Advance(Source, Codepoint);

            if (Step < 0.0f)
            {
                continue;
            }

            if (Previous)
            {
                Pen += Source.GetKerning(Previous, Codepoint);
            }

            Pen     += Step;
            Previous = Codepoint;
        }
        return Pen * Size;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt Ruler::Locate(ConstRef<Face> Source, Text Line, Real32 Size, Real32 Distance)
    {
        const Real32 Goal     = Size > 0.0f ? Distance / Size : 0.0f;
        Real32       Pen      = 0.0f;
        UInt32       Previous = 0;

        // A caret goes before the first character whose middle stands past the point.
        for (UInt Cursor = 0; Cursor < Line.GetSize();)
        {
            const UInt   At        = Cursor;
            const UInt32 Codepoint = Unicode::Decode(Line, Cursor);
            const Real32 Step      = Advance(Source, Codepoint);

            if (Step < 0.0f)
            {
                continue;
            }

            if (Previous)
            {
                Pen += Source.GetKerning(Previous, Codepoint);
            }

            if (Goal < Pen + Step * 0.5f)
            {
                return At;
            }

            Pen     += Step;
            Previous = Codepoint;
        }
        return Line.GetSize();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Vector2 Ruler::Wrap(ConstRef<Face>        Source,
                        Text                  Content,
                        Real32                Size,
                        Real32                Width,
                        Ref<Str>              Lines,
                        Ref<Sequence<Real32>> Widths,
                        Ref<Real32>           Natural)
    {
        // The walk is done in font units, as the font's own layout is, and only its results are scaled.
        const Real32 Limit = (Width > 0.0f && Size > 0.0f) ? Width / Size : kMaximum<Real32>;

        // The lines are nearly always the text with some spaces broken, so they start as a copy of it.
        Lines = Content;
        Widths.Clear();

        if (!Walk<true>(Source, Content, Size, Limit, Lines, Widths, Natural))
        {
            Lines.Clear();
            Widths.Clear();
            Walk<false>(Source, Content, Size, Limit, Lines, Widths, Natural);
        }

        Real32 Widest = 0.0f;

        for (const Real32 Line : Widths)
        {
            Widest = Max(Widest, Line);
        }
        return Vector2(Widest, Source.Font->GetLineHeight(Size) * Widths.GetSize());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Ruler::Ellipsize(ConstRef<Face>        Source,
                          Real32                Size,
                          Real32                Width,
                          Ref<Str>              Lines,
                          Ref<Sequence<Real32>> Widths)
    {
        if (Size <= 0.0f || Width <= 0.0f)
        {
            return false;
        }

        // The ellipsis is one glyph where the typeface draws it, never the fallback glyph standing in for it.
        const Text   Mark  = Source.Font->GetGlyph(0x2026, 0) ? Text("\xE2\x80\xA6") : Text("...");
        const Real32 Tail  = Measure(Source, Mark, 1.0f);
        const Real32 Limit = Width / Size;

        Str  Result;
        Bool Cut   = false;
        UInt Start = 0;

        for (UInt32 Line = 0; Line < Widths.GetSize() && Start <= Lines.GetSize(); ++Line)
        {
            const SInt Found = StrFind(Text(Lines).Slice(Start), '\n');
            const UInt End   = Found < 0 ? Lines.GetSize() : Start + static_cast<UInt>(Found);

            const Text Row = Text(Lines).Slice(Start, End - Start);

            if (Widths[Line] <= Width + 0.01f)
            {
                Result.Append(Row);
            }
            else
            {
                Real32 Pen      = 0.0f;
                Real32 Reach    = 0.0f;
                UInt32 Previous = 0;
                UInt   Kept     = 0;

                // As much of the line is kept as leaves room for the ellipsis, without the spaces it would follow.
                for (UInt Cursor = 0; Cursor < Row.GetSize();)
                {
                    const UInt32 Codepoint = Unicode::Decode(Row, Cursor);
                    const Real32 Step      = Advance(Source, Codepoint);

                    if (Step < 0.0f)
                    {
                        continue;
                    }

                    const Real32 After = Pen + (Previous ? Source.GetKerning(Previous, Codepoint) : 0.0f) + Step;

                    if (After + Tail > Limit)
                    {
                        break;
                    }

                    Pen      = After;
                    Previous = Codepoint;

                    if (Codepoint != ' ')
                    {
                        Reach = Pen;
                        Kept  = Cursor;
                    }
                }

                Result.Append(Row.Slice(0, Kept));
                Result.Append(Mark);
                Widths[Line] = (Reach + Tail) * Size;
                Cut          = true;
            }

            if (End < Lines.GetSize())
            {
                Result.Append('\n');
            }
            Start = End + 1;
        }

        if (Cut)
        {
            Lines = Move(Result);
        }
        return Cut;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Ruler::Break(Text Content, Real32 Width, Ref<Block> Output)
    {
        Output.Starts.Clear();
        Output.Widths.Clear();
        Output.Starts.Append(0);

        // The broken text is kept from call to call, since a field being edited is broken every frame.
        thread_local Str Lines;
        Lines.Clear();

        if (Output.Source)
        {
            Real32 Natural = 0.0f;
            Wrap(* Output.Source, Content, Output.Size, Width, Lines, Output.Widths, Natural);
        }

        // Wrapping only turns spaces into breaks, keeping every byte; otherwise the text breaks only where it does.
        const Bool Kept = Output.Source && Lines.GetSize() == Content.GetSize();
        const Text Read = Kept ? Text(Lines) : Content;

        if (!Kept)
        {
            Output.Widths.Clear();
        }

        for (UInt Start = 0; Start <= Read.GetSize();)
        {
            const SInt Found = StrFind(Read.Slice(Start), '\n');
            const UInt End   = Found < 0 ? Read.GetSize() : Start + static_cast<UInt>(Found);

            if (!Kept)
            {
                const Text Row = Content.Slice(Start, End - Start);
                Output.Widths.Append(Output.Source ? Measure(* Output.Source, Row, Output.Size) : 0.0f);
            }

            if (End < Read.GetSize())
            {
                Output.Starts.Append(static_cast<UInt32>(End + 1));
            }
            Start = End + 1;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Ruler::GetLine(ConstRef<Block> Shape, UInt Offset)
    {
        // The starts climb from nought, so the line is the last one starting at or before the place.
        const SInt Line = Shape.Starts.FindLast([Offset](UInt32 Start)
        {
            return Start <= Offset;
        });
        return static_cast<UInt32>(Max<SInt>(Line, 0));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt Ruler::GetEnd(ConstRef<Block> Shape, Text Content, UInt32 Line)
    {
        return Line + 1 < Shape.Starts.GetSize() ? Shape.Starts[Line + 1] - 1 : Content.GetSize();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Vector2 Ruler::Place(ConstRef<Block> Shape, Text Content, UInt Offset)
    {
        const UInt32 Line  = GetLine(Shape, Offset);
        const UInt   Start = Shape.Starts[Line];
        const UInt   End   = Clamp(Offset, Start, GetEnd(Shape, Content, Line));
        const Real32 Width = Line < Shape.Widths.GetSize() ? Shape.Widths[Line] : 0.0f;

        const Real32 Across = Shape.Source
            ? Measure(* Shape.Source, Content.Slice(Start, End - Start), Shape.Size)
            : 0.0f;
        return Vector2(Lead(Shape.Alignment, Shape.Inner, Width) + Across,
                       Shape.Top + Shape.Leading * static_cast<Real32>(Line));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt Ruler::Find(ConstRef<Block> Shape, Text Content, Vector2 Point)
    {
        // A point above the first line finds the first, and one below the last finds the last.
        const Real32 Down = Shape.Leading > 0.0f ? (Point.GetY() - Shape.Top) / Shape.Leading : 0.0f;
        const UInt32 Last = static_cast<UInt32>(Shape.Starts.GetSize() - 1);
        const UInt32 Line = static_cast<UInt32>(Clamp(Down, 0.0f, static_cast<Real32>(Last)));

        const UInt   Start = Shape.Starts[Line];
        const UInt   End   = GetEnd(Shape, Content, Line);
        const Real32 Width = Line < Shape.Widths.GetSize() ? Shape.Widths[Line] : 0.0f;
        const Real32 Left  = Lead(Shape.Alignment, Shape.Inner, Width);

        if (!Shape.Source)
        {
            return Point.GetX() <= Left ? Start : End;
        }
        return Start + Locate(* Shape.Source, Content.Slice(Start, End - Start), Shape.Size, Point.GetX() - Left);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<Bool Copied>
    Bool Ruler::Walk(ConstRef<Face>        Source,
                     Text                  Content,
                     Real32                Size,
                     Real32                Limit,
                     Ref<Str>              Lines,
                     Ref<Sequence<Real32>> Widths,
                     Ref<Real32>           Natural)
    {
        Real32 Pen      = 0.0f;
        Real32 Reach    = 0.0f;
        Real32 Furthest = 0.0f;
        Bool   Broke    = false;
        UInt32 Previous = 0;

        // The last space of the line, where it breaks when a word overruns, and how far the line reached before it.
        UInt   Break       = 0;
        Bool   Breakable   = false;
        Real32 BreakReach  = 0.0f;
        Real32 BreakResume = 0.0f;

        for (UInt Cursor = 0; Cursor < Content.GetSize();)
        {
            const UInt  At   = Cursor;
            const UInt8 Lead = static_cast<UInt8>(Content[Cursor]);

            // A byte below the tables' end is a codepoint of its own, read straight from them.
            UInt32 Codepoint;
            Real32 Step;

            if (Lead < Face::kCovered)
            {
                Codepoint = Lead;
                Step      = Source.Advances[Lead];
                ++Cursor;
            }
            else
            {
                Codepoint = Unicode::Decode(Content, Cursor);
                Step      = Advance(Source, Codepoint);

                if constexpr (Copied)
                {
                    if (Codepoint == 0 || Cursor - At != Unicode::GetLength(Codepoint))
                    {
                        return false;
                    }
                }
            }

            if (Codepoint == '\n')
            {
                Widths.Append(Reach * Size);

                if constexpr (!Copied)
                {
                    Lines.Append('\n');
                }

                Pen       = 0.0f;
                Reach     = 0.0f;
                Previous  = 0;
                Breakable = false;
                continue;
            }

            if (Step == Face::kNone)
            {
                if constexpr (Copied)
                {
                    return false;
                }
                else
                {
                    continue;
                }
            }

            const Real32 Start = Pen + (Previous ? Source.GetKerning(Previous, Codepoint) : 0.0f);
            Real32       End   = Start + Step;

            // The word overruns, so the line ends at its last space and what followed moves down with the word.
            if (Codepoint != ' ' && End > Limit && Breakable)
            {
                Widths.Append(BreakReach * Size);
                Lines[Break] = '\n';

                End      -= BreakResume;
                Reach     = Max(Reach - BreakResume, 0.0f);
                Breakable = false;
                Broke     = true;
            }

            if (Codepoint == ' ')
            {
                Break       = Copied ? At : Lines.GetSize();
                Breakable   = true;
                BreakReach  = Reach;
                BreakResume = End;
            }
            else
            {
                Reach    = End;
                Furthest = Max(Furthest, End);
            }

            if constexpr (!Copied)
            {
                Lines.AppendCodepoint(Codepoint);
            }

            Pen      = End;
            Previous = Codepoint;
        }

        Widths.Append(Reach * Size);

        // Unbroken, the walk went as it would at any width, and only a glyph past the width could have broken it.
        Natural = Broke ? kMaximum<Real32> : Furthest;
        return true;
    }
}