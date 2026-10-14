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

#include "Facet.hpp"
#include "Facet/Layout/Box.hpp"
#include "Handle.hpp"
#include "Pane.hpp"
#include "Service.hpp"
#include "Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the elements of an interface, with the panes they stand in, their slots and their facets.
    class Tree
    {
    public:

        /// \brief Constructs a tree holding no pane.
        Tree();

        /// \brief Deletes the copy, since facets and handles name slots of this tree alone.
        Tree(ConstRef<Tree> Other) = delete;

        /// \brief Opens a pane holding a root, drawn over every pane of its band opened before it.
        ///
        /// \param Band The band the pane is drawn in.
        /// \param Name The identifier the root is found by, or zero for none.
        /// \return The root of the new pane, which every element of the pane is made beneath.
        Handle Open(Layer Band, UInt64 Name = 0);

        /// \brief Draws a pane over every other of its band.
        ///
        /// \param Element The element whose pane is raised, any one of the pane.
        void Raise(Handle Element);

        /// \brief Moves a pane to another band, over every pane of it.
        ///
        /// \param Element The element whose pane is moved, any one of the pane.
        /// \param Band    The band the pane is drawn in from now on.
        void SetBand(Handle Element, Layer Band);

        /// \brief Gets the band the pane an element stands in is drawn in.
        ///
        /// \param Element The element whose pane is asked about, which must be alive.
        /// \return The band of the pane.
        ZY_INLINE Layer GetBand(Handle Element) const
        {
            return mPanes[mSlots[Element.GetSlot()].Pane].Band;
        }

        /// \brief Creates an element as a child of another, standing just before one of its children or last.
        ///
        /// \param Parent The element to hold it, which must be alive.
        /// \param Name   The identifier it is found by, or zero for none.
        /// \param Before The child of the parent it goes before, or one naming nothing to make it the last.
        /// \return The element, which stands in the tree once the running walk ends when there is one.
        Handle Create(Handle Parent, UInt64 Name = 0, Handle Before = Handle());

        /// \brief Destroys an element and everything it holds, closing the pane when it is a root.
        ///
        /// \param Element The element to destroy, which must be alive.
        void Destroy(Handle Element);

        /// \brief Moves an element and everything it holds under a parent, just before one of its children or last.
        ///
        /// \param Element The element to move, which must be alive and not a root.
        /// \param Parent  The new parent, which must not stand beneath the element.
        /// \param Before  The child of the parent it goes before, or one naming nothing to make it the last.
        void Move(Handle Element, Handle Parent, Handle Before = Handle());

        /// \brief Checks whether an element is alive, standing in the tree or waiting to.
        ///
        /// \param Element The element asked about, which may be stale or name nothing.
        /// \return `true` if it is, `false` otherwise.
        ZY_INLINE Bool IsAlive(Handle Element) const
        {
            const UInt32 Slot = Element.GetSlot();

            return Element.IsValid()
                && Slot < mSlots.GetSize()
                && mSlots[Slot].Pane != kFree
                && mSlots[Slot].Generation == Element.GetGeneration();
        }

        /// \brief Checks whether an element waits for the running walk to end before it stands in the tree.
        ///
        /// \param Element The element asked about, which must be alive.
        /// \return `true` if it does, `false` otherwise.
        ZY_INLINE Bool IsPending(Handle Element) const
        {
            return mSlots[Element.GetSlot()].Row == kPending;
        }

        /// \brief Changes the identifier an element is found by.
        ///
        /// \param Element The element to rename, which must be alive.
        /// \param Name    The identifier, or zero for none.
        void SetName(Handle Element, UInt64 Name);

        /// \brief Gets the identifier an element is found by.
        ///
        /// \param Element The element asked about, which must be alive.
        /// \return The identifier, or zero for none.
        ZY_INLINE UInt64 GetName(Handle Element) const
        {
            return IsPending(Element) ? Sketch(Element).Name : Locate(Element).Names[mSlots[Element.GetSlot()].Row];
        }

        /// \brief Gets the element holding another.
        ///
        /// \param Element The element whose parent is asked for, which may be dead.
        /// \return The parent, or one naming nothing for a root or a dead element.
        Handle GetParent(Handle Element) const;

        /// \brief Gets the first element another holds.
        ///
        /// \param Element The element whose first child is asked for, which may be dead.
        /// \return The first child, or one naming nothing when it holds none.
        Handle GetFirst(Handle Element) const;

        /// \brief Gets the element standing after another under the same parent.
        ///
        /// \param Element The element whose next sibling is asked for, which may be dead.
        /// \return The next sibling, or one naming nothing for the last.
        Handle GetNext(Handle Element) const;

        /// \brief Gets the place of an element among its siblings.
        ///
        /// \param Element The element whose place is asked for.
        /// \return The place, counted from its parent's first child.
        UInt32 GetIndex(Handle Element) const;

        /// \brief Gets the root of the pane an element stands in.
        ///
        /// \param Element The element whose root is asked for, which may be dead or waiting.
        /// \return The root, or one naming nothing for a dead element.
        Handle GetRoot(Handle Element) const;

        /// \brief Finds the first element by an identifier among those another holds, however deep, in preorder.
        ///
        /// \param Scope The element to search beneath.
        /// \param Name  The identifier to look for.
        /// \return The element, or one naming nothing when none beneath has it.
        Handle Find(Handle Scope, UInt64 Name) const;

        /// \brief Hands every child of an element to a callback, in order.
        ///
        /// \param Element  The element whose children are walked, where a dead or waiting one has none.
        /// \param Callback The callable, taking each child.
        template<typename Callable>
        void Children(Handle Element, AnyRef<Callable> Callback) const
        {
            if (!IsAlive(Element) || IsPending(Element))
            {
                return;
            }

            ConstRef<Pane> Pane = Locate(Element);
            const UInt32   Row  = mSlots[Element.GetSlot()].Row;

            for (UInt32 Cursor = Row + 1, End = Row + Pane.Spans[Row]; Cursor < End; Cursor += Pane.Spans[Cursor])
            {
                Callback(GetHandle(Pane.Slots[Cursor]));
            }
        }

        /// \brief Turns states of an element on or off, its look to be worked out again when one changes.
        ///
        /// \param Element The element whose states change, which must be alive.
        /// \param Bits    The states to turn on or off.
        /// \param Enabled The flag, `true` to turn them on, `false` to turn them off.
        void SetState(Handle Element, State Bits, Bool Enabled);

        /// \brief Checks whether an element is in every one of some states.
        ///
        /// \param Element The element asked about, which must be alive.
        /// \param Bits    The states it must all be in.
        /// \return `true` if it is in all of them, `false` otherwise.
        ZY_INLINE Bool HasState(Handle Element, State Bits) const
        {
            return HasBit(GetStates(Element), Bits);
        }

        /// \brief Gets every state an element is in.
        ///
        /// \param Element The element asked about, which must be alive.
        /// \return The states it is in, one bit each.
        ZY_INLINE State GetStates(Handle Element) const
        {
            return IsPending(Element) ? Sketch(Element).States : Locate(Element).States[mSlots[Element.GetSlot()].Row];
        }

        /// \brief Gets the facets an element holds.
        ///
        /// \param Element The element asked about, which must be alive.
        /// \return The mask, one bit for each kind of facet it holds.
        ZY_INLINE ConstRef<Mask> GetMask(Handle Element) const
        {
            return mSlots[Element.GetSlot()].Facets;
        }

        /// \brief Checks whether the element in a slot holds a kind of facet that stops the pointer.
        ///
        /// \param Slot The slot of the element asked about.
        /// \return `true` if it does, `false` otherwise.
        ZY_INLINE Bool IsInteractive(UInt32 Slot) const
        {
            return (mSlots[Slot].Facets & Facets::GetInteractive()).Any();
        }

        /// \brief Keeps a module's state on the tree, found by its type, replacing any state of that type.
        ///
        /// \param Parameters The arguments the state is built from.
        /// \return The state, which lives as long as the tree or until it is provided again.
        template<typename Type, typename... Arguments>
        Ref<Type> Provide(AnyRef<Arguments>... Parameters)
        {
            const UInt32 Index = Services::Index<Type>();

            if (Index >= mServices.GetSize())
            {
                mServices.Advance(Index + 1 - mServices.GetSize());
            }

            const Ptr<Kept<Type>> Made = new Kept<Type>(Forward<Arguments>(Parameters)...);
            mServices[Index] = Unique<Service>(Made);
            return Made->Get();
        }

        /// \brief Finds a module's state kept on the tree.
        ///
        /// \return The state, or `nullptr` when none of that type was provided.
        template<typename Type>
        Ptr<Type> Find()
        {
            const UInt32 Index = Services::Index<Type>();

            if (Index >= mServices.GetSize() || !mServices[Index])
            {
                return nullptr;
            }
            return AddressOf(static_cast<Ptr<Kept<Type>>>(mServices[Index].Grab())->Get());
        }

        /// \brief Finds a module's state kept on the tree.
        ///
        /// \return The state, or `nullptr` when none of that type was provided.
        template<typename Type>
        ConstPtr<Type> Find() const
        {
            const UInt32 Index = Services::Index<Type>();

            if (Index >= mServices.GetSize() || !mServices[Index])
            {
                return nullptr;
            }
            return AddressOf(static_cast<ConstPtr<Kept<Type>>>(AddressOf(* mServices[Index]))->Get());
        }

        /// \brief Marks what of an element must be worked out again.
        ///
        /// \param Element The element to mark, which must be alive, where a waiting one is worked out whole anyway.
        /// \param Bits    The parts that must be worked out again.
        void Mark(Handle Element, Dirty Bits);

        /// \brief Dresses an element in a style of the interface's theme.
        ///
        /// \param Element The element to dress, which must be alive.
        /// \param Name    The identifier of the style, or zero to wear none.
        void SetStyle(Handle Element, UInt64 Name);

        /// \brief Gets the style an element wears.
        ///
        /// \param Element The element asked about, which must be alive.
        /// \return The identifier of the style, or zero for none.
        ZY_INLINE UInt64 GetStyle(Handle Element) const
        {
            return IsPending(Element) ? Sketch(Element).Style : Locate(Element).Styles[mSlots[Element.GetSlot()].Row];
        }

        /// \brief Gets how an element takes its place and lays out what it holds, to change it.
        ///
        /// \param Element The element whose box is changed, which must be alive.
        /// \return The box, its pane to be laid out again before it is next drawn.
        Ref<Box> EditBox(Handle Element);

        /// \brief Gets how an element takes its place and lays out what it holds.
        ///
        /// \param Element The element asked about, which must be alive.
        /// \return The box the layout reads the element's place and its children's from.
        ZY_INLINE ConstRef<Box> GetBox(Handle Element) const
        {
            return IsPending(Element) ? Sketch(Element).Shape : Locate(Element).Boxes[mSlots[Element.GetSlot()].Row];
        }

        /// \brief Gets the box an element was last laid out in.
        ///
        /// \param Element The element asked about, which must stand in the tree.
        /// \return The box, in pixels of the target.
        ZY_INLINE Rect GetFrame(Handle Element) const
        {
            return Locate(Element).Frames[mSlots[Element.GetSlot()].Row];
        }

        /// \brief Adds a facet to an element.
        ///
        /// \param Element    The element to add it to, which must be alive and hold no such facet.
        /// \param Parameters The arguments the facet is built from.
        /// \return The facet, which moves when another of its type is added or removed.
        template<IsFacet Type, typename... Arguments>
        Ref<Type> Add(Handle Element, AnyRef<Arguments>... Parameters)
        {
            ZY_ASSERT(IsAlive(Element), "The element is not alive");

            Touch<Type>(Element);
            mSlots[Element.GetSlot()].Facets.Set(Facets::Index<Type>());
            return Stock<Type>().Add(Element.GetSlot(), Forward<Arguments>(Parameters)...);
        }

        /// \brief Gets a facet of an element, adding one built by default when the element holds none.
        ///
        /// \param Element The element the facet is found on or added to, which must be alive.
        /// \return The facet, which moves when another of its type is added or removed.
        template<IsFacet Type>
        Ref<Type> Obtain(Handle Element)
        {
            const Ptr<Type> Found = Get<Type>(Element);
            return Found ? * Found : Add<Type>(Element);
        }

        /// \brief Gets a facet of an element to change, marking what a change to it must have worked out again.
        ///
        /// \param Element The element whose facet is changed, which may be dead.
        /// \return The facet, or `nullptr` for a dead element or one holding none.
        template<IsFacet Type>
        Ptr<Type> Edit(Handle Element)
        {
            const Ptr<Type> Found = Get<Type>(Element);

            if (Found)
            {
                Touch<Type>(Element);
            }
            return Found;
        }

        /// \brief Gets a facet of an element.
        ///
        /// \param Element The element whose facet is asked for, which may be dead.
        /// \return The facet, or `nullptr` for a dead element or one holding none.
        template<IsFacet Type>
        Ptr<Type> Get(Handle Element)
        {
            return IsAlive(Element) ? Peek<Type>(Element.GetSlot()) : nullptr;
        }

        /// \brief Gets a facet of an element.
        ///
        /// \param Element The element whose facet is asked for, which may be dead.
        /// \return The facet, or `nullptr` for a dead element or one holding none.
        template<IsFacet Type>
        ConstPtr<Type> Get(Handle Element) const
        {
            if (!Has<Type>(Element))
            {
                return nullptr;
            }
            return static_cast<ConstPtr<Facet<Type>>>(AddressOf(* mStores[Facets::Index<Type>()]))
                ->Find(Element.GetSlot());
        }

        /// \brief Checks whether an element holds a facet.
        ///
        /// \param Element The element asked about, which may be dead.
        /// \return `true` if it is alive and holds one, `false` otherwise.
        template<IsFacet Type>
        ZY_INLINE Bool Has(Handle Element) const
        {
            return IsAlive(Element) && mSlots[Element.GetSlot()].Facets.Test(Facets::Index<Type>());
        }

        /// \brief Removes a facet from an element.
        ///
        /// \param Element The element to remove it from, which must be alive and hold one.
        template<IsFacet Type>
        void Remove(Handle Element)
        {
            ZY_ASSERT(Has<Type>(Element), "The element holds no such facet");

            Touch<Type>(Element);
            mSlots[Element.GetSlot()].Facets.Reset(Facets::Index<Type>());
            mStores[Facets::Index<Type>()]->Remove(Element.GetSlot());
        }

        /// \brief Gets every facet of a type, for a pass that walks them densely.
        ///
        /// \return The store of the facets, made empty when no element held one yet.
        template<IsFacet Type>
        Ref<Facet<Type>> GetFacets()
        {
            return Stock<Type>();
        }

        /// \brief Finds a facet of the element a slot holds, for a pass that walks the rows of a pane.
        ///
        /// \param Slot The slot, which must hold a living element.
        /// \return The facet, or `nullptr` when the element holds none.
        template<IsFacet Type>
        ZY_INLINE Ptr<Type> Peek(UInt32 Slot)
        {
            if (!mSlots[Slot].Facets.Test(Facets::Index<Type>()))
            {
                return nullptr;
            }
            return static_cast<Ptr<Facet<Type>>>(mStores[Facets::Index<Type>()].Grab())->Find(Slot);
        }

        /// \brief Marks a walk as started, holding back every change to the shape of the tree until it ends.
        ZY_INLINE void Enter()
        {
            ++mDepth;
        }

        /// \brief Marks a walk as ended, applying what it held back once the outermost one ends.
        void Leave();

        /// \brief Checks whether a walk is running.
        ///
        /// \return `true` if one is, `false` otherwise.
        ZY_INLINE Bool IsWalking() const
        {
            return mDepth > 0;
        }

        /// \brief Gets the panes in the order they are drawn, back to front.
        ///
        /// \return The indices of the open panes.
        ZY_INLINE ConstSpan<UInt16> GetOrder() const
        {
            return mOrder;
        }

        /// \brief Gets a pane.
        ///
        /// \param Index The index of the pane, as \ref GetOrder hands it out.
        /// \return The pane, its rows read in preorder.
        ZY_INLINE ConstRef<Pane> GetPane(UInt16 Index) const
        {
            return mPanes[Index];
        }

        /// \brief Gets the element a slot holds.
        ///
        /// \param Slot The slot, which must hold a living element.
        /// \return The handle naming the element at its current generation.
        ZY_INLINE Handle GetHandle(UInt32 Slot) const
        {
            return Handle(Slot, mSlots[Slot].Generation);
        }

        /// \brief Gets the row of its pane a slot's element stands at.
        ///
        /// \param Slot The slot, which must hold a living element standing in the tree.
        /// \return The row, which moves whenever its pane changes shape.
        ZY_INLINE UInt32 GetRow(UInt32 Slot) const
        {
            return mSlots[Slot].Row;
        }

    protected:

        /// \brief Gets a pane to work on.
        ///
        /// \param Index The index of the pane, as \ref GetOrder hands it out.
        /// \return The pane, for a pass to dress, lay out or draw.
        ZY_INLINE Ref<Pane> Access(UInt16 Index)
        {
            return mPanes[Index];
        }

    private:

        /// The pane a free slot names, so a slot holding no element is told apart at once.
        static constexpr UInt16 kFree    = 0xFFFF;

        /// The row an element names while it waits for the running walk to end.
        static constexpr UInt32 kPending = 0xFFFFFFFF;

        /// \brief Represents where an element stands, and what of it outlives a shift of its pane.
        struct Slot final
        {
            /// The generation of the slot, raised each time its element is destroyed.
            UInt32 Generation = 0;

            /// The row of the element in its pane, \ref kPending while it waits, or the next free slot when free.
            UInt32 Row        = kPending;

            /// The kinds of facet the element holds, one bit each.
            Mask   Facets;

            /// The pane the element stands in, or \ref kFree.
            UInt16 Pane       = kFree;
        };

        /// \brief Specifies a change to the shape of the tree held back until the running walk ends.
        enum class Operation : UInt8
        {
            Create,      ///< Places an element made during the walk.
            Destroy,     ///< Destroys an element and everything it holds.
            Move,        ///< Moves an element under a parent, before a sibling.
        };

        /// \brief Represents one change held back until the running walk ends.
        struct Change final
        {
            /// What the change does to the element.
            Operation Kind = Operation::Create;

            /// The element the change is about.
            Handle    Element;

            /// The parent it goes under, naming nothing when it is destroyed.
            Handle    Parent;

            /// The sibling it goes before, or one naming nothing to make it the last.
            Handle    Before;

            /// The identifier a created element is found by.
            UInt64    Name = 0;
        };

        /// \brief Represents what an element made during a walk is given, written into its row once the walk ends.
        struct Draft final
        {
            /// The identifier it is found by.
            UInt64 Name   = 0;

            /// The identifier of the style it wears.
            UInt64 Style  = 0;

            /// The states it is in.
            State  States = State::None;

            /// How it takes its place and lays out what it holds.
            Box    Shape;
        };

        /// \brief Gets what an element made during the running walk was given.
        ///
        /// \param Element The element, which must be waiting to stand in the tree.
        /// \return The draft its row is written from once the walk ends.
        ZY_INLINE ConstRef<Draft> Sketch(Handle Element) const
        {
            return * mDrafts.Find(Element.GetSlot());
        }

        /// \brief Gets what an element made during the running walk was given, to change it.
        ///
        /// \param Element The element, which must be waiting to stand in the tree.
        /// \return The draft its row is written from once the walk ends.
        ZY_INLINE Ref<Draft> Sketch(Handle Element)
        {
            return * mDrafts.Find(Element.GetSlot());
        }

        /// \brief Finds the change that makes an element during the running walk.
        ///
        /// \param Element The element, which must be waiting to stand in the tree.
        /// \return The index of the change among those held back.
        UInt Pending(Handle Element) const;

        /// \brief Gathers the children of an element made during the running walk, in the order they will stand.
        ///
        /// \param Parent The element, which must be waiting to stand in the tree.
        /// \param Output The receiver of the children.
        void Waiting(Handle Parent, Ref<Sequence<Handle>> Output) const;

        /// \brief Gets the pane an element stands in.
        ///
        /// \param Element The element, which must stand in the tree.
        /// \return The pane, whose row for the element its slot names.
        ZY_INLINE ConstRef<Pane> Locate(Handle Element) const
        {
            ZY_ASSERT(IsAlive(Element) && !IsPending(Element), "The element does not stand in the tree");
            return mPanes[mSlots[Element.GetSlot()].Pane];
        }

        /// \brief Gets the pane an element stands in, to change it.
        ///
        /// \param Element The element, which must stand in the tree.
        /// \return The pane, whose row for the element its slot names.
        ZY_INLINE Ref<Pane> Locate(Handle Element)
        {
            ZY_ASSERT(IsAlive(Element) && !IsPending(Element), "The element does not stand in the tree");
            return mPanes[mSlots[Element.GetSlot()].Pane];
        }

        /// \brief Marks what a change to a facet of an element must have worked out again, once it stands in the tree.
        ///
        /// \param Element The element whose facet changes, which must be alive.
        template<IsFacet Type>
        ZY_INLINE void Touch(Handle Element)
        {
            if (!IsPending(Element))
            {
                Mark(Element, Type::kMarks);
            }
        }

        /// \brief Gets the store of a facet type, making it on first use.
        ///
        /// \return The store of every facet of the type.
        template<IsFacet Type>
        Ref<Facet<Type>> Stock()
        {
            const UInt32 Index = Facets::Index<Type>();

            if (!mStores[Index])
            {
                mStores[Index] = Unique<Store>(new Facet<Type>());
            }
            return * static_cast<Ptr<Facet<Type>>>(mStores[Index].Grab());
        }

        /// \brief Takes a free slot, or a new one when none is free.
        ///
        /// \param Pane The pane its element stands in.
        /// \return The slot, naming no row and holding no facet.
        UInt32 Allocate(UInt16 Pane);

        /// \brief Gives a slot back, raising its generation and removing every facet its element held.
        ///
        /// \param Slot The slot of the element that is gone.
        void Release(UInt32 Slot);

        /// \brief Puts an element into its pane right away.
        ///
        /// \param Slot   The slot of the element, which names no row yet.
        /// \param Parent The slot of the parent, which stands in the same pane.
        /// \param Before The slot of the sibling it goes before, or \ref kPending to make it the last.
        /// \param Name   The identifier it is found by.
        void Place(UInt32 Slot, UInt32 Parent, UInt32 Before, UInt64 Name);

        /// \brief Destroys an element and everything it holds right away.
        ///
        /// \param Element The element to destroy, which must be alive.
        void Erase(Handle Element);

        /// \brief Moves an element right away.
        ///
        /// \param Element The element to move, which must stand in the tree.
        /// \param Parent  The new parent, which must stand in the same pane.
        /// \param Before  The sibling it goes before, or one naming nothing to make it the last.
        void Shift(Handle Element, Handle Parent, Handle Before);

        /// \brief Moves an element right away under a parent standing in another pane.
        ///
        /// \param Element The element to move, which must stand in the tree and not be a root.
        /// \param Parent  The new parent, which must stand in another pane.
        /// \param Before  The sibling it goes before, or one naming nothing to make it the last.
        void Transfer(Handle Element, Handle Parent, Handle Before);

        /// \brief Adds to the span of an element and of everything holding it.
        ///
        /// \param Pane  The pane the element stands in.
        /// \param Slot  The slot of the element whose subtree grew or shrank.
        /// \param Delta The rows to add, or to take away when negative.
        void Grow(Ref<Pane> Pane, UInt32 Slot, SInt32 Delta);

        /// \brief Writes the row of every element in a run of rows into its slot.
        ///
        /// \param Pane  The pane whose rows moved.
        /// \param First The first row of the run.
        /// \param Last  The row one past the last of the run.
        void Renumber(ConstRef<Pane> Pane, UInt32 First, UInt32 Last);

        /// \brief Puts a pane in the drawing order, behind every pane of a later band and before the rest of its own.
        ///
        /// \param Index The index of the pane, which must not stand in the drawing order yet.
        void Arrange(UInt16 Index);

        /// \brief Applies every change the walks held back, those they cause included.
        void Flush();

        /// \brief Fills a new row of a pane as an element nothing was given to yet.
        ///
        /// \param Target The pane the row was opened in.
        /// \param Row    The row to fill.
        /// \param Slot   The slot of the element the row is for.
        /// \param Parent The slot of its parent, or \ref Pane::kNoParent for a root.
        /// \param Depth  The depth it stands at, nought for a root.
        /// \param Name   The identifier it is found by.
        static void Prime(Ref<Pane> Target, UInt32 Row, UInt32 Slot, UInt32 Parent, UInt16 Depth, UInt64 Name);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Sequence<Slot>                   mSlots;
        UInt32                           mFree;
        Sequence<Pane>                   mPanes;
        Sequence<UInt16>                 mOrder;
        UInt32                           mRaise;
        Array<Unique<Store>, kMaxFacets> mStores;
        Sequence<Change>                 mChanges;
        Table<UInt32, Draft>             mDrafts;
        UInt32                           mDepth;
        Sequence<Unique<Service>>        mServices;
    };
}