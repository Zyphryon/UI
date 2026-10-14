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
    /// \brief Represents a dropdown, a button whose text never changes with a pick, opening a \ref Menu of actions.
    class Dropdown final
    {
    public:

        /// \brief Creates a dropdown, its menu holding no item.
        ///
        /// \param Parent    The element to hold it.
        /// \param Content   The text it shows, as `+ Add Component`.
        /// \param Style     The name of the style it wears, its parts wearing it followed by `.Label` and `.Arrow`.
        /// \param MenuStyle The name of the style its menu wears.
        /// \return The dropdown, whose menu takes items through \ref Menu::AddItem and the like on \ref GetMenu.
        static Element Create(Element Parent, Text Content, Text Style = "Dropdown", Text MenuStyle = "Menu");

        /// \brief Creates a dropdown out of a node of a layout, showing its `Text` and opening the menu of its `Items`.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the dropdown is read from, whose `MenuStyle` names the style of its menu.
        /// \return The dropdown, made as \ref Create makes one.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Gets the menu a dropdown opens.
        ///
        /// \param Button The dropdown to look in.
        /// \return The menu, whose actions read the button through \ref Menu::GetSubject.
        static Element GetMenu(Element Button);

    private:

        /// \brief Runs a dropdown, where a click opens or closes its menu, and down opens it with its first item lit.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching it.
        /// \param Self    The dropdown hearing the event.
        /// \return `true` if the key opened the menu, `false` otherwise, so the click still reaches the action.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);
    };
}