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

#include "Checkbox.hpp"
#include "Common.hpp"
#include "Label.hpp"
#include "Panel.hpp"
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

    Element Checkbox::Create(Element Parent, Text Content, Bool Checked, Text Style)
    {
        const Element Result = Panel::Create(Parent, Style, Direction::Row);
        Result.EditBox().Items = Align::Center;
        Widget::Govern(Result, Handler::Create<&Checkbox::OnEvent>(), true);

        // The mark fills the box, and the theme shows it only while the check box is checked.
        const Element Holder = Panel::Create(Result, Widget::Part(Style, "Box"));
        const Element Mark   = Panel::Create(Holder, Widget::Part(Style, "Mark"));
        Mark.EditBox().Grow = 1.0f;

        Label::Create(Result, Content, Widget::Part(Style, "Label"));

        Result.SetState(Widget::Checked(), Checked);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Checkbox::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Text    Content = Markup::GetText(Node, "Text");
        const Text    Style   = Markup::GetText(Node, "Style", "Checkbox");
        const Element Result  = Create(Parent, Content, Markup::GetBool(Node, "Checked"), Style);
        Reader.Bind(Result, Node, "Text", Binder::PutText<&Checkbox::SetText>());
        Reader.Bind(Result, Node, "Checked", Binder::PutState<&Widget::Checked>(),
                    Binder::TakeState<&Widget::Checked>());
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Checkbox::SetChecked(Ref<Interface> Owner, Handle Target, Bool Checked)
    {
        if (Owner.HasState(Target, Widget::Checked()) != Checked)
        {
            Owner.SetState(Target, Widget::Checked(), Checked);
            Owner.Dispatch(Event(Signal::kChange, Target));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Checkbox::SetText(Element Target, Text Content)
    {
        // The label stands after the box.
        Label::SetText(Target.GetFirst().GetNext(), Content);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Checkbox::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        if (Message.Kind != Signal::kClick || Message.Target != Self)
        {
            return false;
        }

        // A click is a whole edit, so it opens and closes around the turn for an undo step to hold it.
        Owner.Dispatch(Event(Signal::kBegin, Self));
        SetChecked(Owner, Self, !Owner.HasState(Self, Widget::Checked()));
        Owner.Dispatch(Event(Signal::kEnd, Self));
        return true;
    }
}