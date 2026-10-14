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

#include "Grid.hpp"
#include "Common.hpp"
#include "Label.hpp"
#include "Panel.hpp"
#include "Repeater.hpp"
#include "Scroller.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Facet/Layout/Virtual.hpp"
#include "UI.Core/Facet/Visual/Caption.hpp"
#include "UI.Core/Facet/Visual/Look.hpp"
#include "UI.Core/Facet/Visual/Scroll.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Tree.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Binder.hpp"
#include "UI.Data/Datum.hpp"
#include "UI.Data/Model.hpp"
#include "UI.Data/Path.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Grid::Create(Element Parent, Text Style)
    {
        const Element Result = Panel::Create(Parent, Style);
        Widget::Govern(Result, Handler::Create<&Grid::OnEvent>(), false);
        Result.EditBox().Height = Length::Pixels(kHeight);

        // The headings and the rows scroll sideways together in a viewer, while the rows scroll down on their own.
        const Element Viewer = Scroller::Create(Result, Widget::Part(Style, "Viewer"));
        Ref<Box>      Frame  = Viewer.EditBox();
        Frame.Height = Length::Auto();
        Frame.Grow   = 1.0f;
        Frame.Basis  = Length::Pixels(0.0f);

        const Element Content = Scroller::GetContent(Viewer);
        const Element Header  = Panel::Create(Content, Widget::Part(Style, "Header"), Direction::Row);
        Header.EditBox().Shrink = 0.0f;

        const Element List = Repeater::Create(Content, Widget::Part(Style, "List"));
        Ref<Box>      Rows = List.EditBox();
        Rows.Height = Length::Auto();
        Rows.Grow   = 1.0f;
        Rows.Basis  = Length::Pixels(0.0f);

        // The rows are named one by one, in the order the grid sorts them.
        Repeater::SetTemplate(List, Maker::Create<&Grid::OnMake>());
        Repeater::SetRecords(List, Sequence<Str>());

        Ref<Ledger> Own = Result.Add<Ledger>();
        Own.Header = Header.GetHandle();
        Own.List   = List.GetHandle();
        Own.Style  = Style;

        Result.Add<Virtual>().Realize = Realizer::Create<&Grid::OnRealize>();
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Grid::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Element Result = Create(Parent, Markup::GetText(Node, "Style", "Grid"));

        if (const ConstPtr<JsonValue> List = Markup::GetField(Node, "Columns"); List && List->IsArray())
        {
            ConstRef<JsonValue::Array> Entries = List->GetArray();

            for (UInt Index = 0; Index < Entries.GetSize(); ++Index)
            {
                BuildColumn(Reader, Result, Entries[Index]);
            }
        }
        else if (List)
        {
            LOG_W("A grid's 'Columns' must be a list of columns, so it shows none");
        }

        Repeater::SetItemHeight(GetList(Result), Markup::GetNumber(Node, "ItemHeight", 0.0f));
        Repeater::BuildRows(Reader, GetList(Result), Node);
        SetMultiple(Result, Markup::GetBool(Node, "Multiple"));
        SetGroup(Result, Markup::GetText(Node, "Group"), Markup::GetBool(Node, "GroupDescending", true));

        // The records are one list in braces, read from the scope the grid stands in.
        if (const Text Source = Markup::GetText(Node, "Items"); !Source.IsEmpty())
        {
            if (Text Inside; Repeater::Unwrap(Source, Inside))
            {
                SetItems(Result, Inside, Reader.GetScope());
            }
            else
            {
                LOG_W("A grid's 'Items' must name one list alone in braces, which '{0}' does not", Source);
            }
        }

        // The column sorted by is named by its key.
        if (const Text Sort = Markup::GetText(Node, "Sort"); !Sort.IsEmpty())
        {
            const SInt32 Found = static_cast<SInt32>(Result.Get<Ledger>()->Columns.Find([Sort](ConstRef<Column> Entry)
            {
                return Entry.Key == Sort;
            }));

            if (Found < 0)
            {
                LOG_W("A grid is sorted by '{0}', which no column of it shows", Sort);
            }
            SetSort(Result, Found, Markup::GetBool(Node, "Descending"));
        }

        Select(Result, static_cast<SInt32>(Markup::GetNumber(Node, "Selected", -1.0f)));
        Reader.Bind(Result, Node, "Selected", Binder::Put<SInt32, &Grid::Select>(),
                    Binder::Take<SInt32, &Grid::GetSelected>());
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Grid::AddColumn(Element Owner, Column Entry, Ptr<Markup> Reader)
    {
        const Ptr<Ledger> Own = Owner.Get<Ledger>();

        if (!Own)
        {
            return;
        }

        if (Reader)
        {
            Own->Reader = Reader;
        }

        Own->Columns.Append(Move(Entry));
        Own->Widths.Clear();
        MakeHeading(Owner, static_cast<UInt32>(Own->Columns.GetSize() - 1), Reader ? Reader->GetScope() : Text());

        // Every row is made again, a cell for the new column included.
        Repeater::SetTemplate(GetList(Owner), Maker::Create<&Grid::OnMake>());
        Mark(Owner);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Grid::SetItems(Element Owner, Text Source, Text Scope)
    {
        const Ptr<Ledger> Own = Owner.Get<Ledger>();

        if (!Own || Source.IsEmpty())
        {
            return;
        }

        Own->Base  = Repeater::Resolve(Source, Scope);
        Own->Stale = true;
        Own->Order.Clear();

        if (!Path::Parse(Own->Base, Own->Where))
        {
            Own->Where.Steps.Clear();
        }
        Repeater::SetIndex(GetList(Owner), -1);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Grid::GetCount(Element Owner)
    {
        const ConstPtr<Ledger> Own = Owner.Get<Ledger>();
        return Own ? static_cast<UInt32>(Own->Columns.GetSize()) : 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Grid::SetWidth(Element Owner, UInt32 Index, Real32 Width)
    {
        if (const Ptr<Ledger> Own = Owner.Get<Ledger>(); Own && Index < Own->Columns.GetSize())
        {
            Ref<Column> Entry = Own->Columns[Index];
            Entry.Width = Width > 0.0f ? Max(Width, Entry.Minimum) : 0.0f;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Grid::GetWidth(Element Owner, UInt32 Index)
    {
        const ConstPtr<Ledger> Own = Owner.Get<Ledger>();
        return Own && Index < Own->Widths.GetSize() ? Own->Widths[Index] : 0.0f;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Grid::SetSort(Element Owner, SInt32 Index, Bool Descending)
    {
        const Ptr<Ledger> Own = Owner.Get<Ledger>();

        if (!Own)
        {
            return;
        }

        Own->Sorted     = Index >= 0 && static_cast<UInt>(Index) < Own->Columns.GetSize() ? Index : -1;
        Own->Descending = Own->Sorted >= 0 && Descending;
        Own->Stale      = true;
        Mark(Owner);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    SInt32 Grid::GetSort(Element Owner)
    {
        const ConstPtr<Ledger> Own = Owner.Get<Ledger>();
        return Own ? Own->Sorted : -1;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Grid::IsDescending(Element Owner)
    {
        const ConstPtr<Ledger> Own = Owner.Get<Ledger>();
        return Own && Own->Descending;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Grid::SetComparer(Element Owner, Comparer Compare)
    {
        if (const Ptr<Ledger> Own = Owner.Get<Ledger>())
        {
            Own->Compare = Move(Compare);
            Own->Stale   = true;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Grid::SetGroup(Element Owner, Text Key, Bool Descending)
    {
        if (const Ptr<Ledger> Own = Owner.Get<Ledger>())
        {
            Own->Group     = Key;
            Own->GroupDown = Descending;
            Own->Stale     = true;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Grid::Refresh(Element Owner)
    {
        if (const Ptr<Ledger> Own = Owner.Get<Ledger>())
        {
            Own->Stale = true;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Grid::SetMultiple(Element Owner, Bool Enabled)
    {
        Repeater::SetMultiple(GetList(Owner), Enabled);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Grid::Select(Element Owner, SInt32 Record)
    {
        const ConstPtr<Ledger> Own = Owner.Get<Ledger>();

        if (!Own)
        {
            return;
        }

        // Before the rows were first ordered, a row stands where its record does.
        SInt32 Position = Record;

        if (Record >= 0 && !Own->Order.IsEmpty())
        {
            const SInt Found = Own->Order.Find(static_cast<UInt32>(Record));
            Position = Found >= 0 ? static_cast<SInt32>(Found) : -1;
        }
        Repeater::SetIndex(GetList(Owner), Position);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    SInt32 Grid::GetSelected(Element Owner)
    {
        const SInt32 Position = Repeater::GetIndex(GetList(Owner));
        return Position >= 0 ? GetRecord(Owner, static_cast<UInt32>(Position)) : -1;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Sequence<UInt32> Grid::GetSelection(Element Owner)
    {
        Sequence<UInt32> Result = Repeater::GetPicked(GetList(Owner));

        for (Ref<UInt32> Entry : Result)
        {
            Entry = static_cast<UInt32>(GetRecord(Owner, Entry));
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    SInt32 Grid::GetRecord(Element Owner, UInt32 Position)
    {
        const ConstPtr<Ledger> Own = Owner.Get<Ledger>();

        if (!Own || Position >= Repeater::GetCount(GetList(Owner)))
        {
            return -1;
        }
        return Position < Own->Order.GetSize()
            ? static_cast<SInt32>(Own->Order[Position]) : static_cast<SInt32>(Position);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Grid::GetRow(Element Owner, UInt32 Record)
    {
        const ConstPtr<Ledger> Own = Owner.Get<Ledger>();

        if (!Own)
        {
            return Element();
        }

        const SInt Found = Own->Order.Find(Record);
        return Found >= 0 ? Repeater::GetRow(GetList(Owner), static_cast<UInt32>(Found)) : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Grid::GetHeading(Element Owner, UInt32 Index)
    {
        const ConstPtr<Ledger> Own     = Owner.Get<Ledger>();
        Element                Heading = Own ? Element(Owner.GetTree(), Own->Header).GetFirst() : Element();

        for (UInt32 Position = 0; Heading.IsAlive() && Position < Index; ++Position)
        {
            Heading = Heading.GetNext();
        }
        return Heading;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Grid::GetList(Element Owner)
    {
        const ConstPtr<Ledger> Own = Owner.Get<Ledger>();
        return Own ? Element(Owner.GetTree(), Own->List) : Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Grid::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        const Element          Root(Owner, Self);
        const ConstPtr<Ledger> Own = Owner.Get<Ledger>(Self);

        // A pick the rows report is the grid's, so its binding and its observers hear it as its own.
        if (Message.Kind == Signal::kChange && Message.Target == Own->List)
        {
            Owner.Dispatch(Event(Signal::kChange, Self));
            return true;
        }

        // So are its rows' activations and renames, still naming the record, heard as stopped only when the grid's are.
        const Bool Reported = Message.Kind == Signal::kActivate || Message.Kind == Repeater::kRename;

        if (Reported && Message.Target == Own->List)
        {
            Event Forward = Message;
            Forward.Target = Self;
            return Owner.Dispatch(Forward);
        }

        const Handle Parent = Owner.GetParent(Message.Target);

        // A click on a heading sorts; one on the grip it holds does not.
        if (Message.Kind == Signal::kClick && Parent == Own->Header)
        {
            Toggle(Owner, Root, Element(Owner, Message.Target).GetIndex());
            return true;
        }

        if (!Parent.IsValid() || Owner.GetParent(Parent) != Own->Header)
        {
            return false;
        }

        // A grip is the last a heading holds and a control; a drag on it moves the right edge of its column.
        const Element Heading(Owner, Parent);
        const UInt32  Index = Heading.GetIndex();

        if (Message.Kind == Signal::kPress && Message.Button == ZyInput::Button::Left)
        {
            Owner.Edit<Ledger>(Self)->Grab = Heading.GetFrame().GetMaximumX() - Message.Position.GetX();
            return true;
        }

        if (Message.Kind == Signal::kDrag)
        {
            const Real32 Edge = Message.Position.GetX() + Own->Grab;
            SetWidth(Root, Index, Max(Edge - Heading.GetFrame().GetMinimumX(), 1.0f));
            return true;
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Grid::OnRealize(Ref<Interface> Owner, Handle Self)
    {
        const Element     Root(Owner, Self);
        const Ptr<Ledger> Own   = Owner.Get<Ledger>(Self);
        const Ptr<Model>  Data  = Binder::GetModel(Owner);
        const UInt32      Stamp = Data ? Data->GetRevision() : 0;
        Bool              Due   = Own->Stale || Data != Own->Source;

        // A model left as it was costs one comparison; a change elsewhere in it, one lookup of the grid's own list.
        if (Due || Stamp != Own->Revision)
        {
            const UInt32 Deep = Data && !Own->Where.Steps.IsEmpty() ? Data->GetSubtreeStamp(Own->Where) : 0;

            // A sorted or grouped grid orders its rows again when its list or its records' values changed.
            if (!Due && Deep != Own->Deep)
            {
                const UInt32 Count = Data && !Own->Base.IsEmpty() ? Data->GetCount(Own->Base) : 0;
                Due = Own->Sorted >= 0 || !Own->Group.IsEmpty() || Count != Own->Order.GetSize();
            }

            Own->Revision = Stamp;
            Own->Deep     = Deep;
        }

        Bool Changed = false;

        if (Due)
        {
            Own->Stale  = false;
            Own->Source = Data;
            Changed = Arrange(Root);
        }
        return Widen(Root) || Changed;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Grid::OnMake(Element Row, Text Scope)
    {
        const Element Root = Seek(Row);

        if (!Root.IsAlive())
        {
            return;
        }

        // The cells stand side by side from the row's left edge, each as wide as its column.
        Ref<Box> Shape = Row.EditBox();
        Shape.Flow    = Direction::Row;
        Shape.Items   = Align::Stretch;
        Shape.Content = Justify::Start;

        const Ptr<Ledger> Sheet  = Root.Get<Ledger>();
        const Ptr<Markup> Reader = Sheet->Reader;
        const Str         Style  = Widget::Part(Sheet->Style, "Cell");
        const Str         Plain  = Widget::Part(Style, "Label");
        const Str         Rooted = Str::Print<"/{0}">(Scope);
        const UInt32      Count  = static_cast<UInt32>(Sheet->Columns.GetSize());

        for (UInt32 Index = 0; Index < Count; ++Index)
        {
            // Copied, since a cell holding a list of its own may move this facet.
            const Ptr<Ledger>         Own      = Root.Get<Ledger>();
            Ref<Column>               Entry    = Own->Columns[Index];
            const Real32              Width    = Index < Own->Widths.GetSize() ? Own->Widths[Index] : Entry.Minimum;
            const ConstPtr<JsonValue> Node     = Entry.Node ? Entry.Node.Grab() : nullptr;
            const Str                 Template = Entry.Template;
            const Str                 Key      = Entry.Key;
            const Align               Side     = Entry.Alignment;

            const Element Cell = Panel::Create(Row, Style);
            Cell.Add<Look>().Clip = true;

            Ref<Box> Fixed = Cell.EditBox();
            Fixed.Width   = Length::Pixels(Width);
            Fixed.Shrink  = 0.0f;
            Fixed.Content = Justify::Center;

            if (Reader && (Node || !Template.IsEmpty()))
            {
                Reader->PushScope(Rooted);

                if (Node)
                {
                    Reader->Build(Cell, * Node);
                }
                else
                {
                    Reader->Spawn(Cell, Template);
                }
                Reader->PopScope();
                continue;
            }

            // A column naming no template shows its value, cut short with an ellipsis when it runs past the cell.
            const Element Words = Label::Create(Cell, "", Plain);
            Label::SetOverflow(Words, Overflow::Ellipsis);
            Words.Edit<Caption>()->Alignment = Side;

            if (!Key.IsEmpty())
            {
                const Str Bound = Str::Print<"{{{0}}}">(Key);
                Binder::Attach(Words, Bound, Binder::PutText<&Label::SetText>(), UI::Reader(), Scope);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Grid::BuildColumn(Ref<Markup> Reader, Element Owner, ConstRef<JsonValue> Node)
    {
        Column Entry;
        Entry.Title     = Markup::GetText(Node, "Title");
        Entry.Key       = Markup::GetText(Node, "Key");
        Entry.Width     = Max(Markup::GetNumber(Node, "Width"), 0.0f);
        Entry.Share     = Max(Markup::GetNumber(Node, "Share", 1.0f), 0.0f);
        Entry.Minimum   = Max(Markup::GetNumber(Node, "Minimum", 40.0f), 0.0f);
        Entry.Sortable  = Markup::GetBool(Node, "Sortable", true);
        Entry.Alignment = ZyEnum::Cast(Markup::GetText(Node, "Align"), Align::Start);

        if (const ConstPtr<JsonValue> Cell = Markup::GetField(Node, "Cell"); Cell && Cell->IsString())
        {
            Entry.Template = Cell->GetString();
        }
        else if (Cell && Cell->IsObject())
        {
            Entry.Node = Unique<JsonValue>::Create(* Cell);
        }
        else if (Cell)
        {
            LOG_W("A grid column's 'Cell' must name a template or be a node, so it shows its value");
        }

        AddColumn(Owner, Move(Entry), AddressOf(Reader));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Grid::MakeHeading(Element Owner, UInt32 Index, Text Scope)
    {
        const ConstPtr<Ledger> Own   = Owner.Get<Ledger>();
        ConstRef<Column>       Entry = Own->Columns[Index];
        const Str              Style = Own->Style;
        const Str              Title = Entry.Title;
        const Bool             Bound = Binder::IsBound(Title) || Binder::IsLocalized(Title);
        const Real32           Width = Max(Entry.Width, Entry.Minimum);
        const Align            Side  = Entry.Alignment;
        const Element          Header(Owner.GetTree(), Own->Header);

        // A control of its own, so a click lands on it rather than on its title.
        const Element Heading = Header.Create();
        Widget::Govern(Heading, Handler(), false);
        Heading.SetStyle(Widget::Part(Style, "Heading"));

        // The title stands where the column's cells do, with the sort mark right after it.
        Ref<Box> Shape = Heading.EditBox();
        Shape.Flow    = Direction::Row;
        Shape.Items   = Align::Center;
        Shape.Content = Side == Align::End ? Justify::End : (Side == Align::Center ? Justify::Center : Justify::Start);
        Shape.Width   = Length::Pixels(Width);
        Shape.Shrink  = 0.0f;

        const Element Words = Label::Create(Heading, Bound ? Text() : Text(Title),
                                            Widget::Part(Style, "Heading.Label"));
        Label::SetOverflow(Words, Overflow::Ellipsis);

        // A title naming values or a text of the lexicon is read from the scope the grid was made in.
        if (Bound)
        {
            Binder::Attach(Words, Title, Binder::PutText<&Label::SetText>(), Reader(), Scope);
        }

        const Element Mark = Label::Create(Heading, ">", Widget::Part(Style, "Sort"));
        Mark.SetState(State::Hidden, true);

        // The grip stands over the heading's right edge, the height of it.
        const Element Grip = Heading.Create();
        Widget::Govern(Grip, Handler(), false);
        Grip.SetStyle(Widget::Part(Style, "Grip"));

        Ref<Box> Hold = Grip.EditBox();
        Hold.Placement = Position::Absolute;
        Hold.Top       = Length::Pixels(0.0f);
        Hold.Bottom    = Length::Pixels(0.0f);
        Hold.Right     = Length::Pixels(0.0f);
        Hold.Width     = Length::Pixels(kGrip);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Grid::Arrange(Element Owner)
    {
        const Ptr<Ledger> Own   = Owner.Get<Ledger>();
        const Ptr<Model>  Data  = Binder::GetModel(Owner.GetTree());
        const Element     List  = GetList(Owner);
        const UInt32      Count = Data && !Own->Base.IsEmpty() ? Data->GetCount(Own->Base) : 0;

        Sequence<UInt32> Order;
        Order.Advance(Count);

        for (UInt32 Index = 0; Index < Count; ++Index)
        {
            Order[Index] = Index;
        }

        if ((Own->Sorted >= 0 || !Own->Group.IsEmpty()) && Count > 1)
        {
            Sequence<Ptr<Model>> Records;
            Records.Advance(Count);

            for (UInt32 Index = 0; Index < Count; ++Index)
            {
                Records[Index] = Data->GetRecord(Own->Base, Index);
            }

            Rule Weighing;
            Weighing.Code       = Own->Compare;
            Weighing.Key        = Own->Sorted >= 0 ? Str(Own->Columns[Own->Sorted].Key) : Str();
            Weighing.Descending = Own->Descending;
            Weighing.Group      = Own->Group;
            Weighing.GroupDown  = Own->GroupDown;

            Order.Sort([&Records, &Weighing](UInt32 A, UInt32 B)
            {
                return Precedes(Records, Weighing, A, B);
            });
        }

        // Unchanged, the rows are left as they stand; the first order is always handed to the list.
        const Bool Same = Order.GetSize() == Own->Order.GetSize();

        if (Same && Repeater::GetCount(List) == Count && Compare(Order.GetData(), Own->Order.GetData(), Count))
        {
            return false;
        }

        // What was picked stays picked wherever its record moved; before any order a row stood where its record does.
        const Sequence<UInt32> Old    = Move(Own->Order);
        const Sequence<UInt32> Picked = List.Get<Items>()->Picked;
        const SInt32           Lead   = List.Get<Items>()->Selected;

        Sequence<SInt32> Where;
        Sequence<Str>    Scopes;
        Where.Advance(Count);
        Scopes.Advance(Count);

        for (UInt32 Index = 0; Index < Count; ++Index)
        {
            Where[Order[Index]] = static_cast<SInt32>(Index);
            Scopes[Index].Format<"{0}[{1}]">(Text(Own->Base), Order[Index]);
        }

        Own->Order = Move(Order);
        Repeater::SetRecords(List, Move(Scopes));

        const UInt32 Before = static_cast<UInt32>(Lead);
        const UInt32 Record = Lead >= 0 && Before < Old.GetSize() ? Old[Before] : Before;
        const SInt32 Head   = Lead >= 0 && Record < Count ? Where[Record] : -1;
        Repeater::SetIndex(List, Head);

        for (const UInt32 Position : Picked)
        {
            const UInt32 Moved = Position < Old.GetSize() ? Old[Position] : Position;

            if (Moved < Count)
            {
                Repeater::SetPicked(List, static_cast<UInt32>(Where[Moved]), true);
            }
        }

        // The pick made last stays the one the keyboard moves from.
        if (Head >= 0)
        {
            List.Get<Items>()->Selected = Head;
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Grid::Widen(Element Owner)
    {
        Ref<Tree>         Owned   = Owner.GetTree();
        const Ptr<Ledger> Own     = Owner.Get<Ledger>();
        const Element     Header(Owned, Own->Header);
        const Element     List(Owned, Own->List);
        const Element     Content = Header.GetParent();

        if (!Content.IsAlive() || Owned.IsPending(Content.GetHandle()))
        {
            return false;
        }

        // The room is the viewer's, less the gutter the rows keep for their thumb.
        ConstRef<Edges>        Padding = Owned.GetBox(Content.GetHandle()).Padding;
        const ConstPtr<Scroll> Roll    = List.Get<Scroll>();
        const Real32           Gutter  = Roll ? Roll->Gutter : 0.0f;
        const Real32           Room    = Max(Content.GetFrame().GetWidth() - Padding.Left - Padding.Right - Gutter,
                                             0.0f);

        Real32 Kept   = 0.0f;
        Real32 Shares = 0.0f;

        for (ConstRef<Column> Entry : Own->Columns)
        {
            Kept   += Entry.Width > 0.0f ? Max(Entry.Width, Entry.Minimum) : 0.0f;
            Shares += Entry.Width > 0.0f ? 0.0f : Entry.Share;
        }

        const Real32 Left  = Max(Room - Kept, 0.0f);
        const UInt   Count = Own->Columns.GetSize();
        Bool         Same  = Own->Widths.GetSize() == Count;

        // Weighed against the widths kept first, so a grid whose room holds still makes nothing.
        for (UInt Index = 0; Same && Index < Count; ++Index)
        {
            Same = Abs(Measure(Own->Columns[Index], Left, Shares) - Own->Widths[Index]) < 0.5f;
        }

        if (Same)
        {
            return false;
        }

        ConstRef<Sequence<Real32>> Widths = Own->Widths;
        Real32                     Total  = 0.0f;
        Own->Widths.Clear();

        for (ConstRef<Column> Entry : Own->Columns)
        {
            Own->Widths.Append(Measure(Entry, Left, Shares));
            Total += Widths.GetBack();
        }

        // The headings and the rows stand as wide as the columns together, the gutter above the thumb kept free.
        Header.EditBox().Width = Length::Pixels(Total + Gutter);
        List.EditBox().Width   = Length::Pixels(Total + Gutter);

        UInt Index = 0;

        for (Element Heading = Header.GetFirst(); Heading.IsAlive() && Index < Count; Heading = Heading.GetNext())
        {
            Heading.EditBox().Width = Length::Pixels(Widths[Index++]);
        }

        // Copied, since a row's cells are edited one by one.
        const Sequence<Cell> Cells = List.Get<Items>()->Cells;

        for (ConstRef<Cell> Entry : Cells)
        {
            Fit(Element(Owned, Entry.Row), Widths);
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Grid::Measure(ConstRef<Column> Entry, Real32 Left, Real32 Shares)
    {
        const Real32 Share = Shares > 0.0f ? Left * Entry.Share / Shares : 0.0f;
        return Floor(Entry.Width > 0.0f ? Max(Entry.Width, Entry.Minimum) : Max(Share, Entry.Minimum));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Grid::Fit(Element Row, ConstRef<Sequence<Real32>> Widths)
    {
        UInt Index = 0;

        for (Element Cell = Row.GetFirst(); Cell.IsAlive() && Index < Widths.GetSize(); Cell = Cell.GetNext(), ++Index)
        {
            // A cell already at its width is left alone, so it is not laid out again.
            const Length Width = Length::Pixels(Widths[Index]);

            if (Row.GetTree().GetBox(Cell.GetHandle()).Width != Width)
            {
                Cell.EditBox().Width = Width;
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Grid::Mark(Element Owner)
    {
        const ConstPtr<Ledger> Own   = Owner.Get<Ledger>();
        UInt32                 Index = 0;

        if (!Own)
        {
            return;
        }

        const Element Strip(Owner.GetTree(), Own->Header);

        for (Element Heading = Strip.GetFirst(); Heading.IsAlive(); Heading = Heading.GetNext(), ++Index)
        {
            // The mark follows the title, the second part of a heading.
            const Element Sign = Heading.GetFirst().GetNext();

            if (Sign.IsAlive())
            {
                Sign.SetState(State::Hidden, static_cast<SInt32>(Index) != Own->Sorted);
                Sign.SetState(Widget::Checked(), Own->Descending);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Grid::Toggle(Ref<Interface> Owner, Element Root, UInt32 Index)
    {
        const ConstPtr<Ledger> Own = Root.Get<Ledger>();

        if (!Own || Index >= Own->Columns.GetSize() || !Own->Columns[Index].Sortable)
        {
            return;
        }

        const Bool Again = Own->Sorted == static_cast<SInt32>(Index);
        SetSort(Root, static_cast<SInt32>(Index), Again && !Own->Descending);
        Owner.Dispatch(Event(kSort, Root.GetHandle()));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    SInt32 Grid::Weigh(ConstRef<Model> First, ConstRef<Model> Second, Text Key)
    {
        const ConstPtr<Datum> One = First.Find(Key);
        const ConstPtr<Datum> Two = Second.Find(Key);

        // A record lacking the value sorts first.
        if (!One || !Two)
        {
            return (One ? 1 : 0) - (Two ? 1 : 0);
        }

        const Bool Numeric =
               (One->GetForm() == Form::Number || One->GetForm() == Form::Bool)
            && (Two->GetForm() == Form::Number || Two->GetForm() == Form::Bool);

        if (Numeric)
        {
            const Real64 Left  = One->GetNumber();
            const Real64 Right = Two->GetNumber();
            return static_cast<SInt32>(Sign(Left - Right));
        }
        return StrCompareCaseInsensitive(One->Print(), Two->Print());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Grid::Precedes(ConstRef<Sequence<Ptr<Model>>> Records, ConstRef<Rule> Order, UInt32 First, UInt32 Second)
    {
        ConstRef<Model> Former = * Records[First];
        ConstRef<Model> Latter = * Records[Second];

        // The group comes first and keeps its way whichever way the column runs, as folders before files.
        if (!Order.Group.IsEmpty())
        {
            const SInt32 Grouped = Weigh(Former, Latter, Order.Group);

            if (Grouped != 0)
            {
                return Order.GroupDown ? Grouped > 0 : Grouped < 0;
            }
        }

        if (Order.Key.IsEmpty())
        {
            return First < Second;
        }

        const SInt32 Ranked = Order.Code.IsEmpty() ? Weigh(Former, Latter, Order.Key)
                                                   : Order.Code(Former, Latter, Order.Key);
        const SInt32 Signed = Order.Descending ? -Ranked : Ranked;
        return Signed != 0 ? Signed < 0 : First < Second;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Grid::Seek(Element Inside)
    {
        Element Current = Inside;

        while (Current.IsAlive() && !Current.Has<Ledger>())
        {
            Current = Current.GetParent();
        }
        return Current;
    }
}