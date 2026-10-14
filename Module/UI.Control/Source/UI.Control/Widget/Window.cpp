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

#include "Window.hpp"
#include "Button.hpp"
#include "Common.hpp"
#include "Label.hpp"
#include "Panel.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Tree.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Binder.hpp"
#include "UI.Markup/Markup.hpp"
#include "UI.Markup/Parser.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Window::Open(Ref<Tree> Target, Text Title, Text Style, Layer Band)
    {
        const Element Result(Target, Target.Open(Band));

        Result.SetStyle(Style);
        Result.EditBox().Placement = Position::Absolute;
        Result.Add<Grip>().Style = Style;
        Widget::Govern(Result, Handler::Create<&Window::OnEvent>(), false);

        const Element Bar = Panel::Create(Result, Widget::Part(Style, "Bar"), Direction::Row);
        Bar.EditBox().Items = Align::Center;

        const Element Heading = Label::Create(Bar, Title, Widget::Part(Style, "Title"));
        Heading.EditBox().Grow = 1.0f;
        Result.Get<Grip>()->Title = Heading.GetHandle();
        Button::Create(Bar, "x", Widget::Part(Style, "Close"));

        Markup::SetHost(Result, Panel::Create(Result, Widget::Part(Style, "Content")));
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Window::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Element Result = Open(Parent.GetTree(), Markup::GetText(Node, "Title"),
                                    Markup::GetText(Node, "Style", "Window"));
        Reader.Bind(Result, Node, "Title", Binder::PutText<&Window::SetTitle>());

        if (const ConstPtr<JsonValue> Least = Markup::GetField(Node, "Minimum"))
        {
            Real32 Width  = 0.0f;
            Real32 Height = 0.0f;

            if (Least->IsArray() && Least->GetArray().GetSize() == 2
                && Parser::ParseNumber(Least->GetArray()[0], Width)
                && Parser::ParseNumber(Least->GetArray()[1], Height))
            {
                SetMinimum(Result, Vector2(Width, Height));
            }
            else
            {
                LOG_W("A window's 'Minimum' must be a width and a height, so it keeps its own");
            }
        }

        if (Markup::GetBool(Node, "Resizable"))
        {
            SetResizable(Result, true);
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Window::Close(Ref<Interface> Owner, Handle Target)
    {
        Owner.SetState(Target, State::Hidden, true);

        const Ptr<Grip> Hold = Owner.Edit<Grip>(Target);
        Hold->Held   = false;
        Hold->Sizing = false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Window::Request(Ref<Interface> Owner, Handle Target)
    {
        // A handler stopping the request keeps the window open, as one asking whether to discard changes first.
        if (Owner.Dispatch(Event(kClose, Target)) || !Owner.IsAlive(Target))
        {
            return false;
        }

        Close(Owner, Target);
        Owner.Dispatch(Event(Signal::kChange, Target));
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Window::Show(Ref<Interface> Owner, Handle Target)
    {
        Owner.SetState(Target, State::Hidden, false);
        Owner.Raise(Target);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Window::GetContent(Element Target)
    {
        return Markup::GetHost(Target);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Window::MoveTo(Element Target, Vector2 Position)
    {
        Ref<Box> Shape = Target.EditBox();
        Shape.Left = Length::Pixels(Position.GetX());
        Shape.Top  = Length::Pixels(Position.GetY());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Window::SetTitle(Element Target, Text Title)
    {
        // Found through the grip, since the bar of a window made inside a walk is not yet beneath the window.
        Label::SetText(Element(Target.GetTree(), Target.Get<Grip>()->Title), Title);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Window::SetResizable(Element Target, Bool Resizable)
    {
        const Ptr<Grip> Hold = Target.Get<Grip>();

        if (!Hold)
        {
            return;
        }

        if (const Element Sizer(Target.GetTree(), Hold->Sizer); Sizer.IsAlive())
        {
            Sizer.SetState(State::Hidden, !Resizable);
            return;
        }

        if (!Resizable)
        {
            return;
        }

        // Copied, since making the edges may move the store the grip stands in.
        const Str Style = Hold->Style;

        // A panel laid over the whole window, which the pointer passes through but for the edges it holds.
        const Element Sizer = Panel::Create(Target);
        Ref<Box>      Cover = Sizer.EditBox();
        Cover.Placement = Position::Absolute;
        Cover.Left      = Length::Pixels(0.0f);
        Cover.Top       = Length::Pixels(0.0f);
        Cover.Right     = Length::Pixels(0.0f);
        Cover.Bottom    = Length::Pixels(0.0f);

        // The right edge, the bottom edge and the corner, in the order the press reads the axes each sizes along.
        const Str     Edge   = Widget::Part(Style, "Edge");
        const Element Right  = Panel::Create(Sizer, Edge);
        const Element Bottom = Panel::Create(Sizer, Edge);
        const Element Corner = Panel::Create(Sizer, Widget::Part(Style, "Grip"));

        for (const Element Part : { Right, Bottom, Corner })
        {
            Widget::Govern(Part, Handler(), false);

            Ref<Box> Shape = Part.EditBox();
            Shape.Placement = Position::Absolute;
            Shape.Right     = Length::Pixels(0.0f);
            Shape.Bottom    = Length::Pixels(0.0f);
        }

        Ref<Box> Across = Right.EditBox();
        Across.Top    = Length::Pixels(0.0f);
        Across.Bottom = Length::Pixels(kCorner);
        Across.Width  = Length::Pixels(kEdge);

        Ref<Box> Down = Bottom.EditBox();
        Down.Left   = Length::Pixels(0.0f);
        Down.Right  = Length::Pixels(kCorner);
        Down.Height = Length::Pixels(kEdge);

        Ref<Box> Both = Corner.EditBox();
        Both.Width  = Length::Pixels(kCorner);
        Both.Height = Length::Pixels(kCorner);

        // The content grows into the room a sizing gives the window.
        GetContent(Target).EditBox().Grow = 1.0f;
        Target.Get<Grip>()->Sizer = Sizer.GetHandle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Window::IsResizable(Element Target)
    {
        const ConstPtr<Grip> Hold = Target.Get<Grip>();
        const Element        Sizer(Target.GetTree(), Hold ? Hold->Sizer : Handle());
        return Sizer.IsAlive() && !Sizer.HasState(State::Hidden);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Window::SetMinimum(Element Target, Vector2 Minimum)
    {
        if (const Ptr<Grip> Hold = Target.Get<Grip>())
        {
            Hold->Minimum = Vector2::Max(Minimum, Vector2(kCorner));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Window::GetGrip(Element Target)
    {
        // The corner stands last among the edges, so it is drawn and hit over them.
        return IsResizable(Target)
            ? Widget::GetChild(Element(Target.GetTree(), Target.Get<Grip>()->Sizer), 2) : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Window::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        const Element Root(Owner, Self);

        if (Message.Kind == Signal::kPress)
        {
            // A press anywhere on it raises it, but one carried on from a popup it opened leaves it under that popup.
            if (Owner.GetRoot(Message.Target) == Self)
            {
                Owner.Raise(Self);
            }
            return OnPress(Owner, Message, Self);
        }

        // Only the window hears the drags and the release of a press it took, since nothing it holds took them.
        const Ptr<Grip> Hold = Owner.Edit<Grip>(Self);

        if (Message.Kind == Signal::kDrag && Message.Target == Self && Hold->Held)
        {
            // Kept on the target, its whole frame when it fits.
            const Vector2 Spare = Vector2::Max(Owner.GetSize() - Root.GetFrame().GetSize(), Vector2::Zero());
            MoveTo(Root, Vector2::Clamp(Message.Position - Hold->Offset, Vector2::Zero(), Spare));
            return true;
        }

        if (Message.Kind == Signal::kDrag && Hold->Sizing)
        {
            Size(Owner, Root, Message.Position);
            return true;
        }

        if (Message.Kind == Signal::kRelease && ((Hold->Held && Message.Target == Self) || Hold->Sizing))
        {
            Hold->Held   = false;
            Hold->Sizing = false;
            return true;
        }

        if (Message.Kind == Signal::kClick && Message.Target == GetClose(Root).GetHandle())
        {
            Request(Owner, Self);
            return true;
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Window::GetClose(Element Target)
    {
        // The bar holds the title first and the close button after it.
        return GetBar(Target).GetFirst().GetNext();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Window::OnPress(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        if (Message.Button != ZyInput::Button::Left)
        {
            return false;
        }

        const Element   Root(Owner, Self);
        const Ptr<Grip> Hold = Owner.Edit<Grip>(Self);

        // An edge sizes along the axes its place among the edges names, the right one across and the bottom one down.
        if (Hold->Sizer.IsValid() && Owner.GetParent(Message.Target) == Hold->Sizer)
        {
            static constexpr Array<Vector2, 3> kAxes(Vector2(1.0f, 0.0f), Vector2(0.0f, 1.0f), Vector2(1.0f, 1.0f));

            const Rect Frame = Root.GetFrame();
            Hold->Sizing = true;
            Hold->Axes   = kAxes[Min(Element(Owner, Message.Target).GetIndex(), 2u)];
            Hold->Offset = Message.Position - (Frame.GetPosition() + Frame.GetSize());
            return true;
        }

        // A press on the close button targets the button, so one targeting the window landed on no control of it.
        if (Message.Target != Self || !GetBar(Root).GetFrame().Contains(Message.Position))
        {
            return false;
        }

        Hold->Held   = true;
        Hold->Offset = Message.Position - Root.GetFrame().GetPosition();
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Window::Size(Ref<Interface> Owner, Element Root, Vector2 Point)
    {
        const ConstPtr<Grip> Hold  = Root.Get<Grip>();
        const Rect           Frame = Root.GetFrame();

        // The far corner follows the pointer, held where it was pressed, up to the target's edge.
        const Vector2 Most = Vector2::Max(Owner.GetSize() - Frame.GetPosition(), Hold->Minimum);
        const Vector2 Size = Vector2::Clamp(Point - Hold->Offset - Frame.GetPosition(), Hold->Minimum, Most);

        Ref<Box> Shape = Root.EditBox();

        if (Hold->Axes.GetX() > 0.0f)
        {
            Shape.Width = Length::Pixels(Size.GetX());
        }

        if (Hold->Axes.GetY() > 0.0f)
        {
            Shape.Height = Length::Pixels(Size.GetY());
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Window::GetBar(Element Target)
    {
        return Target.GetFirst();
    }
}