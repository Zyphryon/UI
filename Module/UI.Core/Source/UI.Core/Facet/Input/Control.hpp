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

#include "UI.Core/Event.hpp"
#include "UI.Core/Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents an element that answers the pointer and the keyboard, as a widget or a part acting on its own.
    struct Control final
    {
        /// The parts a change to the facet must have worked out again.
        static constexpr Dirty kMarks       = Dirty::None;

        /// `true`, so the pointer stops at an element holding it.
        static constexpr Bool  kInteractive = true;

        /// The widget's own logic, or empty for a control that only reports what happens to it.
        Handler           Behaviour;

        /// The code the game runs on its events, after the behaviour.
        Sequence<Handler> Observers;

        /// The control that opened this popup root, which its events bubble on into and whose press keeps it open.
        Handle            Owner;

        /// The identifier of the action the interface runs after the observers, or zero for none.
        UInt64            Action    = 0;

        /// `true` when a press moves the keyboard to it, `false` when it never takes the keyboard.
        Bool              Focusable = false;

        /// `true` when every button presses, drags and clicks it as the left one does, `false` for the left alone.
        Bool              AnyButton = false;

        /// `true` when it hears the pointer come over it, move and leave, `false` when it is told none of those.
        Bool              Hover     = false;

        /// `true` when it takes typed text, so holding the keyboard counts as typing, `false` otherwise.
        Bool              Typing    = false;
    };
}