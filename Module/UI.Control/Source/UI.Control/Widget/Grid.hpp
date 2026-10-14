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
#include "UI.Data/Model.hpp"
#include "UI.Data/Path.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Defines the code that orders two records of a grid by a column.
    using Comparer = Delegate<SInt32(ConstRef<Model> First, ConstRef<Model> Second, Text Key)>;

    /// \brief Represents a column of a grid, with its heading, value, width and how its cells are made.
    struct Column final
    {
        /// The text of its heading, which may bind values or be the key of a text of the lexicon.
        Str               Title;

        /// The name of the value of each record it shows and sorts by, as `Name`.
        Str               Key;

        /// The width it keeps, in pixels, or nought to share the room the other columns leave.
        Real32            Width     = 0.0f;

        /// The share it takes of the room the columns keeping a width leave, against the others sharing it.
        Real32            Share     = 1.0f;

        /// The least width it is given, by its share or by a drag, in pixels.
        Real32            Minimum   = 40.0f;

        /// The side of each cell its text sits at.
        Align             Alignment = Align::Start;

        /// `true` when a click on its heading sorts the grid by it.
        Bool              Sortable  = true;

        /// The name of the template each cell is made from, or empty for a label showing the value.
        Str               Template;

        /// The node each cell is made from when written in place, or empty to use the template's name.
        Unique<JsonValue> Node;
    };

    /// \brief Represents a grid, with its columns, the list its rows come from, and how they are sorted.
    struct Ledger final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The columns, in the order they stand.
        Sequence<Column> Columns;

        /// The markup making a cell from a template, or `nullptr` for none.
        Ptr<Markup>      Reader     = nullptr;

        /// The path of the list the rows show, from the root, as `Entities`.
        Str              Base;

        /// The steps of \ref Base, read once, so the grid looks its list up without reading the text again.
        Path             Where;

        /// The style the grid wears, its parts wearing it followed by theirs.
        Str              Style;

        /// The row holding a heading per column.
        Handle           Header;

        /// The item list holding the rows.
        Handle           List;

        /// The column the rows are sorted by, or below nought for the model's order.
        SInt32           Sorted     = -1;

        /// `true` when the rows run from the last value to the first.
        Bool             Descending = false;

        /// The name of the value records are grouped by ahead of the sorted column, as `Folder`, or empty for none.
        Str              Group;

        /// `true` when the group whose value reads highest, as a set flag, comes first, `false` when the lowest does.
        Bool             GroupDown  = true;

        /// The code ordering two records, or empty to order them by their values.
        Comparer         Compare;

        /// The record of the list each row shows, in order.
        Sequence<UInt32> Order;

        /// The model the rows were last ordered from, so one put in its place orders them again.
        Ptr<Model>       Source     = nullptr;

        /// The model's count of changes when the grid last looked at its list.
        UInt32           Revision   = 0;

        /// The stamp of the newest change to the list or to what its records hold when the rows were last ordered.
        UInt32           Deep       = 0;

        /// `true` when the rows are to be ordered again at the next update.
        Bool             Stale      = true;

        /// The width each column was last laid out at, in pixels.
        Sequence<Real32> Widths;

        /// The distance the press on a column's grip landed left of its heading's right edge, in pixels.
        Real32           Grab       = 0.0f;
    };

    /// \brief Represents the grid, a table of the records of a model's list, a row per record and a column per value.
    class Grid final
    {
    public:

        /// The height a grid starts with, in pixels, since one sized by its rows would make every one.
        static constexpr Real32 kHeight = 200.0f;

        /// The width of the grip at the right edge of each heading, in pixels.
        static constexpr Real32 kGrip   = 6.0f;

        /// The event reported when the column the grid is sorted by, or the way it runs, changes.
        static constexpr UInt64 kSort   = Identify("Sort");

        /// \brief Creates a grid holding no column.
        ///
        /// \param Parent The element the grid is added to.
        /// \param Style  The name of the style the grid wears.
        /// \return The grid, reporting \ref Signal::kChange, \ref Grid::kSort, \ref Signal::kActivate and row renames.
        static Element Create(Element Parent, Text Style = "Grid");

        /// \brief Creates a grid from a layout node, with its `Items`, `Columns`, sorting, grouping and `Selected`.
        ///
        /// \param Reader The markup reading the layout, which must outlive the grid, since rows are made as it scrolls.
        /// \param Parent The element the grid is added to.
        /// \param Node   The layout node the grid is read from.
        /// \return The grid, showing the columns the node lists.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Adds a column after the last, every row made again.
        ///
        /// \param Owner  The grid the column is added to.
        /// \param Entry  The column added, whose heading and cells are made from it.
        /// \param Reader The markup making its cells from a template, or `nullptr` when it names none.
        static void AddColumn(Element Owner, Column Entry, Ptr<Markup> Reader = nullptr);

        /// \brief Shows the records of a list of the model.
        ///
        /// \param Owner  The grid whose rows are set.
        /// \param Source The path of the list, as `Entities`, read from the root when it opens with `/`.
        /// \param Scope  The path of the record the path is read from, or empty for the root.
        static void SetItems(Element Owner, Text Source, Text Scope = Text());

        /// \brief Gets how many columns a grid holds.
        ///
        /// \param Owner The grid whose columns are counted.
        /// \return The count of columns.
        static UInt32 GetCount(Element Owner);

        /// \brief Gives a column a width it keeps, or lets it share the room left.
        ///
        /// \param Owner The grid holding the column.
        /// \param Index The index of the column sized.
        /// \param Width The width, in pixels, held to the column's least, or nought to share the room left.
        static void SetWidth(Element Owner, UInt32 Index, Real32 Width);

        /// \brief Gets the width a column was last laid out at.
        ///
        /// \param Owner The grid holding the column.
        /// \param Index The index of the column measured.
        /// \return The width, in pixels, or nought before the first update.
        static Real32 GetWidth(Element Owner, UInt32 Index);

        /// \brief Sorts the rows by a column, without reporting it.
        ///
        /// \param Owner      The grid whose rows are sorted.
        /// \param Index      The index of the column sorted by, or below nought for the model's order.
        /// \param Descending The flag, `true` to run from the last value to the first, `false` the other way.
        static void SetSort(Element Owner, SInt32 Index, Bool Descending = false);

        /// \brief Gets the column the rows are sorted by.
        ///
        /// \param Owner The grid whose order is read.
        /// \return The index of the column, or below nought for the model's order.
        static SInt32 GetSort(Element Owner);

        /// \brief Checks whether the rows run from the last value to the first.
        ///
        /// \param Owner The grid whose order is read.
        /// \return `true` if they do, `false` otherwise.
        static Bool IsDescending(Element Owner);

        /// \brief Orders the rows by code of the game's own rather than by their values.
        ///
        /// \param Owner   The grid whose rows are ordered.
        /// \param Compare The code ordering two records, or empty to order by the values.
        static void SetComparer(Element Owner, Comparer Compare);

        /// \brief Groups records by a value ahead of the sorted column, as folders ahead of files, silently.
        ///
        /// \param Owner      The grid whose rows are grouped.
        /// \param Key        The name of the value the rows are grouped by, as `Folder`, or empty to group none.
        /// \param Descending The flag, `true` to put the highest first, as a set flag does, `false` the lowest.
        static void SetGroup(Element Owner, Text Key, Bool Descending = true);

        /// \brief Orders the rows again at the next update, as when a value a game's comparer reads changed.
        ///
        /// \param Owner The grid whose rows are ordered again.
        static void Refresh(Element Owner);

        /// \brief Lets control and shift pick several rows together.
        ///
        /// \param Owner   The grid whose pick is set up.
        /// \param Enabled The flag, `true` to pick several, `false` to pick one at a time.
        static void SetMultiple(Element Owner, Bool Enabled);

        /// \brief Picks the row of a record alone, without reporting it.
        ///
        /// \param Owner  The grid whose row is picked.
        /// \param Record The position of the record in the model's list, or below nought to pick none.
        static void Select(Element Owner, SInt32 Record);

        /// \brief Gets the record of the row picked last.
        ///
        /// \param Owner The grid whose pick is read.
        /// \return The position of the record in the model's list, or below nought when none is picked.
        static SInt32 GetSelected(Element Owner);

        /// \brief Gets the record of every row picked.
        ///
        /// \param Owner The grid whose pick is read.
        /// \return The positions of the records in the model's list, in the order the rows stand.
        static Sequence<UInt32> GetSelection(Element Owner);

        /// \brief Gets the record a row shows, whichever way the grid is sorted.
        ///
        /// \param Owner    The grid holding the row.
        /// \param Position The position of the row, counted from the top.
        /// \return The position of the record in the model's list, or below nought past the last row.
        static SInt32 GetRecord(Element Owner, UInt32 Position);

        /// \brief Gets the row showing a record.
        ///
        /// \param Owner  The grid holding the row.
        /// \param Record The position of the record in the model's list.
        /// \return The row, or one naming nothing when the record is too far from view to have one.
        static Element GetRow(Element Owner, UInt32 Record);

        /// \brief Gets the heading of a column.
        ///
        /// \param Owner The grid holding the column.
        /// \param Index The index of the column.
        /// \return The heading, or one naming nothing past the last column.
        static Element GetHeading(Element Owner, UInt32 Index);

        /// \brief Gets the item list holding the rows.
        ///
        /// \param Owner The grid holding the list.
        /// \return The item list, a \ref Repeater of the rows.
        static Element GetList(Element Owner);

    private:

        /// \brief Represents how two records of a grid are weighed against each other.
        struct Rule final
        {
            /// The code ordering two records by the column, or empty to order them by their values.
            Comparer Code;

            /// The name of the value sorted by, or empty when no column sorts the rows.
            Str      Key;

            /// `true` when the column runs from the last value to the first.
            Bool     Descending = false;

            /// The name of the value the records are grouped by first, or empty for none.
            Str      Group;

            /// `true` when the group reading highest comes first.
            Bool     GroupDown  = true;
        };

        /// \brief Runs a grid, where a heading click sorts, a grip drag sizes a column, and list events pass on.
        ///
        /// \param Owner   The interface the grid stands in.
        /// \param Message The event the grid receives.
        /// \param Self    The grid receiving the event.
        /// \return `true` if the event sorted, sized or was reported again, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Orders the rows again when the grid's own list changed, and lays the columns out for the last room.
        ///
        /// \param Owner The interface the grid stands in.
        /// \param Self  The grid to order and lay out.
        /// \return `true` if a row or a width changed, `false` otherwise.
        static Bool OnRealize(Ref<Interface> Owner, Handle Self);

        /// \brief Fills a row of the list, a cell per column.
        ///
        /// \param Row   The row filled, standing in the grid's list.
        /// \param Scope The path of the record the row shows.
        static void OnMake(Element Row, Text Scope);

        /// \brief Adds a column out of an entry of a node's `Columns`.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Owner  The grid the column is added to.
        /// \param Node   The entry of `Columns` the column is read from.
        static void BuildColumn(Ref<Markup> Reader, Element Owner, ConstRef<JsonValue> Node);

        /// \brief Makes the heading of a column, its title bound when it names values or a text of the lexicon.
        ///
        /// \param Owner The grid the heading is made for.
        /// \param Index The index of the column the heading names.
        /// \param Scope The path of the record a bound title is read from, or empty for the root.
        static void MakeHeading(Element Owner, UInt32 Index, Text Scope);

        /// \brief Orders the rows by the column sorted by, keeping every row picked picked.
        ///
        /// \param Owner The grid whose rows are ordered.
        /// \return `true` if the order changed, `false` otherwise.
        static Bool Arrange(Element Owner);

        /// \brief Lays each column out at its width for the room the grid's content has, the headings and every cell.
        ///
        /// \param Owner The grid laid out.
        /// \return `true` if a width changed, `false` otherwise.
        static Bool Widen(Element Owner);

        /// \brief Gets the width a column is laid out at.
        ///
        /// \param Entry  The column measured.
        /// \param Left   The room the columns keeping a width leave, in pixels.
        /// \param Shares The shares of every column sharing that room, together.
        /// \return The width, in whole pixels, never below the column's least.
        static Real32 Measure(ConstRef<Column> Entry, Real32 Left, Real32 Shares);

        /// \brief Gives every cell of a row the width of its column.
        ///
        /// \param Row    The row whose cells are sized.
        /// \param Widths The width of each column, in pixels.
        static void Fit(Element Row, ConstRef<Sequence<Real32>> Widths);

        /// \brief Shows the sort mark on the heading sorted by and hides the others.
        ///
        /// \param Owner The grid whose headings are marked.
        static void Mark(Element Owner);

        /// \brief Sorts by a column, the other way when it is sorted by already, and reports it.
        ///
        /// \param Owner The interface the grid stands in.
        /// \param Root  The grid sorted.
        /// \param Index The index of the column whose heading was clicked.
        static void Toggle(Ref<Interface> Owner, Element Root, UInt32 Index);

        /// \brief Orders two records of the list by a column's values.
        ///
        /// \param First  The record weighed first.
        /// \param Second The record weighed against it.
        /// \param Key    The name of the value.
        /// \return Below nought when the first sorts first, above nought when the second does, nought on a tie.
        static SInt32 Weigh(ConstRef<Model> First, ConstRef<Model> Second, Text Key);

        /// \brief Checks whether one record sorts before another, by group, then column, then the model's order.
        ///
        /// \param Records The records of the list, by their position in it.
        /// \param Order   The way the records are weighed.
        /// \param First   The position of the record asked about.
        /// \param Second  The position of the record it is weighed against.
        /// \return `true` if the first sorts before the second, `false` otherwise.
        static Bool Precedes(ConstRef<Sequence<Ptr<Model>>> Records, ConstRef<Rule> Order, UInt32 First, UInt32 Second);

        /// \brief Finds the grid an element stands in.
        ///
        /// \param Inside The element whose grid is looked for, such as a row of it.
        /// \return The grid, or one naming nothing.
        static Element Seek(Element Inside);
    };
}