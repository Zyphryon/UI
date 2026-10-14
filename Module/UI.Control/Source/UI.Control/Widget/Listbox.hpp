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
#include "UI.Core/Types.hpp"
#include "UI.Data/Datum.hpp"
#include "UI.Data/Path.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents what a list, or a widget keeping one, has picked, and the parts it is made of.
    struct Choice final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The item picked, or one naming nothing.
        Handle Selected;

        /// The list the items stand in, for a widget keeping one, as a combo box.
        Handle List;

        /// The root of the popup the list opens in, for a widget keeping one there.
        Handle Popup;

        /// The label showing the item picked, for a widget showing it.
        Handle Label;
    };

    /// \brief Represents where the items of a list come from when each stands for a record of a list of the model.
    struct Listing final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The path of the list of records from the root, as the items were first bound to it, as `Assets.Kinds`.
        Str    Source;

        /// The text each item shows, read in its record, as `{Name}` or `{Icon}  {Name}`.
        Str    Content;

        /// The flag hiding an item, read in its record, as `{!Allowed}`, or empty to hide none.
        Str    Hidden;

        /// The flag disabling an item, read in its record, as `{Taken}`, or empty to disable none.
        Str    Disabled;

        /// The count of records the items were made for.
        UInt32 Count = 0;
    };

    /// \brief Represents a list, a scrolling column of items of which one at a time is picked.
    class Listbox final
    {
    public:

        /// \brief Creates a list.
        ///
        /// \param Parent The element to hold it.
        /// \param Style  The name of the style it wears.
        /// \return The list, which reports \ref Signal::kChange when an item is picked.
        static Element Create(Element Parent, Text Style = "Listbox");

        /// \brief Creates a list from a layout node, holding its `Items` and picking `Selected`, both bindable.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the list is read from.
        /// \return The list, made as \ref Create makes one.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Adds the items a node lists, as texts or records of a model list in braces, picking the one it names.
        ///
        /// \param Reader The markup reading the layout, whose scope a list of the model is read in.
        /// \param Owner  The list or the widget to fill.
        /// \param Node   The layout node giving `Items` and `Selected`.
        static void Fill(Ref<Markup> Reader, Element Owner, ConstRef<JsonValue> Node);

        /// \brief Makes the items of a list stand for the records of a model list, again when their count changes.
        ///
        /// \param Owner   The list or the widget whose items follow the records, any item it held before going.
        /// \param Source  The path of the record list, as `Assets.Kinds`, read in the scope unless it opens with `/`.
        /// \param Content The text each item shows, read in its record, as `{Name}` or `{Icon}  {Name}`.
        /// \param Scope   The path of the record the path is read from (\ref Markup::GetScope), or empty for the root.
        /// \return `true` if the list binds, `false` otherwise.
        static Bool SetItems(Element Owner, Text Source, Text Content = "{Name}", Text Scope = Text());

        /// \brief Hides or disables the items standing for records by flags they hold, once \ref SetItems has run.
        ///
        /// \param Owner    The list or the widget whose items are filtered.
        /// \param Hidden   The flag hiding an item, read in its record, as `{!Allowed}`, or empty to hide none.
        /// \param Disabled The flag disabling an item, read in its record, as `{Taken}`, or empty to disable none.
        static void SetFilter(Element Owner, Text Hidden, Text Disabled = Text());

        /// \brief Adds an item to a list or a widget keeping one.
        ///
        /// \param Owner   The list or the widget to add the item to.
        /// \param Content The text of the item.
        /// \return The item, standing last in the list.
        static Element AddItem(Element Owner, Text Content);

        /// \brief Picks an item of a list or a widget keeping one, without reporting it.
        ///
        /// \param Owner The list or the widget to pick in.
        /// \param Item  The item to pick, or one naming nothing to pick none.
        static void Select(Element Owner, Element Item);

        /// \brief Gets the item a list or a widget keeping one has picked.
        ///
        /// \param Owner The list or the widget to look in.
        /// \return The item picked, or one naming nothing.
        static Element GetSelected(Element Owner);

        /// \brief Picks an item of a list or a widget keeping one by its position, without reporting it.
        ///
        /// \param Owner The list or the widget to pick in.
        /// \param Index The position of the item, counted from nought, or one past the items to pick none.
        static void SetIndex(Element Owner, SInt32 Index);

        /// \brief Gets the position of the item a list or a widget keeping one has picked.
        ///
        /// \param Owner The list or the widget to look in.
        /// \return The position, counted from nought, or below nought when none is picked.
        static SInt32 GetIndex(Element Owner);

        /// \brief Gets the text of an item.
        ///
        /// \param Item The item whose text is wanted.
        /// \return The text, or empty when it names nothing.
        static Text GetText(Element Item);

        /// \brief Gets the list a list or a widget keeping one keeps its items in.
        ///
        /// \param Owner The list or the widget to look in.
        /// \return The list holding the items, which is the owner itself for a plain list.
        static Element GetList(Element Owner);

        /// \brief Picks the item some places from the one picked, skipping hidden and disabled ones, and reports it.
        ///
        /// \param Owner  The interface it stands in.
        /// \param Self   The list or the widget, which hears the change.
        /// \param Offset The places to move, below nought to move up; with none picked, the first or last item is.
        /// \return `true` if another item was picked, `false` otherwise, as when the move would run past either end.
        static Bool Step(Ref<Interface> Owner, Handle Self, SInt32 Offset);

    private:

        /// \brief Makes the items again when the count of the records they stand for changed.
        ///
        /// \param Owner The list or the widget whose records are bound.
        /// \param Value The list of records, read as its count.
        static void OnItems(Element Owner, ConstRef<Datum> Value);

        /// \brief Makes an item for each record a list or a widget keeping one stands for, keeping the position picked.
        ///
        /// \param Owner The list or the widget whose items are made, every item it held before going.
        static void Remake(Element Owner);

        /// \brief Finds the path of a record a list's items stand for, as its own binding of the records reads it now.
        ///
        /// \param Owner  The list or the widget whose records are bound.
        /// \param Record The position of the record.
        /// \param Output The variable receiving the path of the record.
        /// \return `true` if the records are bound, `false` otherwise.
        static Bool Locate(Element Owner, UInt32 Record, Ref<Path> Output);

        /// \brief Checks whether two paths name the same record.
        ///
        /// \param First  The path of one record.
        /// \param Second The path of the other.
        /// \return `true` if they take the same steps, `false` otherwise.
        static Bool Same(ConstRef<Path> First, ConstRef<Path> Second);

        /// \brief Checks whether an item can be picked by the keyboard, as one that is neither hidden nor disabled.
        ///
        /// \param Item The item to check.
        /// \return `true` if it can, `false` otherwise.
        static Bool IsUsable(Element Item);

        /// \brief Gets the binding a field naming a key of a record stands for, as `{Name}` for `Name`.
        ///
        /// \param Field The field as the layout writes it, a key, a binding or a lexicon key.
        /// \return The binding, the field itself when it binds already, or empty for an empty field.
        static Str Phrase(Text Field);

        /// \brief Runs a list, where a click on an item picks it and the up and down arrows pick the one beside it.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching it.
        /// \param Self    The list hearing the event.
        /// \return `true` if the event was a click on one of its items or an arrow that picked one, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);
    };
}