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

#include "UI.Core/Element.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents a button, a panel answering the pointer that holds a label.
    class Button final
    {
    public:

        /// \brief Creates a button.
        ///
        /// \param Parent  The element to hold it.
        /// \param Content The text on it.
        /// \param Style   The name of the style it wears.
        /// \return The button, whose clicks its observers and its action hear.
        static Element Create(Element Parent, Text Content, Text Style = "Button");

        /// \brief Creates a button out of a node of a layout, showing its `Text`, which may bind values.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the button is read from.
        /// \return The button, made as \ref Create makes one.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Gets the label of a button.
        ///
        /// \param Target The button whose label is wanted.
        /// \return The label showing the button's text.
        static Element GetLabel(Element Target);

        /// \brief Changes the text on a button.
        ///
        /// \param Target  The button whose text changes.
        /// \param Content The text to show on it.
        static void SetText(Element Target, Text Content);

    private:

        /// \brief Runs a toggle button, turning its checked state over on a click.
        ///
        /// \param Owner   The interface routing the event, through which the turned state is reported as a change.
        /// \param Message The event climbing through the button; only a click whose target is the button turns it.
        /// \param Self    The handle of the toggle button whose checked state is turned over.
        /// \return `false` always, so the click goes on to the button's action.
        static Bool OnToggle(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);
    };
}