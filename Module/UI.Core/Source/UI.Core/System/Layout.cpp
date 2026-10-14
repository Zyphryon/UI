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

#include "Layout.hpp"
#include "Animator.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Facet/Visual/Caption.hpp"
#include "UI.Core/Facet/Visual/Picture.hpp"
#include "UI.Core/Facet/Visual/Scroll.hpp"
#include "UI.Core/Pane.hpp"
#include "UI.Core/Text/Face.hpp"
#include "UI.Core/Text/Ruler.hpp"
#include "UI.Core/Tree.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Layout::Layout(Ref<Tree> Owner, Ref<Animator> Animator)
        : mTree     { Owner },
          mAnimator { Animator }
    {
        AddMeasurer<Picture>(Measurer::Create<&Layout::MeasurePicture>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Layout::Run(Ref<Pane> Target)
    {
        ZY_PROFILE_SCOPE("UI::Layout::Run");

        mWaiting.Clear();
        mFits.Clear();

        if (Target.Whole)
        {
            Overhaul(Target);
        }
        else
        {
            Amend(Target);
        }

        // What the last pass fit to a new room is measured again by the next layout, as one more pass would have.
        Requeue(Target);

        // A font or image still loading is measured again next frame, once it has.
        for (const UInt32 Row : mWaiting)
        {
            Target.Marks[Row] |= Dirty::Measure;
        }

        Target.Stale = !mWaiting.IsEmpty();
        Target.Whole = false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Layout::Catch(Ref<Pane> Target, UInt32 Row)
    {
        // Only a box sized in pixels holds text back, so any other is left to the layout.
        if (!IsFixed(Target.Boxes[Row]))
        {
            return false;
        }

        const UInt32 End  = Row + Target.Spans[Row];
        Bool         Held = false;

        mWaiting.Clear();
        mFits.Clear();

        for (UInt32 Child = Row + 1; Child < End; Child += Target.Spans[Child])
        {
            const Bool Leaf    = Target.Spans[Child] == 1;
            const Bool Changed = HasBit(Target.Marks[Child], Dirty::Measure);

            if (Leaf && Changed && mTree.Peek<Caption>(Target.Slots[Child]))
            {
                Refit(Target, Child, Target.Marks[Child]);
                Held = true;
            }
        }

        if (!Held)
        {
            return false;
        }

        // The box is arranged again around what it now holds, as a layout would, text fit to a new room measured anew.
        for (UInt32 Pass = 0; Pass < kPasses; ++Pass)
        {
            Target.Marks[Row] |= Dirty::Arrange;

            if (!Sweep(Target, Row, End))
            {
                break;
            }

            for (const UInt32 Fit : mFits)
            {
                Refit(Target, Fit, Target.Marks[Fit] | Dirty::Resize);
            }
            mFits.Clear();
        }
        Requeue(Target);

        // A font still loading is measured again by the next layout, once it has.
        for (const UInt32 Waiting : mWaiting)
        {
            Target.Marks[Waiting] |= Dirty::Measure;
            Target.Stale           = true;
        }

        Target.Touched = true;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstRef<Face> Layout::FaceOf(ConstRef<Retainer<ZyRender::Font>> Font)
    {
        const SInt Index = mFaces.Find([&Font](ConstRef<Face> Entry)
        {
            return Entry.Font == Font;
        });
        return Index >= 0 ? mFaces[static_cast<UInt>(Index)] : mFaces.Append(Font);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Layout::Forget()
    {
        // A typeface only the tables still hold is let go with them, so a theme dropping a font frees it.
        mFaces.RemoveFastSomeIf([](ConstRef<Face> Entry)
        {
            return Entry.Font->IsUnique();
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Layout::Overhaul(Ref<Pane> Target)
    {
        constexpr Dirty kKept = Dirty::Style | Dirty::Paint;

        const UInt32 Size = Target.GetSize();

        // Changed text is measured unbroken, so a label sized by its text grows to it rather than wrapping to the old.
        for (UInt32 Row = 0; Row < Size; ++Row)
        {
            if (HasBit(Target.Marks[Row], Dirty::Measure) && mTree.Peek<Caption>(Target.Slots[Row]))
            {
                Target.Rooms[Row] = Vector2();
            }
            Target.Marks[Row] = (Target.Marks[Row] & kKept) | Dirty::Arrange;
        }

        // Wrapped text is fit to the room the pass before gave it, so a pass that changed that room runs again.
        for (UInt32 Pass = 0; Pass < kPasses; ++Pass)
        {
            Bool Unsettled = false;
            mWaiting.Clear();
            mFits.Clear();

            for (UInt32 Row = Size; Row-- > 0;)
            {
                if (Measure(Target, Row))
                {
                    mWaiting.Append(Row);
                }
            }

            Place(Target, 0, Rect(Vector2(), mSize));

            for (UInt32 Row = 0; Row < Size; ++Row)
            {
                if (!mAnimator.IsDrawn(Target, Row))
                {
                    Row += Target.Spans[Row] - 1;
                    continue;
                }
                Target.Marks[Row] = ClearBit(Target.Marks[Row], Dirty::Arrange);
                Unsettled = Arrange(Target, Row) || Unsettled;
            }

            if (!Unsettled)
            {
                break;
            }
        }

        // Boxes moved, so every reach is worked out again at the next paint.
        Target.Moved = true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Layout::Amend(Ref<Pane> Target)
    {
        // A pass that fit text to a room that changed runs again, as a whole layout does.
        for (UInt32 Pass = 0; Pass < kPasses; ++Pass)
        {
            Survey(Target);

            if (!Sweep(Target, 0, Target.GetSize()))
            {
                break;
            }
            Requeue(Target);
        }

        // Every row measured or arranged was marked to have its reach worked out again.
        Target.Touched = true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Layout::Survey(Ref<Pane> Target)
    {
        // Walked backward so children come first; what a row asks of its parent waits at its depth until the parent.
        mClimb.Clear();

        for (UInt32 Row = Target.GetSize(); Row-- > 0;)
        {
            const UInt32 Depth = Target.Depths[Row];

            if (mClimb.GetSize() < Depth + 2)
            {
                mClimb.Advance(Depth + 2 - mClimb.GetSize());
            }

            Dirty Bits = Target.Marks[Row];

            if (const Dirty Asked = mClimb[Depth + 1]; Asked != Dirty::None)
            {
                Bits |= Asked;
                mClimb[Depth + 1] = Dirty::None;
            }

            // Most rows are marked for nothing the layout does, and are passed over at once.
            if (!HasAnyBit(Bits, kAsked | kSought))
            {
                continue;
            }

            // Text in a box out of view keeps its mark until the box is painted again, and its reach waits with it.
            if (HasBit(Bits, Dirty::Measure) && Defers(Target, Row))
            {
                Target.Marks[Row] = ClearBit(Bits, Dirty::Paint);
                continue;
            }

            if (HasAnyBit(Bits, kAsked))
            {
                mClimb[Depth] |= Refit(Target, Row, Bits);
                Bits = Target.Marks[Row];
            }
            else
            {
                Target.Marks[Row] = Bits;
            }

            // Every row holding one to arrange is walked through by the sweep to reach it.
            if (HasAnyBit(Bits, kSought))
            {
                mClimb[Depth] |= Dirty::Below;
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Dirty Layout::Refit(Ref<Pane> Target, UInt32 Row, Dirty Bits)
    {
        const Bool Told  = HasBit(Bits, Dirty::Measure);
        const Bool Words = mTree.Peek<Caption>(Target.Slots[Row]) != nullptr;

        // What a row draws of its own moves only with its frame, which the sweep tells, its text, or its box.
        Target.Marks[Row] = ClearBit(Bits, kAsked) | Dirty::Arrange | ((Told || Words) ? Dirty::Paint : Dirty::None);

        // New text is measured unbroken first, as a whole layout does.
        if (Told && Words)
        {
            Target.Rooms[Row] = Vector2();
        }

        // A box sized in pixels on both axes keeps its size, so only a change told to it, or its own text, measures it.
        const Bool    Fixed  = !Told && !Words && IsFixed(Target.Boxes[Row]);
        const Vector2 Before = Target.Sizes[Row];

        if (!Fixed && Measure(Target, Row))
        {
            mWaiting.Append(Row);
        }

        if (Target.Parents[Row] == Pane::kNoParent)
        {
            Place(Target, Row, Rect(Vector2(), mSize));
            return Dirty::None;
        }

        // A told change may alter the parent's layout whatever the size; otherwise only a new size reaches the parent.
        return (Told || Target.Sizes[Row] != Before) ? Dirty::Resize : Dirty::None;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Layout::Requeue(Ref<Pane> Target)
    {
        for (const UInt32 Row : mFits)
        {
            Target.Marks[Row] |= Dirty::Resize;
        }
        mFits.Clear();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Layout::Defers(ConstRef<Pane> Target, UInt32 Row) const
    {
        if (Target.Spans[Row] != 1 || Target.Parents[Row] == Pane::kNoParent || !mTree.Peek<Caption>(Target.Slots[Row]))
        {
            return false;
        }

        // A box sized in pixels asks for that size whatever its text, so its text moves nothing past it.
        const UInt32 Above = mTree.GetRow(Target.Parents[Row]);
        return HasBit(Target.Marks[Above], Dirty::Unseen) && IsFixed(Target.Boxes[Above]);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Layout::Sweep(Ref<Pane> Target, UInt32 First, UInt32 Last)
    {
        Bool Unsettled = false;

        for (UInt32 Row = First; Row < Last;)
        {
            const Dirty Bits  = Target.Marks[Row];
            const Bool  Moved = Target.Frames[Row] != Target.Laid[Row];
            const Bool  Due   = Moved || HasBit(Bits, Dirty::Arrange);

            // A subtree nothing reached stays as it was, and a hidden one keeps its marks until it shows.
            if ((!Due && !HasBit(Bits, Dirty::Below)) || !mAnimator.IsDrawn(Target, Row))
            {
                Row += Target.Spans[Row];
                continue;
            }

            Target.Marks[Row] = ClearBit(Bits, kSought) | (Moved ? Dirty::Paint : Dirty::None);

            if (Due)
            {
                Unsettled = Arrange(Target, Row) || Unsettled;
            }
            ++Row;
        }
        return Unsettled;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Layout::Measure(Ref<Pane> Target, UInt32 Row)
    {
        ConstRef<Box> Shape   = Target.Boxes[Row];
        const UInt32  Slot    = Target.Slots[Row];
        const Bool    Along   = (Shape.Flow == Direction::Row);
        Bool          Pending = false;
        Vector2       Content;

        if (const Ptr<Caption> Text = mTree.Peek<Caption>(Slot); Text && Text->Font)
        {
            if (Text->Font->HasCompleted())
            {
                // Text is broken, or cut short, to the room the last pass gave it, which the first pass has not yet.
                const Bool   Cuts     = (Text->Overflow == Overflow::Ellipsis);
                const Real32 Width    = (Text->Breaks || Cuts) ? Target.Rooms[Row].GetX() : 0.0f;
                const UInt64 Words    = Text->Content.Hash();
                const UInt64 Typeface = Hash(AddressOf(* Text->Font));
                const UInt64 Digest   = HashCombine(Words, Typeface, Text->Size, Cuts);

                // The same text broken the same way measures as it did, so only new text or a new break walks it.
                if (Digest != Text->Measured || !Text->Holds(Width))
                {
                    Typeset(* Text, Width);
                    Text->Measured = Digest;
                    Text->Limit    = (Width > 0.0f && Text->Size > 0.0f) ? Width / Text->Size : kMaximum<Real32>;

                    // Glyphs are placed again only when the lines or font changed; unbroken lines reuse the digest.
                    const Bool   Unbroken = (Text->Lines == Text->Content);
                    const UInt64 Lined    = Unbroken ? Words : Text->Lines.Hash();
                    const UInt64 Stamp    = HashCombine(Lined, Typeface);

                    if (Stamp != Text->Stamp)
                    {
                        Text->Stamp  = Stamp;
                        Text->Shaped = false;
                    }
                }
                Content = Text->Extent;
            }
            else
            {
                Pending = true;
            }
        }

        // What a facet shows asks for its own size, a later measurer's replacing an earlier one's, as the picture's.
        ConstRef<Mask> Held = mTree.GetMask(mTree.GetHandle(Slot));

        for (ConstRef<Probe> Each : mProbes)
        {
            if (Held.Test(Each.Facet) && Each.Code(mTree, Target, Row, Content))
            {
                Pending = true;
            }
        }

        // What flows in the element lines up along its axis, starting a new line where it wraps past its room.
        const Real32 Room = Along ? Target.Rooms[Row].GetX() : Target.Rooms[Row].GetY();
        const Bool   Wrap = Shape.Wrap && Room > 0.0f;

        Real32 Main     = 0.0f;
        Real32 Side     = 0.0f;
        Real32 LineMain = 0.0f;
        Real32 LineSide = 0.0f;
        UInt32 Count    = 0;

        for (UInt32 Child = Row + 1, End = Row + Target.Spans[Row]; Child < End; Child += Target.Spans[Child])
        {
            ConstRef<Box> Inner = Target.Boxes[Child];

            if (!mAnimator.IsDrawn(Target, Child) || Inner.Placement != Position::Flow)
            {
                continue;
            }

            const Vector2 Size      = Target.Sizes[Child];
            const Real32  OuterMain = (Along ? Size.GetX() : Size.GetY()) + MainOf(Along, Inner.Margin);
            const Real32  OuterSide = (Along ? Size.GetY() : Size.GetX()) + SideOf(Along, Inner.Margin);

            if (Wrap && Count > 0 && LineMain + Shape.Gap + OuterMain > Room)
            {
                Main     = Max(Main, LineMain);
                Side    += LineSide + Shape.Gap;
                LineMain = 0.0f;
                LineSide = 0.0f;
                Count    = 0;
            }

            LineMain += (Count > 0 ? Shape.Gap : 0.0f) + OuterMain;
            LineSide  = Max(LineSide, OuterSide);
            ++Count;
        }

        Main  = Max(Main, LineMain);
        Side += LineSide;

        // Content that scrolls sideways asks for no width of its own, or it would widen what holds it past scrolling.
        const ConstPtr<Scroll> Roll     = mTree.Peek<Scroll>(Slot);
        const Vector2          Children = Along ? Vector2(Main, Side) : Vector2(Side, Main);
        const Real32           Asked    = (Roll && Roll->Horizontal) ? 0.0f : Max(Content.GetX(), Children.GetX());
        const Real32           Wide     = Asked + Shape.Padding.Left + Shape.Padding.Right;
        const Real32           Tall     = Max(Content.GetY(), Children.GetY()) + Shape.Padding.Top +
                                          Shape.Padding.Bottom;

        // A share of the parent is unknown until the parent is placed, so here it asks only for its content.
        Target.Sizes[Row] = Vector2(
            Shape.Width.Measure  == Length::Unit::Pixel ? Shape.Width.Value  : Wide,
            Shape.Height.Measure == Length::Unit::Pixel ? Shape.Height.Value : Tall);
        return Pending;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Layout::Arrange(Ref<Pane> Target, UInt32 Row)
    {
        ConstRef<Box>     Shape = Target.Boxes[Row];
        const UInt32      Slot  = Target.Slots[Row];
        const UInt32      End   = Row + Target.Spans[Row];
        const Rect        Frame = Target.Frames[Row];
        const Ptr<Scroll> Roll  = mTree.Peek<Scroll>(Slot);

        Target.Laid[Row] = Frame;

        // A scrolling element keeps a gutter free for its thumb, so its content never runs under it.
        const Rect Bounds = Shape.Padding.Inset(Frame);
        const Rect Padded = Roll ? Bounds - Rect(0.0f, 0.0f, Roll->Gutter, 0.0f) : Bounds;
        const Bool Along  = (Shape.Flow == Direction::Row);

        // One scrolling sideways lays its content out as wide as it asks, when that is wider than the element.
        const Real32  Wide   = (Roll && Roll->Horizontal && !Shape.Wrap) ? Breadth(Target, Row) : 0.0f;
        const Rect    Inner  = Wide > Padded.GetWidth()
            ? Rect(Padded.GetMinimumX(), Padded.GetMinimumY(), Padded.GetMinimumX() + Wide, Padded.GetMaximumY())
            : Padded;
        const Vector2 Room   = Inner.GetSize();

        // What was fit to the room of the pass before is fit again when the room changed, unless its lines would hold.
        const ConstPtr<Caption> Text   = mTree.Peek<Caption>(Slot);
        const Bool              Fits   = Text && (Text->Breaks || Text->Overflow == Overflow::Ellipsis);
        const Bool              Fitted = (Fits && !Text->Holds(Room.GetX())) || Shape.Wrap;
        const Vector2           Before = Target.Rooms[Row];
        const Bool              Moved  = Room.GetDistanceChebyshev(Before) > 0.5f;

        Target.Rooms[Row] = Room;

        // Any change at all is fit again by the next pass, should one run.
        if (Fitted && Room != Before)
        {
            mFits.Append(Row);
        }

        // A scrolling element lays its content out unscrolled, and moves it up once the offset is held within it.
        const Real32 RoomMain = Along ? Room.GetX() : Room.GetY();
        const Real32 RoomSide = Along ? Room.GetY() : Room.GetX();

        Real32 LineStart = Along ? Inner.GetMinimumY() : Inner.GetMinimumX();
        Real32 Bottom    = Inner.GetMinimumY();
        Real32 Right     = Inner.GetMinimumX();
        UInt32 Cursor    = Row + 1;

        // Each pass of the loop gathers one line of children, the whole flow when the element does not wrap.
        while (Cursor < End)
        {
            UInt32 Rest   = End;
            Real32 Used   = 0.0f;
            Real32 Shares = 0.0f;
            Real32 Weight = 0.0f;
            Real32 Cross  = 0.0f;
            UInt32 Count  = 0;

            for (UInt32 Child = Cursor; Child < End; Child += Target.Spans[Child])
            {
                ConstRef<Box> Next = Target.Boxes[Child];

                if (!mAnimator.IsDrawn(Target, Child) || Next.Placement != Position::Flow)
                {
                    continue;
                }

                const Vector2 Size  = Target.Sizes[Child];
                const Real32  Base  = BaseOf(Along, Next, Size, RoomMain);
                const Real32  Outer = Base + MainOf(Along, Next.Margin);

                if (Shape.Wrap && Count > 0 && Used + Shape.Gap + Outer > RoomMain)
                {
                    Rest = Child;
                    break;
                }

                Used   += (Count > 0 ? Shape.Gap : 0.0f) + Outer;
                Shares += Next.Grow;
                Weight += Next.Shrink * Base;
                Cross   = Max(Cross, (Along ? Size.GetY() : Size.GetX()) + SideOf(Along, Next.Margin));
                ++Count;
            }

            if (Count == 0)
            {
                break;
            }

            // A single line takes the whole room across, and a wrapped one only as much as its tallest child.
            const Real32 LineSide = Shape.Wrap ? Cross : RoomSide;
            const Real32 Free     = Max(RoomMain - Used, 0.0f);

            // A row that overruns shrinks its children, the larger more; a column that overruns is left to overflow.
            const Real32 Deficit  = (Along && Weight > 0.0f) ? Max(Used - RoomMain, 0.0f) : 0.0f;

            // Room grown into is gone, so only what no child grew into is left for the justification to spread.
            Real32 Leading = 0.0f;
            Real32 Between = 0.0f;

            if (Shares <= 0.0f)
            {
                switch (Shape.Content)
                {
                case Justify::Center:
                    Leading = Free * 0.5f;
                    break;
                case Justify::End:
                    Leading = Free;
                    break;
                case Justify::Between:
                    Between = Count > 1 ? Free / static_cast<Real32>(Count - 1) : 0.0f;
                    break;
                default:
                    break;
                }
            }

            Real32 Pen = (Along ? Inner.GetMinimumX() : Inner.GetMinimumY()) + Leading;

            for (UInt32 Child = Cursor; Child < Rest; Child += Target.Spans[Child])
            {
                ConstRef<Box> Next = Target.Boxes[Child];

                if (!mAnimator.IsDrawn(Target, Child) || Next.Placement != Position::Flow)
                {
                    continue;
                }

                const Vector2   Size   = Target.Sizes[Child];
                ConstRef<Edges> Margin = Next.Margin;
                const Real32    Base   = BaseOf(Along, Next, Size, RoomMain);

                Real32 Extent = Base;

                if (Shares > 0.0f && Free > 0.0f)
                {
                    Extent += Free * (Next.Grow / Shares);
                }
                else if (Deficit > 0.0f)
                {
                    Extent = Max(Extent - Deficit * (Next.Shrink * Extent / Weight), 0.0f);
                }

                ConstRef<Length> Across = Along ? Next.Height : Next.Width;
                const Real32     Space  = LineSide - SideOf(Along, Margin);
                const Real32     Wanted = Along ? Size.GetY() : Size.GetX();
                const Real32     Thick  = (Across.IsAuto() && Shape.Items == Align::Stretch)
                    ? Max(Space, 0.0f)
                    : Across.Resolve(RoomSide, Wanted);

                Real32 Offset = 0.0f;

                if (Shape.Items == Align::Center)
                {
                    Offset = (Space - Thick) * 0.5f;
                }
                else if (Shape.Items == Align::End)
                {
                    Offset = Space - Thick;
                }

                const Real32 Start = LineStart + Offset + (Along ? Margin.Top : Margin.Left);
                const Real32 Lead  = Pen + (Along ? Margin.Left : Margin.Top);

                const Rect Placed = Along
                    ? Rect(Lead, Start, Lead + Extent, Start + Thick)
                    : Rect(Start, Lead, Start + Thick, Lead + Extent);

                Target.Frames[Child] = Placed;
                Bottom    = Max(Bottom, Placed.GetMaximumY() + Margin.Bottom);
                Right     = Max(Right, Placed.GetMaximumX() + Margin.Right);
                Pen      += Extent + MainOf(Along, Margin) + Shape.Gap + Between;
            }

            LineStart += LineSide + Shape.Gap;
            Cursor     = Rest;
        }

        // The offset is held within the content now its size is known; moving the placed children moves all they hold.
        if (Roll)
        {
            const Real32 Limit  = Max(Bottom - Inner.GetMinimumY() - Room.GetY(), 0.0f);
            const Real32 LimitX = Roll->Horizontal ? Max(Right - Inner.GetMinimumX() - Padded.GetWidth(), 0.0f) : 0.0f;

            Roll->Overflow  = Bottom - Inner.GetMinimumY();
            Roll->Offset    = Roll->Follow ? Limit : Clamp(Roll->Offset, 0.0f, Limit);
            Roll->OverflowX = Right - Inner.GetMinimumX();
            Roll->OffsetX   = Clamp(Roll->OffsetX, 0.0f, LimitX);
            Roll->View      = Padded.GetSize();

            if (Roll->Offset > 0.0f || Roll->OffsetX > 0.0f)
            {
                const Vector2 Shift(-Roll->OffsetX, -Roll->Offset);

                for (UInt32 Child = Row + 1; Child < End; Child += Target.Spans[Child])
                {
                    if (Target.Boxes[Child].Placement == Position::Flow)
                    {
                        Target.Frames[Child] += Shift;
                    }
                }
            }
        }

        // Elements that stand over the flow are placed against the element's own box, however it scrolled.
        for (UInt32 Child = Row + 1; Child < End; Child += Target.Spans[Child])
        {
            if (mAnimator.IsDrawn(Target, Child) && Target.Boxes[Child].Placement == Position::Absolute)
            {
                Place(Target, Child, Frame);
            }
        }
        return Fitted && Moved;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Layout::Typeset(Ref<Caption> Words, Real32 Width)
    {
        ConstRef<Face> Source = FaceOf(Words.Font);
        const Bool     Cuts   = (Words.Overflow == Overflow::Ellipsis);

        // A line to be cut short is laid out unbroken, asking for its whole text, then cut to the room it was given.
        const Real32 Allowed = Cuts ? 0.0f : Width;
        Words.Extent = Ruler::Wrap(Source, Words.Content, Words.Size, Allowed, Words.Lines, Words.Widths,
                                   Words.Natural);

        if (Cuts && Ruler::Ellipsize(Source, Words.Size, Width, Words.Lines, Words.Widths))
        {
            Words.Natural = kMaximum<Real32>;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Layout::Breadth(Ref<Pane> Target, UInt32 Row) const
    {
        ConstRef<Box> Shape = Target.Boxes[Row];
        const Bool    Along = (Shape.Flow == Direction::Row);
        Real32        Wide  = 0.0f;
        UInt32        Count = 0;

        for (UInt32 Child = Row + 1, End = Row + Target.Spans[Row]; Child < End; Child += Target.Spans[Child])
        {
            ConstRef<Box> Inner = Target.Boxes[Child];

            if (!mAnimator.IsDrawn(Target, Child) || Inner.Placement != Position::Flow)
            {
                continue;
            }

            // Only a width the child sets is its own; one taken as a share of the room cannot widen the room.
            const Real32 Asked = Inner.Width.Measure == Length::Unit::Percent ? 0.0f : Target.Sizes[Child].GetX();
            const Real32 Outer = Asked + Inner.Margin.Left + Inner.Margin.Right;

            Wide = Along ? Wide + Outer + (Count > 0 ? Shape.Gap : 0.0f) : Max(Wide, Outer);
            ++Count;
        }
        return Wide;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Layout::IsFixed(ConstRef<Box> Shape)
    {
        return Shape.Width.Measure == Length::Unit::Pixel && Shape.Height.Measure == Length::Unit::Pixel;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Layout::MainOf(Bool Row, ConstRef<Edges> Margin)
    {
        return Row ? Margin.Left + Margin.Right : Margin.Top + Margin.Bottom;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Layout::SideOf(Bool Row, ConstRef<Edges> Margin)
    {
        return Row ? Margin.Top + Margin.Bottom : Margin.Left + Margin.Right;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Layout::BaseOf(Bool Row, ConstRef<Box> Shape, Vector2 Measured, Real32 Room)
    {
        ConstRef<Length> Size = Shape.Basis.IsAuto() ? (Row ? Shape.Width : Shape.Height) : Shape.Basis;
        return Size.Resolve(Room, Row ? Measured.GetX() : Measured.GetY());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Layout::SpanOf(
        ConstRef<Length> Lead,
        ConstRef<Length> Trail,
        ConstRef<Length> Size,
        Real32           Minimum,
        Real32           Maximum,
        Real32           Wanted,
        Real32           Before,
        Real32           After,
        Ref<Real32>      Start,
        Ref<Real32>      End)
    {
        // Pinned on both sides it stretches between them unless its size is set; the margin pushes it off its edge.
        const Real32 Room   = Maximum - Minimum;
        const Real32 Head   = Lead.Resolve(Room, 0.0f) + Before;
        const Real32 Tail   = Trail.Resolve(Room, 0.0f) + After;
        Real32       Extent = Size.Resolve(Room, Wanted);

        if (Size.IsAuto() && !Lead.IsAuto() && !Trail.IsAuto())
        {
            Extent = Max(Room - Head - Tail, 0.0f);
        }

        Start = (Lead.IsAuto() && !Trail.IsAuto()) ? Maximum - Tail - Extent : Minimum + Head;
        End   = Start + Extent;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Layout::MeasurePicture(Ref<Tree> Owner, ConstRef<Pane> Target, UInt32 Row, Ref<Vector2> Content)
    {
        const ConstPtr<Picture> Image = Owner.Peek<Picture>(Target.Slots[Row]);

        if (!Image->Source)
        {
            return false;
        }

        // A picture asks for the pixels it shows, so a frame of a sheet asks for the frame's size alone.
        if (!Image->Source->HasCompleted())
        {
            return true;
        }

        Content = Vector2(Image->Source->GetWidth(), Image->Source->GetHeight()) * Image->Region.GetSize();
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Layout::Place(Ref<Pane> Target, UInt32 Row, Rect Parent)
    {
        ConstRef<Box> Shape    = Target.Boxes[Row];
        const Vector2 Measured = Target.Sizes[Row];

        Real32 MinimumX, MaximumX, MinimumY, MaximumY;

        SpanOf(Shape.Left, Shape.Right, Shape.Width, Parent.GetMinimumX(), Parent.GetMaximumX(),
            Measured.GetX(), Shape.Margin.Left, Shape.Margin.Right, MinimumX, MaximumX);
        SpanOf(Shape.Top, Shape.Bottom, Shape.Height, Parent.GetMinimumY(), Parent.GetMaximumY(),
            Measured.GetY(), Shape.Margin.Top, Shape.Margin.Bottom, MinimumY, MaximumY);

        Target.Frames[Row] = Rect(MinimumX, MinimumY, MaximumX, MaximumY);
    }
}