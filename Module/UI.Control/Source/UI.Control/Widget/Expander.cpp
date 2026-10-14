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

#include "Expander.hpp"
#include "Button.hpp"
#include "Common.hpp"
#include "Label.hpp"
#include "Memory.hpp"
#include "Panel.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Visual/Caption.hpp"
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

    Element Expander::Create(Element Parent, Text Title, Bool Expanded, Text Style)
    {
        const Element Result = Panel::Create(Parent, Style);
        Widget::Govern(Result, Handler::Create<&Expander::OnEvent>(), true);

        // A header reads from its leading edge, where a button centres what it holds.
        const Element Header = Button::Create(Result, Title, Widget::Part(Style, "Header"));
        Header.EditBox().Content = Justify::Start;

        // Made by hand, since a label is not made before the title the button holds.
        const Element Arrow = Header.Create(Text(), Button::GetLabel(Header));
        Arrow.Add<Caption>();
        Arrow.SetStyle(Widget::Part(Style, "Arrow"));

        Markup::SetHost(Result, Panel::Create(Result, Widget::Part(Style, "Content")));

        Show(Result, Expanded);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Expander::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Text    Title  = Markup::GetText(Node, "Title");
        const Text    Style  = Markup::GetText(Node, "Style", "Expander");
        const Element Result = Create(Parent, Title, Markup::GetBool(Node, "Expanded"), Style);

        // Remembered before the binding is read, so a bound flag still wins over what was left.
        Remember(Result, Memory::GetKey(Reader, Node));
        Reader.Bind(Result, Node, "Title", Binder::PutText<&Expander::SetTitle>());
        Reader.Bind(Result, Node, "Expanded", Binder::Put<Bool, &Expander::Show>(),
                    Binder::TakeState<&Widget::Checked>());
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Expander::SetExpanded(Ref<Interface> Owner, Handle Target, Bool Expanded)
    {
        if (IsExpanded(Owner, Target) != Expanded)
        {
            Show(Element(Owner, Target), Expanded);
            Owner.Dispatch(Event(Signal::kChange, Target));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Expander::IsExpanded(ConstRef<Interface> Owner, Handle Target)
    {
        return Owner.HasState(Target, Widget::Checked());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Expander::GetContent(Element Target)
    {
        return Markup::GetHost(Target);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Expander::Show(Element Target, Bool Expanded)
    {
        Target.SetState(Widget::Checked(), Expanded);
        GetContent(Target).SetState(State::Hidden, !Expanded);

        // The arrow is the first thing in the header, the first thing in the expander.
        Label::SetText(Target.GetFirst().GetFirst(), Expanded ? "v" : ">");
        Memory::SetOpened(Target, Expanded);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Expander::SetTitle(Element Target, Text Title)
    {
        // The title stands after the arrow in the header.
        Label::SetText(Target.GetFirst().GetFirst().GetNext(), Title);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Expander::Remember(Element Target, Text Key)
    {
        Memory::Hold(Target, Key);

        if (Key.IsEmpty())
        {
            return;
        }

        // One new to the key keeps the state it was made in, read back by the next made in its place.
        if (Bool Opened = false; Memory::FindOpened(Target, Opened))
        {
            Show(Target, Opened);
        }
        else
        {
            Memory::SetOpened(Target, Target.HasState(Widget::Checked()));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Expander::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        const Handle Header  = Owner.GetFirst(Self);
        const Bool   Clicked = Message.Kind == Signal::kClick && Message.Target == Header;
        const Bool   Pressed = Message.Key == ZyInput::Key::Enter || Message.Key == ZyInput::Key::Space;
        const Bool   Keyed   = Message.Kind == Signal::kKey && Pressed
            && (Message.Target == Header || Message.Target == Self);

        if (!Clicked && !Keyed)
        {
            return false;
        }

        SetExpanded(Owner, Self, !IsExpanded(Owner, Self));
        return true;
    }
}