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
#include "UI.Core/Types.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the group a radio button is exclusive within.
    struct Cohort final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The identifier of the group, spanning the whole pane, or zero for the radio buttons sharing its parent.
        UInt64 Group = 0;
    };

    /// \brief Represents a radio button, a round box holding a mark beside a label, one checked at a time per group.
    class Radio final
    {
    public:

        /// \brief Creates a radio button.
        ///
        /// \param Parent  The element to hold it.
        /// \param Content The text beside it.
        /// \param Checked The flag, `true` to start checked, clearing the rest of its group, `false` to start clear.
        /// \param Group   The name of the group it is exclusive within, or empty for the one its parent holds.
        /// \param Style   The name of the style it wears, its parts adding `.Box`, `.Mark` and `.Label`.
        /// \return The radio button, which reports \ref Signal::kChange when it is checked.
        static Element Create(Element Parent, Text Content, Bool Checked = false, Text Group = Text(),
                              Text Style = "Radio");

        /// \brief Creates a radio button from a node, beside its `Text`, in its `Group`, checked by `Checked`.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the radio button is read from.
        /// \return The radio button, made as \ref Create makes one.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Checks or clears a radio button as a click does, reporting it and clearing its group's rest.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Target  The radio button to turn.
        /// \param Checked The flag, `true` to check it, `false` to clear it.
        static void SetChecked(Ref<Interface> Owner, Handle Target, Bool Checked);

        /// \brief Gets the radio button checked in a group.
        ///
        /// \param Any The radio button of the group, or the element holding a group of radio buttons that name none.
        /// \return The radio button checked, or one naming nothing when none is.
        static Element GetChecked(Element Any);

        /// \brief Checks or clears a radio button silently, as a bound flag does, its group's rest writing back.
        ///
        /// \param Target  The radio button to turn.
        /// \param Checked The flag, `true` to check it, `false` to clear it.
        static void Toggle(Element Target, Bool Checked);

        /// \brief Changes the text beside a radio button.
        ///
        /// \param Target  The radio button whose text changes.
        /// \param Content The text to show beside it.
        static void SetText(Element Target, Text Content);

    private:

        /// \brief Runs a radio button, where a click checks it and an arrow key checks its neighbour in the group.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching it.
        /// \param Self    The radio button hearing the event.
        /// \return `true` if the event was a click or an arrow key on it, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Checks a radio button and clears the rest of its group silently, each writing its flag back.
        ///
        /// \param Target The radio button to check.
        static void Pick(Element Target);

        /// \brief Moves the check and the keyboard from a radio button to the next one of its group that answers.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Self   The radio button the check moves from.
        /// \param Offset The way to move, `-1` toward the first and `1` toward the last, wrapping around.
        static void Step(Ref<Interface> Owner, Handle Self, SInt32 Offset);

        /// \brief Gathers the radio buttons of a group, in the order they stand.
        ///
        /// \param Any     The radio button of the group, or the element holding groups of radio buttons naming none.
        /// \param Members The variable receiving the radio buttons.
        static void Gather(Element Any, Ref<Sequence<Element>> Members);

        /// \brief Gathers the radio buttons of a group beneath an element, in the order they stand.
        ///
        /// \param Scope   The element to search beneath.
        /// \param Group   The identifier of the group, or zero for the radio buttons naming none.
        /// \param Deep    The flag, `true` to search the whole subtree, `false` to search the element's children alone.
        /// \param Members The variable receiving the radio buttons.
        static void Collect(Element Scope, UInt64 Group, Bool Deep, Ref<Sequence<Element>> Members);
    };
}