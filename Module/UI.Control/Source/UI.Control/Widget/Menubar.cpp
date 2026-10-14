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

#include "Menubar.hpp"
#include "Common.hpp"
#include "Label.hpp"
#include "Menu.hpp"
#include "Panel.hpp"
#include "Popup.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Input/Control.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Facet/Layout/Virtual.hpp"
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

    Element Menubar::Create(Element Parent, Text Style)
    {
        const Element Result = Panel::Create(Parent, Style, Direction::Row);
        Result.EditBox().Items = Align::Center;
        Widget::Govern(Result, Handler::Create<&Menubar::OnEvent>(), false);

        // What stands past the menus is pushed to the far end, the room left over lying between them.
        const Element Trail = Panel::Create(Result, Widget::Part(Style, "Trail"), Direction::Row);

        Ref<Box> Shape = Trail.EditBox();
        Shape.Grow    = 1.0f;
        Shape.Items   = Align::Center;
        Shape.Content = Justify::End;

        Ref<Strip> Own = Result.Add<Strip>();
        Own.Style = Style;
        Own.Trail = Trail.GetHandle();

        Result.Add<Virtual>().Realize = Realizer::Create<&Menubar::Track>();
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Menubar::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Element Result = Create(Parent, Markup::GetText(Node, "Style", "Menubar"));

        if (const ConstPtr<JsonValue> Menus = Markup::GetField(Node, "Menus"); Menus && Menus->IsArray())
        {
            for (ConstRef<JsonValue> Entry : Menus->GetArray())
            {
                Add(Reader, Result, Entry, Markup::GetText(Node, "MenuStyle", "Menu"));
            }
        }
        else if (Menus)
        {
            LOG_W("A menu bar's 'Menus' must be a list, so the bar holds none");
        }

        // What the layout gives the bar besides its menus stands at its far end, as a readout of the frame rate.
        Markup::SetHost(Result, GetTrail(Result));
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Menubar::AddMenu(Element Bar, Text Content, Text Style)
    {
        // Copied, since making the title may move the store the bar's own facet stands in.
        const ConstPtr<Strip> Own = Bar.Get<Strip>();

        if (!Own)
        {
            return Element();
        }

        const Str     Face  = Widget::Part(Text(Own->Style), "Title");
        const Element Trail(Bar.GetTree(), Own->Trail);

        // Titles stand in the order they were added, before the part at the far end.
        const Element Title = Bar.Create(Text(), Trail);
        Title.SetStyle(Face);

        Ref<Box> Shape = Title.EditBox();
        Shape.Flow  = Direction::Row;
        Shape.Items = Align::Center;

        Widget::Govern(Title, Handler(), false);
        Label::Create(Title, Content, Widget::Part(Face, "Label"));

        // Opened from its title, the menu leaves a press there to the bar, and its events bubble through the title.
        const Element Result = Menu::Create(Title, Style);
        Result.Get<Roster>()->Passes = true;

        Title.Add<Pulldown>().Menu = Result.GetHandle();
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Menubar::GetMenu(Element Title)
    {
        const ConstPtr<Pulldown> Found = Title.IsAlive() ? Title.Get<Pulldown>() : nullptr;
        return Found ? Element(Title.GetTree(), Found->Menu) : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Menubar::GetTitle(Element Menu)
    {
        const ConstPtr<Control> Found = Menu.IsAlive() ? Menu.Get<Control>() : nullptr;

        if (!Found)
        {
            return Element();
        }

        // A menu of a bar is opened from a title standing in the bar.
        const Element Title(Menu.GetTree(), Found->Owner);
        return Title.IsAlive() && Title.Has<Pulldown>() && Title.GetParent().Has<Strip>() ? Title : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menubar::SetTitle(Element Title, Text Content)
    {
        Label::SetText(Title.GetFirst(), Content);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Menubar::GetTrail(Element Bar)
    {
        const ConstPtr<Strip> Own = Bar.IsAlive() ? Bar.Get<Strip>() : nullptr;
        return Own ? Element(Bar.GetTree(), Own->Trail) : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menubar::Open(Ref<Interface> Owner, Handle Title, Bool Keyboard)
    {
        const Handle             Bar   = Owner.GetParent(Title);
        const ConstPtr<Pulldown> Found = Owner.Get<Pulldown>(Title);

        if (!Found || !Owner.Has<Strip>(Bar))
        {
            return;
        }

        const Handle Drop   = Found->Menu;
        const Handle Before = Owner.Get<Strip>(Bar)->Open;
        Handle       Return;

        // The menu open before gives way, handing on what held the keyboard before the bar's menus took it.
        if (Before.IsValid() && Before != Title)
        {
            if (const ConstPtr<Pulldown> Old = Owner.Get<Pulldown>(Before))
            {
                Return = Owner.Get<Roster>(Old->Menu)->Return;
                Menu::Close(Owner, Old->Menu);
            }

            if (Owner.IsAlive(Before))
            {
                Owner.SetState(Before, Widget::Selected(), false);
            }
        }

        Owner.Get<Strip>(Bar)->Open = Title;
        Owner.SetState(Title, Widget::Selected(), true);
        Menu::Drop(Owner, Drop, Title, false, Keyboard);

        if (Owner.IsAlive(Return))
        {
            Owner.Get<Roster>(Drop)->Return = Return;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menubar::Close(Ref<Interface> Owner, Handle Bar)
    {
        const Ptr<Strip> Own = Owner.Get<Strip>(Bar);

        if (!Own || !Own->Open.IsValid())
        {
            return;
        }

        const Handle Title = Own->Open;
        Own->Open = Handle();

        if (!Owner.IsAlive(Title))
        {
            return;
        }

        Owner.SetState(Title, Widget::Selected(), false);

        const ConstPtr<Pulldown> Found = Owner.Get<Pulldown>(Title);

        if (Found && Popup::IsOpen(Owner, Found->Menu))
        {
            Menu::Dismiss(Owner, Found->Menu);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Menubar::GetOpen(Element Bar)
    {
        const ConstPtr<Strip> Own = Bar.IsAlive() ? Bar.Get<Strip>() : nullptr;

        if (!Own)
        {
            return Element();
        }

        // The title is open only while its menu still shows, though the bar lets it go only at the next update.
        const Element Title(Bar.GetTree(), Own->Open);
        const Element Drop = GetMenu(Title);
        return Drop.IsAlive() && !Drop.HasState(State::Hidden) ? Title : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Menubar::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        // A click on a title opens its menu, or closes it when it stands open already.
        const Bool Inside = Message.Kind == Signal::kClick && Owner.GetParent(Message.Target) == Self;

        if (Inside && Owner.Has<Pulldown>(Message.Target))
        {
            const Handle Title = Message.Target;

            if (Owner.Get<Strip>(Self)->Open == Title && Popup::IsOpen(Owner, Owner.Get<Pulldown>(Title)->Menu))
            {
                Close(Owner, Self);
            }
            else
            {
                Open(Owner, Title, false);
            }
            return true;
        }

        // Left and right reach the bar from one of its menus only once the menu had no use for them.
        const Bool Leftward  = Message.Key == ZyInput::Key::Left;
        const Bool Rightward = Message.Key == ZyInput::Key::Right;

        const Bool Arrowed = Message.Kind == Signal::kKey && (Leftward || Rightward);

        if (Arrowed && Owner.Has<Roster>(Owner.GetRoot(Message.Target)))
        {
            return Shift(Owner, Self, Leftward ? -1 : 1);
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Menubar::Track(Ref<Interface> Owner, Handle Self)
    {
        const ConstPtr<Strip> Own = Owner.Get<Strip>(Self);

        // Nothing is followed while every menu is closed, which is all a bar costs an update then.
        if (!Own || !Own->Open.IsValid() || Owner.IsPending(Self))
        {
            return false;
        }

        const Handle             Title = Own->Open;
        const ConstPtr<Pulldown> Found = Owner.Get<Pulldown>(Title);

        // Its menu was closed by a pick, by escape or by a press elsewhere, so the title lets go.
        if (!Found || !Popup::IsOpen(Owner, Found->Menu))
        {
            if (Owner.IsAlive(Title))
            {
                Owner.SetState(Title, Widget::Selected(), false);
            }

            const Ptr<Strip> Closed = Owner.Get<Strip>(Self);
            Closed->Open    = Handle();
            Closed->Hovered = Handle();
            return false;
        }

        Handle Hovered;

        for (Element Each = Element(Owner, Self).GetFirst(); Each.IsAlive(); Each = Each.GetNext())
        {
            if (Each.HasState(State::Hover) && IsUsable(Owner, Each.GetHandle()))
            {
                Hovered = Each.GetHandle();
            }
        }

        // Only the pointer coming to rest on another title counts, so the keyboard keeps the menu it moved to.
        const Ptr<Strip> Kept = Owner.Get<Strip>(Self);

        if (Hovered == Kept->Hovered)
        {
            return false;
        }

        Kept->Hovered = Hovered;

        if (!Hovered.IsValid() || Hovered == Title)
        {
            return false;
        }

        Open(Owner, Hovered, false);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Menubar::Shift(Ref<Interface> Owner, Handle Self, SInt32 Direction)
    {
        const Handle Current = Owner.Get<Strip>(Self)->Open;

        if (!Current.IsValid())
        {
            return false;
        }

        Sequence<Handle> Titles;

        for (Element Each = Element(Owner, Self).GetFirst(); Each.IsAlive(); Each = Each.GetNext())
        {
            if (Each.GetHandle() == Current || IsUsable(Owner, Each.GetHandle()))
            {
                Titles.Append(Each.GetHandle());
            }
        }

        const SInt32 Count = static_cast<SInt32>(Titles.GetSize());
        const SInt32 Index = static_cast<SInt32>(Titles.Find(Current));

        if (Index < 0)
        {
            return false;
        }

        Open(Owner, Titles[Wrap(Index + Direction, Count)], true);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Menubar::IsUsable(ConstRef<Interface> Owner, Handle Title)
    {
        return Owner.Has<Pulldown>(Title) && !Owner.HasState(Title, State::Hidden)
            && !Owner.HasState(Title, State::Disabled);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Menubar::Add(Ref<Markup> Reader, Element Bar, ConstRef<JsonValue> Entry, Text Style)
    {
        if (!Entry.IsObject())
        {
            LOG_W("A menu of a menu bar must be an object, so it is left out");
            return;
        }

        const Element Drop  = AddMenu(Bar, Markup::GetText(Entry, "Text"), Style);
        const Element Title = GetTitle(Drop);

        if (const Text Name = Markup::GetText(Entry, "Name"); !Name.IsEmpty())
        {
            Title.SetName(Name);
        }

        Title.SetState(State::Hidden, Markup::GetBool(Entry, "Hidden"));
        Title.SetState(State::Disabled, Markup::GetBool(Entry, "Disabled"));

        Reader.Bind(Title, Entry, "Text", Binder::PutText<&Menubar::SetTitle>());
        Reader.Bind(Title, Entry, "Hidden", Binder::PutState<State::Hidden>());
        Reader.Bind(Title, Entry, "Disabled", Binder::PutState<State::Disabled>());

        if (const ConstPtr<JsonValue> Items = Markup::GetField(Entry, "Items"))
        {
            Menu::Fill(Reader, Drop, * Items);
        }
    }
}