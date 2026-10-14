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

#include "Slider.hpp"
#include "Common.hpp"
#include "Panel.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Facet/Visual/Bar.hpp"
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

    Element Slider::Create(Element Parent, Real32 Value, Real32 Minimum, Real32 Maximum, Text Style)
    {
        const Element Result = Parent.Create();

        // The thumb stands taller than the track, so the track keeps room above and below for it.
        Result.EditBox().Margin = Edges(0.0f, 6.0f, 0.0f, 6.0f);

        Widget::Govern(Result, Handler::Create<&Slider::OnEvent>(), true);
        Result.SetStyle(Style);
        Result.Add<Bar>();

        Ref<Level> Span = Result.Add<Level>();
        Span.Minimum = Minimum;
        Span.Maximum = Maximum;

        const Element Thumb = Panel::Create(Result, Widget::Part(Style, "Thumb"));
        Ref<Box>      Shape = Thumb.EditBox();
        Shape.Placement = Position::Absolute;
        Shape.Top       = Length::Pixels(-5.0f);

        SetValue(Result, Value);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Slider::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Element Result = Create(Parent,
            Markup::GetNumber(Node, "Value"),
            Markup::GetNumber(Node, "Minimum", 0.0f),
            Markup::GetNumber(Node, "Maximum", 1.0f),
            Markup::GetText(Node, "Style", "Slider"));

        // The range binds before the value, so the value shown first is held within the range it is given.
        Reader.Bind(Result, Node, "Minimum", Binder::Put<Real32, &Slider::SetMinimum>());
        Reader.Bind(Result, Node, "Maximum", Binder::Put<Real32, &Slider::SetMaximum>());
        Reader.Bind(Result, Node, "Value", Binder::Put<Real32, &Slider::SetValue>(),
                    Binder::Take<Real32, &Slider::GetValue>());
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Slider::SetValue(Element Target, Real32 Value)
    {
        const Ptr<Level> Span = Target.Edit<Level>();
        Span->Value = Clamp(Value, Span->Minimum, Span->Maximum);

        const Real32 Share = Span->GetShare();
        Target.Edit<Bar>()->Value = Share;

        // The thumb's middle stands on the value, whatever width the theme gives it.
        const Element Thumb = Target.GetFirst();
        Ref<Box>      Shape = Thumb.EditBox();
        const Real32  Half  = (Shape.Width.Measure == Length::Unit::Pixel ? Shape.Width.Value : 16.0f) * 0.5f;

        Shape.Left        = Length::Percent(Share);
        Shape.Margin.Left = -Half;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Slider::GetValue(Element Target)
    {
        return Target.Get<Level>()->Value;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Slider::SetMinimum(Element Target, Real32 Minimum)
    {
        Target.Edit<Level>()->Minimum = Minimum;
        SetValue(Target, GetValue(Target));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Slider::SetMaximum(Element Target, Real32 Maximum)
    {
        Target.Edit<Level>()->Maximum = Maximum;
        SetValue(Target, GetValue(Target));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Slider::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        if (Message.Target != Self)
        {
            return false;
        }

        if (Message.Kind == Signal::kKey)
        {
            return OnKey(Owner, Self, Message.Key);
        }

        const Bool Pointed = Message.Kind == Signal::kPress || Message.Kind == Signal::kDrag
            || Message.Kind == Signal::kRelease;

        if (!Pointed || Message.Button != ZyInput::Button::Left)
        {
            return false;
        }

        // A press opens the edit before it moves the value and the release closes it, so a drag is one undo step.
        if (Message.Kind == Signal::kRelease)
        {
            Owner.Dispatch(Event(Signal::kEnd, Self));
            return true;
        }

        if (Message.Kind == Signal::kPress)
        {
            Owner.Dispatch(Event(Signal::kBegin, Self));
        }

        // The value follows the pointer along the track, held within it.
        const Rect            Frame = Owner.GetFrame(Self);
        const ConstPtr<Level> Span  = Owner.Get<Level>(Self);
        const Real32          Left  = Frame.GetMinimumX();
        const Real32          Right = Frame.GetMaximumX();

        Move(Owner, Self, Remap(Message.Position.GetX(), Left, Right, Span->Minimum, Span->Maximum));
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Slider::OnKey(Ref<Interface> Owner, Handle Self, ZyInput::Key Key)
    {
        const ConstPtr<Level> Span = Owner.Get<Level>(Self);
        const Real32          Step = (Span->Maximum - Span->Minimum) * kStep;

        switch (Key)
        {
        case ZyInput::Key::Left:
            return Nudge(Owner, Self, Span->Value - Step);
        case ZyInput::Key::Right:
            return Nudge(Owner, Self, Span->Value + Step);
        case ZyInput::Key::Home:
            return Nudge(Owner, Self, Span->Minimum);
        case ZyInput::Key::End:
            return Nudge(Owner, Self, Span->Maximum);
        case ZyInput::Key::Enter:
        case ZyInput::Key::Space:
            // Kept, since a click from the keyboard lands in the middle of the track and would move the value there.
            return true;
        default:
            return false;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Slider::Move(Ref<Interface> Owner, Handle Self, Real32 Value)
    {
        const ConstPtr<Level> Span = Owner.Get<Level>(Self);

        if (const Real32 Held = Clamp(Value, Span->Minimum, Span->Maximum); Held != Span->Value)
        {
            SetValue(Element(Owner, Self), Held);
            Owner.Dispatch(Event(Signal::kChange, Self));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Slider::Nudge(Ref<Interface> Owner, Handle Self, Real32 Value)
    {
        const ConstPtr<Level> Span = Owner.Get<Level>(Self);

        // A key that moves the value is an edit of its own, and one against an end is none.
        if (Clamp(Value, Span->Minimum, Span->Maximum) != Span->Value)
        {
            Owner.Dispatch(Event(Signal::kBegin, Self));
            Move(Owner, Self, Value);
            Owner.Dispatch(Event(Signal::kEnd, Self));
        }
        return true;
    }
}