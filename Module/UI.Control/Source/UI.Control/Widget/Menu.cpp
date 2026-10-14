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

#include "Menu.hpp"
#include "Common.hpp"
#include "Label.hpp"
#include "Panel.hpp"
#include "Popup.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Input/Control.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Facet/Layout/Virtual.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Tree.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Binder.hpp"
#include "UI.Gesture/Facet/Context.hpp"
#include "UI.Gesture/Tooltip.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Menu::Create(Element Opener, Text Style)
    {
        const Element Result = Popup::Create(Opener, Style);

        Widget::Govern(Result, Handler::Create<&Menu::OnEvent>(), true);
        Result.Add<Roster>().Style = Style;
        Result.Add<Virtual>().Realize = Realizer::Create<&Menu::Track>();
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Menu::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Element Result = Create(Parent, Markup::GetText(Node, "Style", "Menu"));

        if (const ConstPtr<JsonValue> Items = Markup::GetField(Node, "Items"))
        {
            Fill(Reader, Result, * Items);
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menu::Read(Ref<Markup> Reader, Element Target, ConstRef<JsonValue> Node)
    {
        const ConstPtr<JsonValue> Field = Markup::GetField(Node, "ContextMenu");

        if (!Field)
        {
            return;
        }

        Element Made;

        if (Field->IsArray())
        {
            Made = Create(Target);
            Fill(Reader, Made, * Field);
        }
        else if (Field->IsString())
        {
            Made = Reader.Spawn(Target, Field->GetString());
        }
        else if (Field->IsObject())
        {
            Made = Reader.Build(Target, * Field);
        }

        // Whatever is not a menu would be left standing in the element, so it goes.
        if (!Made.IsAlive() || !Made.Has<Roster>())
        {
            LOG_W("A node's 'ContextMenu' must list items or make a menu, so it opens none");

            if (Made.IsAlive())
            {
                Made.Destruct();
            }
            return;
        }
        Attach(Target, Made);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menu::Fill(Ref<Markup> Reader, Element Target, ConstRef<JsonValue> Items)
    {
        if (!Items.IsArray())
        {
            LOG_W("A menu's 'Items' must be a list, so the menu holds none");
            return;
        }

        for (ConstRef<JsonValue> Entry : Items.GetArray())
        {
            Add(Reader, Target, Entry);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Menu::AddItem(Element Target, Text Content, Text Action, Text Shortcut)
    {
        const Str Style = GetStyle(Target);
        const Str Face  = Widget::Part(Style, "Item");

        const Element Item = Target.Create();

        Ref<Box> Shape = Item.EditBox();
        Shape.Flow  = Direction::Row;
        Shape.Items = Align::Center;

        Widget::Govern(Item, Handler(), false);
        Item.SetStyle(Face);

        if (!Action.IsEmpty())
        {
            Item.Get<Control>()->Action = Identify(Action);
        }

        Label::Create(Item, Content, Widget::Part(Face, "Label")).EditBox().Grow = 1.0f;

        if (!Shortcut.IsEmpty())
        {
            Label::Create(Item, Shortcut, Widget::Part(Style, "Shortcut"));
        }
        return Item;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Menu::AddCheck(Element Target, Text Content, Bool Checked, Text Action, Text Shortcut)
    {
        const Str     Style = GetStyle(Target);
        const Element Item  = AddItem(Target, Content, Action, Shortcut);

        // The mark fills its box at the far end of the item, and the theme shows it only while the item is checked.
        const Element Holder = Panel::Create(Item, Widget::Part(Style, "Check"));
        Panel::Create(Holder, Widget::Part(Style, "Check.Mark")).EditBox().Grow = 1.0f;

        Widget::Govern(Item, Handler::Create<&Menu::OnCheck>(), false);
        Item.SetState(Widget::Checked(), Checked);
        Item.Add<Toggle>();
        return Item;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Menu::AddHeading(Element Target, Text Content)
    {
        // A heading holds no control, so neither the pointer nor the keyboard can pick it.
        const Str     Face = Widget::Part(GetStyle(Target), "Heading");
        const Element Item = Panel::Create(Target, Face, Direction::Row);

        Item.EditBox().Items = Align::Center;
        Label::Create(Item, Content, Widget::Part(Face, "Label"));
        return Item;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Menu::AddSeparator(Element Target)
    {
        return Panel::Create(Target, Widget::Part(GetStyle(Target), "Separator"));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Menu::AddSubmenu(Element Target, Text Content)
    {
        const Str     Style = GetStyle(Target);
        const Element Item  = AddItem(Target, Content);

        Label::Create(Item, ">", Widget::Part(Style, "Arrow"));

        const Element Submenu = Create(Item, Style);
        Item.Add<Branch>().Submenu = Submenu.GetHandle();
        return Submenu;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menu::SetText(Element Item, Text Content)
    {
        // The text is the first thing on an item, before its shortcut or its arrow, unless an icon stands before it.
        const ConstPtr<Option> Parts = Item.Get<Option>();
        Label::SetText(Parts ? Element(Item.GetTree(), Parts->Label) : Item.GetFirst(), Content);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menu::SetIcon(Element Item, Text Content)
    {
        if (const ConstPtr<Option> Parts = Item.Get<Option>())
        {
            Label::SetText(Element(Item.GetTree(), Parts->Icon), Content);
            return;
        }

        // The icon goes before the text, which the item then finds through its facet rather than first.
        const Element Words = Item.GetFirst();
        const Element Icon  = Label::Create(Item, Content, Widget::Part(GetStyle(Item), "Icon"));
        Icon.Attach(Item, Words);

        Ref<Option> Made = Item.Add<Option>();
        Made.Label = Words.GetHandle();
        Made.Icon  = Icon.GetHandle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menu::SetChecked(Element Item, Bool Checked)
    {
        Item.SetState(Widget::Checked(), Checked);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Menu::IsChecked(Element Item)
    {
        return Item.HasState(Widget::Checked());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menu::Attach(Element Target, Element Menu)
    {
        Target.Obtain<Context>().Menu = Menu.GetHandle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menu::Open(Ref<Interface> Owner, Handle Target, Vector2 At, Handle Subject)
    {
        Owner.Summon(Target, At, Subject);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menu::Drop(Ref<Interface> Owner, Handle Target, Handle Anchor, Bool Wide, Bool Keyboard)
    {
        if (!Owner.Has<Roster>(Target))
        {
            return;
        }

        if (Wide)
        {
            Popup::Show(Owner, Target, Anchor);
        }
        else
        {
            Popup::Drop(Owner, Target, Anchor);
        }
        Owner.Confine(Target);

        // It learns what it was opened over and takes the keyboard, as a menu opened at the pointer does.
        Event Message(Signal::kOpen, Target);
        Message.Other = Anchor;
        Owner.Dispatch(Message);

        if (Keyboard)
        {
            Step(Owner, Target, 1);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menu::Close(Ref<Interface> Owner, Handle Target)
    {
        if (!Owner.Has<Roster>(Target))
        {
            return;
        }

        // Every submenu open from it closes first, each closing its own.
        Collapse(Owner, Target, Handle());

        Light(Owner, Target, Handle());
        Owner.Get<Roster>(Target)->Hovered = Handle();

        if (Popup::IsOpen(Owner, Target))
        {
            Popup::Hide(Owner, Target);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menu::Dismiss(Ref<Interface> Owner, Handle Target)
    {
        if (Owner.Has<Roster>(Target))
        {
            Retreat(Owner, GetTopMenu(Owner, Target));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Menu::GetSubject(Element Part)
    {
        if (!Part.IsAlive())
        {
            return Element();
        }

        // A submenu was opened over nothing; the menu it was reached from knows.
        Ref<Tree>              Owner = Part.GetTree();
        const ConstPtr<Roster> Own   = Owner.Get<Roster>(GetTopMenu(Owner, Owner.GetRoot(Part.GetHandle())));
        return Own && Owner.IsAlive(Own->Subject) ? Element(Owner, Own->Subject) : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Menu::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        // Opened at the pointer, it learns what it was opened over and takes the keyboard, minding who held it.
        if (Message.Kind == Signal::kOpen && Message.Target == Self)
        {
            const Ptr<Roster> Own     = Owner.Get<Roster>(Self);
            const Handle      Focused = Owner.GetFocused();

            Own->Subject = Message.Other;

            if (Owner.GetRoot(Focused) != Self)
            {
                Own->Return = Focused;
            }

            Light(Owner, Self, Handle());
            Owner.Focus(Self);
            return true;
        }

        // The item's action ran already, as the click bubbled through it on its way here.
        if (Message.Kind == Signal::kClick && Owner.GetParent(Message.Target) == Self)
        {
            Choose(Owner, Self, Message.Target);
            return true;
        }

        if (Message.Kind == Signal::kKey && Message.Target == Self)
        {
            return OnKey(Owner, Message.Key, Self);
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Menu::OnKey(Ref<Interface> Owner, ZyInput::Key Key, Handle Self)
    {
        const Handle Active = Owner.Get<Roster>(Self)->Active;

        switch (Key)
        {
        case ZyInput::Key::Up:
            Step(Owner, Self, -1);
            return true;
        case ZyInput::Key::Down:
            Step(Owner, Self, 1);
            return true;
        case ZyInput::Key::Enter:
        case ZyInput::Key::Space:
            // Picked as a click would, so the item's action runs and the menus close behind it.
            if (IsPickable(Owner, Active))
            {
                if (Owner.Has<Branch>(Active))
                {
                    Expand(Owner, Active, true);
                }
                else
                {
                    Owner.Dispatch(Event(Signal::kClick, Active));
                }
            }
            return true;
        case ZyInput::Key::Right:
            if (IsPickable(Owner, Active) && Owner.Has<Branch>(Active))
            {
                Expand(Owner, Active, true);
                return true;
            }

            // With no submenu to open, a menu bar's menu passes the key on, so the bar moves to the next menu.
            return !Owner.Get<Roster>(GetTopMenu(Owner, Self))->Passes;
        case ZyInput::Key::Left:
            if (!GetParentMenu(Owner, Self).IsValid())
            {
                return false;
            }
            Retreat(Owner, Self);
            return true;
        case ZyInput::Key::Escape:
            Retreat(Owner, Self);
            return true;
        default:
            return false;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Menu::OnCheck(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        // Turned first, so the model holds the new value by the time the item's action hears the click.
        if (Message.Kind == Signal::kClick && Message.Target == Self)
        {
            Owner.SetState(Self, Widget::Checked(), !Owner.HasState(Self, Widget::Checked()));
            Owner.Dispatch(Event(Signal::kChange, Self));
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Menu::Track(Ref<Interface> Owner, Handle Self)
    {
        if (!Popup::IsOpen(Owner, Self) || Owner.IsPending(Self))
        {
            return false;
        }

        Handle Hovered;

        for (Element Item = Element(Owner, Self).GetFirst(); Item.IsAlive(); Item = Item.GetNext())
        {
            if (Item.HasState(State::Hover) && IsPickable(Owner, Item.GetHandle()))
            {
                Hovered = Item.GetHandle();
            }
        }

        // Only the pointer coming to rest on another item counts, so the keyboard keeps what it lit meanwhile.
        const Ptr<Roster> Own = Owner.Get<Roster>(Self);

        if (Hovered == Own->Hovered)
        {
            return false;
        }

        Own->Hovered = Hovered;

        if (!Hovered.IsValid())
        {
            return false;
        }

        Light(Owner, Self, Hovered);

        // Resting on an item opens its submenu, and closes the one another item opened.
        Bool Changed = Collapse(Owner, Self, Hovered);

        if (const ConstPtr<Branch> Inner = Owner.Get<Branch>(Hovered); Inner && !Popup::IsOpen(Owner, Inner->Submenu))
        {
            Expand(Owner, Hovered, false);
            Changed = true;
        }
        return Changed;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menu::Choose(Ref<Interface> Owner, Handle Self, Handle Item)
    {
        if (Owner.Has<Branch>(Item))
        {
            Expand(Owner, Item, true);
            return;
        }

        // A check item turns where it stands, so several can be tried in a row; another item or escape closes the menu.
        if (Owner.Has<Toggle>(Item))
        {
            return;
        }

        // Every menu the item was reached through closes, and the keyboard goes back to whoever held it.
        Retreat(Owner, GetTopMenu(Owner, Self));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menu::Expand(Ref<Interface> Owner, Handle Item, Bool Keyboard)
    {
        const ConstPtr<Branch> Inner = Owner.Get<Branch>(Item);

        if (!Inner || !Owner.IsAlive(Inner->Submenu))
        {
            return;
        }

        const Handle Submenu = Inner->Submenu;
        const Handle Parent  = Owner.GetRoot(Item);

        // One submenu stands open at a time, beside the item that opened it.
        Collapse(Owner, Parent, Item);

        if (!Popup::IsOpen(Owner, Submenu))
        {
            Popup::Show(Owner, Submenu, Item, false);
            Owner.Confine(Submenu);
        }

        Light(Owner, Parent, Item);

        if (Keyboard)
        {
            Owner.Focus(Submenu);
            Light(Owner, Submenu, Handle());
            Step(Owner, Submenu, 1);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Menu::Collapse(Ref<Interface> Owner, Handle Self, Handle Kept)
    {
        Bool Closed = false;

        for (Element Item = Element(Owner, Self).GetFirst(); Item.IsAlive(); Item = Item.GetNext())
        {
            const ConstPtr<Branch> Inner = Item.Get<Branch>();

            if (Inner && Item.GetHandle() != Kept && Popup::IsOpen(Owner, Inner->Submenu))
            {
                Close(Owner, Inner->Submenu);
                Closed = true;
            }
        }
        return Closed;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menu::Light(Ref<Interface> Owner, Handle Self, Handle Item)
    {
        const Ptr<Roster> Own = Owner.Get<Roster>(Self);

        if (!Own || Own->Active == Item)
        {
            return;
        }

        if (Owner.IsAlive(Own->Active))
        {
            Owner.SetState(Own->Active, Widget::Selected(), false);
        }

        Own->Active = Item;

        if (Owner.IsAlive(Item))
        {
            Owner.SetState(Item, Widget::Selected(), true);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menu::Step(Ref<Interface> Owner, Handle Self, SInt32 Direction)
    {
        Sequence<Handle> Choices;

        for (Element Item = Element(Owner, Self).GetFirst(); Item.IsAlive(); Item = Item.GetNext())
        {
            if (IsPickable(Owner, Item.GetHandle()))
            {
                Choices.Append(Item.GetHandle());
            }
        }

        if (Choices.IsEmpty())
        {
            return;
        }

        const Handle Active = Owner.Get<Roster>(Self)->Active;
        const SInt32 Count  = static_cast<SInt32>(Choices.GetSize());
        const SInt32 Index  = static_cast<SInt32>(Choices.Find(Active));

        // From nothing lit, down starts at the top and up at the bottom; past either end it wraps around.
        const SInt32 Next = Index < 0 ? (Direction > 0 ? 0 : Count - 1) : Wrap(Index + Direction, Count);
        Light(Owner, Self, Choices[Next]);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menu::Retreat(Ref<Interface> Owner, Handle Self)
    {
        const Handle Parent = GetParentMenu(Owner, Self);
        const Handle Return = Owner.Get<Roster>(Self)->Return;

        Close(Owner, Self);

        if (Parent.IsValid())
        {
            Owner.Focus(Parent);
        }
        else if (Owner.IsAlive(Return) && !Owner.IsPending(Return) && !Owner.HasState(Return, State::Hidden))
        {
            Owner.Focus(Return);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Menu::GetParentMenu(ConstRef<Tree> Owner, Handle Self)
    {
        const ConstPtr<Control> Found = Owner.Get<Control>(Self);

        if (!Found || !Owner.IsAlive(Found->Owner))
        {
            return Handle();
        }

        // A submenu is opened by an item, which stands in the menu it was reached from.
        const Handle Root = Owner.GetRoot(Found->Owner);
        return Owner.Has<Roster>(Root) ? Root : Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Menu::GetTopMenu(ConstRef<Tree> Owner, Handle Self)
    {
        Handle Top = Self;

        for (Handle Parent = GetParentMenu(Owner, Top); Parent.IsValid(); Parent = GetParentMenu(Owner, Top))
        {
            Top = Parent;
        }
        return Top;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Menu::IsPickable(ConstRef<Tree> Owner, Handle Item)
    {
        return Owner.IsAlive(Item) && Owner.Has<Control>(Item)
            && !Owner.HasState(Item, State::Hidden) && !Owner.HasState(Item, State::Disabled);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menu::Add(Ref<Markup> Reader, Element Target, ConstRef<JsonValue> Entry)
    {
        if (Entry.IsString())
        {
            if (Entry.GetString() == "-")
            {
                AddSeparator(Target);
            }
            else if (const Element Item = AddItem(Target, Entry.GetString()); Binder::IsLocalized(Entry.GetString()))
            {
                Binder::Attach(Item, Entry.GetString(), Binder::PutText<&Menu::SetText>());
            }
            return;
        }

        if (!Entry.IsObject())
        {
            LOG_W("An item of a menu must be text or an object, so it is left out");
            return;
        }

        if (Markup::GetBool(Entry, "Separator"))
        {
            AddSeparator(Target);
            return;
        }

        Element Item;

        if (Markup::GetField(Entry, "Heading"))
        {
            Item = AddHeading(Target, Markup::GetText(Entry, "Heading"));
            Reader.Bind(Item, Entry, "Heading", Binder::PutText<&Menu::SetText>());
        }
        else if (const ConstPtr<JsonValue> Inner = Markup::GetField(Entry, "Items"))
        {
            // An item holding items of its own opens them as a submenu beside it.
            const Element Submenu = AddSubmenu(Target, Markup::GetText(Entry, "Text"));
            Fill(Reader, Submenu, * Inner);

            Item = Element(Target.GetTree(), Submenu.Get<Control>()->Owner);
        }
        else if (Markup::GetField(Entry, "Checked"))
        {
            Item = AddCheck(Target, Markup::GetText(Entry, "Text"), Markup::GetBool(Entry, "Checked"),
                Markup::GetText(Entry, "Action"), Markup::GetText(Entry, "Shortcut"));
            Reader.Bind(Item, Entry, "Checked", Binder::PutState<&Widget::Checked>(),
                        Binder::TakeState<&Widget::Checked>());
        }
        else
        {
            Item = AddItem(Target, Markup::GetText(Entry, "Text"), Markup::GetText(Entry, "Action"),
                Markup::GetText(Entry, "Shortcut"));
        }

        if (const Text Name = Markup::GetText(Entry, "Name"); !Name.IsEmpty())
        {
            Item.SetName(Name);
        }

        // An icon that binds is shown its value at once, rather than the braces first.
        if (const Text Icon = Markup::GetText(Entry, "Icon"); !Icon.IsEmpty())
        {
            SetIcon(Item, Binder::IsBound(Icon) || Binder::IsLocalized(Icon) ? Text() : Icon);
            Reader.Bind(Item, Entry, "Icon", Binder::PutText<&Menu::SetIcon>());
        }

        Item.SetState(State::Hidden, Markup::GetBool(Entry, "Hidden"));
        Item.SetState(State::Disabled, Markup::GetBool(Entry, "Disabled"));

        Reader.Bind(Item, Entry, "Text", Binder::PutText<&Menu::SetText>());
        Reader.Bind(Item, Entry, "Hidden", Binder::PutState<State::Hidden>());
        Reader.Bind(Item, Entry, "Disabled", Binder::PutState<State::Disabled>());

        // An item explains itself as any node does, its text read in the record the menu reads.
        Tooltip::Read(Reader, Item, Entry);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str Menu::GetStyle(Element Part)
    {
        const ConstPtr<Roster> Own = Part.GetRoot().Get<Roster>();
        return Own ? Str(Text(Own->Style)) : Str("Menu");
    }
}