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

#include "Binder.hpp"
#include "Facet/Binding.hpp"
#include "Lexicon.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ref<Table<UInt32, Binder::Scoped>> Binder::GetHolders()
    {
        static Table<UInt32, Scoped> Instance;
        return Instance;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::SetModel(Ref<Tree> Owner, Ptr<Model> Source)
    {
        Supply(Owner).Source = Source;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ptr<Model> Binder::GetModel(ConstRef<Tree> Owner)
    {
        const ConstPtr<Feed> Fed = Owner.Find<Feed>();
        return Fed ? Fed->Source : nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::SetLexicon(Ref<Tree> Owner, Ptr<Lexicon> Source)
    {
        Supply(Owner).Tables = Source;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ptr<Lexicon> Binder::GetLexicon(ConstRef<Tree> Owner)
    {
        const ConstPtr<Feed> Fed = Owner.Find<Feed>();
        return Fed ? Fed->Tables : nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::Declare(Ref<Interface> Target)
    {
        Ref<Feed> Fed = Supply(Target);

        // Each module standing on this one declares it again.
        if (Fed.Declared)
        {
            return;
        }
        Fed.Declared = true;

        // The game's changes reach their bindings before anything is dressed or laid out.
        Target.AddStage(Interface::Phase::Prepare, 20, Interface::Stage::Create<&Binder::OnRefresh>());

        // Heard first, so the model holds a widget's new value before any handler reads it.
        Target.GetDispatcher().AddListener(Handler::Create<&Binder::OnChange>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Binder::Attach(Element Target, Text Source, Writer Write, Reader Read, Text Scope)
    {
        if (!Target.IsAlive())
        {
            return false;
        }

        Sequence<Piece> Pieces;
        Str             Phrase;

        // A doubled at sign writes one out instead of naming a key.
        if (IsLocalized(Source))
        {
            if (Source[1] != '@')
            {
                Phrase = Source.Slice(1);
            }
            Speak(Phrase.IsEmpty() ? Source.Slice(1) : Lookup(Target, Phrase), Scope, Pieces);
        }
        else if (!Parse(Source, Scope, false, Pieces))
        {
            return false;
        }

        Ref<Binding> Found = Target.Obtain<Binding>();
        Ref<Link>    Entry = Found.Links.Append();
        Entry.Pieces = Move(Pieces);
        Entry.Write  = Move(Write);
        Entry.Read   = Move(Read);

        // Only a lexicon text is read again, so only it keeps its scope.
        if (!Phrase.IsEmpty())
        {
            Entry.Phrase = Move(Phrase);
            Entry.Scope  = Scope;
            Locate(Scope, Entry.Within);
        }

        // Shown at once, so a template reads right the moment it is made.
        Show(Target, Found.Links.GetSize() - 1, true);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::Detach(Element Target)
    {
        if (Target.Has<Binding>())
        {
            Target.Remove<Binding>();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Binder::IsBound(Text Source)
    {
        for (UInt Cursor = 0; Cursor < Source.GetSize(); ++Cursor)
        {
            const Char Current = Source[Cursor];
            const Bool Doubled = Cursor + 1 < Source.GetSize() && Source[Cursor + 1] == Current;

            if (Current != '{' && Current != '}')
            {
                continue;
            }

            // A doubled brace is written out as one, and a lone one names a value once another closes it.
            if (Doubled || (Current == '{' && StrFind(Source.Slice(Cursor), '}') > 0))
            {
                return true;
            }
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Binder::IsLocalized(Text Source)
    {
        return Source.GetSize() > 1 && Source[0] == '@';
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::Refresh(Ref<Tree> Owner, Bool Force)
    {
        Sweep(Owner, [Force](Element Target, UInt Index)
        {
            Show(Target, Index, Force);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::Translate(Ref<Tree> Owner)
    {
        Sweep(Owner, [](Element Target, UInt Index)
        {
            Restate(Target, Index);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::Mirror(Element Source, Element Target)
    {
        if (!Target.IsAlive())
        {
            return;
        }

        Detach(Target);

        const ConstPtr<Binding> Found = Source.Get<Binding>();

        if (!Found)
        {
            return;
        }

        // Copied first, since adding the facet may move the source's store.
        Sequence<Link> Links = Found->Links;
        Target.Add<Binding>().Links = Move(Links);

        for (UInt Index = 0; Index < CountLinks(Target); ++Index)
        {
            Show(Target, Index, true);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::Store(Element Target)
    {
        const Ptr<Model> Data = Target.IsAlive() ? GetModel(Target.GetTree()) : nullptr;

        if (!Data)
        {
            return;
        }

        for (UInt Index = 0; Index < CountLinks(Target); ++Index)
        {
            Keep(Target, Index, * Data);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::Rescope(Element Target, ConstRef<Path> From, ConstRef<Path> To)
    {
        if (!Target.IsAlive() || From.Steps.IsEmpty() || To.Steps.IsEmpty())
        {
            return;
        }

        // Popups opened for it, as a row's context menu, read its record too.
        Sequence<Handle> Opened;
        Gather(Target.GetTree(), Target.GetHandle(), Opened);

        Descend(Target, From, To);

        for (const Handle Root : Opened)
        {
            Descend(Element(Target.GetTree(), Root), From, To);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::Locate(Text Scope, Ref<Path> Output)
    {
        Output.Steps.Clear();

        // A path of one step read in the scope shows which of its steps the scope gave.
        Piece Probe;

        if (Scope.IsEmpty() || !ParseHole("Probe", Scope, Probe))
        {
            return;
        }

        Output.Steps.Append(ConstSpan(Probe.Where.Steps.GetData(), Probe.Scoped));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ref<Binder::Feed> Binder::Supply(Ref<Tree> Owner)
    {
        if (const Ptr<Feed> Found = Owner.Find<Feed>())
        {
            return * Found;
        }
        return Owner.Provide<Feed>();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::OnRefresh(Ref<Interface> Owner, Real32)
    {
        Ref<Feed> Fed = Supply(Owner);

        // A switched language or a replaced table reads every text again.
        const UInt32 Spoken = Fed.Tables ? Fed.Tables->GetVersion() : 0;

        if (Fed.Tables != Fed.Heard || Spoken != Fed.Spoken)
        {
            Fed.Heard  = Fed.Tables;
            Fed.Spoken = Spoken;
            Translate(Owner);
        }

        const Ptr<Model> Data     = Fed.Source;
        const Bool       Replaced = (Data != Fed.Watched);

        if (!Replaced && (!Data || Data->GetRevision() == Fed.Revision))
        {
            return;
        }

        Fed.Watched  = Data;
        Fed.Revision = Data ? Data->GetRevision() : 0;

        // A new model may stamp its values as the last one did, so every one is shown anew.
        Refresh(Owner, Replaced);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Binder::OnChange(Ref<Interface> Owner, ConstRef<Event> Message, Handle)
    {
        if (Message.Kind == Signal::kChange)
        {
            Store(Element(Owner, Message.Target));
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename Callable>
    void Binder::Sweep(Ref<Tree> Owner, AnyRef<Callable> Action)
    {
        // A sweep begun inside another takes a list of its own.
        thread_local Sequence<UInt32> Kept;
        thread_local Bool             Busy = false;

        Sequence<UInt32>      Own;
        Ref<Sequence<UInt32>> Slots = Busy ? Own : Kept;
        const Bool            Lent  = !Busy;

        Busy = true;
        Slots.Clear();

        Owner.GetFacets<Binding>().ForEach([&Slots](UInt32 Slot, Ref<Binding>)
        {
            Slots.Append(Slot);
        });

        for (const UInt32 Slot : Slots)
        {
            const Element Target(Owner, Owner.GetHandle(Slot));

            for (UInt Index = 0; Index < CountLinks(Target); ++Index)
            {
                Action(Target, Index);
            }
        }
        Busy = !Lent;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt Binder::CountLinks(Element Target)
    {
        const ConstPtr<Binding> Found = Target.Get<Binding>();
        return Found ? Found->Links.GetSize() : 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Binder::Parse(Text Source, Text Scope, Bool Plain, Ref<Sequence<Piece>> Output)
    {
        Sequence<Piece> Result;
        Str             Literal;
        Bool            Found = false;

        for (UInt Cursor = 0; Cursor < Source.GetSize();)
        {
            const Char Current = Source[Cursor];
            const Char Next    = Cursor + 1 < Source.GetSize() ? Source[Cursor + 1] : '\0';

            // A doubled brace is written out as one, and still makes the text bound.
            if ((Current == '{' || Current == '}') && Next == Current)
            {
                Literal.Append(Current);
                Found   = true;
                Cursor += 2;
                continue;
            }

            const SInt Close = Current == '{' ? StrFind(Source.Slice(Cursor), '}') : -1;

            if (Close < 0)
            {
                Literal.Append(Current);
                ++Cursor;
                continue;
            }

            Piece Hole;

            if (!ParseHole(Source.Slice(Cursor + 1, static_cast<UInt>(Close) - 1), Scope, Hole))
            {
                LOG_W("The binding '{0}' names a value badly, so it is left unbound", Source);
                return false;
            }

            Flush(Literal, Result);
            Result.Append(Move(Hole));
            Found   = true;
            Cursor += static_cast<UInt>(Close) + 1;
        }

        Flush(Literal, Result);

        if (!Found && !Plain)
        {
            return false;
        }

        Output = Move(Result);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::Speak(Text Source, Text Scope, Ref<Sequence<Piece>> Output)
    {
        if (Parse(Source, Scope, true, Output))
        {
            return;
        }

        // A translator's mistake still shows the words as written.
        Output.Clear();
        Output.Append().Literal = Source;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Text Binder::Lookup(Element Target, Text Key)
    {
        const Ptr<Lexicon> Words = GetLexicon(Target.GetTree());
        return Words ? Words->Translate(Key) : Key;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::Descend(Element Target, ConstRef<Path> From, ConstRef<Path> To)
    {
        Retarget(Target, From, To);
        Target.Children([&From, &To](Element Child)
        {
            Descend(Child, From, To);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::Restate(Element Target, UInt Index)
    {
        Ref<Link> Entry = Target.Get<Binding>()->Links[Index];

        // A value may hold a lexicon key itself, which only showing it again translates.
        if (Entry.Phrase.IsEmpty())
        {
            Show(Target, Index, true);
            return;
        }

        // Read where it was made, then moved to the record its row shows now.
        Sequence<Piece> Pieces;
        Speak(Lookup(Target, Entry.Phrase), Entry.Scope, Pieces);

        Path Made;
        Locate(Entry.Scope, Made);

        for (Ref<Piece> Part : Pieces)
        {
            if (!Made.Steps.IsEmpty() && Part.Scoped == Made.Steps.GetSize())
            {
                Repoint(Part, Made, Entry.Within);
            }
        }

        Entry.Pieces = Move(Pieces);
        Show(Target, Index, true);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Binder::ParseHole(Text Source, Text Scope, Ref<Piece> Output)
    {
        Text Inside = StrTrim(Source, ' ');

        if (!Inside.IsEmpty() && Inside[0] == '!')
        {
            Output.Negate = true;
            Inside        = StrTrim(Inside.Slice(1), ' ');
        }

        // A colon fixes the digits a number keeps after the point.
        if (const SInt Colon = StrFind(Inside, ':'); Colon >= 0)
        {
            const Text   Digits = StrTrim(Inside.Slice(static_cast<UInt>(Colon) + 1), ' ');
            UInt         Cursor = 0;
            const UInt32 Places = StrExtractNumber<10, UInt32>(Digits, Cursor);

            if (Cursor == 0 || Cursor != Digits.GetSize())
            {
                return false;
            }

            Output.Decimals = static_cast<SInt32>(Places);
            Inside          = StrTrim(Inside.Slice(0, static_cast<UInt>(Colon)), ' ');
        }

        // A leading slash reads from the root, whatever the scope.
        const Bool Slashed = !Inside.IsEmpty() && Inside[0] == '/';
        const Bool Rooted  = Slashed || Scope.IsEmpty();
        const Text Named   = Slashed ? Inside.Slice(1) : Inside;
        const Str  Full    = Rooted ? Str(Named) : Str::Join(Scope, '.', Named);

        Output.Hole = true;

        if (!Path::Parse(Full, Output.Where))
        {
            return false;
        }

        // The steps the scope gave are the ones beyond what the path names alone.
        if (Path Own; !Rooted && Path::Parse(Inside, Own) && Own.Steps.GetSize() < Output.Where.Steps.GetSize())
        {
            Output.Scoped = static_cast<UInt32>(Output.Where.Steps.GetSize() - Own.Steps.GetSize());
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Binder::IsWhole(ConstRef<Link> Entry)
    {
        return Entry.Pieces.GetSize() == 1 && Entry.Pieces[0].Hole && Entry.Pieces[0].Decimals < 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Binder::Stamp(ConstPtr<Model> Data, ConstRef<Link> Entry)
    {
        UInt32 Newest = 0;

        for (UInt Index = 0; Data && Index < Entry.Pieces.GetSize(); ++Index)
        {
            UInt32 Found = 0;

            if (Entry.Pieces[Index].Hole)
            {
                Data->Resolve(Entry.Pieces[Index].Where, Found);
                Newest = Max(Newest, Found);
            }
        }
        return Newest;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Datum Binder::Evaluate(ConstPtr<Model> Data, ConstRef<Link> Entry)
    {
        UInt32 Ignored = 0;

        if (IsWhole(Entry))
        {
            ConstRef<Piece>       Hole  = Entry.Pieces[0];
            const ConstPtr<Datum> Found = Data ? Data->Resolve(Hole.Where, Ignored) : nullptr;
            const Datum           Value = Found ? * Found : Datum();
            return Hole.Negate ? Datum(!Value.GetBool()) : Value;
        }

        const UInt Room = Accumulate(Entry.Pieces.GetData(), Entry.Pieces.GetSize(), UInt(0), [](ConstRef<Piece> Part)
        {
            return Part.Hole ? kRoom : Part.Literal.GetSize();
        });

        Str Result;
        Result.Reserve(Room);

        for (ConstRef<Piece> Part : Entry.Pieces)
        {
            if (!Part.Hole)
            {
                Result.Append(Part.Literal);
                continue;
            }

            const ConstPtr<Datum> Found = Data ? Data->Resolve(Part.Where, Ignored) : nullptr;

            if (Part.Negate)
            {
                Datum(!(Found && Found->GetBool())).Print(Result);
            }
            else if (Found)
            {
                Found->Print(Result, Part.Decimals);
            }
        }
        return Datum(Move(Result));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::Show(Element Target, UInt Index, Bool Force)
    {
        const ConstPtr<Model> Data   = GetModel(Target.GetTree());
        Ref<Link>             Entry  = Target.Get<Binding>()->Links[Index];
        const UInt32          Newest = Stamp(Data, Entry);

        if (!Force && Newest == Entry.Seen)
        {
            return;
        }

        Entry.Seen = Newest;

        // Copied, since the writer may add a facet and move the binding.
        const Writer Write = Entry.Write;
        Datum        Value = Evaluate(Data, Entry);

        // Only a whole value held as `@Key` is read through the lexicon, and `@@` drops an at sign.
        if (const Text Words = Value.GetText(); IsWhole(Entry) && IsLocalized(Words))
        {
            Value = Datum(Words[1] == '@' ? Words.Slice(1) : Lookup(Target, Words.Slice(1)));
        }

        if (!Write.IsEmpty())
        {
            Write(Target, Value);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::Keep(Element Target, UInt Index, Ref<Model> Data)
    {
        ConstRef<Link> Current = Target.Get<Binding>()->Links[Index];

        if (Current.Read.IsEmpty() || !IsWhole(Current))
        {
            return;
        }

        // Found again after the reader runs, since reading may add a facet and move the binding.
        const Reader Read  = Current.Read;
        const Datum  Value = Read(Target);
        Ref<Link>    Entry = Target.Get<Binding>()->Links[Index];

        Data.Assign(Entry.Pieces[0].Where, Entry.Pieces[0].Negate ? Datum(!Value.GetBool()) : Value);

        // Marked seen, so the next refresh passes it over.
        Entry.Seen = Stamp(AddressOf(Data), Entry);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::Retarget(Element Target, ConstRef<Path> From, ConstRef<Path> To)
    {
        // What a facet keeps of the record, as a tooltip's template scope, follows the row.
        for (ConstRef<Table<UInt32, Scoped>::Pair> Each : GetHolders())
        {
            if (const Ptr<Path> Kept = Each.Second(Target))
            {
                Reroute(* Kept, Kept->Steps.GetSize(), From, To);
            }
        }

        // Found again each time, since showing a value may add a facet and move the binding.
        for (UInt Index = 0; Index < CountLinks(Target); ++Index)
        {
            Bool      Moved = false;
            Ref<Link> Entry = Target.Get<Binding>()->Links[Index];

            for (Ref<Piece> Part : Entry.Pieces)
            {
                Moved = Repoint(Part, From, To) || Moved;
            }

            // A lexicon text read again later reads from the row's record by then.
            Reroute(Entry.Within, Entry.Within.Steps.GetSize(), From, To);

            if (Moved)
            {
                Show(Target, Index, true);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Binder::Repoint(Ref<Piece> Part, ConstRef<Path> From, ConstRef<Path> To)
    {
        if (!Part.Hole || !Reroute(Part.Where, Part.Scoped, From, To))
        {
            return false;
        }

        Part.Scoped = static_cast<UInt32>(Part.Scoped + To.Steps.GetSize() - From.Steps.GetSize());
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Binder::Reroute(Ref<Path> Where, UInt Scoped, ConstRef<Path> From, ConstRef<Path> To)
    {
        const UInt Count = From.Steps.GetSize();

        // A path the scope gave fewer steps than the record has cannot be reading from it.
        if (Count == 0 || Scoped < Count || Where.Steps.GetSize() < Count)
        {
            return false;
        }

        if (!Compare(Where.Steps.GetData(), From.Steps.GetData(), Count))
        {
            return false;
        }

        // A record at another depth takes as many steps as its own path.
        if (To.Steps.GetSize() != Count)
        {
            Sequence<Step> Steps = To.Steps;
            Steps.Append(ConstSpan(Where.Steps.GetData() + Count, Where.Steps.GetSize() - Count));
            Where.Steps = Move(Steps);
            return true;
        }

        Copy(Where.Steps.GetData(), Count, To.Steps.GetData());
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Binder::IsOpenedFor(ConstRef<Tree> Owner, Handle Root, Handle Target)
    {
        const ConstPtr<Control> Opener = Owner.Get<Control>(Root);

        for (Handle Current = Opener ? Opener->Owner : Handle(); Owner.IsAlive(Current);)
        {
            if (Current == Target)
            {
                return true;
            }

            if (const Handle Parent = Owner.GetParent(Current); Parent.IsValid())
            {
                Current = Parent;
            }
            else
            {
                const ConstPtr<Control> Found = Owner.Get<Control>(Current);
                Current = Found ? Found->Owner : Handle();
            }
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::Gather(ConstRef<Tree> Owner, Handle Target, Ref<Sequence<Handle>> Output)
    {
        for (const UInt16 Index : Owner.GetOrder())
        {
            if (const Handle Root = Owner.GetHandle(Owner.GetPane(Index).Slots[0]); IsOpenedFor(Owner, Root, Target))
            {
                Output.Append(Root);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::Flush(Ref<Str> Literal, Ref<Sequence<Piece>> Output)
    {
        if (!Literal.IsEmpty())
        {
            Output.Append().Literal = Literal;
            Literal.Clear();
        }
    }
}