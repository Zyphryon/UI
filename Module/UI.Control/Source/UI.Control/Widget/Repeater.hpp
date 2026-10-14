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
#include "UI.Core/Tree.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Datum.hpp"
#include "UI.Data/Path.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Defines the code that fills one row of an item list, binding what it makes to the row's record.
    using Maker = Delegate<void(Element Row, Text Scope)>;

    /// \brief Represents a row an item list made, and the record it shows.
    struct Cell final
    {
        /// The element showing the record, placed by the list over its spacer.
        Handle Row;

        /// The position of the record in the list, or of the one it last showed while it is parked.
        UInt32 Record;

        /// `true` while the row is hidden and kept for a record coming into view.
        Bool   Parked;

        /// The path of the record the row's bindings read, kept for records named one by one.
        Path   Within;
    };

    /// \brief Represents a state each row wears while a value of its record reads as set.
    struct RowState final
    {
        /// The state the row wears, which a theme dresses it by.
        State Bit = State::None;

        /// The binding of the value read in the row's record, as `{Sown}` or `{!Usable}`.
        Str   Source;
    };

    /// \brief Represents an item list, with where its records come from, how a row is made, and the rows that exist.
    struct Items final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The code filling a row, or empty to fill it from a template.
        Maker              Make;

        /// The markup filling a row from a template, or `nullptr` for none.
        Ptr<Markup>        Reader;

        /// The name of the template a row is filled from.
        Str                Template;

        /// The node a row is filled from when written in place, or empty for the template; held apart, never moved.
        Unique<JsonValue>  Node;

        /// The path of the list as text, the scope it was read in included, as `Party[2].Bag`.
        Str                Base;

        /// The name of the style the list wears, its rows wearing it followed by `.Item`.
        Str                Style;

        /// The flowing panel as tall as every row together, which gives the scroll its range.
        Handle             Spacer;

        /// The rows that exist, in no order.
        Sequence<Cell>     Cells;

        /// The count of records the list holds, as the model last showed it.
        UInt32             Count      = 0;

        /// The position of the record picked, or below nought for none.
        SInt32             Selected   = -1;

        /// The columns asked for, or nought to fit as many as the width holds when a width is given, else one.
        UInt32             Columns    = 0;

        /// The height of a row, in pixels, or nought to measure the row showing the first record.
        Real32             ItemHeight = 0.0f;

        /// The width of a row in a grid, in pixels, or nought to share the width among the columns.
        Real32             ItemWidth  = 0.0f;

        /// The height the row showing the first record was measured at, or nought before it was.
        Real32             Measured   = 0.0f;

        /// The columns the last layout laid out.
        UInt32             Across     = 1;

        /// The height of a row and the room after it, as the last layout found them, or nought before.
        Real32             Stride     = 0.0f;

        /// The height of the room the rows are seen through, as the last layout found it.
        Real32             View       = 0.0f;

        /// `true` when control and shift pick several records together.
        Bool               Multiple   = false;

        /// The positions of every record picked, lowest first, the one at \ref Selected among them.
        Sequence<UInt32>   Picked;

        /// The position a pick made with shift reaches from, or below nought for none.
        SInt32             Anchor     = -1;

        /// `true` when a right press on a row picks its record before the menu opens, unless it is picked already.
        Bool               RightPick  = true;

        /// The states each row wears by a value of its record, bound when the row is made.
        Sequence<RowState> RowStates;

        /// `true` when the records are named one by one rather than counted from one list.
        Bool               Named      = false;

        /// The path of each record, in order, from the root, as `Scene[0].Children[2]`, for records named one by one.
        Sequence<Str>      Records;

        /// The paths of \ref Records, read once.
        Sequence<Path>     Paths;
    };

    /// \brief Represents an item list, a scrolling column or grid of template rows, of which only those in view exist.
    class Repeater final
    {
    public:

        /// The pick was asked to be renamed, as by `F2`; `Content` names its record and `Other` its row.
        static constexpr UInt64 kRename = Identify("Rename");

    public:

        /// \brief Creates an item list showing no record.
        ///
        /// \param Parent The element to hold it.
        /// \param Style  The name of the style it wears.
        /// \return The list, reporting \ref Signal::kChange, \ref Signal::kActivate and \ref kRename on a row.
        static Element Create(Element Parent, Text Style = "Repeater");

        /// \brief Creates an item list from a layout node, its scroll remembered under its `Name`.
        ///
        /// \param Reader The markup reading the layout, which must outlive the list.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the list is read from.
        /// \return The list, which reports \ref Signal::kChange when a record is picked.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Shows the records of a list of the model, every row made again.
        ///
        /// \param List   The item list to show the records in.
        /// \param Source The path of the list, as `Inventory`, read from the root when it opens with `/`.
        /// \param Scope  The path of the record the path is read from, or empty for the root.
        /// \return `true` if the path is valid, `false` otherwise.
        static Bool SetItems(Element List, Text Source, Text Scope = Text());

        /// \brief Fills each row with code, every row made again.
        ///
        /// \param List The item list whose rows are filled.
        /// \param Make The code filling a row, run once for each row made.
        static void SetTemplate(Element List, Maker Make);

        /// \brief Fills each row from a template, every row made again.
        ///
        /// \param List     The item list whose rows are filled.
        /// \param Reader   The markup knowing the template, which must outlive the list.
        /// \param Template The name of the template.
        static void SetTemplate(Element List, Ref<Markup> Reader, Text Template);

        /// \brief Fills each row from a node written in place, every row made again.
        ///
        /// \param List   The item list whose rows are filled.
        /// \param Reader The markup making the node, which must outlive the list.
        /// \param Node   The node each row is made from, copied by the list.
        static void SetTemplate(Element List, Ref<Markup> Reader, ConstRef<JsonValue> Node);

        /// \brief Changes the height of every row.
        ///
        /// \param List   The item list whose rows are sized.
        /// \param Height The height, in pixels, or nought to measure the row showing the first record.
        static void SetItemHeight(Element List, Real32 Height);

        /// \brief Changes the width of every row, for a grid.
        ///
        /// \param List  The item list whose rows are sized.
        /// \param Width The width, in pixels, or nought to share the list's width among its columns.
        static void SetItemWidth(Element List, Real32 Width);

        /// \brief Changes how many rows stand side by side, making the list a grid.
        ///
        /// \param List  The item list to lay out in columns.
        /// \param Count The count, or nought for as many as the width holds when a row width is given, else one.
        static void SetColumns(Element List, UInt32 Count);

        /// \brief Picks a record by its position, without reporting it.
        ///
        /// \param List  The item list to pick in.
        /// \param Index The position, counted from nought, or one below nought or past the records to pick none.
        static void SetIndex(Element List, SInt32 Index);

        /// \brief Gets the position of the record picked.
        ///
        /// \param List The item list the pick is read from.
        /// \return The position, counted from nought, or below nought when none is picked.
        static SInt32 GetIndex(Element List);

        /// \brief Gets the count of records the list shows.
        ///
        /// \param List The item list the count is read from.
        /// \return The count, as the model last showed it.
        static UInt32 GetCount(Element List);

        /// \brief Gets how many rows show a record, parked ones aside, which is what the list costs whatever its count.
        ///
        /// \param List The item list whose rows are counted.
        /// \return The count of rows showing a record.
        static UInt32 GetRealized(Element List);

        /// \brief Gets the row showing a record.
        ///
        /// \param List   The item list the row stands in.
        /// \param Record The position of the record.
        /// \return The row, or one naming nothing when the record is too far from view to have one.
        static Element GetRow(Element List, UInt32 Record);

        /// \brief Gets the record a row shows.
        ///
        /// \param List The item list the row stands in.
        /// \param Row  The row, or any element it holds.
        /// \return The position of the record, or below nought when the element stands in no row of the list.
        static SInt32 GetRecord(Element List, Element Row);

        /// \brief Gets the key of the record a row shows, as \ref Model::Mirror gave it.
        ///
        /// \param List The item list the row stands in.
        /// \param Row  The row, or any element it holds, as the control an action heard.
        /// \return The key, nought when the element stands in no row or the record was given none.
        static UInt64 GetKey(Element List, Element Row);

        /// \brief Scrolls the list just enough to bring a record into view.
        ///
        /// \param List   The item list to scroll.
        /// \param Record The position of the record.
        static void ScrollTo(Element List, UInt32 Record);

        /// \brief Shows records named one by one instead of a list's, counted alone, so never with \ref SetItems.
        ///
        /// \param List    The item list to show the records in.
        /// \param Records The path of each record from the root, in order, as `Scene[0].Children[2]`.
        static void SetRecords(Element List, Sequence<Str> Records);

        /// \brief Gets the path of a record of the list, from the root.
        ///
        /// \param List   The item list holding the record.
        /// \param Record The position of the record.
        /// \return The path, as `Inventory[3]`, or empty when the list holds no such record.
        static Str GetScope(Element List, UInt32 Record);

        /// \brief Lets control and shift pick several records together, and control and `A` pick them all.
        ///
        /// \param List    The item list to pick in.
        /// \param Enabled The flag, `true` to pick several, `false` to pick one at a time.
        static void SetMultiple(Element List, Bool Enabled);

        /// \brief Checks whether the list picks several records together.
        ///
        /// \param List The item list to check.
        /// \return `true` if it does, `false` otherwise.
        static Bool IsMultiple(Element List);

        /// \brief Adds a record to the records picked, or takes it away, without reporting it.
        ///
        /// \param List    The item list to pick in.
        /// \param Record  The position of the record.
        /// \param Enabled The flag, `true` to pick it, `false` to let it go.
        static void SetPicked(Element List, UInt32 Record, Bool Enabled);

        /// \brief Checks whether a record is picked.
        ///
        /// \param List   The item list the pick is read from.
        /// \param Record The position of the record.
        /// \return `true` if it is, `false` otherwise.
        static Bool IsPicked(Element List, UInt32 Record);

        /// \brief Gets the positions of every record picked.
        ///
        /// \param List The item list the pick is read from.
        /// \return The positions, lowest first, of the records the list holds.
        static Sequence<UInt32> GetPicked(Element List);

        /// \brief Lets a right press pick a row's record before its context menu, keeping a pick it is already in.
        ///
        /// \param List    The item list to pick in.
        /// \param Enabled The flag, `true` to pick by a right press, `false` to leave the pick to the left button.
        static void SetRightPick(Element List, Bool Enabled);

        /// \brief Turns a state on rows while a record's value reads set, the state declared by \ref States::Declare.
        ///
        /// \param List The item list whose rows wear the state.
        /// \param Bit  The state, as \ref Marked or \ref State::Disabled.
        /// \param Key  The name of the value in each record, as `Sown`, or `!Usable` to wear it while clear.
        static void AddRowState(Element List, State Bit, Text Key);

        /// \brief Reads the shared list fields of a node, as `RightPick`, `RowStates` and `Name`.
        ///
        /// \param Reader The markup reading the layout, whose scope the name is remembered within.
        /// \param List   The item list read into, as a grid's or a tree view's list.
        /// \param Node   The layout node of the widget holding the list.
        static void BuildRows(Ref<Markup> Reader, Element List, ConstRef<JsonValue> Node);

        /// \brief Makes the interface remember a list's scroll under a key, restoring it once its rows reach that far.
        ///
        /// \param List The item list remembered.
        /// \param Key  The key, unique among what the interface remembers, or empty to remember it no more.
        static void Remember(Element List, Text Key);

        /// \brief Gets the state of a row its record marks, declaring it the first time.
        ///
        /// \return The state a theme dresses a marked row by, as an entry among several being scattered.
        static State Marked();

        /// \brief Takes the path out of a binding naming one list alone, as `{Inventory}`.
        ///
        /// \param Source The text of the binding, as a layout writes it.
        /// \param Output The variable receiving the path between the braces.
        /// \return `true` if the text is one path in braces and nothing more, `false` otherwise.
        static Bool Unwrap(Text Source, Ref<Text> Output);

        /// \brief Gets the path of a list from the root, as \ref SetItems reads it.
        ///
        /// \param Source The path of the list, as `Inventory`, read from the root when it opens with `/`.
        /// \param Scope  The path of the record the path is read from, or empty for the root.
        /// \return The path from the root, as `Party[2].Inventory`.
        static Str Resolve(Text Source, Text Scope);

        /// \brief Makes a row's content from a node written in place or a template, its bindings reading one record.
        ///
        /// \param Reader   The markup making the content.
        /// \param Parent   The element the content is made in.
        /// \param Scope    The path of the record from the root, as `Inventory[3]`.
        /// \param Node     The node the content is made from, or `nullptr` to make it from the template.
        /// \param Template The name of the template the content is made from when no node is given.
        static void Stamp(Ref<Markup> Reader, Element Parent, Text Scope, ConstPtr<JsonValue> Node, Text Template);

    private:

        /// The lines of rows kept past each edge of the view, so a short scroll finds its rows made already.
        static constexpr UInt32 kMargin = 2;

        /// The height an item list starts with, in pixels, since one sized by its rows would make every one.
        static constexpr Real32 kHeight = 200.0f;

        /// \brief Represents where the rows of an item list stand, as the last layout and the scroll left it.
        struct Grid final
        {
            /// The distance of the first row from the list's left edge, in pixels.
            Real32 Left;

            /// The distance of the first row from the list's top edge, unscrolled, in pixels.
            Real32 Top;

            /// The width of a row, in pixels.
            Real32 Width;

            /// The height of a row, in pixels, or nought before it was measured.
            Real32 Pitch;

            /// The room between rows, across and down, in pixels.
            Real32 Gap;

            /// How far the rows are scrolled, in pixels.
            Real32 Offset;

            /// The height of the room the rows are seen through, in pixels.
            Real32 View;

            /// The count of rows side by side.
            UInt32 Columns;
        };

        /// \brief Runs a list, where a click picks a row's record, a double press activates it, and keys move the pick.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event the list is offered.
        /// \param Self    The handle of the list.
        /// \return `true` if the event picked or moved, or an activation it sent was stopped, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Reports a row's record activated or asked to be renamed, naming the record and the row.
        ///
        /// \param Owner  The interface it stands in.
        /// \param List   The list holding the record.
        /// \param Record The position of the record.
        /// \param Kind   The signal sent, \ref Signal::kActivate or \ref kRename.
        /// \return `true` if a handler stopped it, `false` otherwise.
        static Bool Announce(Ref<Interface> Owner, Element List, UInt32 Record, UInt64 Kind);

        /// \brief Finds the record of the row an element stands in, unless a list nearer to it holds it.
        ///
        /// \param List   The list the row stands in.
        /// \param Target The element, as what a press landed on.
        /// \return The position of the record, or below nought when no row of this list holds the element.
        static SInt32 Seat(Element List, Handle Target);

        /// \brief Moves the pick by a key, or activates or renames it by enter or `F2`.
        ///
        /// \param Owner The interface it stands in.
        /// \param List  The list whose pick moves.
        /// \param Key   The key pressed.
        /// \return `true` if the key moved the list or its activation or rename was stopped, `false` otherwise.
        static Bool OnKey(Ref<Interface> Owner, Element List, ZyInput::Key Key);

        /// \brief Makes the rows in view as the last layout left it, handing rows leaving the view to records entering.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The handle of the list.
        /// \return `true` if a row was made, handed over, moved or destroyed, `false` otherwise.
        static Bool OnRealize(Ref<Interface> Owner, Handle Self);

        /// \brief Takes the count of records from the list of the model.
        ///
        /// \param Target The list taking the count.
        /// \param Value  The list of the model, read by its count of records.
        static void OnItems(Element Target, ConstRef<Datum> Value);

        /// \brief Picks a record, brings it into view and reports it, when it is not picked already.
        ///
        /// \param Owner The interface it stands in.
        /// \param List  The list to pick in.
        /// \param Index The position of the record.
        static void Choose(Ref<Interface> Owner, Element List, SInt32 Index);

        /// \brief Picks a record as the pointer or the keyboard asks, brings it into view and reports a changed pick.
        ///
        /// \param Owner  The interface it stands in.
        /// \param List   The list to pick in.
        /// \param Index  The position of the record.
        /// \param Toggle The flag, `true` to add the record to the pick or take it away, `false` to replace the pick.
        /// \param Extend The flag, `true` to pick every record from the anchor to it, `false` to pick only it.
        static void Gather(Ref<Interface> Owner, Element List, SInt32 Index, Bool Toggle, Bool Extend);

        /// \brief Marks every row picked or not, as the records it shows are.
        ///
        /// \param List The list whose rows are marked.
        static void Mark(Element List);

        /// \brief Works out where the rows stand, keeping what the keyboard and scrolling read of it.
        ///
        /// \param List The list to survey.
        /// \return Where the rows stand.
        static Grid Survey(Element List);

        /// \brief Measures the row showing the first record, once it was laid out, when no height is given.
        ///
        /// \param List The list whose row is measured.
        static void Measure(Element List);

        /// \brief Makes a row for a record and fills it.
        ///
        /// \param List   The list to make the row in.
        /// \param Record The position of the record.
        static void Make(Element List, UInt32 Record);

        /// \brief Fills a row from the list's code or template, bound to a record.
        ///
        /// \param List   The list the row stands in.
        /// \param Row    The row to fill.
        /// \param Record The position of the record.
        static void Fill(Element List, Element Row, UInt32 Record);

        /// \brief Hands a row to another record, moving its bindings there, and shows it again when it was parked.
        ///
        /// \param List   The list the row stands in.
        /// \param Index  The index of the row's cell.
        /// \param Record The position of the record.
        static void Rebind(Element List, UInt Index, UInt32 Record);

        /// \brief Hides the rows no record in view needs, keeping as many as a view may need and destroying the rest.
        ///
        /// \param List  The list the rows stand in.
        /// \param Free  The indices of the cells no record in view needs, lowest first.
        /// \param Limit The most rows a view may need.
        /// \param Gone  The variable receiving the rows to destroy once nothing more is read of the list's facet.
        /// \return `true` if a row was hidden or taken out, `false` otherwise.
        static Bool Park(Element List, ConstRef<Sequence<UInt>> Free, UInt Limit, Ref<Sequence<Handle>> Gone);

        /// \brief Places a row where its record stands, and marks it picked or not.
        ///
        /// \param List   The list the row stands in.
        /// \param Entry  The row's cell.
        /// \param Layout The place of the rows.
        /// \return `true` if the row moved or changed its size, `false` otherwise.
        static Bool Place(Element List, ConstRef<Cell> Entry, ConstRef<Grid> Layout);

        /// \brief Gets the path of a record of the list, as the list's binding reads it now.
        ///
        /// \param List   The list holding the record.
        /// \param Record The position of the record.
        /// \param Output The variable receiving the path, its last step naming the record.
        /// \return `true` if the list is bound to one, `false` otherwise.
        static Bool Locate(Element List, UInt32 Record, Ref<Path> Output);

        /// \brief Destroys every row, so the next update makes them again.
        ///
        /// \param List The list whose rows are destroyed.
        static void Reset(Element List);

        /// \brief Checks whether two lengths come to the same size, so a row standing where it should is left alone.
        ///
        /// \param First  The length the row stands at.
        /// \param Second The length the row should stand at.
        /// \return `true` if they do, `false` otherwise.
        static Bool Same(ConstRef<Length> First, ConstRef<Length> Second);

        /// \brief Checks whether two paths take the same steps.
        ///
        /// \param First  The path the row's bindings read.
        /// \param Second The path they should read.
        /// \return `true` if they do, `false` otherwise.
        static Bool Same(ConstRef<Path> First, ConstRef<Path> Second);

        /// \brief Checks whether an element stands in a row, or is it.
        ///
        /// \param Owner   The tree the row stands in.
        /// \param Row     The row the element may stand in.
        /// \param Element The element, or one naming nothing.
        /// \return `true` if it does, `false` otherwise.
        static Bool Holds(ConstRef<Tree> Owner, Handle Row, Handle Element);
    };
}