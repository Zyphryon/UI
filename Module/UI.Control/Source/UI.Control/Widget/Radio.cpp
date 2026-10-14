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

#include "Radio.hpp"
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

    Element Radio::Create(Element Parent, Text Content, Bool Checked, Text Group, Text Style)
    {
        const Element Result = Panel::Create(Parent, Style, Direction::Row);
        Result.EditBox().Items = Align::Center;
        Widget::Govern(Result, Handler::Create<&Radio::OnEvent>(), true);
        Result.Add<Cohort>().Group = Identify(Group);

        // The mark fills the box, and the theme shows it only while the radio button is checked.
        const Element Holder = Panel::Create(Result, Widget::Part(Style, "Box"));
        const Element Mark   = Panel::Create(Holder, Widget::Part(Style, "Mark"));
        Mark.EditBox().Grow = 1.0f;

        Label::Create(Result, Content, Widget::Part(Style, "Label"));

        if (Checked)
        {
            Pick(Result);
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Radio::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Element Result = Create(Parent,
            Markup::GetText(Node, "Text"),
            Markup::GetBool(Node, "Checked"),
            Markup::GetText(Node, "Group"),
            Markup::GetText(Node, "Style", "Radio"));

        Reader.Bind(Result, Node, "Text", Binder::PutText<&Radio::SetText>());
        Reader.Bind(Result, Node, "Checked", Binder::Put<Bool, &Radio::Toggle>(),
                    Binder::TakeState<&Widget::Checked>());
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Radio::SetChecked(Ref<Interface> Owner, Handle Target, Bool Checked)
    {
        if (Owner.HasState(Target, Widget::Checked()) == Checked)
        {
            return;
        }

        Toggle(Element(Owner, Target), Checked);
        Owner.Dispatch(Event(Signal::kChange, Target));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Radio::GetChecked(Element Any)
    {
        Sequence<Element> Members;
        Gather(Any, Members);

        const SInt Found = Members.Find([](ConstRef<Element> Member)
        {
            return Member.HasState(Widget::Checked());
        });
        return Found >= 0 ? Members[static_cast<UInt>(Found)] : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Radio::Toggle(Element Target, Bool Checked)
    {
        if (Checked)
        {
            Pick(Target);
        }
        else
        {
            Target.SetState(Widget::Checked(), false);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Radio::SetText(Element Target, Text Content)
    {
        // The label stands after the box.
        Label::SetText(Target.GetFirst().GetNext(), Content);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Radio::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        if (Message.Target != Self)
        {
            return false;
        }

        // A click only ever checks it; it is cleared by checking another of its group.
        if (Message.Kind == Signal::kClick)
        {
            SetChecked(Owner, Self, true);
            return true;
        }

        if (Message.Kind != Signal::kKey)
        {
            return false;
        }

        switch (Message.Key)
        {
        case ZyInput::Key::Left:
        case ZyInput::Key::Up:
            Step(Owner, Self, -1);
            return true;
        case ZyInput::Key::Right:
        case ZyInput::Key::Down:
            Step(Owner, Self, 1);
            return true;
        default:
            return false;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Radio::Pick(Element Target)
    {
        Sequence<Element> Members;
        Gather(Target, Members);

        for (ConstRef<Element> Member : Members)
        {
            // One cleared reports nothing, so its bound flag is written back here or the model would keep it set.
            if (Member != Target && Member.HasState(Widget::Checked()))
            {
                Member.SetState(Widget::Checked(), false);
                Binder::Store(Member);
            }
        }
        Target.SetState(Widget::Checked(), true);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Radio::Step(Ref<Interface> Owner, Handle Self, SInt32 Offset)
    {
        Sequence<Element> Members;
        Gather(Element(Owner, Self), Members);

        const SInt32 Count = static_cast<SInt32>(Members.GetSize());
        const SInt32 Start = static_cast<SInt32>(Members.Find(Element(Owner, Self)));

        // A hidden or disabled neighbour answers nothing, so the check passes over it to the next one.
        for (SInt32 Moved = 1; Moved < Count; ++Moved)
        {
            const Element Next = Members[Wrap(Start + Offset * Moved, Count)];

            if (!Next.HasState(State::Hidden) && !Next.HasState(State::Disabled))
            {
                Owner.Focus(Next.GetHandle());
                SetChecked(Owner, Next.GetHandle(), true);
                return;
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Radio::Gather(Element Any, Ref<Sequence<Element>> Members)
    {
        const ConstPtr<Cohort> Facet = Any.Get<Cohort>();

        if (!Facet)
        {
            Collect(Any, 0, false, Members);
        }
        else if (Facet->Group == 0)
        {
            Collect(Any.GetParent(), 0, false, Members);
        }
        else
        {
            Collect(Any.GetRoot(), Facet->Group, true, Members);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Radio::Collect(Element Scope, UInt64 Group, Bool Deep, Ref<Sequence<Element>> Members)
    {
        for (Element Child = Scope.GetFirst(); Child.IsAlive(); Child = Child.GetNext())
        {
            if (const ConstPtr<Cohort> Facet = Child.Get<Cohort>(); Facet && Facet->Group == Group)
            {
                Members.Append(Child);
            }

            if (Deep)
            {
                Collect(Child, Group, true, Members);
            }
        }
    }
}