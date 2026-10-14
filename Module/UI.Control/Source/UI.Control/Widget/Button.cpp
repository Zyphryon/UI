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

#include "Button.hpp"
#include "Checkbox.hpp"
#include "Common.hpp"
#include "Label.hpp"
#include "Panel.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
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

    Element Button::Create(Element Parent, Text Content, Text Style)
    {
        const Element Result = Panel::Create(Parent, Style, Direction::Row);

        Ref<Box> Shape = Result.EditBox();
        Shape.Items   = Align::Center;
        Shape.Content = Justify::Center;

        Widget::Govern(Result, Handler(), true);

        Label::Create(Result, Content, Widget::Part(Style, "Label"));
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Button::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Element Result = Create(Parent, Markup::GetText(Node, "Text"), Markup::GetText(Node, "Style", "Button"));
        Reader.Bind(Result, Node, "Text", Binder::PutText<&Button::SetText>());

        // A button standing for a flag wears the checked state, which a toggle turns over itself.
        Result.SetState(Widget::Checked(), Markup::GetBool(Node, "Checked"));
        Reader.Bind(Result, Node, "Checked", Binder::PutState<&Widget::Checked>(), Binder::TakeState<&Widget::Checked>());

        if (Markup::GetBool(Node, "Toggle"))
        {
            Widget::Govern(Result, Handler::Create<&Button::OnToggle>(), true);
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Button::GetLabel(Element Target)
    {
        return Target.GetFirst();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Button::SetText(Element Target, Text Content)
    {
        Label::SetText(GetLabel(Target), Content);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Button::OnToggle(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        if (Message.Kind == Signal::kClick && Message.Target == Self)
        {
            Owner.Dispatch(Event(Signal::kBegin, Self));
            Checkbox::SetChecked(Owner, Self, !Owner.HasState(Self, Widget::Checked()));
            Owner.Dispatch(Event(Signal::kEnd, Self));
        }
        return false;
    }
}