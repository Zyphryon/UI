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

#include "Tabs.hpp"
#include "Button.hpp"
#include "Common.hpp"
#include "Panel.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Input/Control.hpp"
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

    Element Tabs::Create(Element Parent, Text Style)
    {
        const Element Result = Panel::Create(Parent, Style);
        Widget::Govern(Result, Handler::Create<&Tabs::OnEvent>(), false);

        const Element Header = Panel::Create(Result, Widget::Part(Style, "Header"), Direction::Row);
        const Element Pages  = Panel::Create(Result, Widget::Part(Style, "Pages"));
        Pages.EditBox().Grow = 1.0f;

        Ref<Deck> Parts = Result.Add<Deck>();
        Parts.Header = Header.GetHandle();
        Parts.Pages  = Pages.GetHandle();
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Tabs::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Text    Style  = Markup::GetText(Node, "Style", "Tabs");
        const Element Result = Create(Parent, Style);

        if (const ConstPtr<JsonValue> List = Markup::GetField(Node, "Tabs"); List && List->IsArray())
        {
            const Bool Closable = Markup::GetBool(Node, "Closable");

            for (ConstRef<JsonValue> Entry : List->GetArray())
            {
                BuildTab(Reader, Result, Entry, Style, Closable);
            }
        }

        // A negative index names no tab, so the first one stays shown.
        if (const Real32 Picked = Markup::GetNumber(Node, "Selected"); Picked > 0.0f)
        {
            Select(Result, static_cast<UInt32>(Picked));
        }

        Reader.Bind(Result, Node, "Selected", Binder::Put<UInt32, &Tabs::Select>(),
                    Binder::Take<UInt32, &Tabs::GetSelected>());
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Tabs::AddTab(Element Owner, Text Title, Text Style, Bool Closable)
    {
        const ConstPtr<Deck> Parts = Owner.Get<Deck>();
        const Element        Header(Owner.GetTree(), Parts->Header);
        const Element        Pages(Owner.GetTree(), Parts->Pages);
        const Bool           Shown = (GetCount(Owner) == Parts->Selected);

        const Element Tab  = Button::Create(Header, Title, Widget::Part(Style, "Tab"));
        const Element Page = Panel::Create(Pages, Widget::Part(Style, "Page"));
        Page.EditBox().Grow = 1.0f;

        Tab.SetState(Widget::Selected(), Shown);
        Page.SetState(State::Hidden, !Shown);

        if (Closable)
        {
            Equip(Tab, true, Style);
        }
        return Page;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tabs::SetClosable(Element Owner, UInt32 Index, Bool Closable, Text Style)
    {
        if (const Element Tab = GetTab(Owner, Index); Tab.IsAlive())
        {
            Equip(Tab, Closable, Style);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Tabs::GetClose(Element Owner, UInt32 Index)
    {
        // A tab holds its title first and its close button, when it has one, after it.
        const Element Tab = GetTab(Owner, Index);
        return Tab.IsAlive() ? Tab.GetFirst().GetNext() : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tabs::RemoveTab(Element Owner, UInt32 Index)
    {
        const UInt32 Count = GetCount(Owner);

        if (Index >= Count)
        {
            return;
        }

        const UInt32  Shown = GetSelected(Owner);
        const Element Tab   = GetTab(Owner, Index);
        const Element Page  = GetPage(Owner, Index);

        // Selected before the tab goes, since one destroyed inside a walk still stands until the walk ends.
        UInt32 Kept = Shown;

        if (Shown == Index)
        {
            Kept = Index + 1 < Count ? Index + 1 : (Index > 0 ? Index - 1 : Index);
        }

        if (Kept != Index)
        {
            Select(Owner, Kept);
        }

        Tab.Destruct();

        if (Page.IsAlive())
        {
            Page.Destruct();
        }

        Owner.Get<Deck>()->Selected = Kept > Index ? Kept - 1 : (Kept == Index ? 0 : Kept);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tabs::Select(Element Owner, UInt32 Index)
    {
        if (Index >= GetCount(Owner))
        {
            return;
        }

        const Ptr<Deck> Parts = Owner.Get<Deck>();
        Parts->Selected = Index;

        Element Tab  = Element(Owner.GetTree(), Parts->Header).GetFirst();
        Element Page = Element(Owner.GetTree(), Parts->Pages).GetFirst();

        for (UInt32 Position = 0; Tab.IsAlive(); ++Position)
        {
            Tab.SetState(Widget::Selected(), Position == Index);

            if (Page.IsAlive())
            {
                Page.SetState(State::Hidden, Position != Index);
                Page = Page.GetNext();
            }
            Tab = Tab.GetNext();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Tabs::GetSelected(Element Owner)
    {
        return Owner.Get<Deck>()->Selected;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Tabs::GetCount(Element Owner)
    {
        return Widget::GetChildCount(Element(Owner.GetTree(), Owner.Get<Deck>()->Header));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Tabs::GetTab(Element Owner, UInt32 Index)
    {
        return Widget::GetChild(Element(Owner.GetTree(), Owner.Get<Deck>()->Header), Index);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Tabs::GetPage(Element Owner, UInt32 Index)
    {
        return Widget::GetChild(Element(Owner.GetTree(), Owner.Get<Deck>()->Pages), Index);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tabs::BuildTab(Ref<Markup> Reader, Element Owner, ConstRef<JsonValue> Node, Text Style, Bool Closable)
    {
        const Element Page = AddTab(Owner, Markup::GetText(Node, "Title"), Style,
                                    Markup::GetBool(Node, "Closable", Closable));
        Reader.Bind(GetTab(Owner, GetCount(Owner) - 1), Node, "Title", Binder::PutText<&Button::SetText>());

        if (const Text Name = Markup::GetText(Node, "Name"); !Name.IsEmpty())
        {
            Page.SetName(Name);
        }

        const ConstPtr<JsonValue> Children = Markup::GetField(Node, "Children");

        if (Children && Children->IsArray())
        {
            for (ConstRef<JsonValue> Child : Children->GetArray())
            {
                Reader.Build(Page, Child);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tabs::Step(Ref<Interface> Owner, Handle Self, SInt32 Offset)
    {
        const Element Root(Owner, Self);
        const SInt32  Count = static_cast<SInt32>(GetCount(Root));
        const UInt32  Shown = GetSelected(Root);
        const UInt32  Index = static_cast<UInt32>(Wrap(static_cast<SInt32>(Shown) + Offset, Count));

        if (Index == Shown)
        {
            return;
        }

        Select(Root, Index);
        Owner.Focus(GetTab(Root, Index).GetHandle());
        Owner.Dispatch(Event(Signal::kChange, Self));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Tabs::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        // Only its own tabs answer; a click deeper down, as on a button in a page, belongs to whatever holds it.
        const Handle Header = Owner.Get<Deck>(Self)->Header;

        if (Message.Target == Self)
        {
            return false;
        }

        // A close button stands inside its tab, after the title.
        const Element Pressed(Owner, Message.Target);
        const Bool    Clicked = Message.Kind == Signal::kClick;

        if (Clicked && Owner.GetParent(Owner.GetParent(Message.Target)) == Header)
        {
            const Element Tab = Pressed.GetParent();

            if (Tab.GetFirst().GetNext() == Pressed)
            {
                Shut(Owner, Self, Tab);
                return true;
            }
        }

        if (Owner.GetParent(Message.Target) != Header)
        {
            return false;
        }

        const Bool Sideways = Message.Key == ZyInput::Key::Left || Message.Key == ZyInput::Key::Right;

        if (Message.Kind == Signal::kKey && Sideways)
        {
            Step(Owner, Self, Message.Key == ZyInput::Key::Left ? -1 : 1);
            return true;
        }

        if (Message.Kind != Signal::kClick)
        {
            return false;
        }

        const Element Root(Owner, Self);
        const UInt32  Index = Element(Owner, Message.Target).GetIndex();

        if (Index != GetSelected(Root))
        {
            Select(Root, Index);
            Owner.Dispatch(Event(Signal::kChange, Self));
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tabs::Shut(Ref<Interface> Owner, Handle Self, Element Tab)
    {
        Event Request(kClose, Self);
        Request.Other = Tab.GetHandle();

        if (Owner.Dispatch(Request) || !Tab.IsAlive())
        {
            return;
        }

        const Element Root(Owner, Self);
        const UInt32  Index = Tab.GetIndex();
        const UInt32  Shown = GetSelected(Root);
        RemoveTab(Root, Index);

        // Closing the tab shown shows another, and closing one before it moves the index of the one shown.
        if (Shown >= Index)
        {
            Owner.Dispatch(Event(Signal::kChange, Self));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tabs::Equip(Element Tab, Bool Closable, Text Style)
    {
        const Element Found = Tab.GetFirst().GetNext();

        if (Found.IsAlive() == Closable)
        {
            return;
        }

        if (!Closable)
        {
            Found.Destruct();
            return;
        }

        // Never focused, so the keyboard steps from tab to tab and not onto the buttons inside them.
        const Element Close = Button::Create(Tab, "x", Widget::Part(Style, "Close"));
        Close.Get<Control>()->Focusable = false;
    }
}