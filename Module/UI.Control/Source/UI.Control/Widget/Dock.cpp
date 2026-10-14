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

#include "Dock.hpp"
#include "Button.hpp"
#include "Common.hpp"
#include "Panel.hpp"
#include "Splitter.hpp"
#include "Tabs.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Facet/Layout/Virtual.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Tree.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Binder.hpp"
#include "UI.Gesture/Drag.hpp"
#include "UI.Markup/Markup.hpp"
#include "UI.Markup/Parser.hpp"
#include "Window.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Dock::Create(Element Parent, Text Style)
    {
        const Element Result = Panel::Create(Parent, Style);
        Widget::Govern(Result, Handler::Create<&Dock::OnEvent>(), false);

        Ref<Box> Shape = Result.EditBox();
        Shape.Grow  = 1.0f;
        Shape.Basis = Length::Pixels(0.0f);

        // The top, the middle and the bottom stand one over another, the left, the centre and the right side by side.
        const Str     Split  = Widget::Part(Style, "Split");
        const Element Outer  = Splitter::Create(Result, Direction::Column, Split);
        const Element Top    = Splitter::AddPane(Outer, 150.0f, 60.0f, Split);
        const Element Center = Splitter::AddPane(Outer, 0.0f, 80.0f, Split);
        const Element Bottom = Splitter::AddPane(Outer, 150.0f, 60.0f, Split);
        const Element Middle = Splitter::Create(Center, Direction::Row, Split);
        const Element Left   = Splitter::AddPane(Middle, 220.0f, 80.0f, Split);
        const Element Main   = Splitter::AddPane(Middle, 0.0f, 80.0f, Split);
        const Element Right  = Splitter::AddPane(Middle, 240.0f, 80.0f, Split);

        Ref<Harbor> Own = Result.Add<Harbor>();
        Own.Outer  = Outer.GetHandle();
        Own.Middle = Middle.GetHandle();
        Own.Style  = Style;

        // Made in the order the sides are counted, each region a tab control filling its pane.
        const Array<Element, 5> Panes(Left, Right, Top, Bottom, Main);
        const Str               Region = Widget::Part(Style, "Region");

        for (const Element Pane : Panes)
        {
            const Element Book = Tabs::Create(Pane, Region);
            Book.EditBox().Grow = 1.0f;

            // Found again, as making the region may move this facet; counted unseen, so the first update shows each.
            const Ptr<Harbor> Kept = Result.Get<Harbor>();
            Kept->Regions.Append(Book.GetHandle());
            Kept->Counts.Append(kMaximum<UInt32>);
        }

        Result.Add<Virtual>().Realize = Realizer::Create<&Dock::OnRealize>();
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Dock::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        constexpr Side kNone = static_cast<Side>(0xFF);

        const Element Result = Create(Parent, Markup::GetText(Node, "Style", "Dock"));

        if (const ConstPtr<JsonValue> Panels = Markup::GetField(Node, "Panels"); Panels && Panels->IsArray())
        {
            ConstRef<JsonValue::Array> Entries = Panels->GetArray();

            for (UInt Index = 0; Index < Entries.GetSize(); ++Index)
            {
                ConstRef<JsonValue> Entry = Entries[Index];
                const Text          Name  = Markup::GetText(Entry, "Name");
                const Text          Given = Markup::GetText(Entry, "Side", "Center");
                Side                Where = ZyEnum::Cast(Given, kNone);

                if (Name.IsEmpty())
                {
                    LOG_W("A dock's panel must have a 'Name', so it is left out");
                    continue;
                }

                if (Where == kNone)
                {
                    LOG_W("A dock's panel '{0}' names no side as '{1}', so it docks in the centre", Name, Given);
                    Where = Side::Center;
                }

                const Element Content = AddPanel(Result, Name, Markup::GetText(Entry, "Title", Name), Where);

                const ConstPtr<JsonValue> Children = Markup::GetField(Entry, "Children");

                if (Children && Children->IsArray())
                {
                    ConstRef<JsonValue::Array> List = Children->GetArray();

                    for (UInt Child = 0; Child < List.GetSize(); ++Child)
                    {
                        Reader.Build(Content, List[Child]);
                    }
                }
            }
        }
        else if (Panels)
        {
            LOG_W("A dock's 'Panels' must be a list of panels, so it holds none");
        }

        if (const ConstPtr<JsonValue> Sizes = Markup::GetField(Node, "Sizes"))
        {
            Resize(Result, * Sizes);
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Dock::AddPanel(Element Owner, Text Name, Text Title, Side Where)
    {
        const Ptr<Harbor> Own = Owner.Get<Harbor>();

        if (!Own || Name.IsEmpty())
        {
            return Element();
        }

        if (const SInt32 Found = Find(* Own, Name); Found >= 0)
        {
            return Element(Owner.GetTree(), Own->Berths[Found].Content);
        }

        Berth Entry;
        Entry.Name  = Name;
        Entry.Title = Title;
        Entry.Where = Where;
        Entry.Home  = Where == Side::Float ? Side::Center : Where;
        Entry.Goal  = Where;

        const Str Style = Own->Style;
        Element   Holder;

        // A floating panel opens a step down and right of the last, so none hides another.
        if (Where == Side::Float)
        {
            const Real32 Step = 80.0f + 24.0f * static_cast<Real32>(Own->Berths.GetSize());
            Entry.Spot = Vector2(Step, Step);

            const Element Opened = Launch(Owner, Entry);
            Entry.Window = Opened.GetHandle();
            Holder       = Window::GetContent(Opened);
        }
        else
        {
            Holder = Moor(GetRegion(Owner, Where), Title, Style);
        }

        const Element Content = Panel::Create(Holder, Widget::Part(Style, "Panel"));
        Content.EditBox().Grow = 1.0f;
        Content.SetName(Name);

        Entry.Content = Content.GetHandle();
        Owner.Get<Harbor>()->Berths.Append(::Move(Entry));
        return Content;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dock::Move(Element Owner, Text Name, Side Where, Vector2 Position)
    {
        const Ptr<Harbor> Own   = Owner.Get<Harbor>();
        const SInt32      Found = Own ? Find(* Own, Name) : -1;

        if (Found < 0)
        {
            return;
        }

        Ref<Berth> Entry = Own->Berths[Found];
        Entry.Goal  = Where;
        Entry.Spot  = Position;
        Entry.Due   = true;
        Entry.Front = true;

        if (!Owner.GetTree().IsWalking())
        {
            Settle(Owner);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Side Dock::GetSide(Element Owner, Text Name)
    {
        const ConstPtr<Harbor> Own   = Owner.Get<Harbor>();
        const SInt32           Found = Own ? Find(* Own, Name) : -1;
        return Found >= 0 ? Own->Berths[Found].Where : Side::Center;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Dock::GetPanel(Element Owner, Text Name)
    {
        const ConstPtr<Harbor> Own   = Owner.Get<Harbor>();
        const SInt32           Found = Own ? Find(* Own, Name) : -1;
        return Found >= 0 ? Element(Owner.GetTree(), Own->Berths[Found].Content) : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Dock::GetWindow(Element Owner, Text Name)
    {
        const ConstPtr<Harbor> Own   = Owner.Get<Harbor>();
        const SInt32           Found = Own ? Find(* Own, Name) : -1;
        const Element          Result(Owner.GetTree(), Found >= 0 ? Own->Berths[Found].Window : Handle());
        return Result.IsAlive() ? Result : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Dock::GetRegion(Element Owner, Side Where)
    {
        const ConstPtr<Harbor> Own = Owner.Get<Harbor>();
        const UInt32           At  = static_cast<UInt32>(Where);
        return Own && At < Own->Regions.GetSize() ? Element(Owner.GetTree(), Own->Regions[At]) : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Dock::GetCount(Element Owner)
    {
        const ConstPtr<Harbor> Own = Owner.Get<Harbor>();
        return Own ? static_cast<UInt32>(Own->Berths.GetSize()) : 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Rect Dock::GetTarget(Element Owner, Side Where)
    {
        return Mark(Owner.GetFrame(), Where);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Dock::GetOverlay(Element Owner)
    {
        const ConstPtr<Harbor> Own = Owner.Get<Harbor>();

        const Element Result(Owner.GetTree(), Own ? Own->Overlay : Handle());
        return Result.IsAlive() ? Result : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str Dock::Save(Element Owner)
    {
        const ConstPtr<Harbor> Own = Owner.Get<Harbor>();

        if (!Own)
        {
            return Str();
        }

        Ref<Tree> Owned = Owner.GetTree();
        JsonValue Root;
        Root.SetObject();

        JsonObject Fields(Root);
        JsonArray  Sizes = Fields.SetArray("Sizes");

        // Each edge keeps the size its pane, the parent of its region, was given, laid out or not.
        for (UInt32 Region = 0; Region < static_cast<UInt32>(Side::Center); ++Region)
        {
            const Element Pane = Element(Owned, Own->Regions[Region]).GetParent();
            Sizes.AddNumber(Pane.IsAlive() ? Owned.GetBox(Pane.GetHandle()).Basis.Value : 0.0f);
        }

        // Added once the sizes are in, since adding a field may move the one added before it.
        JsonArray Listed = Fields.SetArray("Panels");

        // Docked panels in the order of their region's tabs, then the floating ones.
        for (UInt32 Region = 0; Region < Own->Regions.GetSize(); ++Region)
        {
            const Element Book(Owned, Own->Regions[Region]);
            const UInt32  Count = Tabs::GetCount(Book);

            for (UInt32 Index = 0; Index < Count; ++Index)
            {
                const SInt32 Found = Find(Owner, Tabs::GetTab(Book, Index));

                if (Found < 0)
                {
                    continue;
                }

                JsonObject Item = Listed.AddObject();
                Item.SetString("Name", Own->Berths[Found].Name);
                Item.SetEnum("Side", static_cast<Side>(Region));
                Item.SetBool("Shown", Tabs::GetSelected(Book) == Index);
            }
        }

        for (ConstRef<Berth> Floating : Own->Berths)
        {
            const Element Opened(Owned, Floating.Window);

            if (Floating.Where != Side::Float || !Opened.IsAlive())
            {
                continue;
            }

            const Rect Frame = Opened.GetFrame();

            JsonObject Item = Listed.AddObject();
            Item.SetString("Name", Floating.Name);
            Item.SetEnum("Side", Side::Float);

            // Added last, since a field added after it may move it.
            JsonArray Corners = Item.SetArray("Frame");
            Corners.AddNumber(Frame.GetMinimumX());
            Corners.AddNumber(Frame.GetMinimumY());
            Corners.AddNumber(Frame.GetWidth());
            Corners.AddNumber(Frame.GetHeight());
        }
        return JsonDocument::Dump(Root);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dock::Load(Element Owner, Text Source, Ref<Str> Error)
    {
        const Ptr<Harbor> Own = Owner.Get<Harbor>();
        JsonValue         Root;

        if (!Own || !Parser::Parse(Source, Root, Error))
        {
            return false;
        }

        // The parser takes no document but an object, so the root is one.
        const JsonObject Fields(Root);

        if (const ConstPtr<JsonValue> Sizes = Fields.GetValue("Sizes"))
        {
            Resize(Owner, * Sizes);
        }

        const JsonArray Entries = Fields.GetArray("Panels");

        if (Entries.IsNull())
        {
            return true;
        }

        // The panels named come first, in the order named, so docking each again in turn rebuilds every region's tabs.
        Sequence<Berth> Ordered;

        for (UInt Index = 0; Index < Entries.GetSize(); ++Index)
        {
            // An entry that is no object gives no name, which no panel goes by.
            const JsonObject Entry = Entries.GetObject(Index);
            const Text       Named = Entry.IsNull() ? Text() : Entry.GetString("Name");
            const SInt32     Found = Find(* Own, Named);

            if (Found < 0)
            {
                LOG_W("A dock layout names a panel '{0}' the dock does not hold, so it is skipped", Named);
                continue;
            }

            Berth Moved = ::Move(Own->Berths[Found]);
            Own->Berths.Remove(static_cast<UInt>(Found));

            Moved.Goal = ZyEnum::Cast(Entry.GetString("Side", "Center"), Moved.Goal);

            if (const JsonArray Frame = Entry.GetArray("Frame"); !Frame.IsNull() && Frame.GetSize() == 4)
            {
                Moved.Spot = Vector2(Frame.GetNumber<Real32>(0), Frame.GetNumber<Real32>(1));
                Moved.Size = Vector2(Frame.GetNumber<Real32>(2), Frame.GetNumber<Real32>(3));
            }

            Moved.Due   = true;
            Moved.Again = true;
            Moved.Front = Entry.GetBool("Shown");
            Ordered.Append(::Move(Moved));
        }

        for (Ref<Berth> Rest : Own->Berths)
        {
            Ordered.Append(::Move(Rest));
        }
        Own->Berths = ::Move(Ordered);

        if (!Owner.GetTree().IsWalking())
        {
            Settle(Owner);
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dock::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        if (Message.Kind != Signal::kDrag && Message.Kind != Signal::kRelease)
        {
            return false;
        }

        const Element     Root(Owner, Self);
        const Element     Tab(Owner, Message.Target);
        const Ptr<Harbor> Own = Owner.Get<Harbor>(Self);

        // Only a tab of one of its regions is dragged out; the tab is a child of its region's header.
        const Element Header = Tab.GetParent();
        const Element Region = Header.GetParent();

        const Bool Known = Region.IsAlive() && Own->Regions.Contains(Region.GetHandle());

        if (!Known || Owner.Get<Deck>(Region.GetHandle())->Header != Header.GetHandle())
        {
            return false;
        }

        if (Message.Kind == Signal::kRelease)
        {
            if (Own->Lifted != Message.Target)
            {
                return false;
            }

            // The tab dragged out goes at the next update, its panel docked where it was let go or left floating.
            const SInt32 Found = Find(Root, Tab);
            Own->Loose   = true;
            Own->Carried = Handle();

            if (Found >= 0)
            {
                Land(Owner, Root, static_cast<UInt>(Found), Message.Position);
            }
            return true;
        }

        // The window follows the pointer, held by its title bar.
        const Vector2 Spot = Message.Position - Vector2(40.0f, 14.0f);

        if (Own->Lifted == Message.Target)
        {
            const SInt Found = Own->Berths.Find([Own](ConstRef<Berth> Entry)
            {
                return Entry.Content == Own->Carried;
            });

            if (Found >= 0)
            {
                Ref<Berth> Entry = Own->Berths[Found];
                Entry.Spot = Spot;

                if (const Element Opened(Owner, Entry.Window); Opened.IsAlive())
                {
                    Window::MoveTo(Opened, Spot);
                }
            }
            Hover(Root, Message.Position);
            return true;
        }

        // A tab floats its panel once dragged past its region's tabs.
        const Rect   Reach = Header.GetFrame().Expand(kTear);
        const SInt32 Found = Find(Root, Tab);

        if (Reach.Contains(Message.Position) || Found < 0 || Own->Lifted.IsValid())
        {
            return false;
        }

        Ref<Berth> Entry = Own->Berths[Found];
        const Rect Shown = Element(Owner, Entry.Content).GetFrame();
        Entry.Goal  = Side::Float;
        Entry.Spot  = Spot;
        Entry.Size  = Vector2::Max(Shown.GetSize() + Vector2(16.0f, 56.0f), Vector2(240.0f, 180.0f));
        Entry.Due   = true;

        Own->Lifted  = Message.Target;
        Own->Carried = Entry.Content;
        Hover(Root, Message.Position);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dock::OnFloat(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        const Element Opened(Owner, Self);
        UInt          Index = 0;
        const Element Root  = Seek(Owner, Self, Index);

        // Closing the window docks its panel back where it last stood.
        const Bool Clicked = Message.Kind == Signal::kClick;

        if (Root.IsAlive() && Clicked && Message.Target == Window::GetClose(Opened).GetHandle())
        {
            Steer(Owner, Root, Index, Root.Get<Harbor>()->Berths[Index].Home);
            return true;
        }

        const ConstPtr<Grip> Hold = Owner.Get<Grip>(Self);
        const Bool           Held = Hold && Hold->Held;
        const Bool           Kept = Window::OnEvent(Owner, Message, Self);

        if (!Root.IsAlive() || !Held || Message.Target != Self)
        {
            return Kept;
        }

        if (Message.Kind == Signal::kDrag)
        {
            Hover(Root, Message.Position);
        }
        else if (Message.Kind == Signal::kRelease)
        {
            Land(Owner, Root, Index, Message.Position);
        }
        return Kept;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dock::OnRealize(Ref<Interface> Owner, Handle Self)
    {
        return Settle(Element(Owner, Self));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dock::Settle(Element Root)
    {
        Ref<Tree> Owned   = Root.GetTree();
        Bool      Changed = false;

        for (UInt Index = 0; Index < Root.Get<Harbor>()->Berths.GetSize(); ++Index)
        {
            if (Root.Get<Harbor>()->Berths[Index].Due)
            {
                Place(Root, Index);
                Changed = true;
            }
        }

        // The tab dragged out goes once the pointer let it go, after its panel moved out of its page.
        if (const Ptr<Harbor> Own = Root.Get<Harbor>(); Own->Loose)
        {
            const Element Tab(Owned, Own->Lifted);
            Own->Loose  = false;
            Own->Lifted = Handle();

            if (Tab.IsAlive())
            {
                Tabs::RemoveTab(Tab.GetParent().GetParent(), Tab.GetIndex());
                Changed = true;
            }
        }

        // A region shows only while it holds a panel, the middle always; its pane hides with it, its bar with that.
        const Ptr<Harbor> Own = Root.Get<Harbor>();

        for (UInt32 Region = 0; Region < Own->Regions.GetSize(); ++Region)
        {
            const Element Book(Owned, Own->Regions[Region]);
            const UInt32  Count = Tabs::GetCount(Book);

            if (Count == Own->Counts[Region])
            {
                continue;
            }

            Own->Counts[Region] = Count;
            Tabs::Select(Book, Min(Tabs::GetSelected(Book), Max(Count, 1u) - 1u));

            if (static_cast<Side>(Region) != Side::Center)
            {
                Book.GetParent().SetState(State::Hidden, Count == 0);
            }
            Changed = true;
        }

        // A panel just docked shows its tab over the others.
        for (Ref<Berth> Entry : Own->Berths)
        {
            const Element Content(Owned, Entry.Content);

            // A floating panel has no tab to show, and one still due to move shows it once it moved.
            if (Entry.Where == Side::Float)
            {
                Entry.Front = false;
            }

            if (!Entry.Front || Entry.Due || !Content.IsAlive())
            {
                continue;
            }

            Tabs::Select(GetRegion(Root, Entry.Where), Content.GetParent().GetIndex());
            Entry.Front = false;
            Changed     = true;
        }
        return Changed;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dock::Place(Element Root, UInt Index)
    {
        Ref<Tree>         Owned = Root.GetTree();
        const Ptr<Harbor> Own   = Root.Get<Harbor>();
        const Berth       Entry = Own->Berths[Index];
        const Str         Style = Own->Style;

        Own->Berths[Index].Due   = false;
        Own->Berths[Index].Again = false;

        const Element Content(Owned, Entry.Content);

        if (!Content.IsAlive())
        {
            return;
        }

        // Where it stands now, in a window or on a tab's page in a region.
        const Element Opened(Owned, Entry.Window);
        const Bool    Docked  = Entry.Where != Side::Float && !Opened.IsAlive();
        const Element Page    = Docked ? Content.GetParent() : Element();
        const Element Region  = Docked ? GetRegion(Root, Entry.Where) : Element();
        const Element Tab     = Docked ? Tabs::GetTab(Region, Page.GetIndex()) : Element();
        const Bool    Carried = Tab.IsAlive() && Tab.GetHandle() == Own->Lifted;

        if (Entry.Goal == Side::Float)
        {
            // A floating panel only moves its window.
            if (Opened.IsAlive())
            {
                Window::MoveTo(Opened, Entry.Spot);
                return;
            }

            const Element Made = Launch(Root, Entry);
            Content.Attach(Window::GetContent(Made));

            Ref<Berth> Kept = Root.Get<Harbor>()->Berths[Index];
            Kept.Window = Made.GetHandle();
            Kept.Where  = Side::Float;
        }
        else
        {
            // Docked where it stands already it stays, unless its tab is carried away, which goes once let go.
            if (Entry.Goal == Entry.Where && Docked && !Entry.Again && !Carried)
            {
                return;
            }

            const Element Target = GetRegion(Root, Entry.Goal);
            Content.Attach(Moor(Target, Entry.Title, Style));
            Tabs::Select(Target, Tabs::GetCount(Target) - 1);

            if (Opened.IsAlive())
            {
                Opened.Destruct();
            }

            Ref<Berth> Kept = Root.Get<Harbor>()->Berths[Index];
            Kept.Window = Handle();
            Kept.Where  = Entry.Goal;
            Kept.Home   = Entry.Goal;
        }

        // The tab it left goes, unless the pointer still carries it, which takes it away once let go.
        if (Docked && Page.IsAlive() && !Carried)
        {
            Tabs::RemoveTab(Region, Page.GetIndex());
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Dock::Launch(Element Root, ConstRef<Berth> Entry)
    {
        const Str     Style  = Root.Get<Harbor>()->Style;
        const Element Result = Window::Open(Root.GetTree(), Entry.Title, Widget::Part(Style, "Window"));

        // A window of the dock's own, so dragging it can dock it.
        Widget::Govern(Result, Handler::Create<&Dock::OnFloat>(), false);

        Ref<Box> Shape = Result.EditBox();
        Shape.Width  = Length::Pixels(Entry.Size.GetX() > 0.0f ? Entry.Size.GetX() : 280.0f);
        Shape.Height = Length::Pixels(Entry.Size.GetY() > 0.0f ? Entry.Size.GetY() : 240.0f);

        Ref<Box> Fill = Window::GetContent(Result).EditBox();
        Fill.Grow  = 1.0f;
        Fill.Basis = Length::Pixels(0.0f);

        Window::MoveTo(Result, Entry.Spot);
        Entitle(Result, Entry.Title, true);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dock::Entitle(Element Holder, Text Title, Bool Floats)
    {
        if (!Holder.IsAlive() || (!Binder::IsBound(Title) && !Binder::IsLocalized(Title)))
        {
            return;
        }

        if (Floats)
        {
            Binder::Attach(Holder, Title, Binder::PutText<&Window::SetTitle>());
        }
        else
        {
            Binder::Attach(Holder, Title, Binder::PutText<&Button::SetText>());
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Dock::Moor(Element Region, Text Title, Text Style)
    {
        const Element Result = Tabs::AddTab(Region, Title, Widget::Part(Style, "Region"));
        Entitle(Tabs::GetTab(Region, Tabs::GetCount(Region) - 1), Title, false);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dock::Resize(Element Root, ConstRef<JsonValue> Sizes)
    {
        if (!Sizes.IsArray())
        {
            LOG_W("A dock's 'Sizes' must be a list of the left, right, top and bottom sizes, so they are left alone");
            return;
        }

        ConstRef<JsonValue::Array> List = Sizes.GetArray();
        const ConstPtr<Harbor>     Own  = Root.Get<Harbor>();
        const Element              Outer(Root.GetTree(), Own->Outer);
        const Element              Middle(Root.GetTree(), Own->Middle);

        const Array     Holders(Middle, Middle, Outer, Outer);
        constexpr Array Panes(0, 2, 0, 2);

        for (UInt Index = 0; Index < List.GetSize() && Index < 4; ++Index)
        {
            if (Real32 Size = 0.0f; Parser::ParseNumber(List[Index], Size) && Size > 0.0f)
            {
                Splitter::SetSize(Holders[Index], Panes[Index], Size);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dock::Hover(Element Root, Vector2 Point)
    {
        Ref<Tree>  Owned = Root.GetTree();
        const Rect Frame = Root.GetFrame();
        const Str  Style = Root.Get<Harbor>()->Style;
        Element    Overlay(Owned, Root.Get<Harbor>()->Overlay);

        // The targets stand in a pane of the drag band, over every window and never hit, laid over the dock.
        if (!Overlay.IsAlive())
        {
            Overlay = Element(Owned, Owned.Open(Drag::kLayer));
            Overlay.SetStyle(Widget::Part(Style, "Overlay"));
            Pin(Overlay, Frame, Vector2());

            const Element Preview = Panel::Create(Overlay, Widget::Part(Style, "Preview"));
            Preview.EditBox().Placement = Position::Absolute;
            Preview.SetState(State::Hidden, true);

            Sequence<Handle> Targets;

            for (UInt32 Region = 0; Region < static_cast<UInt32>(Side::Float); ++Region)
            {
                const Element Target = Panel::Create(Overlay, Widget::Part(Style, "Target"));
                Pin(Target, Mark(Frame, static_cast<Side>(Region)), Frame.GetPosition());
                Targets.Append(Target.GetHandle());
            }

            const Ptr<Harbor> Own = Root.Get<Harbor>();
            Own->Overlay = Overlay.GetHandle();
            Own->Preview = Preview.GetHandle();
            Own->Targets = ::Move(Targets);
        }

        const Ptr<Harbor> Own = Root.Get<Harbor>();
        const SInt32      Lit = Aim(Frame, Point);

        for (UInt32 Region = 0; Region < Own->Targets.GetSize(); ++Region)
        {
            const Element Target(Owned, Own->Targets[Region]);
            const Bool    Under = static_cast<SInt32>(Region) == Lit;

            if (Target.IsAlive() && Target.HasState(Drag::DropHover()) != Under)
            {
                Target.SetState(Drag::DropHover(), Under);
            }
        }

        // The room the panel would take is lit under the targets.
        const Element Preview(Owned, Own->Preview);

        if (!Preview.IsAlive())
        {
            return;
        }

        Preview.SetState(State::Hidden, Lit < 0);

        if (Lit >= 0)
        {
            Pin(Preview, Shade(Frame, static_cast<Side>(Lit)), Frame.GetPosition());
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dock::Pin(Element Target, Rect Room, Vector2 Origin)
    {
        Ref<Box> Shape = Target.EditBox();
        Shape.Placement = Position::Absolute;
        Shape.Left      = Length::Pixels(Room.GetMinimumX() - Origin.GetX());
        Shape.Top       = Length::Pixels(Room.GetMinimumY() - Origin.GetY());
        Shape.Width     = Length::Pixels(Room.GetWidth());
        Shape.Height    = Length::Pixels(Room.GetHeight());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dock::Clear(Element Root)
    {
        const Ptr<Harbor> Own = Root.Get<Harbor>();

        if (const Element Overlay(Root.GetTree(), Own->Overlay); Overlay.IsAlive())
        {
            Overlay.Destruct();
        }

        Own->Overlay = Handle();
        Own->Preview = Handle();
        Own->Targets.Clear();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dock::Land(Ref<Interface> Owner, Element Root, UInt Index, Vector2 Point)
    {
        const SInt32 Lit = Aim(Root.GetFrame(), Point);
        Clear(Root);

        if (Lit >= 0)
        {
            Steer(Owner, Root, Index, static_cast<Side>(Lit));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dock::Steer(Ref<Interface> Owner, Element Root, UInt Index, Side Goal)
    {
        Ref<Berth> Entry = Root.Get<Harbor>()->Berths[Index];
        Entry.Goal  = Goal;
        Entry.Due   = true;
        Entry.Front = true;

        // Copied, since an observer of the report may move the panel's entry.
        const Str Named = Entry.Name;
        Event     Report(kDock, Root.GetHandle());
        Report.Content = Named;
        Owner.Dispatch(Report);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    SInt32 Dock::Aim(Rect Frame, Vector2 Point)
    {
        for (UInt32 Region = 0; Region < static_cast<UInt32>(Side::Float); ++Region)
        {
            if (Mark(Frame, static_cast<Side>(Region)).Contains(Point))
            {
                return static_cast<SInt32>(Region);
            }
        }
        return -1;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Rect Dock::Mark(Rect Frame, Side Where)
    {
        constexpr Real32 kStep = kTarget + kSpacing;

        // A cross of five mid-dock, each side's target toward its side, in the order the sides are counted.
        static constexpr Array kToward(Vector2(-kStep, 0.0f),
                                       Vector2(kStep, 0.0f),
                                       Vector2(0.0f, -kStep),
                                       Vector2(0.0f, kStep),
                                       Vector2(),
                                       Vector2());

        const Vector2 Corner = Frame.GetCenter() + kToward[static_cast<UInt32>(Where)] - Vector2(kTarget * 0.5f);
        return Rect(Corner, Corner + Vector2(kTarget));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Rect Dock::Shade(Rect Frame, Side Where)
    {
        // A side takes the band of three tenths along it; the middle stands half a band in from every edge.
        static constexpr Array kShares(Rect(0.0f, 0.0f, 0.3f, 1.0f),
                                       Rect(0.7f, 0.0f, 1.0f, 1.0f),
                                       Rect(0.0f, 0.0f, 1.0f, 0.3f),
                                       Rect(0.0f, 0.7f, 1.0f, 1.0f),
                                       Rect(0.15f, 0.15f, 0.85f, 0.85f),
                                       Rect(0.15f, 0.15f, 0.85f, 0.85f));
        return Rect::Within(Frame, kShares[static_cast<UInt32>(Where)]);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Dock::Seek(Ref<Interface> Owner, Handle Window, Ref<UInt> Index)
    {
        Element Result;

        Owner.GetFacets<Harbor>().ForEach([&](UInt32 Slot, Ref<Harbor> Own)
        {
            const SInt Found = Own.Berths.Find([Window](ConstRef<Berth> Entry)
            {
                return Entry.Window == Window;
            });

            if (Found >= 0 && !Result.IsAlive())
            {
                Index  = static_cast<UInt>(Found);
                Result = Element(Owner, Owner.GetHandle(Slot));
            }
        });
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    SInt32 Dock::Find(ConstRef<Harbor> Own, Text Name)
    {
        return static_cast<SInt32>(Own.Berths.Find([Name](ConstRef<Berth> Entry)
        {
            return Entry.Name == Name;
        }));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    SInt32 Dock::Find(Element Root, Element Tab)
    {
        const ConstPtr<Harbor> Own = Root.Get<Harbor>();

        if (!Own || !Tab.IsAlive())
        {
            return -1;
        }

        // The tab's page stands at the same place among the pages as the tab among the tabs.
        const Element Region = Tab.GetParent().GetParent();
        const Element Page   = Tabs::GetPage(Region, Tab.GetIndex());
        const SInt    Found  = Page.IsAlive() ? Own->Berths.Find([Root, Page](ConstRef<Berth> Entry)
        {
            return Element(Root.GetTree(), Entry.Content).GetParent() == Page;
        }) : -1;

        if (Found >= 0)
        {
            return static_cast<SInt32>(Found);
        }

        // A tab whose panel floats away while the pointer carries it still names that panel.
        if (Tab.GetHandle() == Own->Lifted)
        {
            return static_cast<SInt32>(Own->Berths.Find([Own](ConstRef<Berth> Entry)
            {
                return Entry.Content == Own->Carried;
            }));
        }
        return -1;
    }
}