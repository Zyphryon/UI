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

#include "Scroller.hpp"
#include "Common.hpp"
#include "Memory.hpp"
#include "Panel.hpp"
#include "Scrollbar.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Layout/Virtual.hpp"
#include "UI.Core/Facet/Visual/Scroll.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Scroller::Create(Element Parent, Text Style)
    {
        // A row of the body, holding the content over the sideways bar, and the bar down the side.
        const Element Result = Panel::Create(Parent, Style, Direction::Row);
        Widget::Govern(Result, Handler::Create<&Scroller::OnEvent>(), true);
        Result.EditBox().Height = Length::Pixels(kHeight);

        // Both grow from nothing, so the room they take is the viewer's and never what their content asks.
        const Element Body = Panel::Create(Result, Widget::Part(Style, "Body"));
        Body.EditBox().Grow  = 1.0f;
        Body.EditBox().Basis = Length::Pixels(0.0f);

        const Element Content = Panel::Create(Body, Widget::Part(Style, "Content"));
        Content.EditBox().Grow  = 1.0f;
        Content.EditBox().Basis = Length::Pixels(0.0f);

        // A control of its own, so the wheel over its bare background lands on it and scrolls it.
        Widget::Govern(Content, Handler(), false);

        // The bars show where it stands, so it paints no thumb and keeps no gutter for one.
        Ref<Scroll> Roll = Content.Add<Scroll>();
        Roll.Thumb      = Color::Transparent();
        Roll.Gutter     = 0.0f;
        Roll.Horizontal = true;

        Hook(Scrollbar::Create(Body, false, Widget::Part(Style, "Bar")), Content);
        Hook(Scrollbar::Create(Result, true, Widget::Part(Style, "Bar")), Content);

        Markup::SetHost(Result, Content);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Scroller::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Element Result = Create(Parent, Markup::GetText(Node, "Style", "Scroller"));

        SetHorizontal(Result, Markup::GetBool(Node, "Horizontal", true));
        SetFollow(Result, Markup::GetBool(Node, "Follow"));

        if (const Str Key = Memory::GetKey(Reader, Node); !Key.IsEmpty())
        {
            Remember(Result, Key);
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Scroller::GetContent(Element Viewer)
    {
        return Markup::GetHost(Viewer);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Scroller::GetBar(Element Viewer, Bool Vertical)
    {
        // The body comes first, holding the content and then the sideways bar; the other bar stands after the body.
        const Element Body = Viewer.GetFirst();
        return Vertical ? Body.GetNext() : Body.GetFirst().GetNext();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scroller::SetHorizontal(Element Viewer, Bool Horizontal)
    {
        const Element Content = GetContent(Viewer);

        if (const ConstPtr<Scroll> Found = Content.Get<Scroll>(); Found && Found->Horizontal != Horizontal)
        {
            const Ptr<Scroll> Roll = Content.Edit<Scroll>();
            Roll->Horizontal = Horizontal;
            Roll->OffsetX    = 0.0f;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scroller::SetFollow(Element Viewer, Bool Follow)
    {
        if (const Ptr<Scroll> Roll = GetContent(Viewer).Edit<Scroll>())
        {
            Roll->Follow = Follow;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scroller::ScrollTo(Element Viewer, Vector2 Offset)
    {
        const Element Content = GetContent(Viewer);
        Scrollbar::ScrollTo(Content, false, Offset.GetX());
        Scrollbar::ScrollTo(Content, true, Offset.GetY());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Vector2 Scroller::GetOffset(Element Viewer)
    {
        const Element Content = GetContent(Viewer);
        return Vector2(Scrollbar::GetOffset(Content, false), Scrollbar::GetOffset(Content, true));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scroller::Remember(Element Viewer, Text Key)
    {
        Memory::Hold(GetContent(Viewer), Key);

        // Only a remembered viewer runs every update, so the others cost nothing.
        if (!Key.IsEmpty() && !Viewer.Has<Virtual>())
        {
            Viewer.Add<Virtual>().Realize = Realizer::Create<&Scroller::OnRealize>();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Scroller::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        // Only keys it hears itself; one bubbling from a control inside it belongs to that control and the keyboard.
        if (Message.Kind != Signal::kKey || Message.Target != Self)
        {
            return false;
        }
        return OnKey(Element(Owner, Self), Message.Key);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Scroller::OnKey(Element Viewer, ZyInput::Key Key)
    {
        const Element Content = GetContent(Viewer);
        const Real32  Down    = Scrollbar::GetOffset(Content, true);
        const Real32  Across  = Scrollbar::GetOffset(Content, false);
        const Real32  Page    = Scrollbar::GetView(Content, true);

        // A move that finds the end already reached is left to the keyboard, so the arrows walk out of the viewer.
        switch (Key)
        {
        case ZyInput::Key::Up:
            return Scrollbar::ScrollTo(Content, true, Down - kStep);
        case ZyInput::Key::Down:
            return Scrollbar::ScrollTo(Content, true, Down + kStep);
        case ZyInput::Key::Left:
            return Scrollbar::ScrollTo(Content, false, Across - kStep);
        case ZyInput::Key::Right:
            return Scrollbar::ScrollTo(Content, false, Across + kStep);
        case ZyInput::Key::PageUp:
            return Scrollbar::ScrollTo(Content, true, Down - Page);
        case ZyInput::Key::PageDown:
            return Scrollbar::ScrollTo(Content, true, Down + Page);
        case ZyInput::Key::Home:
            return Scrollbar::ScrollTo(Content, true, 0.0f);
        case ZyInput::Key::End:
            return Scrollbar::ScrollTo(Content, true, Scrollbar::GetLimit(Content, true));
        default:
            return false;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Scroller::OnRealize(Ref<Interface> Owner, Handle Self)
    {
        return Memory::Track(GetContent(Element(Owner, Self)));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scroller::Hook(Element Bar, Element Content)
    {
        Scrollbar::SetTarget(Bar, Content);
        Scrollbar::SetAutomatic(Bar, true);
        Bar.SetState(State::Hidden, true);
    }
}