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

#include "Animator.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Pane.hpp"
#include "UI.Core/Style/Skin.hpp"
#include "UI.Core/Style/Style.hpp"
#include "UI.Core/Style/Theme.hpp"
#include "UI.Core/Tree.hpp"
#include "UI.Core/Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the pass that dresses each element of a tree in the skin its style and states resolve to.
    class Dresser final
    {
    public:

        /// \brief Represents a sound waiting for the panes to be dressed before it is played.
        struct Chime final
        {
            /// The element whose style names the sound.
            Handle Element;

            /// The moment the sound is played for.
            Cue    Moment;
        };

    public:

        /// \brief Constructs a dresser holding no theme, over the elements of a tree.
        ///
        /// \param Owner    The tree whose elements it dresses, which must outlive it.
        /// \param Animator The animator that moves an element from one skin to the next.
        Dresser(Ref<Tree> Owner, Ref<Animator> Animator);

        /// \brief Sets the theme every element wearing a style is dressed from.
        ///
        /// \param Source The theme, or `nullptr` to dress nothing.
        void SetTheme(ConstPtr<Theme> Source);

        /// \brief Gets the theme every element wearing a style is dressed from.
        ///
        /// \return The theme, or `nullptr` for none.
        ZY_INLINE ConstPtr<Theme> GetTheme() const
        {
            return mTheme;
        }

        /// \brief Takes the version of the theme, which changes whenever the theme does.
        ///
        /// \return `true` if it changed since the last time, so every element must be dressed again, `false` otherwise.
        Bool Revise();

        /// \brief Dresses every element of a pane whose style or states changed, from the theme.
        ///
        /// \param Target The pane whose marked rows are dressed, its marks cleared as it goes.
        void Dress(Ref<Pane> Target);

        /// \brief Takes the sounds of the panes shown or hidden since the last time, to play once they are dressed.
        ///
        /// \return The sounds, in the order the panes were dressed.
        Sequence<Chime> Drain();

    private:

        /// \brief Represents a row whose descendants take its states, for a walk that dresses a pane.
        struct Owner final
        {
            /// One past the last row it holds.
            UInt32 End;

            /// The states its descendants take.
            State  Bits;
        };

        /// \brief Gives an element the values a skin gives.
        ///
        /// \param Target The pane holding the element.
        /// \param Row    The row of the element in that pane.
        /// \param Values The skin the element's style and states resolve to.
        void Wear(Ref<Pane> Target, UInt32 Row, ConstRef<Skin> Values);

        /// \brief Gives an element the values of a skin that lay it out, which never move.
        ///
        /// \param Target The pane holding the element, marked to be laid out again when a size changes.
        /// \param Row    The row of the element in that pane.
        /// \param Values The skin the element's style and states resolve to.
        void Fit(Ref<Pane> Target, UInt32 Row, ConstRef<Skin> Values);

        /// \brief Records an element as dressed.
        ///
        /// \param Element The element just dressed.
        /// \return `true` if it had been dressed before, `false` otherwise.
        Bool Remember(Handle Element);

        /// \brief Records the values an element's new skin gives, before it is worn.
        ///
        /// \param Element The element about to wear the new skin.
        /// \param Given   The values the new skin gives, or none when its style is not found.
        /// \return The values the last skin it wore gave and the new one does not.
        Skin::Field Renew(Handle Element, Skin::Field Given);

        /// \brief Queues the sound of a pane being shown or hidden, when its root was last seen the other way.
        ///
        /// \param Target The pane, whose root was just dressed.
        void Announce(ConstRef<Pane> Target);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Tree>             mTree;
        Ref<Animator>         mAnimator;
        ConstPtr<Theme>       mTheme;
        UInt32                mVersion;
        Sequence<Owner>       mOwners;
        Sequence<Handle>      mDressed;
        Sequence<Skin::Field> mWorn;
        Sequence<Handle>      mShown;
        Sequence<Chime>       mChimes;
    };
}