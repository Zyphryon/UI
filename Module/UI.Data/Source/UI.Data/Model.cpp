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

#include "Model.hpp"
#include "Datum.hpp"
#include "Path.hpp"
#include "Step.hpp"
#include "UI.Core/Handle.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Model::Set(Text Key, ConstRef<Datum> Value)
    {
        // A name reaching into no record, the common case, is set without a path being built for it.
        if (StrFind(Key, '[') < 0)
        {
            if (!Key.IsEmpty())
            {
                Store(Identify(Key), Value);
            }
            return;
        }

        if (Ref<Path> Where = mRoot->mPath; Path::Parse(Key, Where))
        {
            Assign(Where, Value);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<Datum> Model::Find(Text Key) const
    {
        const ConstPtr<Entry> Found = Seek(Key);
        return Found ? AddressOf(Found->Value) : nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Model::GetBool(Text Key, Bool Default) const
    {
        const ConstPtr<Datum> Found = Find(Key);
        return Found && Found->GetForm() != Form::None ? Found->GetBool() : Default;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real64 Model::GetNumber(Text Key, Real64 Default) const
    {
        const ConstPtr<Datum> Found = Find(Key);
        return Found && Found->GetForm() != Form::None ? Found->GetNumber() : Default;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str Model::GetText(Text Key) const
    {
        const ConstPtr<Datum> Found = Find(Key);
        return Found ? Found->Print() : Str();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Model::GetStamp(Text Key) const
    {
        UInt32 Stamp = 0;

        if (Ref<Path> Where = mRoot->mPath; Path::Parse(Key, Where))
        {
            Reach(Where, Stamp);
        }
        return Stamp;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Model::GetSubtreeStamp(Text Key) const
    {
        Ref<Path> Where = mRoot->mPath;
        return Path::Parse(Key, Where) ? GetSubtreeStamp(Where) : 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Model::GetSubtreeStamp(ConstRef<Path> Where) const
    {
        UInt32                Stamp = 0;
        const ConstPtr<Entry> Found = Reach(Where, Stamp);

        // The lists on the way count by their own changes alone, so a change in a sibling record never reaches here.
        return Found ? Max(Stamp, Found->Deep) : Stamp;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ptr<Model> Model::Append(Text List)
    {
        // An index past the end is held to the count, so the record lands last.
        return Insert(List, Step::kLeaf);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ptr<Model> Model::Insert(Text List, UInt32 Index)
    {
        Ptr<Model>       Owner = nullptr;
        UInt64           Name  = 0;
        const Ptr<Entry> Found = Open(List, Owner, Name);

        if (!Found)
        {
            return nullptr;
        }

        const UInt       At     = Min(static_cast<UInt>(Index), Found->Records.GetSize());
        const Ptr<Model> Record = new Model(* mRoot, Owner, Name);

        Found->Records.Insert(At, Unique(Record));
        Owner->Touch(* Found);
        return Record;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Model::Erase(Text List, UInt32 Index)
    {
        Ptr<Model> Owner = nullptr;
        UInt64     Name  = 0;

        if (const Ptr<Entry> Found = Open(List, Owner, Name); Found && Index < Found->Records.GetSize())
        {
            Found->Records.Remove(Index);
            Owner->Touch(* Found);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Model::Resize(Text List, UInt32 Count)
    {
        Ptr<Model>       Owner = nullptr;
        UInt64           Name  = 0;
        const Ptr<Entry> Found = Open(List, Owner, Name);

        if (!Found || Found->Records.GetSize() == Count)
        {
            return;
        }

        if (Found->Records.GetSize() > Count)
        {
            Found->Records.Remove(Count, Found->Records.GetSize() - Count);
        }
        Found->Records.Reserve(Count);

        while (Found->Records.GetSize() < Count)
        {
            Found->Records.Append(Unique(new Model(* mRoot, Owner, Name)));
        }
        Owner->Touch(* Found);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Model::GetCount(Text List) const
    {
        const ConstPtr<Entry> Found = Seek(List);
        return Found ? static_cast<UInt32>(Found->Records.GetSize()) : 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ptr<Model> Model::GetRecord(Text List, UInt32 Index)
    {
        Ptr<Entry> Found = nullptr;

        // A list of this model by its name alone is found without a path being built for it.
        if (StrFind(List, '[') < 0)
        {
            Found = List.IsEmpty() ? nullptr : mEntries.Find(Digest(Identify(List)));
        }
        else if (Ref<Path> Where = mRoot->mPath; Path::Parse(List, Where))
        {
            const Ptr<Model> Owner = Walk(Where);
            Found = Owner ? Owner->mEntries.Find(Digest(Where.Steps.GetBack().Key)) : nullptr;
        }
        return Found && Index < Found->Records.GetSize() ? Found->Records[Index].Grab() : nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt64 Model::GetKey(Text List, UInt32 Index) const
    {
        const ConstPtr<Entry> Found = Seek(List);
        return Found && Index < Found->Records.GetSize() ? Found->Records[Index]->mKey : 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    SInt32 Model::FindKey(Text List, UInt64 Key) const
    {
        if (const ConstPtr<Entry> Found = Seek(List))
        {
            for (UInt Index = 0; Index < Found->Records.GetSize(); ++Index)
            {
                if (Found->Records[Index]->mKey == Key)
                {
                    return static_cast<SInt32>(Index);
                }
            }
        }
        return -1;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<Datum> Model::Resolve(ConstRef<Path> Where, Ref<UInt32> Stamp) const
    {
        const ConstPtr<Entry> Found = Reach(Where, Stamp);
        return Found ? AddressOf(Found->Value) : nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Model::Assign(ConstRef<Path> Where, ConstRef<Datum> Value)
    {
        if (const Ptr<Model> Owner = Where.Steps.IsEmpty() ? nullptr : Walk(Where))
        {
            Owner->Store(Where.Steps.GetBack().Key, Value);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Model::Model()
        : mRoot     { this },
          mParent   { nullptr },
          mList     { 0 },
          mKey      { 0 },
          mRevision { 0 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Model::Model(Ref<Model> Root, Ptr<Model> Parent, UInt64 List)
        : mRoot     { AddressOf(Root) },
          mParent   { Parent },
          mList     { List },
          mKey      { 0 },
          mRevision { 0 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Model::Store(UInt64 Key, ConstRef<Datum> Value)
    {
        // Setting what is there already is no change, so nothing bound to it is shown again, and nothing is copied.
        const ConstPtr<Entry> Held = mEntries.Find(Digest(Key));

        const Bool Blank = Value.GetForm() == Form::None;
        const Bool Same  = Held ? Held->Value == Value && (Held->Stamp != 0 || Blank) : Blank;

        if (Same)
        {
            return;
        }

        // Copied first, since the value may stand in this model, which inserting may move or clearing free.
        Datum      Kept  = Value;
        Ref<Entry> Found = mEntries.FindOrInsert(Digest(Key));

        Found.Records.Clear();
        Found.Value = Move(Kept);
        Found.Stamp = ++mRoot->mRevision;
        Found.Deep  = Found.Stamp;
        Spread(Found.Stamp);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ptr<Model> Model::Walk(ConstRef<Path> Where)
    {
        Ptr<Model> Current = this;

        for (UInt Index = 0; Index + 1 < Where.Steps.GetSize(); ++Index)
        {
            ConstRef<Step>   Hop   = Where.Steps[Index];
            const Ptr<Entry> Found = Current->mEntries.Find(Digest(Hop.Key));

            if (!Found || Hop.Index >= Found->Records.GetSize())
            {
                return nullptr;
            }
            Current = Found->Records[Hop.Index].Grab();
        }
        return Current;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Model::Spread(UInt32 Stamp)
    {
        // Each record names the list it stands in, so the walk up costs a lookup a level and never a search.
        for (Ptr<Model> Current = this; Current->mParent; Current = Current->mParent)
        {
            const Ptr<Entry> Holder = Current->mParent->mEntries.Find(Digest(Current->mList));

            if (!Holder)
            {
                return;
            }
            Holder->Deep = Stamp;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ptr<Model::Entry> Model::Open(Text List, Ref<Ptr<Model>> Owner, Ref<UInt64> Name)
    {
        Ref<Path> Where = mRoot->mPath;

        if (!Path::Parse(List, Where))
        {
            return nullptr;
        }

        Owner = Walk(Where);
        Name  = Where.Steps.GetBack().Key;

        if (!Owner)
        {
            return nullptr;
        }

        // A value that held anything else, or none, becomes an empty list, a change what is bound to it shows.
        Ref<Entry> Found = Owner->mEntries.FindOrInsert(Digest(Name));

        if (Found.Value.GetForm() != Form::List)
        {
            Found.Records.Clear();
            Owner->Touch(Found);
        }
        return AddressOf(Found);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<Model::Entry> Model::Reach(ConstRef<Path> Where, Ref<UInt32> Stamp) const
    {
        ConstPtr<Model> Current = this;

        Stamp = 0;

        for (UInt Index = 0; Index < Where.Steps.GetSize(); ++Index)
        {
            ConstRef<Step>        Hop   = Where.Steps[Index];
            const ConstPtr<Entry> Found = Current->mEntries.Find(Digest(Hop.Key));

            if (!Found)
            {
                return nullptr;
            }

            // A list newer than the value reached through it changed which record the path lands on.
            Stamp = Max(Stamp, Found->Stamp);

            if (Hop.Index == Step::kLeaf)
            {
                return Found;
            }

            if (Hop.Index >= Found->Records.GetSize())
            {
                return nullptr;
            }
            Current = AddressOf(* Found->Records[Hop.Index]);
        }
        return nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<Model::Entry> Model::Seek(Text Key) const
    {
        Ref<Path> Where = mRoot->mPath;
        UInt32    Stamp = 0;
        return Path::Parse(Key, Where) ? Reach(Where, Stamp) : nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


    Ptr<Model::Entry> Model::Arrange(Text List)
    {
        ConstRef<Sequence<UInt64>> Keys  = mRoot->mKeys;
        Ptr<Model>                 Owner = nullptr;
        UInt64                     Name  = 0;
        const Ptr<Entry>           Found = Open(List, Owner, Name);

        if (!Found)
        {
            return nullptr;
        }

        Ref<Sequence<Unique<Model>>> Records = Found->Records;

        // A list already holding the keys in their order keeps its records and its stamp.
        Bool Same = Records.GetSize() == Keys.GetSize();

        for (UInt Index = 0; Same && Index < Keys.GetSize(); ++Index)
        {
            Same = Records[Index]->mKey == Keys[Index];
        }

        if (Same)
        {
            return Found;
        }

        // Each record is found by its key once, so a long list reordered costs no more than walking it.
        Table<UInt64, UInt> Places;

        for (UInt Index = 0; Index < Records.GetSize(); ++Index)
        {
            if (!Places.Find(Records[Index]->mKey))
            {
                Places.Assign(Records[Index]->mKey, Index);
            }
        }

        Sequence<Unique<Model>> Fresh;
        Fresh.Reserve(Keys.GetSize());

        for (const UInt64 Key : Keys)
        {
            const ConstPtr<UInt> Place = Places.Find(Key);

            // A key named twice keeps its record the first time, and is given a new one after.
            if (Place && Records[* Place])
            {
                Fresh.Append(Move(Records[* Place]));
            }
            else
            {
                Ref<Unique<Model>> Made = Fresh.Append(Unique<Model>(new Model(* mRoot, Owner, Name)));
                Made->mKey = Key;
            }
        }

        Records = Move(Fresh);
        Owner->Touch(* Found);
        return Found;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Model::Touch(Ref<Entry> List)
    {
        List.Value = Datum::Records(static_cast<UInt32>(List.Records.GetSize()));
        List.Stamp = ++mRoot->mRevision;
        List.Deep  = List.Stamp;
        Spread(List.Stamp);
    }
}