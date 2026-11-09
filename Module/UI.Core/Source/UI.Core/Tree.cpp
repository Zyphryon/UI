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

#include "Tree.hpp"
#include "Element.hpp"
#include "Facet/Layout/Box.hpp"
#include "Facet/Visual/Sketch.hpp"
#include "Pane.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Tree::Tree()
        : mFree  { kNone },
          mRaise { 0 },
          mDepth { 0 }
    {
    }

    Handle Tree::Open(Layer Band, UInt64 Name)
    {
        const SInt   Closed = mPanes.Find([](ConstRef<Pane> Entry)
        {
            return !Entry.Open;
        });
        const UInt16 Index  = static_cast<UInt16>(Closed < 0 ? mPanes.GetSize() : static_cast<UInt>(Closed));

        if (Index == mPanes.GetSize())
        {
            ZY_ASSERT(Index < kFree, "An interface holds too many panes");
            mPanes.Append();
        }

        Ref<Pane> Pane = mPanes[Index];
        Pane.Band  = Band;
        Pane.Raise = ++mRaise;
        Pane.Open  = true;

        const UInt32 Slot = Allocate(Index);

        Pane.Widen(0, 1);
        Prime(Pane, 0, Slot, Pane::kNoParent, 0, Name);
        Pane.Whole       = true;
        mSlots[Slot].Row = 0;

        Arrange(Index);
        return GetHandle(Slot);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::Raise(Handle Element)
    {
        ZY_ASSERT(IsAlive(Element), "The element is not alive");

        const UInt16 Index = mSlots[Element.GetSlot()].Pane;
        mPanes[Index].Raise = ++mRaise;

        mOrder.Erase(Index);
        Arrange(Index);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::SetBand(Handle Element, Layer Band)
    {
        ZY_ASSERT(IsAlive(Element), "The element is not alive");

        mPanes[mSlots[Element.GetSlot()].Pane].Band = Band;
        Raise(Element);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Tree::Create(Handle Parent, UInt64 Name, Handle Before)
    {
        ZY_ASSERT(IsAlive(Parent), "The parent is not alive");
        ZY_ASSERT(!Before.IsValid() || GetParent(Before) == Parent,
                  "An element can only go before a child of its parent");

        const UInt32 Slot = Allocate(mSlots[Parent.GetSlot()].Pane);

        Place(Slot, Parent.GetSlot(), Before.IsValid() ? Before.GetSlot() : kNone, Name);
        return GetHandle(Slot);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::Destroy(Handle Element)
    {
        ZY_ASSERT(IsAlive(Element), "The element is not alive");

        if (IsWalking())
        {
            mChanges.Append(Operation::Destroy, Element);
        }
        else
        {
            Erase(Element);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::Move(Handle Element, Handle Parent, Handle Before)
    {
        ZY_ASSERT(IsAlive(Element) && IsAlive(Parent), "The element and its new parent must be alive");

        if (IsWalking())
        {
            mChanges.Append(Operation::Move, Element, Parent, Before);
        }
        else
        {
            Shift(Element, Parent, Before);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::SetName(Handle Element, UInt64 Name)
    {
        ZY_ASSERT(IsAlive(Element), "The element is not alive");

        Locate(Element).Names[mSlots[Element.GetSlot()].Row] = Name;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Tree::GetParent(Handle Element) const
    {
        if (!IsAlive(Element))
        {
            return Handle();
        }

        const UInt32 Parent = Locate(Element).Parents[mSlots[Element.GetSlot()].Row];
        return Parent == Pane::kNoParent ? Handle() : GetHandle(Parent);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Tree::GetFirst(Handle Element) const
    {
        if (!IsAlive(Element))
        {
            return Handle();
        }

        ConstRef<Pane> Pane = Locate(Element);
        const UInt32   Row  = mSlots[Element.GetSlot()].Row;

        return Pane.Spans[Row] > 1 ? GetHandle(Pane.Slots[Row + 1]) : Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Tree::GetNext(Handle Element) const
    {
        if (!IsAlive(Element))
        {
            return Handle();
        }

        ConstRef<Pane> Pane   = Locate(Element);
        const UInt32   Row    = mSlots[Element.GetSlot()].Row;
        const UInt32   Parent = Pane.Parents[Row];

        if (Parent == Pane::kNoParent)
        {
            return Handle();
        }

        // The sibling after it starts where its subtree ends, unless that is where the parent's ends too.
        const UInt32 Above = mSlots[Parent].Row;
        const UInt32 Next  = Row + Pane.Spans[Row];

        return Next < Above + Pane.Spans[Above] ? GetHandle(Pane.Slots[Next]) : Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Tree::GetIndex(Handle Element) const
    {
        const Handle Parent = GetParent(Element);
        UInt32       Index  = 0;

        for (Handle Sibling = GetFirst(Parent); Sibling.IsValid() && Sibling != Element; Sibling = GetNext(Sibling))
        {
            ++Index;
        }
        return Index;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Tree::GetRoot(Handle Element) const
    {
        return IsAlive(Element) ? GetHandle(Locate(Element).Slots[0]) : Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Tree::Find(Handle Scope, UInt64 Name) const
    {
        if (!IsAlive(Scope))
        {
            return Handle();
        }

        ConstRef<Pane>         Pane  = Locate(Scope);
        const UInt32           Row   = mSlots[Scope.GetSlot()].Row;
        const ConstPtr<UInt64> Names = Pane.Names.GetData();

        // The subtree is the run of rows after the scope's own.
        const ConstPtr<UInt64> Hit = ::Find(Names + Row + 1, static_cast<UInt>(Pane.Spans[Row] - 1), Name);
        return Hit ? GetHandle(Pane.Slots[static_cast<UInt32>(Hit - Names)]) : Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::SetState(Handle Element, State Bits, Bool Enabled)
    {
        ZY_ASSERT(IsAlive(Element), "The element is not alive");

        Ref<Pane>    Pane   = Locate(Element);
        const UInt32 Row    = mSlots[Element.GetSlot()].Row;
        const State  Before = Pane.States[Row];
        const State  After  = SetOrClearBit(Before, Bits, Enabled);

        if (After != Before)
        {
            Pane.States[Row] = After;
            Pane.Restyle(Row, Row + 1);

            // The parts of a widget wear its states, but a control it holds wears only its own, as all beneath it does.
            for (UInt32 Cursor = Row + 1, End = Row + Pane.Spans[Row]; Cursor < End;)
            {
                if (mSlots[Pane.Slots[Cursor]].Facets.Test(Facets::Index<Control>()))
                {
                    Cursor += Pane.Spans[Cursor];
                }
                else
                {
                    Pane.Marks[Cursor++] |= Dirty::Style;
                }
            }

            // Hiding takes the element out of the layout, and showing it puts it back.
            if (HasBit(After ^ Before, State::Hidden))
            {
                Pane.Marks[Row] |= Dirty::Measure;
                Pane.Stale       = true;
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::Mark(Handle Element, Dirty Bits)
    {
        ZY_ASSERT(IsAlive(Element), "The element is not alive");

        Ref<Pane> Pane = Locate(Element);
        Pane.Marks[mSlots[Element.GetSlot()].Row] |= Bits;

        if (HasBit(Bits, Dirty::Measure))
        {
            Pane.Stale = true;
        }

        if (HasBit(Bits, Dirty::Style))
        {
            Pane.Styled = true;
        }

        // What it draws may reach elsewhere now, so the next paint works its reach out again.
        if (HasBit(Bits, Dirty::Paint))
        {
            Pane.Touched = true;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::SetStyle(Handle Element, UInt64 Name)
    {
        ZY_ASSERT(IsAlive(Element), "The element is not alive");

        Ref<Pane>    Pane = Locate(Element);
        const UInt32 Row  = mSlots[Element.GetSlot()].Row;

        Pane.Styles[Row] = Name;
        Pane.Restyle(Row, Row + 1);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ref<Box> Tree::EditBox(Handle Element)
    {
        Mark(Element, Dirty::Measure);
        return Locate(Element).Boxes[mSlots[Element.GetSlot()].Row];
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::Leave()
    {
        ZY_ASSERT(mDepth > 0, "A walk ended that never started");

        if (--mDepth == 0)
        {
            Flush();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Tree::Allocate(UInt16 Pane)
    {
        UInt32 Slot = mFree;

        if (Slot != kNone)
        {
            mFree = mSlots[Slot].Row;
        }
        else
        {
            Slot = static_cast<UInt32>(mSlots.GetSize());
            mSlots.Append();
        }

        mSlots[Slot].Row  = kNone;
        mSlots[Slot].Pane = Pane;
        mSlots[Slot].Facets.Reset();
        return Slot;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::Release(UInt32 Slot)
    {
        Ref<Tree::Slot> Entry = mSlots[Slot];

        Entry.Facets.ForEach([this, Slot](UInt Index)
        {
            mStores[Index]->Remove(Slot);
        });

        ++Entry.Generation;
        Entry.Facets.Reset();
        Entry.Pane   = kFree;
        Entry.Row    = mFree;
        mFree        = Slot;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::Place(UInt32 Slot, UInt32 Parent, UInt32 Before, UInt64 Name)
    {
        Ref<Pane>    Pane  = mPanes[mSlots[Parent].Pane];
        const UInt32 Above = mSlots[Parent].Row;
        const UInt32 Row   = Before != kNone ? mSlots[Before].Row : Above + Pane.Spans[Above];

        Pane.Widen(Row, 1);
        Prime(Pane, Row, Slot, Parent, static_cast<UInt16>(Pane.Depths[Above] + 1), Name);

        Renumber(Pane, Row, Pane.GetSize());
        Grow(Pane, Parent, 1);
        Pane.Marks[Above] |= Dirty::Measure;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::Erase(Handle Element)
    {
        const UInt32 Slot   = Element.GetSlot();
        const UInt16 Index  = mSlots[Slot].Pane;
        Ref<Pane>    Pane   = mPanes[Index];
        const UInt32 Row    = mSlots[Slot].Row;
        const UInt32 Count  = Pane.Spans[Row];
        const UInt32 Parent = Pane.Parents[Row];

        for (UInt32 Cursor = Row; Cursor < Row + Count; ++Cursor)
        {
            Release(Pane.Slots[Cursor]);
        }

        // A root takes its whole pane with it.
        if (Parent == Pane::kNoParent)
        {
            Pane.Clear();
            mOrder.Erase(Index);
            return;
        }

        Grow(Pane, Parent, -static_cast<SInt32>(Count));
        Pane.Narrow(Row, Count);
        Renumber(Pane, Row, Pane.GetSize());
        Pane.Marks[mSlots[Parent].Row] |= Dirty::Measure;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::Shift(Handle Element, Handle Parent, Handle Before)
    {
        if (mSlots[Element.GetSlot()].Pane != mSlots[Parent.GetSlot()].Pane)
        {
            Transfer(Element, Parent, Before);
            return;
        }

        Ref<Pane>    Pane  = Locate(Element);
        const UInt32 Row   = mSlots[Element.GetSlot()].Row;
        const UInt32 Count = Pane.Spans[Row];
        const UInt32 Above = mSlots[Parent.GetSlot()].Row;
        const UInt32 Old   = Pane.Parents[Row];

        ZY_ASSERT(Old != Pane::kNoParent, "A root does not move");
        ZY_ASSERT(Above < Row || Above >= Row + Count, "An element cannot move beneath itself");
        ZY_ASSERT(!Before.IsValid() || GetParent(Before) == Parent,
                  "An element can only go before a child of its parent");

        // The rows it lands in front of, counted before anything moves.
        const UInt32 Target = Before.IsValid() ? mSlots[Before.GetSlot()].Row : Above + Pane.Spans[Above];

        if (Before == Element || (Old == Parent.GetSlot() && Target == Row + Count))
        {
            return;
        }

        Grow(Pane, Old, -static_cast<SInt32>(Count));
        Grow(Pane, Parent.GetSlot(), static_cast<SInt32>(Count));

        const SInt32 Depth = static_cast<SInt32>(Pane.Depths[Above]) + 1 - static_cast<SInt32>(Pane.Depths[Row]);

        for (UInt32 Cursor = Row; Cursor < Row + Count; ++Cursor)
        {
            Pane.Depths[Cursor] = static_cast<UInt16>(static_cast<SInt32>(Pane.Depths[Cursor]) + Depth);
        }
        Pane.Parents[Row] = Parent.GetSlot();

        if (Target > Row)
        {
            Pane.Rotate(Row, Row + Count, Target);
            Renumber(Pane, Row, Target);
        }
        else
        {
            Pane.Rotate(Target, Row, Row + Count);
            Renumber(Pane, Target, Row + Count);
        }

        Pane.Marks[mSlots[Old].Row]              |= Dirty::Measure;
        Pane.Marks[mSlots[Parent.GetSlot()].Row] |= Dirty::Measure;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::Transfer(Handle Element, Handle Parent, Handle Before)
    {
        const UInt16 From   = mSlots[Element.GetSlot()].Pane;
        const UInt16 Into   = mSlots[Parent.GetSlot()].Pane;
        Ref<Pane>    Source = mPanes[From];
        Ref<Pane>    Target = mPanes[Into];
        const UInt32 Row    = mSlots[Element.GetSlot()].Row;
        const UInt32 Count  = Source.Spans[Row];
        const UInt32 Old    = Source.Parents[Row];
        const UInt32 Above  = mSlots[Parent.GetSlot()].Row;

        ZY_ASSERT(Old != Pane::kNoParent, "A root does not move");
        ZY_ASSERT(!Before.IsValid() || GetParent(Before) == Parent,
                  "An element can only go before a child of its parent");

        const UInt32 Place = Before.IsValid() ? mSlots[Before.GetSlot()].Row : Above + Target.Spans[Above];
        const SInt32 Depth = static_cast<SInt32>(Target.Depths[Above]) + 1 - static_cast<SInt32>(Source.Depths[Row]);

        Target.Adopt(Place, Source, Row, Count);
        Target.Parents[Place] = Parent.GetSlot();

        // What moved stands under other parents now, so it is dressed, measured and drawn again from scratch.
        for (UInt32 Cursor = Place; Cursor < Place + Count; ++Cursor)
        {
            Target.Depths[Cursor] = static_cast<UInt16>(static_cast<SInt32>(Target.Depths[Cursor]) + Depth);
            Target.Marks[Cursor] |= Dirty::All;
            mSlots[Target.Slots[Cursor]].Pane = Into;
        }

        Renumber(Target, Place, Target.GetSize());
        Grow(Target, Parent.GetSlot(), static_cast<SInt32>(Count));
        Target.Marks[mSlots[Parent.GetSlot()].Row] |= Dirty::Measure;
        Target.Styled = true;
        Target.Whole  = true;

        Grow(Source, Old, -static_cast<SInt32>(Count));
        Source.Narrow(Row, Count);
        Renumber(Source, Row, Source.GetSize());
        Source.Marks[mSlots[Old].Row] |= Dirty::Measure;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::Grow(Ref<Pane> Pane, UInt32 Slot, SInt32 Delta)
    {
        for (UInt32 Cursor = Slot; Cursor != Pane::kNoParent;)
        {
            const UInt32 Row = mSlots[Cursor].Row;

            Pane.Spans[Row] += static_cast<UInt32>(Delta);
            Cursor           = Pane.Parents[Row];
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::Renumber(ConstRef<Pane> Pane, UInt32 First, UInt32 Last)
    {
        for (UInt32 Row = First; Row < Last; ++Row)
        {
            mSlots[Pane.Slots[Row]].Row = Row;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::Arrange(UInt16 Index)
    {
        const SInt Found = mOrder.Find([this, Index](UInt16 Other)
        {
            return mPanes[Other].Band.Order > mPanes[Index].Band.Order;
        });
        mOrder.Insert(Found < 0 ? mOrder.GetSize() : static_cast<UInt>(Found), Index);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::Flush()
    {
        for (UInt Index = 0; Index < mChanges.GetSize(); ++Index)
        {
            const Change Entry = mChanges[Index];

            if (!IsAlive(Entry.Element))
            {
                continue;
            }

            switch (Entry.Kind)
            {
            case Operation::Destroy:
                Erase(Entry.Element);
                break;
            case Operation::Move:
            {
                if (!IsAlive(Entry.Parent))
                {
                    break;
                }

                // A move that would put an element beneath itself, or a root anywhere, became wrong while it waited.
                Ref<Pane>    Pane  = mPanes[mSlots[Entry.Element.GetSlot()].Pane];
                const UInt32 Row   = mSlots[Entry.Element.GetSlot()].Row;
                const UInt32 Above = mSlots[Entry.Parent.GetSlot()].Row;
                const Bool   Same  = mSlots[Entry.Element.GetSlot()].Pane == mSlots[Entry.Parent.GetSlot()].Pane;

                if (Row == 0 || (Same && Above >= Row && Above < Row + Pane.Spans[Row]))
                {
                    break;
                }

                const Bool Stands = IsAlive(Entry.Before) && GetParent(Entry.Before) == Entry.Parent;
                Shift(Entry.Element, Entry.Parent, Stands ? Entry.Before : Handle());
                break;
            }
            }
        }
        mChanges.Clear();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tree::Prime(Ref<Pane> Target, UInt32 Row, UInt32 Slot, UInt32 Parent, UInt16 Depth, UInt64 Name)
    {
        Target.Slots[Row]   = Slot;
        Target.Parents[Row] = Parent;
        Target.Spans[Row]   = 1;
        Target.Depths[Row]  = Depth;
        Target.Names[Row]   = Name;
        Target.States[Row]  = State::None;
        Target.Marks[Row]   = Dirty::All;
        Target.Styles[Row]  = 0;
        Target.Boxes[Row]   = Box();
        Target.Frames[Row]  = Rect();
        Target.Sizes[Row]   = Vector2();
        Target.Rooms[Row]   = Vector2();
        Target.Laid[Row]    = Rect();
        Target.Reaches[Row] = Rect();
        Target.Covers[Row]  = Rect();
        Target.Tapes[Row].Valid = false;
        Target.Styled       = true;
    }
}