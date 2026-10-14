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
#include "UI.Core/Tree.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the parts a dialog answers through, and where it puts the keyboard back once answered.
    struct Verdict final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The name of the style the dialog wears, its buttons wearing it followed by theirs.
        Str    Style;

        /// The label showing the message.
        Handle Message;

        /// The row the buttons stand in.
        Handle Buttons;

        /// The button enter clicks, or one naming nothing.
        Handle Default;

        /// The button escape and the close button click, or one naming nothing to only close.
        Handle Cancel;

        /// The control that held the keyboard before the dialog took it.
        Handle Return;

        /// `true` once the dialog stands where it should, centred on the target or where it was put.
        Bool   Placed = false;

        /// `true` when it was put somewhere, so it is never centred.
        Bool   Pinned = false;
    };

    /// \brief Represents a dialog, a window in the \ref Layers::kModal band centred on the target, asking a question.
    class Dialog final
    {
    public:

        /// \brief Opens a dialog in a modal pane of its own, shown and centred.
        ///
        /// \param Target  The tree to open the pane in.
        /// \param Title   The text on its title bar.
        /// \param Message The question it asks.
        /// \param Style   The name of the style it wears.
        /// \return The dialog, whose children go in its body, between its message and its buttons.
        static Element Open(Ref<Tree> Target, Text Title, Text Message, Text Style = "Dialog");

        /// \brief Creates a dialog from a node's `Title`, `Message`, `Buttons`; `Left` or `Top` stops centring.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element holding the node, whose tree the dialog opens in.
        /// \param Node   The layout node the dialog is read from.
        /// \return The dialog, opened as \ref Open opens one.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Adds a button answering a dialog.
        ///
        /// \param Target  The dialog to add the button to.
        /// \param Content The text on the button.
        /// \param Action  The name of the action clicking it runs, or empty for none.
        /// \param Default The flag, `true` to make it the button enter clicks, worn as `.Default`, `false` otherwise.
        /// \param Cancel  The flag, `true` to make it the button escape and the close button click, `false` otherwise.
        /// \return The button, standing last in the dialog's row of buttons.
        static Element AddButton(
            Element Target, Text Content, Text Action = Text(), Bool Default = false, Bool Cancel = false);

        /// \brief Changes the question a dialog asks.
        ///
        /// \param Target  The dialog whose question changes.
        /// \param Message The question to ask.
        static void SetMessage(Element Target, Text Message);

        /// \brief Shows a dialog, centred, over every other modal pane, giving the keyboard to its default button.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The dialog to show.
        static void Show(Ref<Interface> Owner, Handle Target);

        /// \brief Hides a dialog without answering it, giving the keyboard back to whoever held it.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The dialog to hide.
        static void Close(Ref<Interface> Owner, Handle Target);

    private:

        /// \brief Runs a dialog, where enter and escape click its default and cancel buttons, and a click closes it.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching it.
        /// \param Self    The dialog hearing the event.
        /// \return `true` if the event answered or moved it, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Answers a dialog through one of its buttons, as a click on it would.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Self   The dialog to answer.
        /// \param Button The button to click, or one naming nothing to close it unanswered.
        /// \return `true` always, the key having been the dialog's.
        static Bool Answer(Ref<Interface> Owner, Handle Self, Handle Button);

        /// \brief Centres a dialog on the target once a layout gave it a size, every update until it is placed.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The dialog to centre.
        /// \return `true` if it moved it, `false` otherwise.
        static Bool Center(Ref<Interface> Owner, Handle Self);
    };
}