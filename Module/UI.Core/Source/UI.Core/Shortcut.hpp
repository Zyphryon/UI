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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents a key chord that runs an action or code before the control holding the keyboard hears the key.
    struct Shortcut final
    {
        /// \brief Specifies where the pointer or the keyboard must be for a shortcut an element keeps to run.
        enum class Scope : UInt8
        {
            Anywhere, ///< Wherever they are, while the element can be used.
            Hover,    ///< While the pointer rests on the element or on anything it holds.
            Focus,    ///< While the element or anything it holds has the keyboard.
        };

        /// The key that runs it.
        ZyInput::Key      Key       = ZyInput::Key::Unknown;

        /// The modifier keys held with it, which must be exactly the ones of shift, control and alt held.
        ZyInput::Modifier Modifiers = ZyInput::Modifier::None;

        /// The identifier of the action it runs, as \ref Interface::Bind binds one, or zero to run only its code.
        UInt64            Action    = 0;

        /// The code it runs instead of the action, or empty to run the action.
        Handler           Callback;

        /// Where the pointer or the keyboard must be, for a shortcut an element keeps.
        Scope             Where     = Scope::Anywhere;

        /// `true` to run even while a text field holds the keyboard, which otherwise hears the key.
        Bool              Typing    = false;

        /// `true` to take the keyboard from a text field holding it before running, as an undo drops a half typed edit.
        Bool              Blur      = false;

        /// `true` to run again each time the key repeats while held.
        Bool              Repeat    = true;

        /// \brief Constructs a shortcut no key runs.
        Shortcut() = default;

        /// \brief Constructs a shortcut running an action, from a chord written as text.
        ///
        /// \param Chord  The chord, as \ref Parse reads it, where one written badly makes a shortcut no key runs.
        /// \param Action The name of the action it runs, or empty to run only its code.
        Shortcut(Text Chord, Text Action);

        /// \brief Reads a chord written as text, its modifiers and key joined by plus signs, as `Ctrl+Shift+Z`.
        ///
        /// \param Chord     The chord written as text.
        /// \param Key       The receiver of the key.
        /// \param Modifiers The receiver of the modifier keys.
        /// \return `true` if the chord was read, `false` otherwise.
        static Bool Parse(Text Chord, Ref<ZyInput::Key> Key, Ref<ZyInput::Modifier> Modifiers);
    };
}