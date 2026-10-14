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
#include "Datum.hpp"
#include "Facet/Binding.hpp"
#include "Lexicon.hpp"
#include "Link.hpp"
#include "Model.hpp"
#include "Path.hpp"
#include "Piece.hpp"
#include "Step.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Input/Control.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Tree.hpp"

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

        // Declared again, as by each module standing on this one, it installs nothing twice.
        if (Fed.Declared)
        {
            return;
        }
        Fed.Declared = true;

        // Values the game changed reach what is bound to them before anything is dressed or laid out.
        Target.AddStage(Interface::Phase::Prepare, 20, Interface::Stage::Create<&Binder::OnRefresh>());

        // A widget's new value is heard first, and reaches the model before any handler reads it there.
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

        // A key is read from the lexicon, its text naming values or not; a doubled at sign writes one out instead.
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

        // Only a text of the lexicon is read again, so only it keeps where it was read.
        if (!Phrase.IsEmpty())
        {
            Entry.Phrase = Move(Phrase);
            Entry.Scope  = Scope;
            Locate(Scope, Entry.Within);
        }

        // Shown at once, so a template reads right the moment it is made rather than at the next update.
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
            // A doubled brace is written out, so only a lone one opens a value.
            if (Source[Cursor] == '{' && Cursor + 1 < Source.GetSize() && Source[Cursor + 1] == '{')
            {
                ++Cursor;
            }
            else if (Source[Cursor] == '{' && StrFind(Source.Slice(Cursor), '}') > 0)
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

        // Copied before the facet is added, since adding it may move the store the source's sits in.
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

        // The popups opened for it, as a row's context menu, were made reading its record, so they follow it too.
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

        // A switched language or a read or replaced table reads every text again; otherwise it costs one comparison.
        const UInt32 Spoken = Fed.Tables ? Fed.Tables->GetVersion() : 0;

        if (Fed.Tables != Fed.Heard || Spoken != Fed.Spoken)
        {
            Fed.Heard  = Fed.Tables;
            Fed.Spoken = Spoken;
            Translate(Owner);
        }

        const Ptr<Model> Data     = Fed.Source;
        const Bool       Replaced = (Data != Fed.Watched);

        // A model left as it was costs one comparison a frame; only a change walks the bindings.
        if (!Replaced && (!Data || Data->GetRevision() == Fed.Revision))
        {
            return;
        }

        Fed.Watched  = Data;
        Fed.Revision = Data ? Data->GetRevision() : 0;

        // A model put in another's place may stamp its values as the last one did, so every one is shown anew.
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
        // The slots are kept between sweeps, and a sweep begun inside another takes a list of its own.
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

            if ((Current == '{' || Current == '}') && Next == Current)
            {
                Literal.Append(Current);
                Cursor += 2;
                continue;
            }

            const SInt Close = Current == '{' ? StrFind(Source.Slice(Cursor), '}') : -1;

            // A brace closing nothing is written as it stands.
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

        // A translator's mistake still shows the words, as written, rather than nothing.
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

        if (Entry.Phrase.IsEmpty())
        {
            return;
        }

        // Read where it was first made, then moved to the record its row shows now, as the old pieces were.
        Sequence<Piece> Pieces;
        Speak(Lookup(Target, Entry.Phrase), Entry.Scope, Pieces);

        // The record it was made in may lie deeper or shallower than the one it reads now, as a tree view's row.
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

        // A path opening with a slash is read from the root, whatever scope it stands in.
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

        // Room for every piece is reserved up front, so a text costs one allocation.
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

        // An unchanged value is never written out again, so a frame with nothing new formats nothing.
        if (!Force && Newest == Entry.Seen)
        {
            return;
        }

        Entry.Seen = Newest;

        // Copied, since the writer may add a facet and move the binding under it.
        const Writer Write = Entry.Write;
        const Datum  Value = Evaluate(Data, Entry);

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

        // The binding is found again after the reader runs, since reading the element may add a facet and move it.
        const Reader Read  = Current.Read;
        const Datum  Value = Read(Target);
        Ref<Link>    Entry = Target.Get<Binding>()->Links[Index];

        Data.Assign(Entry.Pieces[0].Where, Entry.Pieces[0].Negate ? Datum(!Value.GetBool()) : Value);

        // The element shows what it was written back as already, so the next refresh passes it over.
        Entry.Seen = Stamp(AddressOf(Data), Entry);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Binder::Retarget(Element Target, ConstRef<Path> From, ConstRef<Path> To)
    {
        // What a facet keeps of the record it reads, as a tooltip made later from a template, follows the row.
        for (ConstRef<Table<UInt32, Scoped>::Pair> Each : GetHolders())
        {
            if (const Ptr<Path> Kept = Each.Second(Target))
            {
                Reroute(* Kept, Kept->Steps.GetSize(), From, To);
            }
        }

        // The facet is found again each time, since showing a value may add a facet and move the store under it.
        for (UInt Index = 0; Index < CountLinks(Target); ++Index)
        {
            Bool      Moved = false;
            Ref<Link> Entry = Target.Get<Binding>()->Links[Index];

            for (Ref<Piece> Part : Entry.Pieces)
            {
                Moved = Repoint(Part, From, To) || Moved;
            }

            // A text of the lexicon read again later reads from the record the row shows by then.
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

        // The steps its scope gave grow or shrink with the record's own.
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

        // A record deeper or shallower than the first takes as many steps as its own path does.
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