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

#include "Dialog.hpp"
#include "Button.hpp"
#include "Common.hpp"
#include "Label.hpp"
#include "Panel.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Input/Control.hpp"
#include "UI.Core/Facet/Layout/Virtual.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Layers.hpp"
#include "UI.Core/Tree.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Binder.hpp"
#include "UI.Markup/Markup.hpp"
#include "Window.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Dialog::Open(Ref<Tree> Target, Text Title, Text Message, Text Style)
    {
        const Element Result = Window::Open(Target, Title, Style, Layers::kModal);

        Widget::Govern(Result, Handler::Create<&Dialog::OnEvent>(), true);
        Result.Add<Virtual>().Realize = Realizer::Create<&Dialog::Center>();

        // The message opens the body, where the children given to it follow; the buttons stand under it all.
        const Element Content = Window::GetContent(Result);
        const Element Body    = Panel::Create(Content, Widget::Part(Style, "Body"));
        const Element Words   = Label::Create(Body, Message, Widget::Part(Style, "Message"));
        const Element Row     = Panel::Create(Content, Widget::Part(Style, "Buttons"), Direction::Row);

        Row.EditBox().Content = Justify::End;
        Markup::SetHost(Result, Body);

        Ref<Verdict> Own = Result.Add<Verdict>();
        Own.Style   = Style;
        Own.Message = Words.GetHandle();
        Own.Buttons = Row.GetHandle();
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Dialog::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Element Result = Open(
            Parent.GetTree(),
            Markup::GetText(Node, "Title"),
            Markup::GetText(Node, "Message"),
            Markup::GetText(Node, "Style", "Dialog"));

        Reader.Bind(Result, Node, "Title", Binder::PutText<&Window::SetTitle>());
        Reader.Bind(Result, Node, "Message", Binder::PutText<&Dialog::SetMessage>());

        // Put somewhere, it stays there rather than being centred.
        if (Markup::GetField(Node, "Left") || Markup::GetField(Node, "Top"))
        {
            Ref<Verdict> Own = * Result.Get<Verdict>();
            Own.Pinned = true;
            Own.Placed = true;
        }

        const ConstPtr<JsonValue> Buttons = Markup::GetField(Node, "Buttons");

        if (!Buttons)
        {
            return Result;
        }

        if (!Buttons->IsArray())
        {
            LOG_W("A dialog's 'Buttons' must be a list, so it shows none");
            return Result;
        }

        for (ConstRef<JsonValue> Entry : Buttons->GetArray())
        {
            const Element Made = AddButton(Result, Markup::GetText(Entry, "Text"), Markup::GetText(Entry, "Action"),
                Markup::GetBool(Entry, "Default"), Markup::GetBool(Entry, "Cancel"));

            if (const Text Name = Markup::GetText(Entry, "Name"); !Name.IsEmpty())
            {
                Made.SetName(Name);
            }
            Reader.Bind(Made, Entry, "Text", Binder::PutText<&Button::SetText>());
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Dialog::AddButton(Element Target, Text Content, Text Action, Bool Default, Bool Cancel)
    {
        // Copied, since making the button may move the store the dialog's own facet stands in.
        const ConstPtr<Verdict> Found = Target.Get<Verdict>();
        const Str               Style(Text(Found->Style));
        const Element           Row(Target.GetTree(), Found->Buttons);
        const Text              Role  = Default ? "Default"_Text : "Button"_Text;
        const Element           Made  = Button::Create(Row, Content, Widget::Part(Style, Role));

        if (!Action.IsEmpty())
        {
            Made.Get<Control>()->Action = Identify(Action);
        }

        const Ptr<Verdict> Own = Target.Get<Verdict>();

        if (Default)
        {
            Own->Default = Made.GetHandle();
        }

        if (Cancel)
        {
            Own->Cancel = Made.GetHandle();
        }
        return Made;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dialog::SetMessage(Element Target, Text Message)
    {
        Label::SetText(Element(Target.GetTree(), Target.Get<Verdict>()->Message), Message);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dialog::Show(Ref<Interface> Owner, Handle Target)
    {
        const Ptr<Verdict> Own     = Owner.Get<Verdict>(Target);
        const Handle       Focused = Owner.GetFocused();

        // Shown again, it is centred again, unless it was put somewhere.
        Own->Placed = Own->Pinned;

        if (Owner.GetRoot(Focused) != Target)
        {
            Own->Return = Focused;
        }

        const Handle Default = Own->Default;

        Window::Show(Owner, Target);
        Owner.Focus(Owner.IsAlive(Default) ? Default : Target);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dialog::Close(Ref<Interface> Owner, Handle Target)
    {
        const Handle Return = Owner.Get<Verdict>(Target)->Return;

        Window::Close(Owner, Target);

        // The keyboard leaves with it, back to whoever held it before when that still can.
        if (Owner.GetRoot(Owner.GetFocused()) == Target)
        {
            const Bool Usable = Owner.IsAlive(Return) && !Owner.IsPending(Return)
                && !Owner.HasState(Return, State::Hidden);
            Owner.Focus(Usable ? Return : Handle());
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dialog::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        const ConstPtr<Verdict> Own     = Owner.Get<Verdict>(Self);
        const Handle            Buttons = Own->Buttons;
        const Handle            Default = Own->Default;
        const Handle            Cancel  = Own->Cancel;

        // Enter clicks the button holding the keyboard, or else the default one; escape clicks the cancel one.
        if (Message.Kind == Signal::kKey && Message.Key == ZyInput::Key::Enter)
        {
            if (Owner.GetParent(Message.Target) == Buttons)
            {
                return Answer(Owner, Self, Message.Target);
            }
            return Owner.IsAlive(Default) && Answer(Owner, Self, Default);
        }

        if (Message.Kind == Signal::kKey && Message.Key == ZyInput::Key::Escape)
        {
            return Answer(Owner, Self, Cancel);
        }

        // A button's action ran already, as its click bubbled through it on its way here.
        if (Message.Kind == Signal::kClick && Owner.GetParent(Message.Target) == Buttons)
        {
            Close(Owner, Self);
            return true;
        }

        const Bool Clicked = Message.Kind == Signal::kClick;

        if (Clicked && Message.Target == Window::GetClose(Element(Owner, Self)).GetHandle())
        {
            return Answer(Owner, Self, Cancel);
        }
        return Window::OnEvent(Owner, Message, Self);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dialog::Answer(Ref<Interface> Owner, Handle Self, Handle Button)
    {
        // Clicked as the pointer would, so its action runs and the click closes the dialog on its way up.
        if (Owner.IsAlive(Button) && !Owner.HasState(Button, State::Disabled))
        {
            Owner.Dispatch(Event(Signal::kClick, Button));
        }
        else
        {
            Close(Owner, Self);
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dialog::Center(Ref<Interface> Owner, Handle Self)
    {
        const Ptr<Verdict> Own = Owner.Get<Verdict>(Self);

        if (!Own || Own->Placed || Owner.IsPending(Self) || Owner.HasState(Self, State::Hidden))
        {
            return false;
        }

        // Only a layout tells its size, so it waits for one.
        const Rect Frame = Owner.GetFrame(Self);

        if (Frame.IsEmpty())
        {
            return false;
        }

        const Vector2 Room  = Owner.GetSize();
        const Vector2 Spare = Vector2::Max(Room - Frame.GetSize(), Vector2::Zero());

        Own->Placed = true;
        Window::MoveTo(Element(Owner, Self), Vector2::Floor(Spare * 0.5f));
        return true;
    }
}