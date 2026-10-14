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

#include "Skin.hpp"
#include "UI.Core/Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Specifies a moment of interaction a style may give a sound for.
    enum class Cue : UInt8
    {
        Hover,           ///< The pointer came to rest on the control.
        Press,           ///< The control was pressed.
        Click,           ///< The control was clicked.
        Focus,           ///< The control took the keyboard.
        Change,          ///< The control's value changed.
        Open,            ///< The pane the element is the root of was shown.
        Close,           ///< The pane the element is the root of was hidden.
    };

    /// \brief Represents one named style, as the skin an element wears and the skins it trades it for in some states.
    struct Style final
    {
        /// \brief Represents a skin worn over the style's own while an element is in every one of some states.
        struct Variant final
        {
            /// The states the element must be in, every one of them, for the variant to be worn.
            State When;

            /// The values worn over the style's own while the element is in those states.
            Skin  Wear;
        };

        /// The identifier of the style this one starts from, or zero for none.
        UInt64                           Base = 0;

        /// The skin worn in every state.
        Skin                             Normal;

        /// The skins worn over it, in the order they were declared, a later one winning a value both give.
        Sequence<Variant>                Variants;

        /// The address of the sound played at each moment of interaction, empty for none.
        Array<Str, ZyEnum::Count<Cue>()> Sounds;

        /// The moments the style names a sound for, one bit each, so an empty address silences the one its base names.
        Bitset<ZyEnum::Count<Cue>()>     Voiced;

        /// \brief Gives everything another style gives over what this one gives, as a later layer of a theme does.
        ///
        /// \param Over The other style, whose variants of other states are added after this one's.
        void Merge(ConstRef<Style> Over);

        /// \brief Adds a skin worn while an element is in every one of some states.
        ///
        /// \param Bits The states the element must all be in for the skin to be worn.
        /// \return The skin, giving nothing yet.
        Ref<Skin> When(State Bits);

        /// \brief Names the sound played at a moment of interaction, over the one its base names.
        ///
        /// \param Moment  The moment of interaction the sound plays at.
        /// \param Address The address of the sound, or empty to play none.
        /// \return This style, so further sounds chain onto it.
        Ref<Style> SetSound(Cue Moment, Text Address);
    };
}