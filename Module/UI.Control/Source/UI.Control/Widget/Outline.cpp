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

#include "Outline.hpp"
#include "Common.hpp"
#include "Label.hpp"
#include "Panel.hpp"
#include "Repeater.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Facet/Layout/Virtual.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Style/Skin.hpp"
#include "UI.Core/Style/Theme.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Binder.hpp"
#include "UI.Data/Datum.hpp"
#include "UI.Data/Model.hpp"
#include "UI.Data/Path.hpp"
#include "UI.Data/Step.hpp"
#include "UI.Gesture/Drag.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Outline::Create(Element Parent, Text Style)
    {
        const Element Result = Panel::Create(Parent, Style);
        Widget::Govern(Result, Handler::Create<&Outline::OnEvent>(), false);
        Result.EditBox().Height = Length::Pixels(kHeight);

        const Element List = Repeater::Create(Result, Widget::Part(Style, "List"));
        Ref<Box>      Rows = List.EditBox();
        Rows.Height = Length::Auto();
        Rows.Grow   = 1.0f;
        Rows.Basis  = Length::Pixels(0.0f);

        // The rows are named one by one, as the open branches lay them out.
        Repeater::SetTemplate(List, Maker::Create<&Outline::OnMake>());
        Repeater::SetRecords(List, Sequence<Str>());

        Ref<Hierarchy> Own = Result.Add<Hierarchy>();
        Own.List  = List.GetHandle();
        Own.Style = Style;

        Result.Add<Virtual>().Realize = Realizer::Create<&Outline::OnRealize>();
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Outline::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Element Result = Create(Parent, Markup::GetText(Node, "Style", "Outline"));

        SetNested(Result, Markup::GetText(Node, "Nested", "Children"));
        Repeater::SetItemHeight(GetList(Result), Markup::GetNumber(Node, "ItemHeight", 0.0f));
        Repeater::BuildRows(Reader, GetList(Result), Node);
        SetMultiple(Result, Markup::GetBool(Node, "Multiple"));
        SetDraggable(Result, Markup::GetText(Node, "DragRows"), Markup::GetBool(Node, "DropBetween"));

        if (const ConstPtr<JsonValue> Shape = Markup::GetField(Node, "ItemTemplate"); Shape && Shape->IsString())
        {
            SetTemplate(Result, Reader, Shape->GetString());
        }
        else if (Shape && Shape->IsObject())
        {
            SetTemplate(Result, Reader, * Shape);
        }
        else if (Shape)
        {
            LOG_W("A tree view's 'ItemTemplate' must name a template or be a node, so its rows show their names");
        }

        // The records are one list in braces, read from the scope the tree view stands in.
        if (const Text Source = Markup::GetText(Node, "Items"); !Source.IsEmpty())
        {
            if (Text Inside; Repeater::Unwrap(Source, Inside))
            {
                SetItems(Result, Inside, Reader.GetScope());
            }
            else
            {
                LOG_W("A tree view's 'Items' must name one list alone in braces, which '{0}' does not", Source);
            }
        }

        if (const Text Picked = Markup::GetText(Node, "Selected"); !Picked.IsEmpty() && !Binder::IsBound(Picked))
        {
            Select(Result, Picked);
        }
        Reader.Bind(Result, Node, "Selected", Binder::PutText<&Outline::Select>(),
                    Binder::Take<Str, &Outline::GetSelected>());
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Outline::SetItems(Element Owner, Text Source, Text Scope)
    {
        if (const Ptr<Hierarchy> Own = Owner.Get<Hierarchy>(); Own && !Source.IsEmpty())
        {
            Own->Base  = Repeater::Resolve(Source, Scope);
            Own->Stale = true;

            if (!Path::Parse(Own->Base, Own->Where))
            {
                Own->Where.Steps.Clear();
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Outline::SetNested(Element Owner, Text Key)
    {
        if (const Ptr<Hierarchy> Own = Owner.Get<Hierarchy>(); Own && !Key.IsEmpty())
        {
            Own->Nested = Key;
            Own->Stale  = true;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Outline::SetTemplate(Element Owner, Ref<Markup> Reader, Text Template)
    {
        if (const Ptr<Hierarchy> Own = Owner.Get<Hierarchy>())
        {
            Own->Reader   = AddressOf(Reader);
            Own->Template = Template;
            Own->Node     = Unique<JsonValue>();
            Repeater::SetTemplate(GetList(Owner), Maker::Create<&Outline::OnMake>());
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Outline::SetTemplate(Element Owner, Ref<Markup> Reader, ConstRef<JsonValue> Node)
    {
        if (const Ptr<Hierarchy> Own = Owner.Get<Hierarchy>())
        {
            Own->Reader = AddressOf(Reader);
            Own->Template.Clear();
            Own->Node   = Unique<JsonValue>::Create(Node);
            Repeater::SetTemplate(GetList(Owner), Maker::Create<&Outline::OnMake>());
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Outline::SetExpanded(Element Owner, Text Path, Bool Expanded)
    {
        const Ptr<Hierarchy> Own = Owner.Get<Hierarchy>();

        if (!Own || Path.IsEmpty())
        {
            return;
        }

        const Digest Branch = GetBranch(Binder::GetModel(Owner.GetTree()), Path);

        if (Expanded)
        {
            Own->Opened.Insert(Branch);
        }
        else
        {
            Own->Opened.Erase(Branch);
        }
        Own->Stale = true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Outline::IsExpanded(Element Owner, Text Path)
    {
        const ConstPtr<Hierarchy> Own = Owner.Get<Hierarchy>();
        return Own && Own->Opened.Contains(GetBranch(Binder::GetModel(Owner.GetTree()), Path));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Outline::SetMultiple(Element Owner, Bool Enabled)
    {
        Repeater::SetMultiple(GetList(Owner), Enabled);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Outline::Select(Element Owner, Text Path)
    {
        const Ptr<Hierarchy> Own = Owner.Get<Hierarchy>();

        if (!Own)
        {
            return;
        }

        if (Path.IsEmpty())
        {
            Own->Wanted.Clear();
            Repeater::SetIndex(GetList(Owner), -1);
            return;
        }

        // Every branch above the record opens, so it has a row; each record on the way ends at a bracket or dot.
        const ConstPtr<Model> Data   = Binder::GetModel(Owner.GetTree());
        Bool                  Opened = false;

        for (UInt Cursor = 1; Cursor < Path.GetSize(); ++Cursor)
        {
            if (Path[Cursor - 1] != ']' || Path[Cursor] != '.')
            {
                continue;
            }

            if (const Digest Branch = GetBranch(Data, Path.Slice(0, Cursor)); Own->Opened.Insert(Branch))
            {
                Opened = true;
            }
        }

        const SInt32 Found = Opened ? -1 : Find(* Own, Path);

        if (Found >= 0)
        {
            Own->Wanted.Clear();
            Repeater::SetIndex(GetList(Owner), Found);
            return;
        }

        // Picked once the tree is laid out with its branch open.
        Own->Wanted = Path;
        Own->Stale  = true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str Outline::GetSelected(Element Owner)
    {
        const ConstPtr<Hierarchy> Own = Owner.Get<Hierarchy>();

        if (!Own)
        {
            return Str();
        }

        if (!Own->Wanted.IsEmpty())
        {
            return Own->Wanted;
        }

        const SInt32 Lead = Repeater::GetIndex(GetList(Owner));
        return Lead >= 0 ? GetPath(Owner, static_cast<UInt32>(Lead)) : Str();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Sequence<Str> Outline::GetSelection(Element Owner)
    {
        const ConstPtr<Hierarchy> Own = Owner.Get<Hierarchy>();
        Sequence<Str>             Result;

        for (const UInt32 Position : Repeater::GetPicked(GetList(Owner)))
        {
            if (Own && Position < Own->Limbs.GetSize())
            {
                Result.Append(Own->Limbs[Position].Path);
            }
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Outline::GetCount(Element Owner)
    {
        const ConstPtr<Hierarchy> Own = Owner.Get<Hierarchy>();
        return Own ? static_cast<UInt32>(Own->Limbs.GetSize()) : 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str Outline::GetPath(Element Owner, UInt32 Position)
    {
        const ConstPtr<Hierarchy> Own = Owner.Get<Hierarchy>();
        return Own && Position < Own->Limbs.GetSize() ? Own->Limbs[Position].Path : Str();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Outline::GetDepth(Element Owner, UInt32 Position)
    {
        const ConstPtr<Hierarchy> Own = Owner.Get<Hierarchy>();
        return Own && Position < Own->Limbs.GetSize() ? Own->Limbs[Position].Depth : 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Outline::GetRow(Element Owner, Text Path)
    {
        const ConstPtr<Hierarchy> Own   = Owner.Get<Hierarchy>();
        const SInt32              Found = Own ? Find(* Own, Path) : -1;
        return Found >= 0 ? Repeater::GetRow(GetList(Owner), static_cast<UInt32>(Found)) : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Outline::GetArrow(Element Row)
    {
        // A row holds its indent first and its arrow after it.
        return Row.IsAlive() ? Row.GetFirst().GetNext() : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Outline::GetList(Element Owner)
    {
        const ConstPtr<Hierarchy> Own = Owner.Get<Hierarchy>();
        return Own ? Element(Owner.GetTree(), Own->List) : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Outline::Refresh(Element Owner)
    {
        if (const Ptr<Hierarchy> Own = Owner.Get<Hierarchy>())
        {
            Own->Stale = true;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Outline::SetDraggable(Element Owner, Text Kind, Bool Between)
    {
        const Ptr<Hierarchy> Own = Owner.Get<Hierarchy>();

        if (!Own || (Own->Carry == Kind && Own->Between == Between))
        {
            return;
        }

        Own->Carry   = Kind;
        Own->Between = Between && !Kind.IsEmpty();

        // Every row is made again, so each is a source and a target, and holds its marker, as asked now.
        Repeater::SetTemplate(GetList(Owner), Maker::Create<&Outline::OnMake>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Landing Outline::GetLanding(Element Owner)
    {
        const ConstPtr<Hierarchy> Own = Owner.Get<Hierarchy>();
        return Own ? Own->Last : Landing();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Outline::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        const Element             Root(Owner, Self);
        const ConstPtr<Hierarchy> Own = Owner.Get<Hierarchy>(Self);
        const Element             List(Owner, Own->List);

        // A pick the rows report is the tree view's, so its binding and its observers hear it as its own.
        if (Message.Kind == Signal::kChange && Message.Target == Own->List)
        {
            Owner.Dispatch(Event(Signal::kChange, Self));
            return true;
        }

        // So are its rows' activations and renames, heard as stopped only when the tree view's are.
        const Bool Reported = Message.Kind == Signal::kActivate || Message.Kind == Repeater::kRename;

        if (Reported && Message.Target == Own->List)
        {
            Event Forward = Message;
            Forward.Target = Self;
            return Owner.Dispatch(Forward);
        }

        const Bool Carried = !Own->Carry.IsEmpty() && Owner.GetParent(Message.Target) == Own->List;

        // A row picked up carries the path of the record it shows then, for any target to read.
        if (Carried && Message.Kind == Drag::kStart)
        {
            if (const SInt32 Position = Repeater::GetRecord(List, Element(Owner, Message.Target)); Position >= 0)
            {
                Drag::SetPayload(Element(Owner, Message.Target), Datum(GetPath(Root, static_cast<UInt32>(Position))));
            }
            return false;
        }

        if (Carried && Message.Kind == Drag::kDrop)
        {
            Drop(Owner, Root, Message);
            return true;
        }

        if (Message.Kind == Signal::kKey && Message.Target == Own->List)
        {
            const SInt32 Lead = Repeater::GetIndex(List);

            if (Lead < 0 || static_cast<UInt>(Lead) >= Own->Limbs.GetSize()
                || (Message.Key != ZyInput::Key::Left && Message.Key != ZyInput::Key::Right))
            {
                return false;
            }

            const UInt32 Position = static_cast<UInt32>(Lead);
            const Limb   Entry    = Own->Limbs[Position];

            if (Message.Key == ZyInput::Key::Right)
            {
                // Right opens a closed branch, then steps to its first child.
                if (Entry.Parent && !Entry.Open)
                {
                    Turn(Owner, Root, Position, true);
                    return true;
                }

                if (Entry.Open && Position + 1 < Own->Limbs.GetSize() && Own->Limbs[Position + 1].Depth > Entry.Depth)
                {
                    Choose(Owner, Root, Position + 1);
                    return true;
                }
                return false;
            }

            // Left closes an open branch, then climbs to the record holding it.
            if (Entry.Open)
            {
                Turn(Owner, Root, Position, false);
                return true;
            }

            for (UInt32 Above = Position; Entry.Depth > 0 && Above-- > 0;)
            {
                if (Own->Limbs[Above].Depth < Entry.Depth)
                {
                    Choose(Owner, Root, Above);
                    return true;
                }
            }
            return false;
        }

        // An arrow is a part of a row, and a row a child of the list.
        const Handle Parent = Owner.GetParent(Message.Target);
        const Bool   Arrow  = Parent.IsValid() && Owner.GetParent(Parent) == Own->List
            && GetArrow(Element(Owner, Parent)).GetHandle() == Message.Target;
        const Bool   Twice  = Message.Kind == Signal::kPress && Message.Count == 2 && Parent == Own->List;

        if ((Message.Kind != Signal::kClick || !Arrow) && !Twice)
        {
            return false;
        }

        const SInt32 Position = Repeater::GetRecord(List, Element(Owner, Arrow ? Parent : Message.Target));

        if (Position < 0 || static_cast<UInt>(Position) >= Own->Limbs.GetSize() || !Own->Limbs[Position].Parent)
        {
            return false;
        }

        Turn(Owner, Root, static_cast<UInt32>(Position), !Own->Limbs[Position].Open);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Outline::OnRealize(Ref<Interface> Owner, Handle Self)
    {
        const Element        Root(Owner, Self);
        const Ptr<Hierarchy> Own   = Owner.Get<Hierarchy>(Self);
        const Ptr<Model>     Data  = Binder::GetModel(Owner);
        const UInt32         Stamp = Data ? Data->GetRevision() : 0;
        Bool                 Due   = Own->Stale || Data != Own->Source;

        // A model left as it was costs one comparison; a change elsewhere in it, one lookup of the tree view's list.
        if (Due || Stamp != Own->Revision)
        {
            const UInt32 Deep = Data && !Own->Where.Steps.IsEmpty() ? Data->GetSubtreeStamp(Own->Where) : 0;

            // A record anywhere in an open branch may have come or gone, so any change beneath the top list lays out.
            Due           = Due || Deep != Own->Deep;
            Own->Revision = Stamp;
            Own->Deep     = Deep;
        }

        Bool Changed = false;

        if (Due)
        {
            Own->Stale  = false;
            Own->Source = Data;
            Changed = Lay(Root);
        }

        Changed = Shape(Root, Owner.GetTheme()) || Changed;
        return Aim(Owner, Root) || Changed;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Outline::OnMake(Element Row, Text Scope)
    {
        const Element Root = Seek(Row);

        if (!Root.IsAlive())
        {
            return;
        }

        // Copied, since the content may hold a list of its own and move this facet.
        const Ptr<Hierarchy>      Own      = Root.Get<Hierarchy>();
        const Ptr<Markup>         Reader   = Own->Reader;
        const ConstPtr<JsonValue> Node     = Own->Node ? Own->Node.Grab() : nullptr;
        const Str                 Template = Own->Template;
        const Str                 Style    = Own->Style;
        const Str                 Carry    = Own->Carry;
        const Bool                Between  = Own->Between;

        Ref<Box> Line = Row.EditBox();
        Line.Flow    = Direction::Row;
        Line.Items   = Align::Center;
        Line.Content = Justify::Start;

        // The indent is sized by the tree view for the record the row shows, the arrow turned by it.
        Panel::Create(Row).EditBox().Shrink = 0.0f;

        const Element Arrow = Label::Create(Row, ">", Widget::Part(Style, "Arrow"));
        Widget::Govern(Arrow, Handler(), false);
        Arrow.EditBox().Shrink = 0.0f;
        Arrow.SetState(State::Disabled, true);

        const Element Holder = Panel::Create(Row, Widget::Part(Style, "Content"), Direction::Row);
        Ref<Box>      Fill   = Holder.EditBox();
        Fill.Grow  = 1.0f;
        Fill.Items = Align::Center;

        if (Reader && (Node || !Template.IsEmpty()))
        {
            Repeater::Stamp(* Reader, Holder, Scope, Node, Template);
        }
        else
        {
            const Element Words = Label::Create(Holder, "", Widget::Part(Style, "Label"));
            Label::SetOverflow(Words, Overflow::Ellipsis);
            Words.EditBox().Grow = 1.0f;
            Binder::Attach(Words, "{Name}", Binder::PutText<&Label::SetText>(), UI::Reader(), Scope);
        }

        if (Carry.IsEmpty())
        {
            return;
        }

        // Each row is carried as the tree view's kind and takes a row of it, the payload set once it is picked up.
        Drag::SetSource(Row, Carry);
        Drag::SetTarget(Row, Carry);

        if (!Between)
        {
            return;
        }

        // The marker stands over the row's top or bottom edge, out of the flow, hidden until a drop would land there.
        const Element Marker = Panel::Create(Row, Widget::Part(Style, "Marker"));
        Ref<Box>      Edge   = Marker.EditBox();
        Edge.Placement = Position::Absolute;
        Edge.Left      = Length::Pixels(0.0f);
        Edge.Right     = Length::Pixels(0.0f);
        Marker.SetState(State::Hidden, true);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Outline::Walk(ConstRef<Model>     Data,
                       ConstRef<Hierarchy> Own,
                       Text                List,
                       UInt16              Depth,
                       Ref<Sequence<Limb>> Output)
    {
        const UInt32 Count = Data.GetCount(List);

        for (UInt32 Index = 0; Index < Count; ++Index)
        {
            Limb Entry;
            Entry.Path.Format<"{0}[{1}]">(List, Index);
            Entry.Depth = Depth;

            const Str Children = Str::Join(Entry.Path, '.', Own.Nested);
            const Str Flag     = Entry.Path + ".Expandable";

            // A record says it may hold children before it does, so a game fills them only once it is opened.
            const UInt32 Held = Data.GetCount(Children);
            Entry.Parent = Held > 0 || Data.GetBool(Flag);
            Entry.Open   = Entry.Parent && Own.Opened.Contains(GetBranch(AddressOf(Data), Entry.Path));

            const Bool Descend = Entry.Open && Held > 0;
            Output.Append(Move(Entry));

            if (Descend)
            {
                Walk(Data, Own, Children, static_cast<UInt16>(Depth + 1), Output);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Digest Outline::GetBranch(ConstPtr<Model> Data, Text Path)
    {
        const SInt Open = StrFindLast(Path, '[');

        // A record of a list ends in its index, which the model reads its key by.
        if (Data && Open > 0 && Path.GetBack() == ']')
        {
            UInt32 Index = 0;

            for (UInt Cursor = static_cast<UInt>(Open) + 1; Cursor + 1 < Path.GetSize() && StrIsDigit(Path[Cursor]);
                 ++Cursor)
            {
                Index = Index * 10 + static_cast<UInt32>(Path[Cursor] - '0');
            }

            if (const UInt64 Key = Data->GetKey(Path.Slice(0, static_cast<UInt>(Open)), Index); Key != 0)
            {
                return Digest(Key);
            }
        }
        return Digest(Identify(Path));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Outline::Lay(Element Owner)
    {
        const Ptr<Hierarchy> Own  = Owner.Get<Hierarchy>();
        const Ptr<Model>     Data = Binder::GetModel(Owner.GetTree());
        const Element        List = GetList(Owner);

        Sequence<Limb> Limbs;

        if (Data && !Own->Base.IsEmpty())
        {
            Walk(* Data, * Own, Own->Base, 0, Limbs);
        }

        Bool Moved = Limbs.GetSize() != Own->Limbs.GetSize();
        Bool Same  = !Moved;

        // Every row is compared, as a row whose branch only opened or closed may come before one that changed record.
        for (UInt Index = 0; !Moved && Index < Limbs.GetSize(); ++Index)
        {
            ConstRef<Limb> Now = Limbs[Index];
            ConstRef<Limb> Was = Own->Limbs[Index];

            Moved = Now.Path != Was.Path;
            Same  = Same && !Moved && Now.Depth == Was.Depth && Now.Parent == Was.Parent && Now.Open == Was.Open;
        }

        if (Same && Own->Wanted.IsEmpty())
        {
            return false;
        }

        if (Moved)
        {
            // What was picked stays picked while its record still has a row.
            Sequence<Str> Picked;
            Str           Lead;

            for (const UInt32 Position : Repeater::GetPicked(List))
            {
                Picked.Append(Own->Limbs[Position].Path);
            }

            if (const SInt32 Index = Repeater::GetIndex(List); Index >= 0)
            {
                Lead = Own->Limbs[Index].Path;
            }

            Sequence<Str> Scopes;

            for (ConstRef<Limb> Entry : Limbs)
            {
                Scopes.Append(Entry.Path);
            }

            Own->Limbs = Move(Limbs);
            Repeater::SetRecords(List, Move(Scopes));

            const Ptr<Hierarchy> Kept = Owner.Get<Hierarchy>();
            const SInt32         Head = Lead.IsEmpty() ? -1 : Find(* Kept, Lead);
            Repeater::SetIndex(List, Head);

            for (ConstRef<Str> Path : Picked)
            {
                if (const SInt32 Found = Find(* Kept, Path); Found >= 0)
                {
                    Repeater::SetPicked(List, static_cast<UInt32>(Found), true);
                }
            }

            // The pick made last stays the one the keyboard moves from.
            if (Head >= 0)
            {
                List.Get<Items>()->Selected = Head;
            }
        }
        else
        {
            Own->Limbs = Move(Limbs);
        }

        // A record picked before it had a row is picked once it has one.
        if (const Ptr<Hierarchy> Kept = Owner.Get<Hierarchy>(); !Kept->Wanted.IsEmpty())
        {
            if (const SInt32 Found = Find(* Kept, Kept->Wanted); Found >= 0)
            {
                Repeater::SetIndex(List, Found);
                Repeater::ScrollTo(List, static_cast<UInt32>(Found));
            }
            Kept->Wanted.Clear();
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Outline::Shape(Element Owner, ConstPtr<Theme> Source)
    {
        const ConstPtr<Hierarchy> Own  = Owner.Get<Hierarchy>();
        const Element             List = GetList(Owner);
        Real32                    Step = kIndent;

        // A level's indent is the theme's width of the style plus `.Indent`, named on the stack for the frame loop.
        String<128> Name;
        Name.Format<"{0}.Indent">(Own->Style);

        if (Skin Found; Source && Source->Resolve(Identify(Name), State::None, Found)
            && Found.Has(Skin::Field::Width) && Found.Width.Measure == Length::Unit::Pixel)
        {
            Step = Found.Width.Value;
        }

        Bool Changed = false;

        // Editing a row only changes its boxes and states, so the cells are read where they stand.
        ConstRef<Sequence<Cell>> Cells = List.Get<Items>()->Cells;

        for (ConstRef<Cell> Entry : Cells)
        {
            const Element Row(Owner.GetTree(), Entry.Row);

            if (Entry.Parked || !Row.IsAlive() || Entry.Record >= Own->Limbs.GetSize())
            {
                continue;
            }

            ConstRef<Limb> Shown  = Own->Limbs[Entry.Record];
            const Element  Indent = Row.GetFirst();
            const Element  Arrow  = GetArrow(Row);
            const Length   Width  = Length::Pixels(Step * static_cast<Real32>(Shown.Depth));

            if (Indent.IsAlive() && Owner.GetTree().GetBox(Indent.GetHandle()).Width != Width)
            {
                Indent.EditBox().Width = Width;
                Changed = true;
            }

            if (Arrow.IsAlive() && Arrow.HasState(State::Disabled) == Shown.Parent)
            {
                Arrow.SetState(State::Disabled, !Shown.Parent);
                Changed = true;
            }

            if (Arrow.IsAlive() && Arrow.HasState(Widget::Checked()) != Shown.Open)
            {
                Arrow.SetState(Widget::Checked(), Shown.Open);
                Changed = true;
            }
        }
        return Changed;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Outline::Turn(Ref<Interface> Owner, Element Root, UInt32 Position, Bool Expanded)
    {
        const Str     Path = GetPath(Root, Position);
        const Element Row  = Repeater::GetRow(GetList(Root), Position);

        SetExpanded(Root, Path, Expanded);

        Event Message(Expanded ? Outline::kExpand : Outline::kCollapse, Root.GetHandle());
        Message.Content = Path;
        Message.Other   = Row.GetHandle();
        Owner.Dispatch(Message);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Outline::Choose(Ref<Interface> Owner, Element Root, UInt32 Position)
    {
        const Element List = GetList(Root);

        Repeater::SetIndex(List, static_cast<SInt32>(Position));
        Repeater::ScrollTo(List, Position);
        Owner.Dispatch(Event(Signal::kChange, Root.GetHandle()));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Outline::Drop(Ref<Interface> Owner, Element Root, ConstRef<Event> Message)
    {
        const Ptr<Hierarchy> Own    = Root.Get<Hierarchy>();
        const Element        List   = GetList(Root);
        const SInt32         Onto   = Repeater::GetRecord(List, Element(Owner, Message.Target));
        const Bool           Ours   = Owner.IsAlive(Message.Other) && Owner.GetParent(Message.Other) == Own->List;
        const SInt32         Picked = Ours ? Repeater::GetRecord(List, Element(Owner, Message.Other)) : -1;

        if (Onto < 0)
        {
            return;
        }

        // Kept here, since a handler may move the facet the drop is read from.
        Landing Found;
        Found.Target = GetPath(Root, static_cast<UInt32>(Onto));
        Found.Source = Picked >= 0 ? GetPath(Root, static_cast<UInt32>(Picked)) : Str();
        Found.Where  = Own->Between ? Land(Element(Owner, Message.Target), Message.Position) : Spot::Inside;

        // A record dropped on itself or on one beneath it would hold itself, so nothing is reported.
        const Str Below = Found.Source + ".";

        if (!Found.Source.IsEmpty() && (Found.Target == Found.Source || StrStartsWith(Found.Target, Below)))
        {
            return;
        }

        Own->Last = Found;

        Event Report(Outline::kDrop, Root.GetHandle());
        Report.Content = Found.Source;
        Report.Other   = Message.Other;
        Owner.Dispatch(Report);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Outline::Aim(Ref<Interface> Owner, Element Root)
    {
        const ConstPtr<Hierarchy> Own = Root.Get<Hierarchy>();

        // Only a tree view landing drops between rows looks at the drag, so the others cost nothing.
        if (!Own->Between)
        {
            return false;
        }

        const Handle Target = Drag::GetTarget(Owner);
        const Bool   Ours   = Owner.IsAlive(Target) && Owner.GetParent(Target) == Own->List;
        const Spot   Where  = Ours ? Land(Element(Owner, Target), Owner.GetPointer()) : Spot::Inside;
        const Handle Shown  = Ours && Where != Spot::Inside ? Target : Handle();
        const Handle Before = Own->Lit;
        Bool         Moved  = false;

        if (Before != Shown && Owner.IsAlive(Before))
        {
            GetMarker(Element(Owner, Before)).SetState(State::Hidden, true);
            Moved = true;
        }

        // Lit as a drop in it only while the pointer is over its middle, the marker showing a drop beside it.
        if (Ours && Owner.HasState(Target, Drag::DropHover()) != (Where == Spot::Inside))
        {
            Owner.SetState(Target, Drag::DropHover(), Where == Spot::Inside);
            Moved = true;
        }

        if (Shown.IsValid())
        {
            const Element Marker = GetMarker(Element(Owner, Shown));
            const Length  Edge   = Length::Pixels(0.0f);
            ConstRef<Box> Shape  = Owner.GetBox(Marker.GetHandle());
            const Bool    Top    = Where == Spot::Before;

            if (Shape.Top != (Top ? Edge : Length::Auto()) || Marker.HasState(State::Hidden))
            {
                Ref<Box> Edit = Marker.EditBox();
                Edit.Top    = Top ? Edge : Length::Auto();
                Edit.Bottom = Top ? Length::Auto() : Edge;
                Marker.SetState(State::Hidden, false);
                Moved = true;
            }
        }

        Root.Get<Hierarchy>()->Lit = Shown;
        return Moved;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Spot Outline::Land(Element Row, Vector2 Point)
    {
        const Rect   Frame = Row.GetFrame();
        const Real32 Share = (Point.GetY() - Frame.GetMinimumY()) / Max(Frame.GetHeight(), 1.0f);

        if (Share < kEdge)
        {
            return Spot::Before;
        }
        return Share > 1.0f - kEdge ? Spot::After : Spot::Inside;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Outline::GetMarker(Element Row)
    {
        Element Last = Row.GetFirst();

        while (Last.GetNext().IsAlive())
        {
            Last = Last.GetNext();
        }
        return Last;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Outline::Seek(Element Inside)
    {
        Element Current = Inside;

        while (Current.IsAlive() && !Current.Has<Hierarchy>())
        {
            Current = Current.GetParent();
        }
        return Current;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    SInt32 Outline::Find(ConstRef<Hierarchy> Own, Text Path)
    {
        return static_cast<SInt32>(Own.Limbs.Find([Path](ConstRef<Limb> Entry)
        {
            return Entry.Path == Path;
        }));
    }
}