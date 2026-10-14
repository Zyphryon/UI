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

#include "Spinner.hpp"
#include "Button.hpp"
#include "Common.hpp"
#include "Label.hpp"
#include "Panel.hpp"
#include "Slider.hpp"
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

    Element Spinner::Create(Element Parent,
                            Real32  Value,
                            Real32  Minimum,
                            Real32  Maximum,
                            Real32  Step,
                            UInt32  Decimals,
                            Text    Style)
    {
        const Element Result = Panel::Create(Parent, Style, Direction::Row);
        Result.EditBox().Items = Align::Center;

        Widget::Govern(Result, Handler::Create<&Spinner::OnEvent>(), true);

        Ref<Level> Span = Result.Add<Level>();
        Span.Minimum = Minimum;
        Span.Maximum = Maximum;

        Ref<Stepper> Pace = Result.Add<Stepper>();
        Pace.Step     = Step;
        Pace.Decimals = Decimals;

        // The buttons leave the keyboard with the box, so a click on them keeps the box focused.
        const Element Decrease = Button::Create(Result, "-", Widget::Part(Style, "Decrease"));
        Widget::Govern(Decrease, Handler(), false);

        const Element Shown = Label::Create(Result, Text(), Widget::Part(Style, "Label"));
        Shown.EditBox().Grow = 1.0f;

        const Element Increase = Button::Create(Result, "+", Widget::Part(Style, "Increase"));
        Widget::Govern(Increase, Handler(), false);

        SetValue(Result, Value);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Spinner::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Element Result = Create(Parent,
            Markup::GetNumber(Node, "Value"),
            Markup::GetNumber(Node, "Minimum", 0.0f),
            Markup::GetNumber(Node, "Maximum", 100.0f),
            Markup::GetNumber(Node, "Step", 1.0f),
            static_cast<UInt32>(Max(Markup::GetNumber(Node, "Decimals", 0.0f), 0.0f)),
            Markup::GetText(Node, "Style", "Spinner"));

        // The range binds before the value, so the value shown first is held within the range it is given.
        Reader.Bind(Result, Node, "Minimum", Binder::Put<Real32, &Spinner::SetMinimum>());
        Reader.Bind(Result, Node, "Maximum", Binder::Put<Real32, &Spinner::SetMaximum>());
        Reader.Bind(Result, Node, "Value", Binder::Put<Real32, &Spinner::SetValue>(),
                    Binder::Take<Real32, &Spinner::GetValue>());
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Spinner::SetValue(Element Target, Real32 Value)
    {
        const Ptr<Level> Span = Target.Edit<Level>();
        Span->Value = Clamp(Value, Span->Minimum, Span->Maximum);

        Str Shown;
        Shown.AppendReal(Span->Value, Min(Target.Get<Stepper>()->Decimals, kMaxDecimals));
        Label::SetText(Target.GetFirst().GetNext(), Shown);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Spinner::GetValue(Element Target)
    {
        return Target.Get<Level>()->Value;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Spinner::SetMinimum(Element Target, Real32 Minimum)
    {
        Target.Edit<Level>()->Minimum = Minimum;
        SetValue(Target, GetValue(Target));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Spinner::SetMaximum(Element Target, Real32 Maximum)
    {
        Target.Edit<Level>()->Maximum = Maximum;
        SetValue(Target, GetValue(Target));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Spinner::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        const Element Decrease = Element(Owner, Self).GetFirst();
        const Element Increase = Decrease.GetNext().GetNext();
        const Bool    Stepping = Message.Target == Decrease.GetHandle() || Message.Target == Increase.GetHandle();

        // A press on a button steps at once and opens the edit, its release closes it, so a press is one undo step.
        if (Stepping && Message.Button == ZyInput::Button::Left)
        {
            switch (Message.Kind)
            {
            case Signal::kPress:
                Owner.Dispatch(Event(Signal::kBegin, Self));
                Move(Owner, Self, Message.Target == Increase.GetHandle() ? 1.0f : -1.0f);

                // Presses bubble on, so a window holding the box hears them.
                return false;
            case Signal::kRelease:
                Owner.Dispatch(Event(Signal::kEnd, Self));
                return true;
            case Signal::kClick:
                return true;
            default:
                break;
            }
        }

        // The wheel reaches the box from whichever part the pointer stands over.
        if (Message.Kind == Signal::kWheel && Message.Delta.GetY() != 0.0f)
        {
            Nudge(Owner, Self, Sign(Message.Delta.GetY()));
            return true;
        }

        if (Message.Kind == Signal::kKey && Message.Target == Self
            && (Message.Key == ZyInput::Key::Up || Message.Key == ZyInput::Key::Down))
        {
            Nudge(Owner, Self, Message.Key == ZyInput::Key::Up ? 1.0f : -1.0f);
            return true;
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Spinner::Move(Ref<Interface> Owner, Handle Self, Real32 Amount)
    {
        const Element Target(Owner, Self);
        const Real32  Previous = GetValue(Target);

        SetValue(Target, Previous + Amount * Target.Get<Stepper>()->Step);

        // A step against an end leaves the value where it was, so nothing changed to report.
        if (GetValue(Target) != Previous)
        {
            Owner.Dispatch(Event(Signal::kChange, Self));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Spinner::Nudge(Ref<Interface> Owner, Handle Self, Real32 Amount)
    {
        const Element         Target(Owner, Self);
        const ConstPtr<Level> Span = Target.Get<Level>();

        // A notch or a key that moves the value is an edit of its own, and one against an end is none.
        const Real32 Moved = Clamp(Span->Value + Amount * Target.Get<Stepper>()->Step, Span->Minimum, Span->Maximum);

        if (Moved != Span->Value)
        {
            Owner.Dispatch(Event(Signal::kBegin, Self));
            Move(Owner, Self, Amount);
            Owner.Dispatch(Event(Signal::kEnd, Self));
        }
    }
}