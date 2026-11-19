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

#include "Dresser.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Facet/Input/Control.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Dresser::Dresser(Ref<Tree> Owner, Ref<Animator> Animator)
        : mTree     { Owner },
          mAnimator { Animator },
          mTheme    { nullptr },
          mVersion  { 0 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dresser::SetTheme(ConstPtr<Theme> Source)
    {
        mTheme   = Source;

        // Taken as a version behind, so the next update dresses whatever wore the last theme in this one.
        mVersion = Source ? Source->GetVersion() - 1 : 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dresser::Revise()
    {
        const Bool Restyled = mTheme && mTheme->GetVersion() != mVersion;
        mVersion = mTheme ? mTheme->GetVersion() : 0;
        return Restyled;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dresser::Dress(Ref<Pane> Target)
    {
        if (!mTheme || !Target.Styled)
        {
            return;
        }

        Target.Styled = false;

        const UInt32 Size = Target.GetSize();
        mOwners.Clear();

        for (UInt32 Row = 0; Row < Size; ++Row)
        {
            while (!mOwners.IsEmpty() && Row >= mOwners.GetBack().End)
            {
                mOwners.RemoveLast();
            }

            // A control wears its own states, and every part beneath it wears the control's as well.
            const UInt32 Slot      = Target.Slots[Row];
            const Bool   Controls  = mTree.Peek<Control>(Slot) != nullptr;
            const State  Inherited = (Controls || mOwners.IsEmpty()) ? State::None : mOwners.GetBack().Bits;
            const State  Current   = Target.States[Row] | Inherited;

            if (Target.Spans[Row] > 1)
            {
                mOwners.Append(Row + Target.Spans[Row], Current);
            }

            if (!HasBit(Target.Marks[Row], Dirty::Style))
            {
                continue;
            }

            Target.Marks[Row] = ClearBit(Target.Marks[Row], Dirty::Style);

            // Showing or hiding a pane restyles its root, so that is where it opens or closes.
            if (Row == 0)
            {
                Announce(Target);
            }

            Skin       Values;
            const Bool Resolved = Target.Styles[Row] && mTheme->Resolve(Target.Styles[Row], Current, Values);

            // What the last skin gave and this one does not goes back to its default.
            const Skin::Field Dropped = Renew(mTree.GetHandle(Slot), Resolved ? Values.Mask : Skin::Field::None);

            if (Resolved || Dropped != Skin::Field::None)
            {
                Values.Mask |= Dropped;
                Wear(Target, Row, Values);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Sequence<Dresser::Chime> Dresser::Drain()
    {
        return ::Exchange(mChimes, Sequence<Chime>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dresser::Wear(Ref<Pane> Target, UInt32 Row, ConstRef<Skin> Values)
    {
        const UInt32 Slot    = Target.Slots[Row];
        const Handle Element = mTree.GetHandle(Slot);
        const Bool   Dressed = Remember(Element);

        Fit(Target, Row, Values);

        const Skin::Field Given = Values.Mask & Motion::kMoving;

        // An element whose style names nothing that moves takes the values at once.
        if (!mTree.Peek<Motion>(Slot) && !HasAnyBit(Values.Mask, Motion::kDriven))
        {
            Appearance Goal = mAnimator.Capture(Slot);
            Animator::Lay(Goal, Values);
            mAnimator.Apply(Element, Goal, Given);
            return;
        }

        // One never dressed counts as hidden until now, so showing up plays its way in.
        Ref<Motion> State = mAnimator.Hold(Element, !Dressed || Target.IsHidden(Row));
        State.Touched |= Given;
        mAnimator.Stir(Target, Row, Values, State, Dressed);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dresser::Fit(Ref<Pane> Target, UInt32 Row, ConstRef<Skin> Values)
    {
        using Field = Skin::Field;

        const UInt32 Slot = Target.Slots[Row];

        // Only a value that changed lays the element out again, so a style worn again costs no layout.
        Bool Sized = false;

        Ref<Box> Shape = Target.Boxes[Row];

        if (Values.Has(Field::Padding) && Shape.Padding != Values.Padding)
        {
            Shape.Padding = Values.Padding;
            Sized         = true;
        }

        if (Values.Has(Field::Width) && Shape.Width != Values.Width)
        {
            Shape.Width = Values.Width;
            Sized       = true;
        }

        if (Values.Has(Field::Height) && Shape.Height != Values.Height)
        {
            Shape.Height = Values.Height;
            Sized        = true;
        }

        if (Values.Has(Field::Gap) && Shape.Gap != Values.Gap)
        {
            Shape.Gap = Values.Gap;
            Sized     = true;
        }

        if (mAnimator.Tailor(mTree.GetHandle(Slot), Values))
        {
            Sized = true;
        }

        if (Sized)
        {
            Target.Marks[Row] |= Dirty::Measure;
            Target.Stale       = true;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dresser::Remember(Handle Element)
    {
        const UInt32 Slot = Element.GetSlot();

        // Kept by slot with the generation, so an element made in a slot another left is never taken for it.
        mDressed.Expand(Slot + 1);

        const Bool Before = (mDressed[Slot] == Element);
        mDressed[Slot] = Element;
        return Before;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Skin::Field Dresser::Renew(Handle Element, Skin::Field Given)
    {
        const UInt32 Slot = Element.GetSlot();
        mWorn.Expand(Slot + 1);

        // Only a new style or theme drops values, since a slot another element left wore nothing.
        const Bool        Same = Slot < mDressed.GetSize() && mDressed[Slot] == Element;
        const Skin::Field Last = Same ? mWorn[Slot] : Skin::Field::None;

        mWorn[Slot] = Given & ~Motion::kDriven;
        return Last & ~Given;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dresser::Announce(ConstRef<Pane> Target)
    {
        const Handle Root  = mTree.GetHandle(Target.Slots[0]);
        const UInt32 Slot  = Root.GetSlot();
        const Bool   Shown = !Target.IsHidden(0);

        // Kept by slot with the generation, as the dressed are, so a pane opened in a slot another left opens anew.
        mShown.Expand(Slot + 1);

        if ((mShown[Slot] == Root) == Shown)
        {
            return;
        }

        // Played once the panes are dressed, so a hook that changes the tree never changes a pane being walked.
        mShown[Slot] = Shown ? Root : Handle();
        mChimes.Append(Root, Shown ? Cue::Open : Cue::Close);
    }
}