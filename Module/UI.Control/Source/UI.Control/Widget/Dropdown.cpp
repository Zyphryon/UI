// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Dropdown.hpp"
#include "Button.hpp"
#include "Common.hpp"
#include "Label.hpp"
#include "Menu.hpp"
#include "Popup.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Data/Binder.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Dropdown::Create(Element Parent, Text Content, Text Style, Text MenuStyle)
    {
        const Element Result = Button::Create(Parent, Content, Style);
        Widget::Govern(Result, Handler::Create<&Dropdown::OnEvent>(), true);

        Label::Create(Result, "v", Widget::Part(Style, "Arrow"));

        // Opened from the button, the menu leaves a press there to it, and its picks bubble through it.
        const Element Drop = Menu::Create(Result, MenuStyle);
        Result.Add<Pulldown>().Menu = Drop.GetHandle();
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Dropdown::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Element Result = Create(Parent, Markup::GetText(Node, "Text"), Markup::GetText(Node, "Style", "Dropdown"),
            Markup::GetText(Node, "MenuStyle", "Menu"));
        Reader.Bind(Result, Node, "Text", Binder::PutText<&Button::SetText>());

        if (const ConstPtr<JsonValue> Items = Markup::GetField(Node, "Items"))
        {
            Menu::Fill(Reader, GetMenu(Result), * Items);
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Dropdown::GetMenu(Element Button)
    {
        const ConstPtr<Pulldown> Found = Button.IsAlive() ? Button.Get<Pulldown>() : nullptr;
        return Found ? Element(Button.GetTree(), Found->Menu) : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dropdown::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        const ConstPtr<Pulldown> Found = Owner.Get<Pulldown>(Self);

        if (!Found || Message.Target != Self)
        {
            return false;
        }

        const Handle Drop = Found->Menu;

        // The click goes on to the button's action, which may ready the items before the menu is laid out.
        if (Message.Kind == Signal::kClick)
        {
            if (Popup::IsOpen(Owner, Drop))
            {
                Menu::Dismiss(Owner, Drop);
            }
            else
            {
                Menu::Drop(Owner, Drop, Self, true, false);
            }
            return false;
        }

        if (Message.Kind == Signal::kKey && Message.Key == ZyInput::Key::Down && !Popup::IsOpen(Owner, Drop))
        {
            Menu::Drop(Owner, Drop, Self, true, true);
            return true;
        }
        return false;
    }
}