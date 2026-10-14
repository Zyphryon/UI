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

#include "Repeater.hpp"
#include "Common.hpp"
#include "Memory.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Facet/Layout/Virtual.hpp"
#include "UI.Core/Facet/Visual/Scroll.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/States.hpp"
#include "UI.Core/Tree.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Binder.hpp"
#include "UI.Data/Datum.hpp"
#include "UI.Data/Facet/Binding.hpp"
#include "UI.Data/Link.hpp"
#include "UI.Data/Model.hpp"
#include "UI.Data/Path.hpp"
#include "UI.Gesture/Drag.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Repeater::Create(Element Parent, Text Style)
    {
        const Element Result = Parent.Create();

        Widget::Govern(Result, Handler::Create<&Repeater::OnEvent>(), true);
        Result.SetStyle(Style);
        Result.Add<Scroll>();
        Result.EditBox().Height = Length::Pixels(kHeight);

        // The spacer flows alone, as tall as every row together, so the scroll ranges over rows never made.
        const Element Spacer = Result.Create();

        Ref<Items> Own = Result.Add<Items>();
        Own.Spacer = Spacer.GetHandle();
        Own.Style  = Style;

        Result.Add<Virtual>().Realize = Realizer::Create<&Repeater::OnRealize>();
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Repeater::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Element Result = Create(Parent, Markup::GetText(Node, "Style", "Repeater"));

        SetColumns(Result, static_cast<UInt32>(Max(Markup::GetNumber(Node, "Columns", 0.0f), 0.0f)));
        SetItemHeight(Result, Markup::GetNumber(Node, "ItemHeight", 0.0f));
        SetItemWidth(Result, Markup::GetNumber(Node, "ItemWidth", 0.0f));
        SetMultiple(Result, Markup::GetBool(Node, "Multiple"));
        BuildRows(Reader, Result, Node);

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
            LOG_W("An item list's 'ItemTemplate' must name a template or be a node, so its rows are left empty");
        }

        // The records are one list in braces, read from the scope the list stands in.
        if (const Text Source = Markup::GetText(Node, "Items"); !Source.IsEmpty())
        {
            if (Text Inside; Unwrap(Source, Inside))
            {
                SetItems(Result, Inside, Reader.GetScope());
            }
            else
            {
                LOG_W("An item list's 'Items' must name one list alone in braces, which '{0}' does not", Source);
            }
        }

        SetIndex(Result, static_cast<SInt32>(Markup::GetNumber(Node, "Selected", -1.0f)));
        Reader.Bind(Result, Node, "Selected",
            Binder::Put<SInt32, &Repeater::SetIndex>(), Binder::Take<SInt32, &Repeater::GetIndex>());
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Repeater::SetItems(Element List, Text Source, Text Scope)
    {
        const Ptr<Items> Own = List.Get<Items>();

        if (!Own || Source.IsEmpty())
        {
            return false;
        }

        // The list keeps one binding of its records, so a new one takes the old one's place.
        if (const Ptr<Binding> Found = List.Get<Binding>())
        {
            Found->Links.RemoveSomeIf([](ConstRef<Link> Entry)
            {
                return Entry.Write.IsBoundTo<&Repeater::OnItems>();
            });
        }

        // The text is kept to name each row's record, as the bindings a template makes are read from text.
        Own->Base  = Resolve(Source, Scope);
        Own->Count = 0;
        Own->Named = false;
        Own->Records.Clear();
        Own->Paths.Clear();

        // Destroying the rows may move this facet, so nothing of it is read past here.
        Reset(List);

        const Str Bound = Str::Print<"{{{0}}}">(Source);
        return Binder::Attach(List, Bound, Writer::Create<&Repeater::OnItems>(), Reader(), Scope);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::SetTemplate(Element List, Maker Make)
    {
        if (const Ptr<Items> Own = List.Get<Items>())
        {
            Own->Make   = Move(Make);
            Own->Reader = nullptr;
            Own->Node   = Unique<JsonValue>();
            Reset(List);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::SetTemplate(Element List, Ref<Markup> Reader, Text Template)
    {
        if (const Ptr<Items> Own = List.Get<Items>())
        {
            Own->Make     = Maker();
            Own->Reader   = AddressOf(Reader);
            Own->Template = Template;
            Own->Node     = Unique<JsonValue>();
            Reset(List);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::SetTemplate(Element List, Ref<Markup> Reader, ConstRef<JsonValue> Node)
    {
        if (const Ptr<Items> Own = List.Get<Items>())
        {
            Own->Make   = Maker();
            Own->Reader = AddressOf(Reader);
            Own->Template.Clear();
            Own->Node   = Unique<JsonValue>::Create(Node);
            Reset(List);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::SetItemHeight(Element List, Real32 Height)
    {
        if (const Ptr<Items> Own = List.Get<Items>())
        {
            Own->ItemHeight = Max(Height, 0.0f);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::SetItemWidth(Element List, Real32 Width)
    {
        if (const Ptr<Items> Own = List.Get<Items>())
        {
            Own->ItemWidth = Max(Width, 0.0f);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::SetColumns(Element List, UInt32 Count)
    {
        if (const Ptr<Items> Own = List.Get<Items>())
        {
            Own->Columns = Count;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::SetIndex(Element List, SInt32 Index)
    {
        const Ptr<Items> Own = List.Get<Items>();

        if (!Own)
        {
            return;
        }

        // Kept even past the records the model holds now, so a pick made before the list fills is not lost.
        Own->Selected = Max(Index, -1);
        Own->Anchor   = Own->Selected;
        Own->Picked.Clear();

        if (Own->Selected >= 0)
        {
            Own->Picked.Append(static_cast<UInt32>(Own->Selected));
        }
        Mark(List);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    SInt32 Repeater::GetIndex(Element List)
    {
        const ConstPtr<Items> Own = List.Get<Items>();
        return Own && Own->Selected >= 0 && static_cast<UInt32>(Own->Selected) < Own->Count ? Own->Selected : -1;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Repeater::GetCount(Element List)
    {
        const ConstPtr<Items> Own = List.Get<Items>();
        return Own ? Own->Count : 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Repeater::GetRealized(Element List)
    {
        const ConstPtr<Items> Own = List.Get<Items>();
        return Own ? Accumulate(Own->Cells.GetData(), Own->Cells.GetSize(), 0u, [](ConstRef<Cell> Entry)
        {
            return Entry.Parked ? 0u : 1u;
        }) : 0u;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Repeater::GetRow(Element List, UInt32 Record)
    {
        if (const ConstPtr<Items> Own = List.Get<Items>())
        {
            const SInt Found = Own->Cells.Find([Record](ConstRef<Cell> Entry)
            {
                return Entry.Record == Record && !Entry.Parked;
            });

            if (Found >= 0)
            {
                return Element(List.GetTree(), Own->Cells[static_cast<UInt>(Found)].Row);
            }
        }
        return Element();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    SInt32 Repeater::GetRecord(Element List, Element Row)
    {
        const ConstPtr<Items> Own     = List.Get<Items>();
        Element               Current = Row;

        // A part of a row climbs to the row, the child of the list.
        while (Current.IsAlive() && Current.GetParent() != List)
        {
            Current = Current.GetParent();
        }

        if (!Own || !Current.IsAlive())
        {
            return -1;
        }

        const SInt Found = Own->Cells.Find([Target = Current.GetHandle()](ConstRef<Cell> Entry)
        {
            return Entry.Row == Target && !Entry.Parked;
        });
        return Found >= 0 ? static_cast<SInt32>(Own->Cells[static_cast<UInt>(Found)].Record) : -1;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt64 Repeater::GetKey(Element List, Element Row)
    {
        const SInt32          Record = GetRecord(List, Row);
        const ConstPtr<Items> Own    = List.Get<Items>();
        const ConstPtr<Model> Data   = List.IsAlive() ? Binder::GetModel(List.GetTree()) : nullptr;

        return Record >= 0 && Own && Data ? Data->GetKey(Own->Base, static_cast<UInt32>(Record)) : 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::ScrollTo(Element List, UInt32 Record)
    {
        const ConstPtr<Items>  Own  = List.Get<Items>();
        const ConstPtr<Scroll> Roll = List.Get<Scroll>();

        // Before the first layout nothing is known of where a record stands.
        if (!Own || !Roll || Own->Stride <= 0.0f)
        {
            return;
        }

        const Real32 Pitch  = Own->ItemHeight > 0.0f ? Own->ItemHeight : Own->Measured;
        const Real32 Top    = static_cast<Real32>(Record / Max(Own->Across, 1u)) * Own->Stride;
        const Real32 Bottom = Top + Pitch;
        Real32       Offset = Roll->Offset;

        if (Top < Offset)
        {
            Offset = Top;
        }
        else if (Bottom > Offset + Own->View)
        {
            Offset = Bottom - Own->View;
        }

        // Edited only when it moves, since an edit lays the pane out again.
        if (Offset != Roll->Offset)
        {
            const Ptr<Scroll> Moved = List.Edit<Scroll>();
            Moved->Offset = Offset;
            Moved->Follow = false;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::SetRecords(Element List, Sequence<Str> Records)
    {
        const Ptr<Items> Own = List.Get<Items>();

        if (!Own)
        {
            return;
        }

        Sequence<Path> Paths;
        Paths.Advance(Records.GetSize());

        for (UInt Index = 0; Index < Records.GetSize(); ++Index)
        {
            Binder::Locate(Records[Index], Paths[Index]);
        }

        Own->Named   = true;
        Own->Count   = static_cast<UInt32>(Records.GetSize());
        Own->Records = Move(Records);
        Own->Paths   = Move(Paths);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str Repeater::GetScope(Element List, UInt32 Record)
    {
        const ConstPtr<Items> Own = List.Get<Items>();

        if (!Own || Record >= Own->Count)
        {
            return Str();
        }
        return Own->Named ? Own->Records[Record] : Str::Print<"{0}[{1}]">(Text(Own->Base), Record);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::SetMultiple(Element List, Bool Enabled)
    {
        if (const Ptr<Items> Own = List.Get<Items>())
        {
            Own->Multiple = Enabled;
        }

        // One at a time again keeps only the record at the pick.
        if (!Enabled)
        {
            SetIndex(List, GetIndex(List));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Repeater::IsMultiple(Element List)
    {
        const ConstPtr<Items> Own = List.Get<Items>();
        return Own && Own->Multiple;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::SetPicked(Element List, UInt32 Record, Bool Enabled)
    {
        const Ptr<Items> Own = List.Get<Items>();

        if (!Own)
        {
            return;
        }

        // A list picking one at a time picks by moving its one pick.
        if (!Own->Multiple)
        {
            if (Enabled || GetIndex(List) == static_cast<SInt32>(Record))
            {
                SetIndex(List, Enabled ? static_cast<SInt32>(Record) : -1);
            }
            return;
        }

        Ref<Sequence<UInt32>> Picked = Own->Picked;
        const SInt            Found  = Picked.Find([Record](UInt32 Value)
        {
            return Value >= Record;
        });
        const UInt            Spot   = Found < 0 ? Picked.GetSize() : static_cast<UInt>(Found);

        const Bool Held = Spot < Picked.GetSize() && Picked[Spot] == Record;

        if (Enabled && !Held)
        {
            Picked.Insert(Spot, Record);
            Own->Selected = static_cast<SInt32>(Record);
        }
        else if (!Enabled && Held)
        {
            Picked.Remove(Spot);

            if (Own->Selected == static_cast<SInt32>(Record))
            {
                Own->Selected = Picked.IsEmpty() ? -1 : static_cast<SInt32>(Picked[Min(Spot, Picked.GetSize() - 1)]);
            }
        }
        Mark(List);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Repeater::IsPicked(Element List, UInt32 Record)
    {
        const ConstPtr<Items> Own = List.Get<Items>();
        return Own && Record < Own->Count && Own->Picked.Contains(Record);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Sequence<UInt32> Repeater::GetPicked(Element List)
    {
        const ConstPtr<Items> Own = List.Get<Items>();
        Sequence<UInt32>      Result;

        for (UInt Index = 0; Own && Index < Own->Picked.GetSize(); ++Index)
        {
            if (Own->Picked[Index] < Own->Count)
            {
                Result.Append(Own->Picked[Index]);
            }
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::SetRightPick(Element List, Bool Enabled)
    {
        if (const Ptr<Items> Own = List.Get<Items>())
        {
            Own->RightPick = Enabled;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::AddRowState(Element List, State Bit, Text Key)
    {
        const Ptr<Items> Own = List.Get<Items>();

        if (!Own || Key.IsEmpty())
        {
            return;
        }

        // A key opening with `!` stays in the braces, so the row wears the state while the value reads as clear.
        Own->RowStates.Append(Bit, Str::Print<"{{{0}}}">(Key));
        Reset(List);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::BuildRows(Ref<Markup> Reader, Element List, ConstRef<JsonValue> Node)
    {
        SetRightPick(List, Markup::GetBool(Node, "RightPick", true));

        if (const ConstPtr<JsonValue> Marks = Markup::GetField(Node, "RowStates"); Marks && Marks->IsObject())
        {
            Marks->GetObject().ForEach([List](ConstRef<Str> Name, ConstRef<JsonValue> Value)
            {
                if (Value.IsString())
                {
                    AddRowState(List, States::Declare(Name), Value.GetString());
                }
                else
                {
                    LOG_W("The row state '{0}' must name a value of the record, so the rows never wear it", Name);
                }
            });
        }
        else if (Marks)
        {
            LOG_W("A list's 'RowStates' must map states to values of the record, so its rows wear none");
        }

        if (const Str Key = Memory::GetKey(Reader, Node); !Key.IsEmpty())
        {
            Remember(List, Key);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::Remember(Element List, Text Key)
    {
        if (List.Has<Items>())
        {
            Memory::Hold(List, Key);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    State Repeater::Marked()
    {
        static const State kBit = States::Declare("Marked");
        return kBit;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Repeater::Unwrap(Text Source, Ref<Text> Output)
    {
        if (Source.GetSize() < 3 || Source.GetFront() != '{' || Source.GetBack() != '}')
        {
            return false;
        }

        const Text Inside = Source.Slice(1, Source.GetSize() - 2);

        // A format, a negation or a second value makes it text to write out, not a list to show.
        if (StrFindAny(Inside, "{}:") >= 0 || Inside.GetFront() == '!')
        {
            return false;
        }

        Output = Inside;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str Repeater::Resolve(Text Source, Text Scope)
    {
        if (StrStartsWith(Source, "/"))
        {
            return Str(Source.Slice(1));
        }
        return Scope.IsEmpty() ? Str(Source) : Str::Join(Scope, '.', Source);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::Stamp(Ref<Markup> Reader, Element Parent, Text Scope, ConstPtr<JsonValue> Node, Text Template)
    {
        Reader.PushScope(Str::Print<"/{0}">(Scope));

        if (Node)
        {
            Reader.Build(Parent, * Node);
        }
        else
        {
            Reader.Spawn(Parent, Template);
        }
        Reader.PopScope();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Repeater::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        const Element List(Owner, Self);

        if (Message.Kind == Signal::kKey && Message.Target == Self)
        {
            return OnKey(Owner, List, Message.Key);
        }

        // A right press in a row picks it and goes on, so the row's or the list's menu opens over the pick.
        if (Message.Kind == Signal::kPress && Message.Button == ZyInput::Button::Right)
        {
            const ConstPtr<Items> Own    = List.Get<Items>();
            const SInt32          Record = Own && Own->RightPick ? Seat(List, Message.Target) : -1;

            if (Record >= 0 && !IsPicked(List, static_cast<UInt32>(Record)))
            {
                Gather(Owner, List, Record, false, false);
            }
            return false;
        }

        // A second press on a row itself activates its record, the first having picked it.
        const Bool Row   = Message.Target != Self && Owner.GetParent(Message.Target) == Self;
        const Bool Twice = Message.Kind == Signal::kPress && Message.Button == ZyInput::Button::Left
            && Message.Count == 2;

        if (Twice && Row)
        {
            const SInt32 Record = GetRecord(List, Element(Owner, Message.Target));
            return Record >= 0 && Announce(Owner, List, static_cast<UInt32>(Record), Signal::kActivate);
        }

        // Only a click on a row itself picks; one on a control the row holds belongs to that control.
        if (Message.Kind != Signal::kClick || Message.Target == Self || Owner.GetParent(Message.Target) != Self)
        {
            return false;
        }

        const SInt32 Record = GetRecord(List, Element(Owner, Message.Target));

        if (Record < 0)
        {
            return false;
        }

        const ZyInput::Modifier Keys = Owner.GetModifiers();
        Gather(Owner, List, Record, HasBit(Keys, ZyInput::Modifier::Control), HasBit(Keys, ZyInput::Modifier::Shift));
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Repeater::Announce(Ref<Interface> Owner, Element List, UInt32 Record, UInt64 Kind)
    {
        // Kept here, since the event only points at the text it names.
        const Str Scope = GetScope(List, Record);

        Event Message(Kind, List.GetHandle());
        Message.Content = Scope;
        Message.Other   = GetRow(List, Record).GetHandle();
        return Owner.Dispatch(Message);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    SInt32 Repeater::Seat(Element List, Handle Target)
    {
        ConstRef<Tree> Owner   = List.GetTree();
        Handle         Current = Target;

        // A list met on the way up to the row holds the element in a row of its own, which that list answers for.
        while (Owner.IsAlive(Current) && Owner.GetParent(Current) != List.GetHandle())
        {
            if (Owner.Has<Items>(Current))
            {
                return -1;
            }
            Current = Owner.GetParent(Current);
        }
        return Owner.IsAlive(Current) ? GetRecord(List, Element(List.GetTree(), Current)) : -1;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Repeater::OnKey(Ref<Interface> Owner, Element List, ZyInput::Key Key)
    {
        const ConstPtr<Items> Own = List.Get<Items>();

        if (!Own || Own->Count == 0)
        {
            return false;
        }

        // Left to bubble on when no one answers, so enter still reaches a dialog's default button.
        if (Key == ZyInput::Key::Enter || Key == ZyInput::Key::F2)
        {
            const SInt32 Current = GetIndex(List);
            const UInt64 Kind    = Key == ZyInput::Key::Enter ? Signal::kActivate : kRename;
            return Current >= 0 && Announce(Owner, List, static_cast<UInt32>(Current), Kind);
        }

        if (Key == ZyInput::Key::A && HasBit(Owner.GetModifiers(), ZyInput::Modifier::Control) && Own->Multiple)
        {
            const UInt Before = Own->Picked.GetSize();

            for (UInt32 Record = 0; Record < Own->Count; ++Record)
            {
                SetPicked(List, Record, true);
            }

            // Picking only ever adds here, so a pick grown by any record changed.
            if (List.Get<Items>()->Picked.GetSize() != Before)
            {
                Owner.Dispatch(Event(Signal::kChange, List.GetHandle()));
            }
            return true;
        }

        const SInt32 Count   = static_cast<SInt32>(Own->Count);
        const SInt32 Across  = static_cast<SInt32>(Max(Own->Across, 1u));
        const SInt32 Current = GetIndex(List);
        const SInt32 Page    = Own->Stride > 0.0f
            ? Max(static_cast<SInt32>(Own->View / Own->Stride), 1) * Across : Across;
        SInt32       Next    = Current;

        switch (Key)
        {
        case ZyInput::Key::Up:
            Next = Current >= Across ? Current - Across : Current;
            break;
        case ZyInput::Key::Down:
            Next = Current + Across < Count ? Current + Across : Current;
            break;
        case ZyInput::Key::Left:
            Next = Across > 1 && Current > 0 ? Current - 1 : Current;
            break;
        case ZyInput::Key::Right:
            Next = Across > 1 && Current + 1 < Count ? Current + 1 : Current;
            break;
        case ZyInput::Key::PageUp:
            Next = Current >= Page ? Current - Page : Current % Across;
            break;
        case ZyInput::Key::PageDown:
            Next = Min(Current + Page, Count - 1);
            break;
        case ZyInput::Key::Home:
            Next = 0;
            break;
        case ZyInput::Key::End:
            Next = Count - 1;
            break;
        default:
            return false;
        }

        // With nothing picked yet, every key but end picks the first record.
        if (Current < 0 && Key != ZyInput::Key::End)
        {
            Next = 0;
        }

        // An arrow that would run past an edge, or sideways in one column, is left alone to move the keyboard on.
        const Bool Arrow = Key == ZyInput::Key::Up || Key == ZyInput::Key::Down || Key == ZyInput::Key::Left
            || Key == ZyInput::Key::Right;

        if (Arrow && Next == Current)
        {
            return false;
        }

        Gather(Owner, List, Next, false, HasBit(Owner.GetModifiers(), ZyInput::Modifier::Shift));
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Repeater::OnRealize(Ref<Interface> Owner, Handle Self)
    {
        const Element List(Owner, Self);

        if (!List.Has<Items>())
        {
            return false;
        }

        // A remembered list scrolls back first, so the rows made next are those of where it was left.
        Bool Changed = List.Has<Recall>() && Memory::Track(List);

        Measure(List);

        const Grid   Layout = Survey(List);
        const UInt32 Count  = List.Get<Items>()->Count;
        const UInt32 Lines  = (Count + Layout.Columns - 1) / Layout.Columns;

        // Until a row was measured only the first is made, to be measured; then the lines in view and a margin.
        UInt32 Begin = 0;
        UInt32 End   = Min(Count, 1u);

        if (Layout.Pitch > 0.0f)
        {
            const Real32 Stride   = Layout.Pitch + Layout.Gap;
            const Real32 Scrolled = Max(Layout.Offset, 0.0f);
            const UInt32 First    = static_cast<UInt32>(Scrolled / Stride);
            const UInt32 Last     = static_cast<UInt32>(Ceil((Scrolled + Layout.View) / Stride));

            Begin = (Max(First, kMargin) - kMargin) * Layout.Columns;
            End   = Min(Min(Last + kMargin, Lines) * Layout.Columns, Count);
        }

        // Scrolled past the last line, the range is empty rather than reversed.
        const UInt32 Window = Max(End, Begin) - Begin;

        // The spacer stands as tall as every row together, so the scroll ranges over the whole list.
        const Real32  Tall = (Layout.Pitch > 0.0f && Lines > 0)
            ? static_cast<Real32>(Lines) * Layout.Pitch + static_cast<Real32>(Lines - 1) * Layout.Gap
            : 0.0f;
        const Element Spacer(Owner, List.Get<Items>()->Spacer);

        if (Spacer.IsAlive() && !Same(Owner.GetBox(Spacer.GetHandle()).Height, Length::Pixels(Tall)))
        {
            Spacer.EditBox().Height = Length::Pixels(Tall);
            Changed = true;
        }

        // Parked and out-of-range rows are free unless carried or held down; the lists are kept between calls.
        thread_local Sequence<UInt> Free;
        thread_local Sequence<Bool> Shown;

        Free.Clear();
        Shown.Clear();
        Shown.Advance(Window);

        {
            ConstRef<Items>          Own     = * List.Get<Items>();
            ConstRef<Sequence<Cell>> Cells   = Own.Cells;
            const Handle             Carried = Drag::GetDragged(Owner);
            const Handle             Pressed = Owner.GetPressed();

            for (UInt Index = 0; Index < Cells.GetSize(); ++Index)
            {
                const UInt32 Record = Cells[Index].Record;
                const Handle Row    = Cells[Index].Row;

                // A row of records named one by one still shows its record only while that record keeps its place.
                const Bool Kept = !Own.Named
                    || (Record < Own.Paths.GetSize() && Same(Cells[Index].Within, Own.Paths[Record]));

                if (!Cells[Index].Parked && Kept && Record >= Begin && Record < End && !Shown[Record - Begin])
                {
                    Shown[Record - Begin] = true;
                }
                else if (Cells[Index].Parked || !(Holds(Owner, Row, Carried) || Holds(Owner, Row, Pressed)))
                {
                    Free.Append(Index);
                }
            }
        }

        for (UInt32 Record = Begin; Record < End; ++Record)
        {
            if (Shown[Record - Begin])
            {
                continue;
            }

            if (Free.IsEmpty())
            {
                Make(List, Record);
            }
            else
            {
                Rebind(List, Free.GetBack(), Record);
                Free.RemoveLast();
            }
            Changed = true;
        }

        // A view never needs more rows than the lines it can show, part of one at each edge, and the margins.
        UInt Limit = Window;

        if (Layout.Pitch > 0.0f)
        {
            const UInt32 Seen = static_cast<UInt32>(Ceil(Layout.View / (Layout.Pitch + Layout.Gap))) + 1 + 2 * kMargin;
            Limit = Max(Limit, static_cast<UInt>(Seen * Layout.Columns));
        }

        Sequence<Handle> Gone;
        Changed = Park(List, Free, Limit, Gone) || Changed;

        // Placing a row only moves it, so the cells stay as they are while they are read.
        ConstRef<Sequence<Cell>> Placing = List.Get<Items>()->Cells;

        for (ConstRef<Cell> Entry : Placing)
        {
            Changed = Place(List, Entry, Layout) || Changed;
        }

        for (const Handle Row : Gone)
        {
            if (Owner.IsAlive(Row))
            {
                Owner.Destroy(Row);
            }
        }
        return Changed;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::OnItems(Element Target, ConstRef<Datum> Value)
    {
        // A list of records named one by one counts its names instead.
        if (const Ptr<Items> Own = Target.Get<Items>(); Own && !Own->Named)
        {
            Own->Count = Value.GetForm() == Form::List ? static_cast<UInt32>(Value.GetNumber()) : 0;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::Choose(Ref<Interface> Owner, Element List, SInt32 Index)
    {
        ScrollTo(List, static_cast<UInt32>(Index));

        if (Index == GetIndex(List))
        {
            return;
        }

        SetIndex(List, Index);
        Owner.Dispatch(Event(Signal::kChange, List.GetHandle()));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::Gather(Ref<Interface> Owner, Element List, SInt32 Index, Bool Toggle, Bool Extend)
    {
        const Ptr<Items> Own = List.Get<Items>();

        // A plain pick leaves only its record, even one already selected among several others.
        if (!Own->Multiple || (!Toggle && !Extend))
        {
            if (Index == GetIndex(List) && Own->Picked.GetSize() > 1)
            {
                SetIndex(List, Index);
                Owner.Dispatch(Event(Signal::kChange, List.GetHandle()));
                return;
            }
            Choose(Owner, List, Index);
            return;
        }

        ScrollTo(List, static_cast<UInt32>(Index));

        const Sequence<UInt32> Before = Own->Picked;
        const SInt32           Was    = Own->Selected;

        if (Extend)
        {
            const SInt32 Anchor = Own->Anchor >= 0 ? Own->Anchor : (Own->Selected >= 0 ? Own->Selected : Index);
            const SInt32 First  = Min(Anchor, Index);
            const SInt32 Last   = Max(Anchor, Index);

            // A range picked with control held as well adds to the pick rather than replacing it.
            if (!Toggle)
            {
                Own->Picked.Clear();
            }

            for (SInt32 Record = First; Record <= Last; ++Record)
            {
                if (!Own->Picked.Contains(static_cast<UInt32>(Record)))
                {
                    Own->Picked.Append(static_cast<UInt32>(Record));
                }
            }

            Own->Picked.Sort();
            Own->Selected = Index;
            Own->Anchor   = Anchor;
            Mark(List);
        }
        else
        {
            SetPicked(List, static_cast<UInt32>(Index), !Own->Picked.Contains(static_cast<UInt32>(Index)));

            // The record clicked is where the next range starts, picked or let go.
            const Ptr<Items> Kept = List.Get<Items>();
            Kept->Anchor = Index;

            if (Kept->Picked.Contains(static_cast<UInt32>(Index)))
            {
                Kept->Selected = Index;
            }
        }

        const Ptr<Items> After = List.Get<Items>();
        const Bool       Moved = After->Selected != Was || After->Picked.GetSize() != Before.GetSize()
            || !Compare(After->Picked.GetData(), Before.GetData(), Before.GetSize());

        if (Moved)
        {
            Owner.Dispatch(Event(Signal::kChange, List.GetHandle()));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::Mark(Element List)
    {
        const ConstPtr<Items> Own = List.Get<Items>();

        for (UInt Index = 0; Own && Index < Own->Cells.GetSize(); ++Index)
        {
            ConstRef<Cell> Entry  = Own->Cells[Index];
            const Element  Row(List.GetTree(), Entry.Row);
            const Bool     Chosen = Own->Picked.Contains(Entry.Record);

            if (Row.IsAlive() && !Entry.Parked && Row.HasState(Widget::Selected()) != Chosen)
            {
                Row.SetState(Widget::Selected(), Chosen);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Repeater::Grid Repeater::Survey(Element List)
    {
        Ref<Tree>        Owner = List.GetTree();
        const Ptr<Items> Own   = List.Get<Items>();
        ConstRef<Box>    Shape = Owner.GetBox(List.GetHandle());
        const Rect       Frame = Owner.IsPending(List.GetHandle()) ? Rect::Zero() : List.GetFrame();
        const Element    Spacer(Owner, Own->Spacer);
        const Real32     Wide  = Spacer.IsAlive() && !Owner.IsPending(Own->Spacer)
            ? Spacer.GetFrame().GetWidth() : 0.0f;

        Grid Result;
        Result.Left   = Shape.Padding.Left;
        Result.Top    = Shape.Padding.Top;
        Result.Gap    = Shape.Gap;
        Result.Offset = List.Get<Scroll>() ? List.Get<Scroll>()->Offset : 0.0f;
        Result.View   = Max(Frame.GetHeight() - Shape.Padding.Top - Shape.Padding.Bottom, 0.0f);
        Result.Pitch  = Own->ItemHeight > 0.0f ? Own->ItemHeight : Own->Measured;

        // The spacer spans the room the scroll leaves beside its thumb; before it was laid out, the padded box does.
        const Real32 Room = Wide > 0.0f ? Wide : Max(Frame.GetWidth() - Shape.Padding.Left - Shape.Padding.Right, 0.0f);

        // A width given fits as many columns as the room holds, unless a count is given too.
        if (Own->Columns > 0)
        {
            Result.Columns = Own->Columns;
        }
        else if (Own->ItemWidth > 0.0f)
        {
            Result.Columns = Max(static_cast<UInt32>((Room + Result.Gap) / (Own->ItemWidth + Result.Gap)), 1u);
        }
        else
        {
            Result.Columns = 1;
        }

        const Real32 Across = static_cast<Real32>(Result.Columns);
        Result.Width = Own->ItemWidth > 0.0f
            ? Own->ItemWidth : Max((Room - Result.Gap * (Across - 1.0f)) / Across, 0.0f);

        // Kept for the keyboard and for scrolling a record into view, which run between layouts.
        Own->Across = Result.Columns;
        Own->Stride = Result.Pitch > 0.0f ? Result.Pitch + Result.Gap : 0.0f;
        Own->View   = Result.View;
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::Measure(Element List)
    {
        const Ptr<Items> Own   = List.Get<Items>();
        Ref<Tree>        Owner = List.GetTree();
        Element          Row;

        if (Own->ItemHeight > 0.0f)
        {
            return;
        }

        for (ConstRef<Cell> Entry : Own->Cells)
        {
            if (Entry.Record == 0 && !Entry.Parked)
            {
                Row = Element(Owner, Entry.Row);
            }
        }

        // A row is measured once it was laid out, which a row made this frame has not been yet.
        if (!Row.IsAlive() || Owner.IsPending(Row.GetHandle()) || Row.GetFrame().GetHeight() <= 0.0f)
        {
            return;
        }

        // What the row holds keeps its own height whatever height the row is given, so the row is measured by it.
        Rect Reach = Rect::Invalid();
        Bool Found = false;

        for (Element Child = Row.GetFirst(); Child.IsAlive(); Child = Child.GetNext())
        {
            ConstRef<Box> Shape = Owner.GetBox(Child.GetHandle());

            if (Child.HasState(State::Hidden) || Shape.Placement != Position::Flow)
            {
                continue;
            }

            const Rect   Frame  = Child.GetFrame();
            const Real32 Top    = Frame.GetMinimumY() - Shape.Margin.Top;
            const Real32 Bottom = Frame.GetMaximumY() + Shape.Margin.Bottom;
            Reach = Rect::Union(Reach, Rect(Frame.GetMinimumX(), Top, Frame.GetMaximumX(), Bottom));
            Found = true;
        }

        ConstRef<Edges> Padding = Owner.GetBox(Row.GetHandle()).Padding;
        const Real32    Natural = (Found ? Reach.GetHeight() : 0.0f) + Padding.Top + Padding.Bottom;

        if (Natural > 0.0f)
        {
            Own->Measured = Natural;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::Make(Element List, UInt32 Record)
    {
        const Element Row = List.Create();

        Widget::Govern(Row, Handler(), false);
        Row.SetStyle(Widget::Part(Text(List.Get<Items>()->Style), "Item"));

        // Placed by the list itself, what it holds sitting in the middle of whatever height it is given.
        Ref<Box> Shape = Row.EditBox();
        Shape.Placement = Position::Absolute;
        Shape.Content   = Justify::Center;

        Ref<Sequence<Cell>> Cells = List.Get<Items>()->Cells;
        const UInt          Index = Cells.GetSize();
        Cells.Append(Row.GetHandle(), Record, false);
        Fill(List, Row, Record);

        // Found again, since filling the row may have moved the facet; only the list's own cells keep their places.
        if (const Ptr<Items> Own = List.Get<Items>(); Own->Named && Record < Own->Paths.GetSize())
        {
            Own->Cells[Index].Within = Own->Paths[Record];
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::Fill(Element List, Element Row, UInt32 Record)
    {
        // Copied, since the code or the template may make another list and move this facet.
        const Ptr<Items>          Own      = List.Get<Items>();
        const Maker               Code     = Own->Make;
        const Ptr<Markup>         Reader   = Own->Reader;
        const Str                 Template = Own->Template;
        const Str                 Base     = Own->Base;
        const ConstPtr<JsonValue> Node     = Own->Node ? Own->Node.Grab() : nullptr;
        const Bool                Named    = Own->Named;
        const Str                 Scope    = GetScope(List, Record);
        const Sequence<RowState>  Marks    = Own->RowStates;

        if (!Code.IsEmpty())
        {
            Code(Row, Scope);
        }
        else if (Reader)
        {
            Stamp(* Reader, Row, Scope, Node, Template);
        }

        // Bound on the row itself, so they follow it to another record as its content does.
        for (ConstRef<RowState> Entry : Marks)
        {
            const State Bit = Entry.Bit;

            Binder::Attach(Row, Entry.Source, Writer::Create([Bit](Element Target, ConstRef<Datum> Value)
            {
                Target.SetState(Bit, Value.GetBool());
            }), UI::Reader(), Scope);
        }

        // A list in another list's row may now read another record, so the row's bindings are moved to where it reads.
        Path From;
        Path To;

        if (!Named && Path::Parse(Base, From) && Locate(List, Record, To))
        {
            From.Steps.GetBack().Index = Record;

            if (!Same(From, To))
            {
                Binder::Rescope(Row, From, To);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::Rebind(Element List, UInt Index, UInt32 Record)
    {
        const Ptr<Items> Own   = List.Get<Items>();
        const Cell       Entry = Own->Cells[Index];
        const Bool       Named = Own->Named;
        const Element    Row(List.GetTree(), Entry.Row);
        Path             From;
        Path             To;

        // A row of records named one by one reads the path it was handed last, which its place no longer tells.
        if (Named)
        {
            From = Entry.Within;
        }

        if ((Named || Locate(List, Entry.Record, From)) && Locate(List, Record, To) && !From.Steps.IsEmpty())
        {
            Binder::Rescope(Row, From, To);
        }

        if (Entry.Parked && Row.IsAlive())
        {
            Row.SetState(State::Hidden, false);
        }

        // Found again, since showing the new record's values may have moved the facet.
        Ref<Cell> Kept = List.Get<Items>()->Cells[Index];
        Kept.Record = Record;
        Kept.Parked = false;

        if (Named)
        {
            Kept.Within = Move(To);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Repeater::Park(Element List, ConstRef<Sequence<UInt>> Free, UInt Limit, Ref<Sequence<Handle>> Gone)
    {
        Ref<Sequence<Cell>> Cells   = List.Get<Items>()->Cells;
        UInt                Excess  = Max(Cells.GetSize(), Limit) - Limit;
        Bool                Changed = false;

        // Highest first, so taking a cell out leaves the indices still to come where they were.
        for (UInt Index = Free.GetSize(); Index-- > 0;)
        {
            const UInt Slot = Free[Index];

            if (Excess > 0)
            {
                Gone.Append(Cells[Slot].Row);
                Cells.Remove(Slot);
                --Excess;
                Changed = true;
            }
            else if (!Cells[Slot].Parked)
            {
                Cells[Slot].Parked = true;
                Element(List.GetTree(), Cells[Slot].Row).SetState(State::Hidden, true);
                Changed = true;
            }
        }
        return Changed;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Repeater::Place(Element List, ConstRef<Cell> Entry, ConstRef<Grid> Layout)
    {
        const Element Row(List.GetTree(), Entry.Row);

        if (Entry.Parked || !Row.IsAlive())
        {
            return false;
        }

        const Bool Chosen = List.Get<Items>()->Picked.Contains(Entry.Record);

        if (Row.HasState(Widget::Selected()) != Chosen)
        {
            Row.SetState(Widget::Selected(), Chosen);
        }

        // Rows stand over the spacer, each at its record's line and column, moved up by the scroll.
        const Real32 Line   = static_cast<Real32>(Entry.Record / Layout.Columns);
        const Real32 Column = static_cast<Real32>(Entry.Record % Layout.Columns);
        const Length Left   = Length::Pixels(Layout.Left + Column * (Layout.Width + Layout.Gap));
        const Length Top    = Length::Pixels(Layout.Top + Line * (Layout.Pitch + Layout.Gap) - Layout.Offset);
        const Length Width  = Length::Pixels(Layout.Width);
        const Length Height = Layout.Pitch > 0.0f ? Length::Pixels(Layout.Pitch) : Length::Auto();

        ConstRef<Box> Shape = List.GetTree().GetBox(Entry.Row);

        // A row already standing where it should is left alone, since an edit lays the pane out again.
        const Bool Placed = Same(Shape.Left, Left) && Same(Shape.Top, Top);

        if (Placed && Same(Shape.Width, Width) && Same(Shape.Height, Height))
        {
            return false;
        }

        Ref<Box> Edit = Row.EditBox();
        Edit.Left   = Left;
        Edit.Top    = Top;
        Edit.Width  = Width;
        Edit.Height = Height;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Repeater::Locate(Element List, UInt32 Record, Ref<Path> Output)
    {
        // Records named one by one are read where they were named.
        if (const ConstPtr<Items> Own = List.Get<Items>(); Own && Own->Named)
        {
            if (Record >= Own->Paths.GetSize())
            {
                return false;
            }

            Output = Own->Paths[Record];
            return true;
        }

        // The binding of the records is the list's own, the one showing them through the list's writer.
        const ConstPtr<Binding> Found = List.Get<Binding>();
        const SInt              Index = Found ? Found->Links.Find([](ConstRef<Link> Entry)
        {
            return Entry.Write.IsBoundTo<&Repeater::OnItems>() && Entry.Pieces.GetSize() == 1 && Entry.Pieces[0].Hole;
        }) : -1;

        if (Index < 0)
        {
            return false;
        }

        Output = Found->Links[static_cast<UInt>(Index)].Pieces[0].Where;
        Output.Steps.GetBack().Index = Record;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repeater::Reset(Element List)
    {
        const Ptr<Items> Own = List.Get<Items>();

        if (!Own)
        {
            return;
        }

        // The rows are let go of first, since destroying one holding a list of its own may move this facet.
        const Sequence<Cell> Gone = Move(Own->Cells);
        Own->Cells.Clear();

        for (ConstRef<Cell> Entry : Gone)
        {
            if (const Element Row(List.GetTree(), Entry.Row); Row.IsAlive())
            {
                Row.Destruct();
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Repeater::Same(ConstRef<Length> First, ConstRef<Length> Second)
    {
        return First.Measure == Second.Measure && Abs(First.Value - Second.Value) < 0.01f;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Repeater::Same(ConstRef<Path> First, ConstRef<Path> Second)
    {
        const UInt Size = First.Steps.GetSize();
        return Size == Second.Steps.GetSize() && Compare(First.Steps.GetData(), Second.Steps.GetData(), Size);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Repeater::Holds(ConstRef<Tree> Owner, Handle Row, Handle Element)
    {
        for (Handle Current = Element; Owner.IsAlive(Current); Current = Owner.GetParent(Current))
        {
            if (Current == Row)
            {
                return true;
            }
        }
        return false;
    }
}