// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// The most kinds of facet an element may hold, one bit each in its facet mask.
    inline constexpr UInt32 kMaxFacets = 128;

    /// \brief Represents which kinds of facet an element holds, one bit for each kind, at the kind's index.
    using Mask = Bitset<kMaxFacets, UInt64>;

    /// \brief Checks whether a type is a facet, which says what a change to it makes stale.
    template<typename Type>
    concept IsFacet = requires { Type::kMarks; };

    /// \brief Provides the index each kind of facet is known by, handed out the first time the kind is named.
    class Facets final
    {
    public:

        /// \brief Gets the index of a kind of facet, handing one out the first time it is asked for.
        ///
        /// \return The index, below \ref kMaxFacets, the same for the life of the process.
        template<IsFacet Type>
        static UInt32 Index()
        {
            static UInt32 kIndex = kMaxFacets;

            if (kIndex == kMaxFacets)
            {
                kIndex = Declare(IsInteractive<Type>());
            }
            return kIndex;
        }

        /// \brief Gets the kinds of facet the pointer stops at.
        ///
        /// \return The mask of their indices.
        static ConstRef<Mask> GetInteractive()
        {
            return GetRegistry().Interactive;
        }

    private:

        /// \brief Holds what the registry has handed out.
        struct Registry final
        {
            /// The index the next kind takes.
            UInt32 Count = 0;

            /// The kinds the pointer stops at.
            Mask   Interactive;
        };

        /// \brief Checks whether a kind of facet stops the pointer at an element holding it.
        ///
        /// \return `true` if it does, `false` otherwise.
        template<IsFacet Type>
        static constexpr Bool IsInteractive()
        {
            if constexpr (requires { Type::kInteractive; })
            {
                return Type::kInteractive;
            }
            else
            {
                return false;
            }
        }

        /// \brief Hands out the next index.
        ///
        /// \param Interactive The flag, `true` when the kind stops the pointer, `false` otherwise.
        /// \return The index the kind is known by from now on.
        static UInt32 Declare(Bool Interactive);

        /// \brief Gets the registry, which is constant-initialized, so reaching it costs no guard.
        ///
        /// \return The registry of every kind handed out so far.
        static Ref<Registry> GetRegistry()
        {
            static Registry kInstance;
            return kInstance;
        }
    };

    /// \brief Defines the interface a store of facets of any one kind is reached through by the tree.
    class Store
    {
    public:

        /// \brief Destroys the store and every facet it holds.
        virtual ~Store() = default;

        /// \brief Removes the facet an element holds.
        ///
        /// \param Slot The slot of the element, which must hold one.
        virtual void Remove(UInt32 Slot) = 0;
    };

    /// \brief Represents the facets of one type, kept densely and found by the slot of the element holding each.
    template<typename Type>
    class Facet final : public Store
    {
    public:

        /// \brief Constructs a store holding no facet.
        Facet() = default;

        /// \brief Adds a facet to an element.
        ///
        /// \param Slot       The slot of the element, which must hold none yet.
        /// \param Parameters The arguments the facet is built from.
        /// \return The facet, which moves when another is added or removed.
        template<typename... Arguments>
        Ref<Type> Add(UInt32 Slot, AnyRef<Arguments>... Parameters)
        {
            Ref<UInt32> Entry = Acquire(Slot);
            ZY_ASSERT(Entry == 0, "The element already holds this facet");

            mOwners.Append(Slot);
            Entry = static_cast<UInt32>(mDense.GetSize() + 1);
            return mDense.Append(Forward<Arguments>(Parameters)...);
        }

        /// \see Store::Remove
        void Remove(UInt32 Slot) override
        {
            const UInt32 Index = Locate(Slot);
            ZY_ASSERT(Index != 0, "The element holds no such facet");

            // The last facet fills the hole, so the store stays dense.
            const UInt32 Last = static_cast<UInt32>(mDense.GetSize());

            mDense.RemoveFast(Index - 1);
            mOwners.RemoveFast(Index - 1);

            if (Index != Last)
            {
                Acquire(mOwners[Index - 1]) = Index;
            }
            Acquire(Slot) = 0;
        }

        /// \brief Finds the facet an element holds.
        ///
        /// \param Slot The slot of the element whose facet is looked for.
        /// \return The facet, or `nullptr` when the element holds none.
        ZY_INLINE Ptr<Type> Find(UInt32 Slot)
        {
            const UInt32 Index = Locate(Slot);
            return Index ? AddressOf(mDense[Index - 1]) : nullptr;
        }

        /// \brief Finds the facet an element holds.
        ///
        /// \param Slot The slot of the element whose facet is looked for.
        /// \return The facet, or `nullptr` when the element holds none.
        ZY_INLINE ConstPtr<Type> Find(UInt32 Slot) const
        {
            const UInt32 Index = Locate(Slot);
            return Index ? AddressOf(mDense[Index - 1]) : nullptr;
        }

        /// \brief Gets how many facets the store holds.
        ///
        /// \return The count of facets, one for each element holding one.
        ZY_INLINE UInt GetSize() const
        {
            return mDense.GetSize();
        }

        /// \brief Hands every facet to a callback with the slot of its element, in no order the tree gives.
        ///
        /// \param Callback The callable, taking the slot and the facet, which must not add or remove facets.
        template<typename Callable>
        ZY_INLINE void ForEach(AnyRef<Callable> Callback)
        {
            for (UInt Index = 0, Count = mDense.GetSize(); Index < Count; ++Index)
            {
                Callback(mOwners[Index], mDense[Index]);
            }
        }

    private:

        /// The slots one page of the index covers.
        static constexpr UInt32 kPageSize = 256;

        /// \brief Finds where in the dense store an element's facet sits.
        ///
        /// \param Slot The slot of the element whose facet is looked for.
        /// \return The position counted from one, or zero when the element holds none.
        ZY_INLINE UInt32 Locate(UInt32 Slot) const
        {
            const UInt32 Page = Slot / kPageSize;
            return Page < mPages.GetSize() && !mPages[Page].IsEmpty() ? mPages[Page][Slot % kPageSize] : 0;
        }

        /// \brief Gets the entry of an element in the index, making its page on first use.
        ///
        /// \param Slot The slot of the element whose entry is wanted.
        /// \return The entry, counted from one, or zero when the element holds none.
        Ref<UInt32> Acquire(UInt32 Slot)
        {
            const UInt32 Page = Slot / kPageSize;

            if (Page >= mPages.GetSize())
            {
                mPages.Advance(Page + 1 - mPages.GetSize());
            }

            // A new page comes zeroed, every slot on it holding none.
            if (mPages[Page].IsEmpty())
            {
                mPages[Page].Advance(kPageSize);
            }
            return mPages[Page][Slot % kPageSize];
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Sequence<Sequence<UInt32>> mPages;
        Sequence<Type>             mDense;
        Sequence<UInt32>           mOwners;
    };
}