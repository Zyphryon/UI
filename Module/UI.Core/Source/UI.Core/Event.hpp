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

#include "Handle.hpp"
#include <Zyphryon.Input/Common.hpp>
#include <Zyphryon.Math/Vector2.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    class Interface;

    /// \brief Provides the signals the interface sends itself, a module making its own by \ref Identify.
    class Signal final
    {
    public:

        /// A pointer button went down over the control.
        static constexpr UInt64 kPress    = Identify("Press");

        /// The button that went down over the control came up, wherever the pointer was.
        static constexpr UInt64 kRelease  = Identify("Release");

        /// A press and release over the same control.
        static constexpr UInt64 kClick    = Identify("Click");

        /// The pointer moved while the button that went down over the control is held.
        static constexpr UInt64 kDrag     = Identify("Drag");

        /// The wheel turned over the control.
        static constexpr UInt64 kWheel    = Identify("Wheel");

        /// The control took the keyboard.
        static constexpr UInt64 kFocus    = Identify("Focus");

        /// The control lost the keyboard.
        static constexpr UInt64 kBlur     = Identify("Blur");

        /// A key went down while the control holds the keyboard.
        static constexpr UInt64 kKey      = Identify("Key");

        /// Text was typed while the control holds the keyboard.
        static constexpr UInt64 kType     = Identify("Type");

        /// The control's value changed, as a box checked or an item picked.
        static constexpr UInt64 kChange   = Identify("Change");

        /// The popup was opened at the pointer; `Other` names what it was opened over.
        static constexpr UInt64 kOpen     = Identify("Open");

        /// The pointer came over the control, sent only to a control that asks for hover events.
        static constexpr UInt64 kEnter    = Identify("Enter");

        /// The pointer left the control, sent only to a control that asks for hover events.
        static constexpr UInt64 kLeave    = Identify("Leave");

        /// The pointer moved over the control, sent only to a control that asks for hover events.
        static constexpr UInt64 kMove     = Identify("Move");

        /// The user started a gesture that edits a value, such as grabbing a number field, so an undo step can open.
        static constexpr UInt64 kBegin    = Identify("Begin");

        /// The gesture that edits a value ended, so the undo step opened by \ref kBegin can close.
        static constexpr UInt64 kEnd      = Identify("End");

        /// The value was committed, as a text field does on enter or when it loses the keyboard.
        static constexpr UInt64 kCommit   = Identify("Commit");

        /// The control was activated, as a row is by a double press or by enter.
        static constexpr UInt64 kActivate = Identify("Activate");
    };

    /// \brief Represents one event, bubbling from the control it targets up through every control holding it.
    struct Event final
    {
        /// What happened, as one of \ref Signal's or a signal a module sends.
        UInt64          Kind;

        /// The control it happened to.
        Handle          Target;

        /// The pointer button, for presses, drags, releases and clicks.
        ZyInput::Button   Button    = ZyInput::Button::Left;

        /// The key, for a key going down.
        ZyInput::Key      Key       = ZyInput::Key::Unknown;

        /// The modifier keys held, for the pointer's buttons and moves, the wheel and keys.
        ZyInput::Modifier Modifiers = ZyInput::Modifier::None;

        /// The pointer, in units of the interface.
        Vector2           Position;

        /// How far the wheel turned, in notches, or how far a drag moved, in units.
        Vector2           Delta;

        /// The text typed, for text typed into the control.
        Text              Content;

        /// The other element a drag or a popup involves, as its signal says, or one naming nothing.
        Handle            Other;

        /// How many presses of the button in a row landed on the control, two making a double click.
        UInt32           Count     = 1;

        /// \brief Constructs an event of a kind, targeting a control.
        ///
        /// \param Kind   The kind of thing that happened.
        /// \param Target The control it happened to.
        ZY_INLINE Event(UInt64 Kind, Handle Target)
            : Kind   { Kind },
              Target { Target }
        {
        }
    };

    /// \brief Defines the code a control runs on an event, returning `true` to stop it bubbling further.
    using Handler = Delegate<Bool(Ref<Interface> Interface, ConstRef<Event> Event, Handle Self)>;
}