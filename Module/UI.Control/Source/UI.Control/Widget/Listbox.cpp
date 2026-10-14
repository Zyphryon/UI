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

#include "Listbox.hpp"
#include "Common.hpp"
#include "Label.hpp"
#include "Panel.hpp"
#include "Repeater.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Visual/Caption.hpp"
#include "UI.Core/Facet/Visual/Scroll.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Binder.hpp"
#include "UI.Data/Datum.hpp"
#include "UI.Data/Facet/Binding.hpp"
#include "UI.Data/Link.hpp"
#include "UI.Data/Path.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Listbox::Create(Element Parent, Text Style)
    {
        const Element Result = Parent.Create();

        Widget::Govern(Result, Handler::Create<&Listbox::OnEvent>(), true);
        Result.SetStyle(Style);
        Result.Add<Scroll>();
        Result.Add<Choice>();
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Listbox::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Element Result = Create(Parent, Markup::GetText(Node, "Style", "Listbox"));
        Fill(Reader, Result, Node);

        Reader.Bind(Result, Node, "Selected",
            Binder::Put<SInt32, &Listbox::SetIndex>(), Binder::Take<SInt32, &Listbox::GetIndex>());
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Listbox::Fill(Ref<Markup> Reader, Element Owner, ConstRef<JsonValue> Node)
    {
        const ConstPtr<JsonValue> Items  = Markup::GetField(Node, "Items");
        const Real32              Picked = Markup::GetNumber(Node, "Selected", -1.0f);

        // A list of the model in braces makes an item for each of its records, filtered before any is made.
        if (Items && Items->IsString())
        {
            if (Text Inside; Repeater::Unwrap(Items->GetString(), Inside))
            {
                SetFilter(Owner, Phrase(Markup::GetText(Node, "ItemHidden")),
                          Phrase(Markup::GetText(Node, "ItemDisabled")));
                SetItems(Owner, Inside, Phrase(Markup::GetText(Node, "ItemText", "Name")), Reader.GetScope());
                SetIndex(Owner, static_cast<SInt32>(Picked));
            }
            else
            {
                LOG_W("A list's 'Items' must list texts or name one list of the model in braces, which '{0}' does not",
                    Items->GetString());
            }
            return;
        }

        if (!Items || !Items->IsArray())
        {
            return;
        }

        ConstRef<JsonValue::Array> List = Items->GetArray();

        for (UInt Index = 0; Index < List.GetSize(); ++Index)
        {
            const Text    Words = List[Index].IsString() ? List[Index].GetString() : Text();
            const Element Item  = AddItem(Owner, Words);

            // An item written `@Key` is read from the lexicon, again whenever the language changes.
            if (Binder::IsLocalized(Words))
            {
                Binder::Attach(Item.GetFirst(), Words, Binder::PutText<&Label::SetText>());
            }

            if (static_cast<Real32>(Index) == Picked)
            {
                Select(Owner, Item);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Listbox::SetItems(Element Owner, Text Source, Text Content, Text Scope)
    {
        if (Source.IsEmpty())
        {
            return false;
        }

        // The list keeps one binding of its records, so a new one takes the old one's place.
        if (const Ptr<Binding> Found = Owner.Get<Binding>())
        {
            Found->Links.RemoveSomeIf([](ConstRef<Link> Entry)
            {
                return Entry.Write.IsBoundTo<&Listbox::OnItems>();
            });
        }

        // The text is kept to name each item's record, as the bindings an item makes are read from text.
        Ref<Listing> Feed = Owner.Obtain<Listing>();
        Feed.Source  = Repeater::Resolve(Source, Scope);
        Feed.Content = Content;
        Feed.Count   = 0;

        // The items it held go before the records are counted, which makes theirs at once.
        Remake(Owner);

        const Str Bound = Str::Print<"{{{0}}}">(Source);
        return Binder::Attach(Owner, Bound, Writer::Create<&Listbox::OnItems>(), Reader(), Scope);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Listbox::SetFilter(Element Owner, Text Hidden, Text Disabled)
    {
        Ref<Listing> Feed = Owner.Obtain<Listing>();
        Feed.Hidden   = Hidden;
        Feed.Disabled = Disabled;

        if (!Feed.Source.IsEmpty())
        {
            Remake(Owner);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Listbox::AddItem(Element Owner, Text Content)
    {
        const Element Item = Panel::Create(GetList(Owner), "ListItem", Direction::Row);
        Item.EditBox().Items = Align::Center;
        Widget::Govern(Item, Handler(), false);

        Label::Create(Item, Content, "ListItem.Label");
        return Item;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Listbox::Select(Element Owner, Element Item)
    {
        const Element     List   = GetList(Owner);
        const Ptr<Choice> Picked = List.Get<Choice>();

        if (const Element Previous(List.GetTree(), Picked->Selected); Previous.IsAlive() && Previous != Item)
        {
            Previous.SetState(Widget::Selected(), false);
        }

        Picked->Selected = Item.GetHandle();

        if (Item.IsAlive())
        {
            Item.SetState(Widget::Selected(), true);
        }

        // A widget keeping the list picks the item too, and shows it in its label when it has one.
        if (const Ptr<Choice> Keeper = Owner.Get<Choice>(); Keeper && Owner != List)
        {
            Keeper->Selected = Item.GetHandle();

            if (const Element Shown(Owner.GetTree(), Keeper->Label); Shown.IsAlive())
            {
                Label::SetText(Shown, GetText(Item));

                // The label repeats the item's binding too, so it follows the item into another language.
                Binder::Mirror(Item.IsAlive() ? Item.GetFirst() : Element(), Shown);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Listbox::GetSelected(Element Owner)
    {
        const Element List = GetList(Owner);
        return Element(List.GetTree(), List.Get<Choice>()->Selected);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Listbox::SetIndex(Element Owner, SInt32 Index)
    {
        // A position below nought or past the items picks none.
        const Element List = GetList(Owner);
        const Element Item = Index >= 0 ? Widget::GetChild(List, static_cast<UInt32>(Index)) : Element();

        if (Item.GetHandle() != List.Get<Choice>()->Selected)
        {
            Select(Owner, Item);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    SInt32 Listbox::GetIndex(Element Owner)
    {
        const Element Picked = GetSelected(Owner);
        SInt32        Index  = 0;

        for (Element Item = GetList(Owner).GetFirst(); Item.IsAlive(); Item = Item.GetNext(), ++Index)
        {
            if (Item == Picked)
            {
                return Index;
            }
        }
        return -1;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Text Listbox::GetText(Element Item)
    {
        const ConstPtr<Caption> Source = Item.IsAlive() ? Item.GetFirst().Get<Caption>() : nullptr;
        return Source ? Text(Source->Content) : Text();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Listbox::GetList(Element Owner)
    {
        const ConstPtr<Choice> Parts = Owner.Get<Choice>();
        return Parts && Parts->List.IsValid() ? Element(Owner.GetTree(), Parts->List) : Owner;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Listbox::Step(Ref<Interface> Owner, Handle Self, SInt32 Offset)
    {
        const Element Root(Owner, Self);
        const Element List    = GetList(Root);
        const SInt32  Count   = static_cast<SInt32>(Widget::GetChildCount(List));
        const SInt32  Current = GetIndex(Root);
        const SInt32  Way     = Offset > 0 ? 1 : -1;
        SInt32        Next    = Current;

        // From none picked the first step lands on the first or the last item, and items filtered out are passed over.
        for (SInt32 Probe = Current < 0 ? (Way > 0 ? -1 : Count) : Current, Left = Abs(Offset); Left > 0;)
        {
            Probe += Way;

            if (Probe < 0 || Probe >= Count)
            {
                break;
            }

            if (IsUsable(Widget::GetChild(List, static_cast<UInt32>(Probe))))
            {
                Next = Probe;
                --Left;
            }
        }

        // Past either end the arrow is left alone, so it moves the keyboard out of the list.
        if (Next == Current)
        {
            return false;
        }

        SetIndex(Root, Next);
        Owner.Reveal(GetSelected(Root).GetHandle());
        Owner.Dispatch(Event(Signal::kChange, Self));
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Listbox::OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
    {
        const Bool Upward   = Message.Key == ZyInput::Key::Up;
        const Bool Downward = Message.Key == ZyInput::Key::Down;

        if (Message.Kind == Signal::kKey && Message.Target == Self && (Upward || Downward))
        {
            return Step(Owner, Self, Upward ? -1 : 1);
        }

        // Only a click on one of its own items picks it; one deeper down belongs to whatever holds it.
        if (Message.Kind != Signal::kClick || Message.Target == Self || Owner.GetParent(Message.Target) != Self)
        {
            return false;
        }

        Select(Element(Owner, Self), Element(Owner, Message.Target));
        Owner.Dispatch(Event(Signal::kChange, Self));
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Listbox::OnItems(Element Owner, ConstRef<Datum> Value)
    {
        const Ptr<Listing> Feed  = Owner.Get<Listing>();
        const UInt32       Count = Value.GetForm() == Form::List ? static_cast<UInt32>(Value.GetNumber()) : 0;

        // Only a change of count makes the items again; a change in a record shows through the item's bindings.
        if (!Feed || Feed->Count == Count)
        {
            return;
        }

        Feed->Count = Count;
        Remake(Owner);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Listbox::Remake(Element Owner)
    {
        const Element List   = GetList(Owner);
        const SInt32  Picked = GetIndex(Owner);

        Select(Owner, Element());

        // The items are taken first, since destroying one moves the next into its place.
        Sequence<Handle> Gone;
        List.Children([&Gone](Element Child)
        {
            Gone.Append(Child.GetHandle());
        });

        for (const Handle Item : Gone)
        {
            Element(List.GetTree(), Item).Destruct();
        }

        // Copied, since making an item may move the store the facet stands in.
        const ConstPtr<Listing> Feed = Owner.Get<Listing>();

        if (!Feed)
        {
            return;
        }

        const Str    Source   = Feed->Source;
        const Str    Content  = Feed->Content;
        const Str    Hidden   = Feed->Hidden;
        const Str    Disabled = Feed->Disabled;
        const UInt32 Count    = Feed->Count;
        Path         Base;
        const Bool   Parsed   = Path::Parse(Source, Base) && !Base.Steps.IsEmpty();

        for (UInt32 Record = 0; Record < Count; ++Record)
        {
            const Element Item  = AddItem(Owner, Text());
            const Str     Scope = Str::Print<"{0}[{1}]">(Text(Source), Record);

            Binder::Attach(Item.GetFirst(), Content, Binder::PutText<&Label::SetText>(), Reader(), Scope);

            if (!Hidden.IsEmpty())
            {
                Binder::Attach(Item, Hidden, Binder::PutState<State::Hidden>(), Reader(), Scope);
            }

            if (!Disabled.IsEmpty())
            {
                Binder::Attach(Item, Disabled, Binder::PutState<State::Disabled>(), Reader(), Scope);
            }

            // A list in a row handed another record reads that record's list, which the bound text no longer names.
            Path From = Base;
            Path To;

            if (Parsed && Locate(Owner, Record, To))
            {
                From.Steps.GetBack().Index = Record;

                if (!Same(From, To))
                {
                    Binder::Rescope(Item, From, To);
                }
            }
        }

        if (Picked >= 0 && static_cast<UInt32>(Picked) < Count)
        {
            SetIndex(Owner, Picked);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Listbox::Locate(Element Owner, UInt32 Record, Ref<Path> Output)
    {
        // The binding of the records is the list's own, the one counting them through the list's writer.
        const ConstPtr<Binding> Found = Owner.Get<Binding>();
        const SInt              Index = Found ? Found->Links.Find([](ConstRef<Link> Entry)
        {
            return Entry.Write.IsBoundTo<&Listbox::OnItems>() && Entry.Pieces.GetSize() == 1 && Entry.Pieces[0].Hole;
        }) : -1;

        if (Index < 0)
        {
            return false;
        }

        Output = Found->Links[static_cast<UInt>(Index)].Pieces[0].Where;

        if (Output.Steps.IsEmpty())
        {
            return false;
        }

        Output.Steps.GetBack().Index = Record;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Listbox::Same(ConstRef<Path> First, ConstRef<Path> Second)
    {
        if (First.Steps.GetSize() != Second.Steps.GetSize())
        {
            return false;
        }

        for (UInt Index = 0; Index < First.Steps.GetSize(); ++Index)
        {
            if (!(First.Steps[Index] == Second.Steps[Index]))
            {
                return false;
            }
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Listbox::IsUsable(Element Item)
    {
        return Item.IsAlive() && !Item.HasState(State::Hidden) && !Item.HasState(State::Disabled);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str Listbox::Phrase(Text Field)
    {
        // A field naming a value or a lexicon key binds as it is written; a bare key reads that value of the record.
        if (Field.IsEmpty() || Binder::IsBound(Field) || Binder::IsLocalized(Field))
        {
            return Str(Field);
        }
        return Str::Print<"{{{0}}}">(Field);
    }
}