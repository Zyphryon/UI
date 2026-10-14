// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Datum.hpp"
#include "Path.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the values a game shows through its interface, each named and stamped by its last change.
    class Model final
    {
    public:

        /// \brief Constructs a root model holding no value.
        Model();

        /// \brief Prevents copying a model, since every record points at the root it belongs to.
        Model(ConstRef<Model> Other) = delete;

        /// \brief Sets a value, stamping it only when it changed.
        ///
        /// \param Key   The path of the value, as `Player.Gold` or `Party[2].Name`; a record it names must exist.
        /// \param Value The value stored there, which what is bound to the path shows.
        void Set(Text Key, ConstRef<Datum> Value);

        /// \brief Sets a flag.
        ///
        /// \param Key   The path of the value, as \ref Set reads it.
        /// \param Value The flag, `true` to set the flag, `false` to clear it.
        ZY_INLINE void SetBool(Text Key, Bool Value)
        {
            Set(Key, Datum(Value));
        }

        /// \brief Sets a number.
        ///
        /// \param Key   The path of the value, as \ref Set reads it.
        /// \param Value The number stored there.
        ZY_INLINE void SetNumber(Text Key, Real64 Value)
        {
            Set(Key, Datum(Value));
        }

        /// \brief Sets a text.
        ///
        /// \param Key   The path of the value, as \ref Set reads it.
        /// \param Value The text stored there, copied in.
        ZY_INLINE void SetText(Text Key, Text Value)
        {
            // Text held already is no change, which is told before a copy of it is made.
            const ConstPtr<Datum> Held = Find(Key);

            if (Held && Held->GetForm() == Form::Text && Held->GetText() == Value)
            {
                return;
            }
            Set(Key, Datum(Value));
        }

        /// \brief Sets a colour.
        ///
        /// \param Key   The path of the value, as \ref Set reads it.
        /// \param Value The colour stored there.
        ZY_INLINE void SetColor(Text Key, Color Value)
        {
            Set(Key, Datum(Value));
        }

        /// \brief Clears a value, so what is bound to it reads none.
        ///
        /// \param Key The path of the value, as \ref Set reads it.
        ZY_INLINE void Clear(Text Key)
        {
            Set(Key, Datum());
        }

        /// \brief Finds a value.
        ///
        /// \param Key The path of the value, as \ref Set reads it.
        /// \return The value, or `nullptr` when there is none.
        ConstPtr<Datum> Find(Text Key) const;

        /// \brief Gets a value as a flag.
        ///
        /// \param Key     The path of the value, as \ref Set reads it.
        /// \param Default The flag given back when there is no value, or it holds nothing.
        /// \return The value read as a flag, or \p Default.
        Bool GetBool(Text Key, Bool Default = false) const;

        /// \brief Gets a value as a number.
        ///
        /// \param Key     The path of the value, as \ref Set reads it.
        /// \param Default The number given back when there is no value, or it holds nothing.
        /// \return The value read as a number, or \p Default.
        Real64 GetNumber(Text Key, Real64 Default = 0.0) const;

        /// \brief Gets a value as text.
        ///
        /// \param Key The path of the value, as \ref Set reads it.
        /// \return The value written as text, empty when there is no value.
        Str GetText(Text Key) const;

        /// \brief Gets the stamp of the last change to a value, or to any list on the way to it.
        ///
        /// \param Key The path of the value, as \ref Set reads it.
        /// \return The stamp, nought when nothing on the way was ever set.
        UInt32 GetStamp(Text Key) const;

        /// \brief Gets the stamp of the last change to a value or to anything beneath it.
        ///
        /// \param Key The path of the value, as \ref Set reads it.
        /// \return The stamp, nought when nothing on the way or beneath was ever set.
        UInt32 GetSubtreeStamp(Text Key) const;

        /// \brief Gets the stamp of the last change to a value or to anything beneath it.
        ///
        /// \param Where The path read from this model.
        /// \return The stamp, nought when nothing on the way or beneath was ever set.
        UInt32 GetSubtreeStamp(ConstRef<Path> Where) const;

        /// \brief Adds a record at the end of a list, making the list when there is none.
        ///
        /// \param List The path of the list, as `Party` or `Party[2].Bag`.
        /// \return The record, or `nullptr` when the path names a record that does not exist.
        Ptr<Model> Append(Text List);

        /// \brief Adds a record to a list, before the one at an index.
        ///
        /// \param List  The path of the list, as `Party` or `Party[2].Bag`.
        /// \param Index The index the record is put at, held to the count of records.
        /// \return The record, or `nullptr` when the path names a record that does not exist.
        Ptr<Model> Insert(Text List, UInt32 Index);

        /// \brief Removes a record from a list, the ones after it moving down an index.
        ///
        /// \param List  The path of the list, as `Party` or `Party[2].Bag`.
        /// \param Index The index of the record removed, where one the list does not hold removes nothing.
        void Erase(Text List, UInt32 Index);

        /// \brief Adds records to the end of a list or removes them from it, until it holds a count.
        ///
        /// \param List  The path of the list, as `Party` or `Party[2].Bag`.
        /// \param Count The count of records the list holds after.
        void Resize(Text List, UInt32 Count);

        /// \brief Gets the count of records a list holds.
        ///
        /// \param List The path of the list, as `Party` or `Party[2].Bag`.
        /// \return The count, nought when there is no such list.
        UInt32 GetCount(Text List) const;

        /// \brief Gets a record of a list.
        ///
        /// \param List  The path of the list, as `Party` or `Party[2].Bag`.
        /// \param Index The index of the record.
        /// \return The record, or `nullptr` when there is no such record.
        Ptr<Model> GetRecord(Text List, UInt32 Index);

        /// \brief Keeps a list matched to a source, record for record by key, and fills each record from its item.
        ///
        /// \param List   The path of the list, as `Party` or `Party[2].Bag`.
        /// \param Source The items, in the order the list shows them, as any container read by `GetSize` and index.
        /// \param KeyOf  The code giving an item's key, an integer that names it for as long as it stands.
        /// \param Fill   The code writing an item into its record, called as `Fill(Record, Item)`.
        template<typename Container, typename Keying, typename Filling>
        void Mirror(Text List, ConstRef<Container> Source, AnyRef<Keying> KeyOf, AnyRef<Filling> Fill)
        {
            // The keys are gathered in the root, which a list nested in a record's fill finds free again by then.
            Ref<Sequence<UInt64>> Keys = mRoot->mKeys;
            Keys.Clear();

            for (UInt Index = 0; Index < Source.GetSize(); ++Index)
            {
                Keys.Append(static_cast<UInt64>(KeyOf(Source[Index])));
            }

            if (const Ptr<Entry> Found = Arrange(List))
            {
                for (UInt Index = 0; Index < Source.GetSize(); ++Index)
                {
                    Fill(* Found->Records[Index], Source[Index]);
                }
            }
        }

        /// \brief Gets the key a record was given by \ref Mirror.
        ///
        /// \return The key, nought for a record made otherwise.
        ZY_INLINE UInt64 GetKey() const
        {
            return mKey;
        }

        /// \brief Gets the key of a record of a list.
        ///
        /// \param List  The path of the list, as `Party` or `Party[2].Bag`.
        /// \param Index The index of the record.
        /// \return The key, nought when there is no such record or it was made otherwise.
        UInt64 GetKey(Text List, UInt32 Index) const;

        /// \brief Finds the record of a list holding a key.
        ///
        /// \param List The path of the list, as `Party` or `Party[2].Bag`.
        /// \param Key  The key, as \ref Mirror gave it.
        /// \return The index of the first record holding it, or below nought when none does.
        SInt32 FindKey(Text List, UInt64 Key) const;

        /// \brief Gets the count of changes made to the root this model belongs to, or to anything beneath it.
        ///
        /// \return The count, which only grows.
        ZY_INLINE UInt32 GetRevision() const
        {
            return mRoot->mRevision;
        }

        /// \brief Finds the value a path reaches, and the newest stamp on the way to it.
        ///
        /// \param Where The path read from this model.
        /// \param Stamp The receiver of the newest stamp of the lists the path goes through and the value it ends on.
        /// \return The value, or `nullptr` when the path reaches none.
        ConstPtr<Datum> Resolve(ConstRef<Path> Where, Ref<UInt32> Stamp) const;

        /// \brief Sets the value a path reaches, stamping it only when it changed.
        ///
        /// \param Where The path read from this model, whose records must exist.
        /// \param Value The value stored there.
        void Assign(ConstRef<Path> Where, ConstRef<Datum> Value);

    private:

        /// \brief Represents one named value of a model, and the records it holds when it is a list.
        struct Entry final
        {
            /// The value, which a list holds as its count of records.
            Datum                   Value;

            /// The records, while the value is a list.
            Sequence<Unique<Model>> Records;

            /// The stamp of the change that last set it.
            UInt32                  Stamp = 0;

            /// The stamp of the newest change to it or to anything its records hold, never older than \ref Stamp.
            UInt32                  Deep  = 0;
        };

        /// \brief Constructs a record of a list.
        ///
        /// \param Root   The root the record belongs to, which counts its changes.
        /// \param Parent The model holding the list the record stands in.
        /// \param List   The identifier of the name of that list.
        Model(Ref<Model> Root, Ptr<Model> Parent, UInt64 List);

        /// \brief Sets a value of this model by the identifier of its name, stamping it only when it changed.
        ///
        /// \param Key   The identifier of the name.
        /// \param Value The value stored there, which may stand in this model.
        void Store(UInt64 Key, ConstRef<Datum> Value);

        /// \brief Finds the model holding the value a path ends on.
        ///
        /// \param Where The path read from this model.
        /// \return The model, or `nullptr` when a record on the way does not exist.
        Ptr<Model> Walk(ConstRef<Path> Where);

        /// \brief Stamps every list on the way up from this record to the root with a change made beneath it.
        ///
        /// \param Stamp The stamp of the change.
        void Spread(UInt32 Stamp);

        /// \brief Finds the entry of the list a path names, making it when there is none.
        ///
        /// \param List  The path of the list, as text.
        /// \param Owner The receiver of the model holding the list, which its new records point back at.
        /// \param Name  The receiver of the identifier of the name of the list.
        /// \return The entry, or `nullptr` when the path is not one or a record on the way does not exist.
        Ptr<Entry> Open(Text List, Ref<Ptr<Model>> Owner, Ref<UInt64> Name);

        /// \brief Finds the entry a path ends on, and the newest stamp on the way to it.
        ///
        /// \param Where The path read from this model.
        /// \param Stamp The receiver of the newest stamp of the lists the path goes through and the entry it ends on.
        /// \return The entry, or `nullptr` when the path reaches none.
        ConstPtr<Entry> Reach(ConstRef<Path> Where, Ref<UInt32> Stamp) const;

        /// \brief Finds the entry a path written as text ends on.
        ///
        /// \param Key The path, as text.
        /// \return The entry, or `nullptr` when the text is not a path or the path reaches none.
        ConstPtr<Entry> Seek(Text Key) const;

        /// \brief Orders the records of a list after the keys \ref Mirror gathered in the root.
        ///
        /// \param List The path of the list, as text.
        /// \return The entry of the list, holding a record for every key, or `nullptr` when the path is not one.
        Ptr<Entry> Arrange(Text List);

        /// \brief Stamps a list of this model after its records changed, and counts them in its value.
        ///
        /// \param List The entry of the list, which this model holds.
        void Touch(Ref<Entry> List);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ptr<Model>           mRoot;
        Ptr<Model>           mParent;
        UInt64               mList;
        Table<Digest, Entry> mEntries;
        Sequence<UInt64>     mKeys;
        Path                 mPath;
        UInt64               mKey;
        UInt32               mRevision;
    };
}