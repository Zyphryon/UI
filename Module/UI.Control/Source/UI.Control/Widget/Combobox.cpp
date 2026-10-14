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

#include "Combobox.hpp"
#include "Common.hpp"
#include "Label.hpp"
#include "Listbox.hpp"
#include "Panel.hpp"
#include "Popup.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Binder.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Combobox::Create(Element Parent, Text Style)
    {
        const Element Result = Panel::Create(Parent, Style, Direction::Row);
        Result.EditBox().Items = Align::Center;
        Widget::Govern(Result, Handler::Create<&Combobox::OnEvent>(), true);

        const Element Shown = Label::Create(Result, Text(), Widget::Part(Style, "Label"));
        Shown.EditBox().Grow = 1.0f;
        Label::Create(Result, "v", Widget::Part(Style, "Arrow"));

        const Element Drop = Popup::Create(Result, Widget::Part(Style, "Popup"));
        const Element List = Listbox::Create(Drop, Widget::Part(Style, "List"));

        Ref<Choice> Parts = Result.Add<Choice>();
        Parts.List  = List.GetHandle();
        Parts.Popup = Drop.GetHandle();
        Parts.Label = Shown.GetHandle();
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Combobox::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Element Result = Create(Parent, Markup::GetText(Node, "Style", "Combobox"));
        Listbox::Fill(Reader, Result, Node);

        Reader.Bind(Result, Node, "Selected",
            Binder::Put<SInt32, &Listbox::SetIndex>(), Binder::Take<SInt32, &Listbox::GetIndex>());
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Combobox::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        const ConstPtr<Choice> Parts = Owner.Get<Choice>(Self);

        // The up and down arrows step the pick whether the popup is open or closed.
        const Bool Upward   = Message.Key == ZyInput::Key::Up;
        const Bool Downward = Message.Key == ZyInput::Key::Down;

        if (Message.Kind == Signal::kKey && Message.Target == Self && (Upward || Downward))
        {
            return Listbox::Step(Owner, Self, Upward ? -1 : 1);
        }

        if (Message.Kind == Signal::kClick && Message.Target == Self)
        {
            if (Popup::IsOpen(Owner, Parts->Popup))
            {
                Popup::Hide(Owner, Parts->Popup);
            }
            else
            {
                Popup::Show(Owner, Parts->Popup, Self);
            }
            return true;
        }

        // An item picked in its list shows in its label, closes the popup, and reaches the game as its own change.
        if (Message.Kind == Signal::kChange && Message.Target == Parts->List)
        {
            const Element Combo(Owner, Self);
            const Handle  Drop = Parts->Popup;

            Listbox::Select(Combo, Listbox::GetSelected(Combo));

            // A pick by the pointer gave the keyboard to the list, which must not keep it once its popup hides.
            if (Owner.GetRoot(Owner.GetFocused()) == Drop)
            {
                Owner.Focus(Self);
            }
            Popup::Hide(Owner, Drop);
            Owner.Dispatch(Event(Signal::kChange, Self));
            return true;
        }
        return false;
    }
}