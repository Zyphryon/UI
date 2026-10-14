// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the string tables of an interface, one per language, that text written `@Key` is read through.
    class Lexicon final
    {
    public:

        /// \brief Constructs a lexicon knowing no language, speaking one named nothing.
        Lexicon();

        /// \brief Reads the table of a language, which replaces the one it had only once all of it was read.
        ///
        /// \param Language The name of the language, as `English`.
        /// \param Source   The text of the table, a JSON object of texts and objects of more keys.
        /// \param Error    The receiver of why the table cannot be read.
        /// \return `true` if it was read, `false` otherwise.
        Bool Read(Text Language, Text Source, Ref<Str> Error);

        /// \brief Gives a key of a language its text, or replaces the text it had.
        ///
        /// \param Language The name of the language, as `English`.
        /// \param Key      The key, dots and all, as `Menu.Play`.
        /// \param Value    The text, which may name values in braces.
        void Set(Text Language, Text Key, Text Value);

        /// \brief Switches the language every text written `@Key` is read in.
        ///
        /// \param Language The name of the language, as `English`.
        void SetLanguage(Text Language);

        /// \brief Gets the language every text written `@Key` is read in.
        ///
        /// \return The name of the language, empty until one is switched to.
        ZY_INLINE Text GetLanguage() const
        {
            return mLanguage;
        }

        /// \brief Checks whether a language was read or given a text.
        ///
        /// \param Language The name of the language, as `English`.
        /// \return `true` if it was, `false` otherwise.
        Bool HasLanguage(Text Language) const;

        /// \brief Gets the languages read or given a text, in the order they first were.
        ///
        /// \return The names of the languages.
        ZY_INLINE ConstSpan<Str> GetLanguages() const
        {
            return mNames;
        }

        /// \brief Checks whether the language spoken gives a key a text.
        ///
        /// \param Key The key, dots and all, as `Menu.Play`.
        /// \return `true` if it does, `false` otherwise.
        Bool Has(Text Key) const;

        /// \brief Gets the text the language spoken gives a key.
        ///
        /// \param Key The key, dots and all, as `Menu.Play`.
        /// \return The text, or the key itself when the language gives it none.
        Text Translate(Text Key);

        /// \brief Gets a count that moves on whenever the language spoken is switched, read or given a text.
        ///
        /// \return The count, which a binder compares to know when to read every text again.
        ZY_INLINE UInt32 GetVersion() const
        {
            return mVersion;
        }

    private:

        /// \brief Defines the texts of one language, found by the identifier of their key.
        using Phrases = Table<Digest, Str>;

        /// \brief Gets the table of a language to be written, noting a language it did not know.
        ///
        /// \param Language The name of the language.
        /// \return The table of the language, empty when it is new.
        Ref<Phrases> Open(Text Language);

    private:

        /// \brief Gathers the keys of an object of a table and of every object inside it.
        ///
        /// \param Prefix The name the keys are put after, with a dot, or empty for the top of the table.
        /// \param Node   The object whose keys are gathered.
        /// \param Output The receiver of the texts, added to those it held.
        /// \param Error  The receiver of why a value cannot be read.
        /// \return `true` if every value is text or an object, `false` otherwise.
        static Bool Gather(Text Prefix, ConstRef<JsonValue> Node, Ref<Phrases> Output, Ref<Str> Error);

        /// \brief Gathers one value of an object of a table, a text or an object of more keys.
        ///
        /// \param Key    The full key of the value, as `Menu.Play`.
        /// \param Value  The value gathered.
        /// \param Output The receiver of the text, or the texts of the object.
        /// \param Error  The receiver of why the value cannot be read.
        /// \return `true` if the value is text or an object of such values, `false` otherwise.
        static Bool Take(ConstRef<Str> Key, ConstRef<JsonValue> Value, Ref<Phrases> Output, Ref<Str> Error);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Table<Digest, Phrases> mTables;
        Sequence<Str>          mNames;
        Str                    mLanguage;
        UInt64                 mSpoken;
        Bag<Digest>            mMissing;
        UInt32                 mVersion;
    };
}