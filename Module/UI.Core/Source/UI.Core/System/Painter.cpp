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

#include "Painter.hpp"
#include "Animator.hpp"
#include "Layout.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Facet/Visual/Backdrop.hpp"
#include "UI.Core/Facet/Visual/Bar.hpp"
#include "UI.Core/Facet/Visual/Caption.hpp"
#include "UI.Core/Facet/Visual/Effect.hpp"
#include "UI.Core/Facet/Visual/Look.hpp"
#include "UI.Core/Facet/Visual/Picture.hpp"
#include "UI.Core/Facet/Visual/Scroll.hpp"
#include "UI.Core/Facet/Visual/Sketch.hpp"
#include "UI.Core/Paint/Canvas.hpp"
#include "UI.Core/Pane.hpp"
#include "UI.Core/Text/Block.hpp"
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

    Painter::Painter(Ref<Tree> Owner, Ref<Animator> Animator, Ref<Layout> Layout)
        : mTree     { Owner },
          mAnimator { Animator },
          mLayout   { Layout },
          mWhole    { false }
    {
        AddDrawer<Effect>(100, Draw::Create<&Painter::DrawHalos>());
        AddDrawer<Look>(200, Draw::Create<&Painter::DrawFill>(this));
        AddDrawer<Effect>(300, Draw::Create<&Painter::DrawGradient>());
        AddDrawer<Backdrop>(400, Draw::Create<&Painter::DrawBackdrop>(this));
        AddDrawer<Picture>(500, Draw::Create<&Painter::DrawPicture>(this));
        AddDrawer<Bar>(600, Draw::Create<&Painter::DrawBar>(this));
        AddDrawer<Sketch>(650, Draw::Create<&Painter::DrawSketch>(this));
        AddDrawer<Effect>(700, Draw::Create<&Painter::DrawInnerGlow>());
        AddDrawer<Look>(800, Draw::Create<&Painter::DrawBorder>());
        AddDrawer<Caption>(900, Draw::Create<&Painter::DrawCaption>(this));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::Paint(Ref<Canvas> Canvas, Ref<Pane> Target)
    {
        // What changed is told by the marks the walk clears, so the parts holding a change are found first.
        Survey(Target);
        Walk(Canvas, Target);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::Survey(ConstRef<Pane> Target)
    {
        mWhole = Target.Moved;
        mDirty.Clear();

        if (mWhole || !Target.Touched)
        {
            return;
        }

        mDirty.Advance(Target.GetSize());

        // Children stand after the row holding them, so walking backward hands every change up to what holds it.
        for (UInt32 Row = Target.GetSize(); Row-- > 0;)
        {
            Bool Changed = HasBit(Target.Marks[Row], Dirty::Paint);

            for (UInt32 Child = Row + 1, End = Row + Target.Spans[Row]; !Changed && Child < End;
                 Child += Target.Spans[Child])
            {
                Changed = mDirty[Child];
            }
            mDirty[Row] = Changed;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Painter::IsBoundary(ConstRef<Pane> Target, UInt32 Row) const
    {
        constexpr UInt32 kLarge = 16;

        if (Row == 0)
        {
            return true;
        }

        const UInt32         Slot  = Target.Slots[Row];
        const ConstPtr<Look> Shade = mTree.Peek<Look>(Slot);
        const Bool           Cuts  = (Shade && Shade->Clip) || mTree.Peek<Scroll>(Slot);

        return Target.Spans[Row] > 1 && (Cuts || Target.Spans[Row] >= kLarge);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Painter::IsKept(ConstRef<Canvas> Canvas, ConstRef<Pane> Target, UInt32 Row, Real32 Opacity) const
    {
        ConstRef<Canvas::Tape>    Kept   = Target.Tapes[Row];
        const ConstPtr<Matrix3x2> Turn   = Canvas.GetTransform();
        const Bool                Moved  = mWhole || (Row < mDirty.GetSize() && mDirty[Row]);
        const Bool                Placed = Kept.Scale == Canvas.GetScale() && Kept.Clip == Canvas.GetClip();
        const Bool                Turned = Kept.Turned == (Turn != nullptr) && (!Turn || Kept.Turn == * Turn);

        return Kept.Valid && !Moved && Placed && Turned && Kept.Opacity == Opacity;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::Replay(Ref<Canvas> Canvas, ConstRef<Pane> Target, UInt32 Row) const
    {
        ZY_PROFILE_SCOPE("UI::Painter::Replay");

        constexpr UInt32 kNested = 0x80000000u;

        // A part replayed inside one being recorded is noted there, so replaying the outer one replays it too.
        const Ptr<Canvas::Tape> Paused = Canvas.Pause(Row | kNested, Target.Tapes[Row].Opacity);

        // A sketch draws anew every frame, and a part held inside replays from its own tape where it stood.
        Canvas.Replay(Target.Tapes[Row], Delegate<void(ConstRef<Canvas::Tape::Live>)>::Create(
            [this, &Canvas, &Target](ConstRef<Canvas::Tape::Live> Part)
            {
                if (Part.Tag & kNested)
                {
                    Replay(Canvas, Target, Part.Tag & ~kNested);
                }
                else
                {
                    Relive(Canvas, Target, Part);
                }
            }));

        Canvas.Resume(Paused);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::Relive(Ref<Canvas> Canvas, ConstRef<Pane> Target, ConstRef<Canvas::Tape::Live> Part) const
    {
        const UInt32 Row = Part.Tag;

        Brush Element;
        Element.Target  = AddressOf(Target);
        Element.Row     = Row;
        Element.Slot    = Target.Slots[Row];
        Element.Frame   = SnapOf(Target.Frames[Row], Canvas.GetScale());
        Element.Inner   = Target.Boxes[Row].Padding.Inset(Element.Frame);
        Element.Opacity = Part.Opacity;
        Element.Shade   = mTree.Peek<Look>(Element.Slot);
        Element.Extra   = mTree.Peek<Effect>(Element.Slot);
        Element.Radius  = Element.Shade ? Element.Shade->Radius : 0.0f;

        DrawSketch(Canvas, Element);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::Walk(Ref<Canvas> Canvas, Ref<Pane> Target)
    {
        ZY_PROFILE_SCOPE("UI::Painter::Walk");

        // What each row's children hide changes only with the pane, so it is worked out again only then.
        if (Target.Moved || Target.Touched)
        {
            Shelter(Target, Canvas.GetScale());
        }
        Trace(Target);

        const UInt32 Size = Target.GetSize();

        for (UInt32 Row = 0; Row < Size; ++Row)
        {
            while (!mScopes.IsEmpty() && Row >= mScopes.GetBack().End)
            {
                Close(Canvas, Target);
            }

            const UInt32 Span = Target.Spans[Row];

            if (!mAnimator.IsDrawn(Target, Row))
            {
                Row += Span - 1;
                continue;
            }

            // A subtree the clip misses is not walked, and text it holds may wait to be laid out until it shows.
            if (!Canvas.IsVisible(Target.Reaches[Row]))
            {
                Target.Marks[Row] |= Dirty::Unseen;
                Row += Span - 1;
                continue;
            }

            // Text held back while it was out of view is laid out before it draws.
            if (HasBit(Target.Marks[Row], Dirty::Unseen))
            {
                Target.Marks[Row] = ClearBit(Target.Marks[Row], Dirty::Unseen);

                if (mLayout.Catch(Target, Row))
                {
                    Retrace(Target, Row);
                }
            }

            // An element faded out draws nothing, and neither does anything it holds.
            const UInt32         Slot    = Target.Slots[Row];
            const ConstPtr<Look> Shade   = mTree.Peek<Look>(Slot);
            const Real32         Above   = mScopes.IsEmpty() ? 1.0f : mScopes.GetBack().Opacity;
            const Real32         Opacity = Clamp(Above * (Shade ? Shade->Opacity : 1.0f), 0.0f, 1.0f);

            if (Opacity <= 0.0f)
            {
                Row += Span - 1;
                continue;
            }

            // A part nothing changed in, drawn under what it was recorded under, is drawn from its tape whole.
            const Bool Bounds = IsBoundary(Target, Row);

            if (Bounds && IsKept(Canvas, Target, Row, Opacity))
            {
                Replay(Canvas, Target, Row);
                Row += Span - 1;
                continue;
            }

            // Otherwise it is recorded as it is walked, noted in the part being recorded around it, until it closes.
            Ptr<Canvas::Tape> Paused = nullptr;

            if (Bounds)
            {
                Paused = Canvas.Pause(Row | 0x80000000u, Opacity);
                Canvas.Record(AddressOf(Target.Tapes[Row]));
                Target.Tapes[Row].Opacity = Opacity;
            }

            // A transform moves the element and all it holds as they are drawn, never where they were laid out.
            const ConstPtr<Effect> Extra       = mTree.Peek<Effect>(Slot);
            const Bool             Transformed = Extra && Extra->IsTransformed();

            if (Transformed)
            {
                Canvas.PushTransform(TransformOf(Target.Frames[Row], * Extra));
            }

            Render(Canvas, Target, Row, Opacity);

            if (Span > 1 || Transformed)
            {
                const Bool Clipped = Span > 1 && ((Shade && Shade->Clip) || mTree.Peek<Scroll>(Slot));

                if (Clipped)
                {
                    Canvas.PushClip(Target.Frames[Row]);
                }
                mScopes.Append(Row, Row + Span, Opacity, Clipped, Transformed, Bounds, Paused);
            }
            else if (Bounds)
            {
                // A part of one row ends with the row, since no scope closes it.
                Canvas.Record(nullptr);
                Canvas.Resume(Paused);
                Target.Tapes[Row].Valid = true;
            }
        }

        while (!mScopes.IsEmpty())
        {
            Close(Canvas, Target);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::Trace(Ref<Pane> Target)
    {
        if (!Target.Moved && !Target.Touched)
        {
            return;
        }

        // After a layout every row is redone, else only marked rows and those holding one; backward, children first.
        const Bool Whole = Target.Moved;

        mRipples.Clear();

        for (UInt32 Row = Target.GetSize(); Row-- > 0;)
        {
            const UInt32 Depth = Target.Depths[Row];

            if (mRipples.GetSize() < Depth + 2)
            {
                mRipples.Advance(Depth + 2 - mRipples.GetSize());
            }

            // The children left their ripple one level down; one leaving their union as it was asks nothing.
            Ref<Ripple> Below = mRipples[Depth + 1];
            Bool        Asked = false;

            if (Below.Raised)
            {
                Asked        = Whole || Spills(Target, Row, Below.Before, Below.After);
                Below.Raised = false;
            }

            if (!Whole && !Asked && !HasBit(Target.Marks[Row], Dirty::Paint))
            {
                continue;
            }

            Bool       Turned = false;
            const Rect Before = Target.Reaches[Row];
            const Rect After  = Reach(Target, Row, Turned);

            // Whether its own transform moved the reach is kept, so children are held against it without a lookup.
            Target.Marks[Row]   = SetOrClearBit(ClearBit(Target.Marks[Row], Dirty::Paint), Dirty::Turned, Turned);
            Target.Reaches[Row] = After;

            if (After.GetMinimumX() != Before.GetMinimumX() || After.GetMinimumY() != Before.GetMinimumY()
                || After.GetMaximumX() != Before.GetMaximumX() || After.GetMaximumY() != Before.GetMaximumY())
            {
                // The first child to change starts the unions, which hold nothing from the last row they were left for.
                Ref<Ripple> Above = mRipples[Depth];
                Above.Before = Above.Raised ? Rect::Union(Above.Before, Before) : Before;
                Above.After  = Above.Raised ? Rect::Union(Above.After, After) : After;
                Above.Raised = true;
            }
        }

        Target.Moved   = false;
        Target.Touched = false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::Retrace(Ref<Pane> Target, UInt32 Row)
    {
        for (UInt32 Cursor = Row + Target.Spans[Row]; Cursor-- > Row;)
        {
            Bool Turned = false;
            Target.Reaches[Cursor] = Reach(Target, Cursor, Turned);
            Target.Marks[Cursor]   = SetOrClearBit(Target.Marks[Cursor], Dirty::Turned, Turned);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Rect Painter::Reach(ConstRef<Pane> Target, UInt32 Row, Ref<Bool> Turned) const
    {
        Turned = false;

        // Nothing hidden draws, so it reaches nowhere and leaves the box of what holds it as it was.
        if (!mAnimator.IsDrawn(Target, Row))
        {
            return Rect::Invalid();
        }

        const UInt32           Slot  = Target.Slots[Row];
        const UInt32           Span  = Target.Spans[Row];
        const ConstPtr<Effect> Extra = mTree.Peek<Effect>(Slot);
        const ConstPtr<Look>   Shade = mTree.Peek<Look>(Slot);

        Rect Result = ReachOf(Target, Row, Extra, mTree.Peek<Caption>(Slot));

        // Children cut to the box never reach past it, and the box is already within what the element draws.
        if (Span > 1 && !(Shade && Shade->Clip) && !mTree.Peek<Scroll>(Slot))
        {
            for (UInt32 Child = Row + 1, End = Row + Span; Child < End; Child += Target.Spans[Child])
            {
                Result = Rect::Union(Result, Target.Reaches[Child]);
            }
        }

        if (Extra && Extra->IsTransformed())
        {
            Result = Rect::Transform(Result, TransformOf(Target.Frames[Row], * Extra));
            Turned = true;
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::Render(Ref<Canvas> Canvas, ConstRef<Pane> Target, UInt32 Row, Real32 Opacity) const
    {
        Brush Element;
        Element.Target  = AddressOf(Target);
        Element.Row     = Row;
        Element.Slot    = Target.Slots[Row];
        Element.Frame   = SnapOf(Target.Frames[Row], Canvas.GetScale());
        Element.Inner   = Target.Boxes[Row].Padding.Inset(Element.Frame);
        Element.Opacity = Opacity;
        Element.Shade   = mTree.Peek<Look>(Element.Slot);
        Element.Extra   = mTree.Peek<Effect>(Element.Slot);
        Element.Radius  = Element.Shade ? Element.Shade->Radius : 0.0f;

        // What it holds is drawn as faded as it is, so a cover only hides what lies under it at full opacity.
        Element.Cover   = Opacity >= 1.0f ? Target.Covers[Row] : Rect();

        // Every stage of drawing whose kind of facet the element holds runs, in order, the interface's own among them.
        ConstRef<Mask> Held = mTree.GetMask(mTree.GetHandle(Element.Slot));

        for (ConstRef<Drawer> Each : mDrawers)
        {
            if (Held.Test(Each.Facet))
            {
                Each.Code(Canvas, Element);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::Shelter(Ref<Pane> Target, Real32 Scale)
    {
        // After a layout every row is redone, else only marked rows and those holding a child whose cover changed.
        const Bool Whole = Target.Moved;

        mRaised.Clear();
        mRaised.Advance(Target.GetSize());

        // Children stand after the row holding them, so walking backward finds every child's cover before its parent.
        for (UInt32 Row = Target.GetSize(); Row-- > 0;)
        {
            const Bool Marked = HasBit(Target.Marks[Row], Dirty::Paint);
            Bool       Due    = Whole || Marked;

            for (UInt32 Child = Row + 1, End = Row + Target.Spans[Row]; !Due && Child < End;
                 Child += Target.Spans[Child])
            {
                Due = mRaised[Child];
            }

            if (!Due)
            {
                continue;
            }

            Rect Best;

            for (UInt32 Child = Row + 1, End = Row + Target.Spans[Row]; Child < End; Child += Target.Spans[Child])
            {
                const UInt32           Slot  = Target.Slots[Child];
                const ConstPtr<Look>   Shade = mTree.Peek<Look>(Slot);
                const ConstPtr<Effect> Extra = mTree.Peek<Effect>(Slot);

                const Bool Altered = (Shade && Shade->Opacity < 1.0f) || (Extra && Extra->IsTransformed());

                if (!mAnimator.IsDrawn(Target, Child) || Altered)
                {
                    continue;
                }

                // A child cutting what it holds to its box hides nothing past it.
                const Bool Cuts  = (Shade && Shade->Clip) || mTree.Peek<Scroll>(Slot);
                const Rect Below = Cuts ? Rect::Intersection(Target.Covers[Child], Target.Frames[Child])
                                       : Target.Covers[Child];

                Best = LargerOf(Best, LargerOf(CoverOf(Target, Child, Scale), Below));
            }

            const UInt32         Slot  = Target.Slots[Row];
            const ConstPtr<Look> Shade = mTree.Peek<Look>(Slot);
            const Bool           Cuts  = (Shade && Shade->Clip) || mTree.Peek<Scroll>(Slot);
            const Rect           Found = Cuts ? Rect::Intersection(Best, Target.Frames[Row]) : Best;

            // What holds the row works its cover out again when the row's own paint changed, or what it holds hides.
            mRaised[Row]       = Marked || Found != Target.Covers[Row];
            Target.Covers[Row] = Found;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Rect Painter::CoverOf(ConstRef<Pane> Target, UInt32 Row, Real32 Scale) const
    {
        const UInt32            Slot  = Target.Slots[Row];
        const Rect              Frame = Target.Frames[Row];
        const ConstPtr<Look>    Shade = mTree.Peek<Look>(Slot);
        const ConstPtr<Effect>  Extra = mTree.Peek<Effect>(Slot);
        const ConstPtr<Picture> Image = mTree.Peek<Picture>(Slot);

        const Rect Filled = HoleOf(Frame, Shade, Extra, Shade ? Shade->Opacity : 1.0f, Scale);

        // A picture covers its box when it shows its colour alone, untinted and stretched over all of it.
        const Bool Ready = Image && (Image->Source ? Image->Source->HasCompleted() : Image->Texture != 0);

        if (!Ready || !Image->Opaque || !Image->Tint.IsOpaque() || Image->Fit != Fit::Stretch)
        {
            return Filled;
        }

        const Real32 Edge  = 1.0f / Max(Scale, 1.0e-6f);
        const Real32 Side  = (Shade ? Max(Shade->Radius, 0.0f) : 0.0f) + Edge;
        const Rect   Shown = Frame.Contract(Side, Edge);

        return LargerOf(Filled, Shown.IsEmpty() ? Rect() : Shown);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::Close(Ref<Canvas> Canvas, Ref<Pane> Target)
    {
        const Scope Ending = mScopes.GetBack();
        mScopes.RemoveLast();

        const ConstPtr<Scroll> Roll = mTree.Peek<Scroll>(Target.Slots[Ending.Row]);

        if (Roll && Roll->Thumb.GetAlpha() > 0.0f)
        {
            const Rect   Frame = Target.Frames[Ending.Row];
            const Rect   Inner = Target.Boxes[Ending.Row].Padding.Inset(Frame);
            const Color  Tint  = Fade(Roll->Thumb, Ending.Opacity);
            const Real32 Wide  = Roll->View.GetX();

            // The thumb stands where the pointer grabs it, as tall to the track as the room is to the content.
            if (const Rect Knob = Roll->GetThumb(Frame, Inner.GetMinimumY()); !Knob.IsEmpty())
            {
                Canvas.DrawRect(Knob, Tint, 2.0f);
            }

            // One scrolling sideways shows a thumb along its bottom edge the same way, over the content.
            if (Roll->Horizontal && Roll->OverflowX > Wide + 0.5f)
            {
                const Real32 Size   = Max(Wide * Wide / Roll->OverflowX, 16.0f);
                const Real32 Travel = Wide - Size;
                const Real32 Left   = Inner.GetMinimumX() + Travel * (Roll->OffsetX / (Roll->OverflowX - Wide));
                const Real32 Top    = Frame.GetMaximumY() - 6.0f;

                Canvas.DrawRect(Rect(Left, Top, Left + Size, Top + 4.0f), Tint, 2.0f);
            }
        }

        if (Ending.Clipped)
        {
            Canvas.PopClip();
        }

        // The transform was pushed before the clip, so it comes off after it.
        if (Ending.Transformed)
        {
            Canvas.PopTransform();
        }

        // A part being recorded ends with its scope, its scroll thumbs included, and the part around it records on.
        if (Ending.Recording)
        {
            Canvas.Record(nullptr);
            Canvas.Resume(Ending.Paused);
            Target.Tapes[Ending.Row].Valid = true;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::DrawFill(Ref<Canvas> Canvas, ConstRef<Brush> Element)
    {
        ConstRef<Look> Shade = * Element.Shade;

        // The background shows through a gradient that is not opaque, as one fading in or out over a transition.
        const Bool Covered = Element.Extra && Element.Extra->Fill.From.IsOpaque() && Element.Extra->Fill.To.IsOpaque();

        if (!Covered && Shade.Background.GetAlpha() > 0.0f)
        {
            Canvas.DrawRect(Element.Frame, Fade(Shade.Background, Element.Opacity), Element.Radius, Element.Cover);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::DrawBackdrop(Ref<Canvas> Canvas, ConstRef<Brush> Element) const
    {
        // A theme's image lies over the fill and under the picture.
        const ConstPtr<Backdrop> Layer = mTree.Peek<Backdrop>(Element.Slot);
        Stamp(Canvas, Element, Layer->Source, Layer->Region, Layer->Slice, Layer->Tint);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::DrawPicture(Ref<Canvas> Canvas, ConstRef<Brush> Element) const
    {
        const ConstPtr<Picture> Image = mTree.Peek<Picture>(Element.Slot);

        Canvas::Surface Texture {
            .Handle = Image->Texture,
            .Layout = Image->Layout,
            .Layer  = Image->Layer,
            .Filter = Image->Filter,
            .Opaque = Image->Opaque,
            .Flip   = Image->Flip
        };
        Vector2 Size;

        // An image of the content waits for its load, while a texture of the GPU stands ready once it is given.
        if (ConstRef<Retainer<ZyGraphic::Image>> Source = Image->Source)
        {
            if (!Source->HasCompleted())
            {
                return;
            }

            Texture.Handle = Source->GetHandle();
            Texture.Layout = Source->GetLayout();
            Size           = Vector2(Source->GetWidth(), Source->GetHeight());
        }
        else if (!Image->Texture)
        {
            return;
        }

        const Color Tint = Fade(Image->Tint, Element.Opacity);
        const Rect  Box  = FitOf(Element.Frame, Size * Image->Region.GetSize(), Image->Fit);

        // A texture of no known size has no border to keep, so only an image is ever sliced.
        if (Image->Slice.IsEmpty() || !Image->Source)
        {
            Canvas.DrawTexture(Texture, Box, Image->Region, Tint, Element.Radius);
        }
        else
        {
            Canvas.DrawSliced(Texture, Size, Box, Image->Region, Image->Slice, Tint);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::DrawSketch(Ref<Canvas> Canvas, ConstRef<Brush> Element) const
    {
        ZY_PROFILE_SCOPE("UI::Painter::Sketch");

        const ConstPtr<Sketch> Drawing = mTree.Peek<Sketch>(Element.Slot);

        if (Drawing->Code.IsEmpty())
        {
            return;
        }

        // What a sketch draws is never recorded; the pane's tape notes where it stands and a replay draws it anew.
        const Ptr<Canvas::Tape> Paused = Canvas.Pause(Element.Row, Element.Opacity);

        if (Drawing->Clip)
        {
            Canvas.PushClip(Element.Frame);
        }

        Drawing->Code(Canvas, Element);

        if (Drawing->Clip)
        {
            Canvas.PopClip();
        }

        Canvas.Resume(Paused);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::DrawBar(Ref<Canvas> Canvas, ConstRef<Brush> Element) const
    {
        const ConstPtr<Bar> Fill = mTree.Peek<Bar>(Element.Slot);

        if (Fill->Value <= 0.0f)
        {
            return;
        }

        const Rect Filled = Rect::Within(Element.Inner, Rect(0.0f, 0.0f, Clamp(Fill->Value, 0.0f, 1.0f), 1.0f));

        Canvas.DrawRect(Filled, Fade(Fill->Fill, Element.Opacity), Element.Radius);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::DrawCaption(Ref<Canvas> Canvas, ConstRef<Brush> Element) const
    {
        const Ptr<Caption> Text = mTree.Peek<Caption>(Element.Slot);

        if (Text->Font && Text->Font->HasCompleted())
        {
            Write(Canvas, * Text, mLayout.FaceOf(Text->Font), Element.Inner, Element.Opacity);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Color Painter::Fade(Color Tint, Real32 Opacity)
    {
        return Color::WithAlpha(Tint, Tint.GetAlpha() * Opacity);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Gradient Painter::Fade(ConstRef<Gradient> Fill, Real32 Opacity)
    {
        Gradient Result = Fill;
        Result.From = Fade(Fill.From, Opacity);
        Result.To   = Fade(Fill.To, Opacity);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Matrix3x2 Painter::TransformOf(Rect Frame, ConstRef<Effect> Extra)
    {
        const Vector2 Middle = Frame.GetCenter();

        // Scaled and turned about the middle, then moved.
        return Matrix3x2::FromTransform(Middle, Middle + Extra.Translate, Extra.Scale,
                                        Angle::FromDegrees(Extra.Rotate));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::Cast(Ref<Canvas> Canvas, Rect Box, ConstRef<Halo> Source, Real32 Radius, Real32 Opacity, Rect Hole)
    {
        // A negative spread shrinks the box, never past its own middle.
        const Real32 MinimumX = Box.GetMinimumX() - Source.Spread;
        const Real32 MinimumY = Box.GetMinimumY() - Source.Spread;
        const Rect   Grown(MinimumX, MinimumY,
            Max(Box.GetMaximumX() + Source.Spread, MinimumX), Max(Box.GetMaximumY() + Source.Spread, MinimumY));

        Canvas.DrawShadow(Grown, Fade(Source.Tint, Opacity), Max(Radius + Source.Spread, 0.0f), Source.Blur, Hole);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::Stamp(
        Ref<Canvas>                          Canvas,
        ConstRef<Brush>                      Element,
        ConstRef<Retainer<ZyGraphic::Image>> Source,
        Rect                                 Region,
        ConstRef<Edges>                      Slice,
        Color                                Tint)
    {
        if (!Source || !Source->HasCompleted())
        {
            return;
        }

        // Only a stretched image can follow the corners.
        if (Slice.IsEmpty())
        {
            Canvas.DrawImage(* Source, Element.Frame, Region, Fade(Tint, Element.Opacity), Element.Radius);
        }
        else
        {
            Canvas.DrawSliced(* Source, Element.Frame, Region, Slice, Fade(Tint, Element.Opacity));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Rect Painter::FitOf(Rect Frame, Vector2 Size, Fit Mode)
    {
        // A picture of no known size, or one asked to stretch, covers the whole frame.
        if (Mode == Fit::Stretch || Size.GetX() <= 0.0f || Size.GetY() <= 0.0f)
        {
            return Frame;
        }

        const Real32  Scale  = Min(Frame.GetWidth() / Size.GetX(), Frame.GetHeight() / Size.GetY());
        const Vector2 Extent = Size * Max(Scale, 0.0f);
        const Vector2 Corner = Frame.GetCenter() - Extent * 0.5f;

        return Rect(Corner, Corner + Extent);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Rect Painter::HoleOf(Rect Frame, ConstPtr<Look> Shade, ConstPtr<Effect> Extra, Real32 Opacity, Real32 Scale)
    {
        const Bool Gradient = Extra && Extra->Fill.From.IsOpaque() && Extra->Fill.To.IsOpaque();
        const Bool Solid    = Shade && Shade->Background.IsOpaque();

        if (Opacity < 1.0f || !(Gradient || Solid))
        {
            return Rect();
        }

        // The rounded corners and a pixel of fading edge are left to show the halo, so only what is solid is cut.
        const Real32 Edge = 1.0f / Max(Scale, 1.0e-6f);
        const Real32 Side = (Shade ? Max(Shade->Radius, 0.0f) : 0.0f) + Edge;
        const Rect   Hole = Frame.Contract(Side, Edge);

        return Hole.IsEmpty() ? Rect() : Hole;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Rect Painter::SnapOf(Rect Frame, Real32 Scale)
    {
        // Each edge is moved on its own, so boxes sharing an edge still meet, and none moves more than half a pixel.
        const Real32 Pixel = Max(Scale, 1.0e-6f);

        return Rect(
            Round(Frame.GetMinimumX() * Pixel) / Pixel, Round(Frame.GetMinimumY() * Pixel) / Pixel,
            Round(Frame.GetMaximumX() * Pixel) / Pixel, Round(Frame.GetMaximumY() * Pixel) / Pixel);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Rect Painter::LargerOf(Rect First, Rect Second)
    {
        const Real32 Area   = First.IsEmpty() ? 0.0f : First.GetWidth() * First.GetHeight();
        const Real32 Other  = Second.IsEmpty() ? 0.0f : Second.GetWidth() * Second.GetHeight();

        return Other > Area ? Second : (Area > 0.0f ? First : Rect());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Painter::OverhangOf(ConstRef<Caption> Words)
    {
        return Words.Size * 0.5f + Max(Words.StrokeWidth, 0.0f)
            + Vector2::Abs(Words.ShadowOffset).GetMaxComponent();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Rect Painter::ReachOf(ConstRef<Pane> Target, UInt32 Row, ConstPtr<Effect> Extra, ConstPtr<Caption> Words)
    {
        const Rect Frame = Target.Frames[Row];
        Rect       Reach = Frame;

        // A halo grows by its spread and then fades over its blur, never less than the half pixel any edge fades over.
        if (Extra && Extra->Shadow.IsVisible())
        {
            const Real32 Grow = Max(Extra->Shadow.Spread, 0.0f) + Max(Extra->Shadow.Blur, 0.5f);
            Reach = Rect::Union(Reach, (Frame + Extra->Shadow.Offset).Expand(Grow));
        }

        if (Extra && Extra->Glow.IsVisible())
        {
            Reach = Rect::Union(Reach, Frame.Expand(Max(Extra->Glow.Spread, 0.0f) + Max(Extra->Glow.Blur, 0.5f)));
        }

        // Text is centred down its box and may outgrow it, as a label too narrow for a word that cannot break.
        if (Words)
        {
            const Rect    Inner = Target.Boxes[Row].Padding.Inset(Frame);
            const Real32  Left  = Ruler::Lead(Words->Alignment, Inner, Words->Extent.GetX());
            const Real32  Top   = Inner.GetMinimumY() + (Inner.GetHeight() - Words->Extent.GetY()) * 0.5f;
            const Vector2 Corner(Left, Top);
            const Rect    Block(Corner, Corner + Words->Extent);

            Reach = Rect::Union(Reach, Block.Expand(OverhangOf(* Words)));
        }
        return Reach;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::Typeset(Ref<Caption> Text, ConstRef<Face> Source)
    {
        Text.Glyphs.Clear();
        Text.Runs.Clear();

        const ZyBase::Text Lines = Text.Lines;

        // A glyph per byte at most, so the placements are made room for once.
        Text.Glyphs.Reserve(Lines.GetSize());

        Real32 Pen      = 0.0f;
        UInt32 Previous = 0;
        UInt   Before   = 0;

        // Each line is placed from its own start, so a centred or right aligned block can set every line apart.
        for (UInt Cursor = 0; Cursor < Lines.GetSize();)
        {
            const UInt32 Codepoint = Unicode::Decode(Lines, Cursor);

            if (Codepoint == '\n' || Codepoint == '\r')
            {
                if (Codepoint == '\n')
                {
                    Text.Runs.Append(static_cast<UInt32>(Text.Glyphs.GetSize() - Before));
                    Before = Text.Glyphs.GetSize();
                }

                Pen      = 0.0f;
                Previous = 0;
                continue;
            }

            const ConstPtr<ZyRender::Font::Glyph> Glyph = Source.GetGlyph(Codepoint);

            if (!Glyph)
            {
                continue;
            }

            if (Previous)
            {
                Pen += Source.GetKerning(Previous, Codepoint);
            }

            const Rect Placed = Glyph->Local + Vector2(Pen, 0.0f);
            Text.Glyphs.Append(Placed, Glyph->Atlas, Glyph->Page);

            Pen     += Glyph->Advance;
            Previous = Codepoint;
        }

        Text.Runs.Append(static_cast<UInt32>(Text.Glyphs.GetSize() - Before));
        Text.Shaped = true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::Write(Ref<Canvas> Canvas, Ref<Caption> Text, ConstRef<Face> Source, Rect Inner, Real32 Opacity)
    {
        ConstRef<ZyRender::Font> Font = * Text.Font;

        if (!Text.Shaped)
        {
            Typeset(Text, Source);
        }

        const Real32 Leading = Font.GetLineHeight(Text.Size);
        const Real32 Top     = Inner.GetMinimumY() + (Inner.GetHeight() - Text.Extent.GetY()) * 0.5f;
        const Bool   Shadow  = Text.Shadow.GetAlpha() > 0.0f;

        const ZyRender::FontEffect Effect = (Text.StrokeWidth > 0.0f && Text.Stroke.GetAlpha() > 0.0f)
            ? ZyRender::FontEffect(Fade(Text.Stroke, Opacity).ToColor8(), 0.0f, 0.0f, Text.StrokeWidth, 0.0f, 1.0f,
                                   0.5f)
            : ZyRender::FontEffect();

        // The element as a whole was found in view, so only a caption of several lines has lines worth testing.
        const Bool   Several  = Text.Runs.GetSize() > 1;
        const Real32 Overhang = Several ? OverhangOf(Text) : 0.0f;

        // Each line is set on its own, so a centred or right aligned block aligns every line and not only its box.
        const ConstSpan Glyphs(Text.Glyphs.GetData(), Text.Glyphs.GetSize());

        for (UInt32 Line = 0, First = 0; Line < Text.Runs.GetSize(); First += Text.Runs[Line], ++Line)
        {
            const ConstSpan Row = Glyphs.Slice(First, Text.Runs[Line]);

            const Real32  Width  = Line < Text.Widths.GetSize() ? Text.Widths[Line] : 0.0f;
            const Vector2 Origin = Vector2(Ruler::Lead(Text.Alignment, Inner, Width), Top + Leading * Line);

            const Bool Seen = !Several
                || Canvas.IsVisible(Rect(Origin, Origin + Vector2(Width, Leading)).Expand(Overhang));

            if (!Row.IsEmpty() && Seen)
            {
                if (Shadow)
                {
                    Canvas.DrawGlyphs(Font, Row, Origin + Text.ShadowOffset, Text.Size, Fade(Text.Shadow, Opacity));
                }

                Canvas.DrawGlyphs(Font, Row, Origin, Text.Size, Fade(Text.Tint, Opacity), Effect);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Painter::Spills(ConstRef<Pane> Target, UInt32 Row, Rect Before, Rect After)
    {
        // Boxes that touched none of the edges of the union and still stand within it leave the union as it was.
        const Rect Outer = Target.Reaches[Row];

        const Bool Inside = Before.GetMinimumX() > Outer.GetMinimumX() && Before.GetMinimumY() > Outer.GetMinimumY()
            && Before.GetMaximumX() < Outer.GetMaximumX() && Before.GetMaximumY() < Outer.GetMaximumY();
        const Bool Stays  = Outer.Contains(After);

        // A row whose reach its own transform moved cannot have its children's boxes held against it.
        return !(Inside && Stays) || HasBit(Target.Marks[Row], Dirty::Turned);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::DrawHalos(Ref<Canvas> Canvas, ConstRef<Brush> Element)
    {
        ConstRef<Effect> Extra = * Element.Extra;

        // A shadow is moved and grown with the box, a glow only grown; what the fill hides of either is never drawn.
        if (!Extra.Shadow.IsVisible() && !Extra.Glow.IsVisible())
        {
            return;
        }

        // As a browser casts one, a shadow or glow lies only outside its element's box, less its corners and edge.
        const Real32 Edge = 1.0f / Max(Canvas.GetScale(), 1.0e-6f);
        const Rect   Box  = Element.Frame.Contract(Max(Element.Radius, 0.0f) + Edge, Edge);
        const Rect   Hole = LargerOf(Box.IsEmpty() ? Rect() : Box, Element.Cover);

        if (Extra.Shadow.IsVisible())
        {
            Cast(Canvas, Element.Frame + Extra.Shadow.Offset, Extra.Shadow, Element.Radius, Element.Opacity, Hole);
        }

        if (Extra.Glow.IsVisible())
        {
            Cast(Canvas, Element.Frame, Extra.Glow, Element.Radius, Element.Opacity, Hole);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::DrawGradient(Ref<Canvas> Canvas, ConstRef<Brush> Element)
    {
        if (Element.Extra->Fill.IsVisible())
        {
            Canvas.DrawGradient(Element.Frame, Fade(Element.Extra->Fill, Element.Opacity), Element.Radius,
                                Element.Cover);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::DrawInnerGlow(Ref<Canvas> Canvas, ConstRef<Brush> Element)
    {
        ConstRef<Halo> Glow = Element.Extra->InnerGlow;

        if (Glow.IsVisible())
        {
            Canvas.DrawInnerGlow(Element.Frame, Fade(Glow.Tint, Element.Opacity), Element.Radius, Glow.Blur);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Painter::DrawBorder(Ref<Canvas> Canvas, ConstRef<Brush> Element)
    {
        ConstRef<Look> Shade = * Element.Shade;

        if (Shade.BorderWidth > 0.0f && Shade.Border.GetAlpha() > 0.0f)
        {
            Canvas.DrawFrame(Element.Frame, Fade(Shade.Border, Element.Opacity), Shade.BorderWidth, Element.Radius);
        }
    }
}