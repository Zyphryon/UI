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
    /// \brief Represents an expander, a header that opens and closes the content beneath it on a click.
    class Expander final
    {
    public:

        /// \brief Creates an expander.
        ///
        /// \param Parent   The element to hold it.
        /// \param Title    The text on its header.
        /// \param Expanded The flag, `true` to start open, `false` to start closed.
        /// \param Style    The name of the style it wears, its parts adding `.Header`, `.Arrow` and `.Content`.
        /// \return The expander, which reports \ref Signal::kChange when it opens or closes.
        static Element Create(Element Parent, Text Title, Bool Expanded = false, Text Style = "Expander");

        /// \brief Creates an expander from a layout node, under its `Title`, open by `Expanded`; a `Name` remembers it.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the expander is read from.
        /// \return The expander, whose content takes the node's children.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Opens or closes an expander as a click on its header would, reporting \ref Signal::kChange.
        ///
        /// \param Owner    The interface it stands in.
        /// \param Target   The expander to open or close.
        /// \param Expanded The flag, `true` to open it, `false` to close it.
        static void SetExpanded(Ref<Interface> Owner, Handle Target, Bool Expanded);

        /// \brief Checks whether an expander is open.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The expander to look at.
        /// \return `true` if it is, `false` otherwise.
        static Bool IsExpanded(ConstRef<Interface> Owner, Handle Target);

        /// \brief Gets the part of an expander its content goes in.
        ///
        /// \param Target The expander whose content is wanted.
        /// \return The panel beneath the header, hidden while the expander is closed.
        static Element GetContent(Element Target);

        /// \brief Shows an expander open or closed, reporting nothing, as a bound flag of the model does.
        ///
        /// \param Target   The expander to show.
        /// \param Expanded The flag, `true` to show it open, `false` to show it closed.
        static void Show(Element Target, Bool Expanded);

        /// \brief Changes the title on an expander's header.
        ///
        /// \param Target The expander whose title changes.
        /// \param Title  The text to show on its header.
        static void SetTitle(Element Target, Text Title);

        /// \brief Makes the interface remember whether an expander is open under a key, restoring it as left.
        ///
        /// \param Target The expander remembered.
        /// \param Key    The key, unique among what the interface remembers, or empty to remember it no more.
        static void Remember(Element Target, Text Key);

    private:

        /// \brief Runs an expander, turning it on a click on its header, or on enter or space while focused.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching it.
        /// \param Self    The expander hearing the event.
        /// \return `true` if the event turned it, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);
    };
}