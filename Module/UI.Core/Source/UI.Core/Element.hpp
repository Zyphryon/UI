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

#include "Event.hpp"
#include "Facet.hpp"
#include "Facet/Input/Control.hpp"
#include "Facet/Layout/Box.hpp"
#include "Handle.hpp"
#include "Tree.hpp"
#include "Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents an element as code sees it, the only surface for work on one element.
    class Element final
    {
    public:

        /// \brief Constructs an element naming nothing.
        ZY_INLINE Element()
            : mTree { nullptr }
        {
        }

        /// \brief Constructs an element naming a handle of a tree.
        ///
        /// \param Tree   The tree the element stands in.
        /// \param Handle The handle of the element.
        ZY_INLINE Element(Ref<Tree> Tree, Handle Handle)
            : mTree   { AddressOf(Tree) },
              mHandle { Handle }
        {
        }

        /// \brief Checks whether the element is alive, standing in the tree or waiting to.
        ///
        /// \return `true` if it is, `false` otherwise.
        ZY_INLINE Bool IsAlive() const
        {
            return mTree && mTree->IsAlive(mHandle);
        }

        /// \brief Gets the handle of the element.
        ///
        /// \return The handle the tree knows the element by.
        ZY_INLINE Handle GetHandle() const
        {
            return mHandle;
        }

        /// \brief Gets the element holding this one.
        ///
        /// \return The parent, or one naming nothing for a root.
        ZY_INLINE Element GetParent() const
        {
            return Wrap(mTree ? mTree->GetParent(mHandle) : Handle());
        }

        /// \brief Gets the first element this one holds.
        ///
        /// \return The first child, or one naming nothing when it holds none.
        ZY_INLINE Element GetFirst() const
        {
            return Wrap(mTree ? mTree->GetFirst(mHandle) : Handle());
        }

        /// \brief Gets the element standing after this one under the same parent.
        ///
        /// \return The next sibling, or one naming nothing for the last.
        ZY_INLINE Element GetNext() const
        {
            return Wrap(mTree ? mTree->GetNext(mHandle) : Handle());
        }

        /// \brief Gets the place of the element among its siblings.
        ///
        /// \return The place, counted from its parent's first child.
        ZY_INLINE UInt32 GetIndex() const
        {
            return mTree ? mTree->GetIndex(mHandle) : 0;
        }

        /// \brief Gets the root of the pane the element stands in.
        ///
        /// \return The root, or one naming nothing for a dead element.
        ZY_INLINE Element GetRoot() const
        {
            return Wrap(mTree ? mTree->GetRoot(mHandle) : Handle());
        }

        /// \brief Finds the first element by name among those this one holds, however deep, in preorder.
        ///
        /// \param Name The name to look for.
        /// \return The element, or one naming nothing when none beneath has it.
        ZY_INLINE Element Find(Text Name) const
        {
            return Wrap(mTree ? mTree->Find(mHandle, Identify(Name)) : Handle());
        }

        /// \brief Hands every child of the element to a callback, in order.
        ///
        /// \param Callback The callable, taking each child.
        template<typename Callable>
        ZY_INLINE void Children(AnyRef<Callable> Callback) const
        {
            if (mTree)
            {
                mTree->Children(mHandle, [&](Handle Child)
                {
                    Callback(Element(* mTree, Child));
                });
            }
        }

        /// \brief Creates an element as a child of this one, standing just before one of its children or last.
        ///
        /// \param Name   The name it is found by, or empty for none.
        /// \param Before The child it goes before, or one naming nothing to make it the last.
        /// \return The element, which stands in the tree once the running walk ends when there is one.
        ZY_INLINE Element Create(Text Name = Text(), Element Before = Element()) const
        {
            return Wrap(mTree->Create(mHandle, Identify(Name), Before.mHandle));
        }

        /// \brief Destroys the element and everything it holds, closing its pane when it is a root.
        ZY_INLINE void Destruct() const
        {
            mTree->Destroy(mHandle);
        }

        /// \brief Moves the element and everything it holds under a parent of its pane, before a child or last.
        ///
        /// \param Parent The new parent, which must not stand beneath this element.
        /// \param Before The child of the parent it goes before, or one naming nothing to make it the last.
        ZY_INLINE void Attach(Element Parent, Element Before = Element()) const
        {
            mTree->Move(mHandle, Parent.mHandle, Before.mHandle);
        }

        /// \brief Turns states of the element on or off.
        ///
        /// \param Bits    The states to turn on or off.
        /// \param Enabled The flag, `true` to turn them on, `false` to turn them off.
        ZY_INLINE void SetState(State Bits, Bool Enabled) const
        {
            mTree->SetState(mHandle, Bits, Enabled);
        }

        /// \brief Checks whether the element is in every one of some states.
        ///
        /// \param Bits The states it must all be in.
        /// \return `true` if it is in all of them, `false` otherwise.
        ZY_INLINE Bool HasState(State Bits) const
        {
            return mTree->HasState(mHandle, Bits);
        }

        /// \brief Gets the tree the element stands in.
        ///
        /// \return The tree the element was made in, which an element naming nothing lacks.
        ZY_INLINE Ref<Tree> GetTree() const
        {
            return * mTree;
        }

        /// \brief Changes the name the element is found by.
        ///
        /// \param Name The name, or empty for none.
        ZY_INLINE void SetName(Text Name) const
        {
            mTree->SetName(mHandle, Identify(Name));
        }

        /// \brief Dresses the element in a style of the theme.
        ///
        /// \param Name The name of the style, or empty to wear none.
        ZY_INLINE void SetStyle(Text Name) const
        {
            mTree->SetStyle(mHandle, Identify(Name));
        }

        /// \brief Runs code on the events of the element, making it a control when it is not one yet.
        ///
        /// \param Callback The code, returning `true` to stop the event bubbling further.
        ZY_INLINE void Observe(Handler Callback) const
        {
            mTree->Obtain<Control>(mHandle).Observers.Append(Move(Callback));
        }

        /// \brief Gets how the element takes its place and lays out what it holds, to change it.
        ///
        /// \return The box, its pane to be laid out again before it is next drawn.
        ZY_INLINE Ref<Box> EditBox() const
        {
            return mTree->EditBox(mHandle);
        }

        /// \brief Gets the box the element was last laid out in.
        ///
        /// \return The box, in pixels of the target.
        ZY_INLINE Rect GetFrame() const
        {
            return mTree->GetFrame(mHandle);
        }

        /// \brief Gets a facet of the element to change, marking what a change to it must have worked out again.
        ///
        /// \return The facet, or `nullptr` when the element is dead or holds none.
        template<IsFacet Type>
        ZY_INLINE Ptr<Type> Edit() const
        {
            return mTree ? mTree->Edit<Type>(mHandle) : nullptr;
        }

        /// \brief Adds a facet to the element.
        ///
        /// \param Parameters The arguments the facet is built from.
        /// \return The facet, which moves when another of its type is added or removed.
        template<IsFacet Type, typename... Arguments>
        ZY_INLINE Ref<Type> Add(AnyRef<Arguments>... Parameters) const
        {
            return mTree->Add<Type>(mHandle, Forward<Arguments>(Parameters)...);
        }

        /// \brief Gets a facet of the element, adding one built by default when the element holds none.
        ///
        /// \return The facet, which moves when another of its type is added or removed.
        template<IsFacet Type>
        ZY_INLINE Ref<Type> Obtain() const
        {
            return mTree->Obtain<Type>(mHandle);
        }

        /// \brief Gets a facet of the element.
        ///
        /// \return The facet, or `nullptr` when the element is dead or holds none.
        template<IsFacet Type>
        ZY_INLINE Ptr<Type> Get() const
        {
            return mTree ? mTree->Get<Type>(mHandle) : nullptr;
        }

        /// \brief Checks whether the element holds a facet.
        ///
        /// \return `true` if it is alive and holds one, `false` otherwise.
        template<IsFacet Type>
        ZY_INLINE Bool Has() const
        {
            return mTree && mTree->Has<Type>(mHandle);
        }

        /// \brief Removes a facet from the element.
        template<IsFacet Type>
        ZY_INLINE void Remove() const
        {
            mTree->Remove<Type>(mHandle);
        }

        /// \brief Checks whether two elements name the same element of the same tree.
        ///
        /// \param Other The element to compare against.
        /// \return `true` if they do, `false` otherwise.
        ZY_INLINE Bool operator==(ConstRef<Element> Other) const
        {
            return mTree == Other.mTree && mHandle == Other.mHandle;
        }

    private:

        /// \brief Gets the element a handle of the same tree names.
        ///
        /// \param Handle The handle of an element of the same tree, or one naming nothing.
        /// \return The element, naming nothing when the handle does.
        ZY_INLINE Element Wrap(Handle Handle) const
        {
            return Handle.IsValid() ? Element(* mTree, Handle) : Element();
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ptr<Tree> mTree;
        Handle    mHandle;
    };
}