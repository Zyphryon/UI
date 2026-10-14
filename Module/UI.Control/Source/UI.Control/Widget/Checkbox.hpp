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
#include "UI.Core/Event.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents a check box, a box holding a mark beside a label, checked and cleared by a click.
    class Checkbox final
    {
    public:

        /// \brief Creates a check box.
        ///
        /// \param Parent  The element to hold it.
        /// \param Content The text beside it.
        /// \param Checked The flag, `true` to start checked, `false` to start clear.
        /// \param Style   The name of the style it wears, its parts adding `.Box`, `.Mark` and `.Label`.
        /// \return The check box, which reports \ref Signal::kChange when it turns.
        static Element Create(Element Parent, Text Content, Bool Checked = false, Text Style = "Checkbox");

        /// \brief Creates a check box from a layout node, beside its `Text`, checked by `Checked` (bindable both ways).
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the check box is read from.
        /// \return The check box, made as \ref Create makes one.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Checks or clears a check box, as a click would, reporting \ref Signal::kChange when it turns.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Target  The check box to turn.
        /// \param Checked The flag, `true` to check it, `false` to clear it.
        static void SetChecked(Ref<Interface> Owner, Handle Target, Bool Checked);

        /// \brief Changes the text beside a check box.
        ///
        /// \param Target  The check box whose text changes.
        /// \param Content The text to show beside it.
        static void SetText(Element Target, Text Content);

    private:

        /// \brief Runs a check box, turning it on a click.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching it.
        /// \param Self    The check box hearing the event.
        /// \return `true` if the event was a click on it, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);
    };
}