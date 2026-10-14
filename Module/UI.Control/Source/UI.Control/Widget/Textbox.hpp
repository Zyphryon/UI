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

#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Text/Block.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Markup/Markup.hpp"
#include <Zyphryon.Math/Range.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents what a text field takes and where its caret and selection stand.
    struct Entry final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The text the field held when it took the keyboard or last committed, which escape puts back.
        Str    Original;

        /// The most characters typing leaves in the field, or nought for no limit.
        UInt32 Limit     = 0;

        /// The place the caret stands at, as a byte of the text.
        UInt32 Caret     = 0;

        /// The other end of the selection, as a byte of the text, the caret's own place while nothing is selected.
        UInt32 Anchor    = 0;

        /// How far along its line the caret aims as it moves up and down, in pixels, negative while it aims nowhere.
        Real32 Goal      = -1.0f;

        /// `true` when the field holds several lines, which wrap and scroll up and down.
        Bool   Multiline = false;

        /// `true` while a press of the pointer drags the selection out.
        Bool   Selecting = false;

        /// `true` from the first change since the field took the keyboard or committed, until it commits or reverts.
        Bool   Editing   = false;

        /// `true` when nothing typed, cut or pasted changes the text, which can still be selected and copied.
        Bool   ReadOnly  = false;

        /// `true` when typing and pasting keep only digits, minus signs and points.
        Bool   Numeric   = false;
    };

    /// \brief Represents a text field of one or several lines of typed text, with a caret, a selection and a hint.
    class Textbox final
    {
    public:

        /// The height a field of several lines starts at, in pixels.
        static constexpr Real32 kHeight = 96.0f;

    public:

        /// \brief Creates a text field holding one line.
        ///
        /// \param Parent  The element to hold it.
        /// \param Content The text it starts with, the caret after it.
        /// \param Hint    The text shown while it is empty.
        /// \param Style   The name of the style it wears, its parts adding `.Text`, `.Caret`, `.Hint` and `.Selection`.
        /// \return The text field, which reports \ref Signal::kChange on every edit made in it.
        static Element Create(Element Parent, Text Content = Text(), Text Hint = Text(), Text Style = "Textbox");

        /// \brief Creates a text field from a node with `Text`, `Hint`, `Limit`, `Multiline`, `ReadOnly`, `Numeric`.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the text field is read from.
        /// \return The text field, which reports \ref Signal::kChange on every edit made in it.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Replaces a text field's text silently, the caret after it; the limit holds back typing only.
        ///
        /// \param Target  The text field whose text is replaced.
        /// \param Content The text it holds from now on.
        static void SetText(Element Target, Text Content);

        /// \brief Gets the text of a text field.
        ///
        /// \param Target The text field to read.
        /// \return The text, valid until the field is next edited.
        static Text GetText(Element Target);

        /// \brief Replaces the hint a text field shows while its text is empty.
        ///
        /// \param Target  The text field whose hint is replaced.
        /// \param Content The text shown while the field is empty.
        static void SetHint(Element Target, Text Content);

        /// \brief Sets whether a text field holds one line or several, several given \ref kHeight if none is set.
        ///
        /// \param Target  The text field to change.
        /// \param Enabled The flag, `true` to hold several lines, `false` to hold one.
        static void SetMultiline(Element Target, Bool Enabled);

        /// \brief Sets whether a text field can be changed, or only selected and copied from.
        ///
        /// \param Target  The text field to change.
        /// \param Enabled The flag, `true` to keep typing, cutting and pasting from changing it, `false` to let them.
        static void SetReadOnly(Element Target, Bool Enabled);

        /// \brief Sets whether a text field takes only what writes a number, as digits, minus signs and points.
        ///
        /// \param Target  The text field to change.
        /// \param Enabled The flag, `true` to drop every other character typed or pasted, `false` to take any.
        static void SetNumeric(Element Target, Bool Enabled);

        /// \brief Selects part of the text of a text field, or places its caret.
        ///
        /// \param Target The text field whose selection is set.
        /// \param Anchor The end of the selection that stays, as a byte of the text.
        /// \param Caret  The end the caret stands at, as a byte of the text; the same as the anchor selects nothing.
        static void Select(Element Target, UInt32 Anchor, UInt32 Caret);

        /// \brief Gets the place the caret of a text field stands at.
        ///
        /// \param Target The text field to read.
        /// \return The place, as a byte of the text, held within the text.
        static UInt32 GetCaret(Element Target);

        /// \brief Gets the text a text field has selected.
        ///
        /// \param Target The text field to read.
        /// \return The text, empty while nothing is selected, valid until the field is next edited.
        static Text GetSelected(Element Target);

    private:

        /// \brief Runs a text field, where typing replaces the selection, keys edit, enter commits and escape reverts.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching the field.
        /// \param Self    The text field the handler governs.
        /// \return `true` if the event was text typed into it or a key it keeps, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Places a focused field's caret and selection and scrolls the caret into view; others hide it.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The text field to place the parts of.
        /// \return `true` if a part moved, showed or hid, or the field scrolled, `false` otherwise.
        static Bool OnRealize(Ref<Interface> Owner, Handle Self);

        /// \brief Runs a key going down in a text field.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The text field the key goes down in.
        /// \param Key    The key going down.
        /// \param Edited The flag receiving `true` when the key changed the text.
        /// \return `true` if the field keeps the key, `false` otherwise.
        static Bool OnKey(Ref<Interface> Owner, Element Target, ZyInput::Key Key, Ref<Bool> Edited);

        /// \brief Commits a field's text, reporting \ref Signal::kCommit and closing the edit with \ref Signal::kEnd.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The text field to commit.
        /// \param Always The flag, `true` to report the commit even with the text unchanged, `false` otherwise.
        static void Commit(Ref<Interface> Owner, Element Target, Bool Always);

        /// \brief Puts back the text a field last committed or took the keyboard with, ending the edit and the focus.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The text field to revert.
        /// \return `true` if the text changed back, `false` if there was nothing to put back.
        static Bool Revert(Ref<Interface> Owner, Element Target);

        /// \brief Runs a left press in a text field, where one press places the caret, two select a word, three all.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Target  The text field pressed.
        /// \param Message The press, whose count tells one, two and three presses apart.
        static void OnPress(Ref<Interface> Owner, Element Target, ConstRef<Event> Message);

        /// \brief Types text in place of a field's selection, dropping control characters and what its limits refuse.
        ///
        /// \param Target  The text field typed into.
        /// \param Content The text typed, in UTF-8.
        /// \return `true` if the text changed, `false` otherwise.
        static Bool Insert(Element Target, Text Content);

        /// \brief Takes the selection of a text field away, or the character or word before or after the caret.
        ///
        /// \param Target The text field to erase from.
        /// \param Ahead  The flag, `true` to take what follows the caret, `false` to take what comes before it.
        /// \param Words  The flag, `true` to take a whole word, `false` to take a character.
        /// \return `true` if the text changed, `false` otherwise.
        static Bool Erase(Element Target, Bool Ahead, Bool Words);

        /// \brief Puts text in place of the selection of a text field, the caret standing after it.
        ///
        /// \param Target  The text field whose selection is replaced.
        /// \param Content The text, already clean and within the limit.
        /// \return `true` if the text changed, `false` otherwise.
        static Bool Replace(Element Target, Text Content);

        /// \brief Moves the caret of a text field.
        ///
        /// \param Target The text field whose caret moves.
        /// \param Offset The place the caret moves to, as a byte of the text.
        /// \param Extend The flag, `true` to select from where the selection started, `false` to select nothing.
        static void Move(Element Target, UInt Offset, Bool Extend);

        /// \brief Moves the caret of a text field of several lines up or down by lines, as near as it can to its aim.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The text field whose caret moves.
        /// \param Lines  The count of lines, negative to move up.
        /// \param Extend The flag, `true` to select from where the selection started, `false` to select nothing.
        static void Step(Ref<Interface> Owner, Element Target, SInt32 Lines, Bool Extend);

        /// \brief Reads where the text of a text field stands on screen, broken into the lines it shows.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The text field to read.
        /// \param Output The variable receiving the lines and where they stand.
        /// \return `true` if the field shows a text or a hint to read, `false` otherwise.
        static Bool Survey(Ref<Interface> Owner, Element Target, Ref<Block> Output);

        /// \brief Gets the place in the text of a text field nearest a point.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Target The text field to look in.
        /// \param Point  The point to look near, in pixels.
        /// \return The place, as a byte of the text, or the caret's when the field shows nothing to read.
        static UInt Locate(Ref<Interface> Owner, Element Target, Vector2 Point);

        /// \brief Places a part standing over a text field, by its distance from the field's corner.
        ///
        /// \param Part   The part to place, as the caret or a band of the selection.
        /// \param Corner The top left corner of the field, in pixels.
        /// \param Box    The place the part goes, in pixels.
        /// \param Sized  The flag, `true` to size the part to the box too, `false` to keep the size its style gives.
        /// \return `true` if the part moved or was sized, `false` otherwise.
        static Bool Put(Element Part, Vector2 Corner, Rect Box, Bool Sized);

        /// \brief Shows or hides a part of a text field.
        ///
        /// \param Part  The part to show or hide.
        /// \param Shown The flag, `true` to show it, `false` to hide it.
        /// \return `true` if it changed, `false` otherwise.
        static Bool Show(Element Part, Bool Shown);

        /// \brief Shows a text field's text while it holds any, and its hint while it is empty.
        ///
        /// \param Target The text field to refresh.
        static void Refresh(Element Target);

        /// \brief Gets the bytes the selection of a text field covers, held within its text.
        ///
        /// \param Data The entry of the text field, whose caret and anchor bound the selection.
        /// \param Size The size of the text, in bytes.
        /// \return The selection from its lower end to its upper, a single place while nothing is selected.
        static AnyRange<UInt> GetSelection(ConstRef<Entry> Data, UInt Size);

        /// \brief Checks whether a length stands at so many pixels, within a hundredth of a pixel.
        ///
        /// \param Value  The length to check.
        /// \param Pixels The count of pixels it should stand at.
        /// \return `true` if it does, `false` otherwise.
        static Bool Near(ConstRef<Length> Value, Real32 Pixels);
    };
}