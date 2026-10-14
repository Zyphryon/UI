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
#include "UI.Core/Style/Theme.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Model.hpp"
#include "UI.Data/Path.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents one row of a tree view laid out, with the record it shows and where it stands in the tree.
    struct Limb final
    {
        /// The path of the record from the root, as `Scene[0].Children[2]`.
        Str    Path;

        /// How deep the record stands, nought for one of the top list.
        UInt16 Depth  = 0;

        /// `true` when the record holds children, or says it may.
        Bool   Parent = false;

        /// `true` while its children are shown under it.
        Bool   Open   = false;
    };

    /// \brief Specifies where a row dropped on another of a tree view lands against it.
    enum class Spot : UInt8
    {
        Before,     ///< Above it, a sibling coming first.
        Inside,     ///< In it, one of its children.
        After,      ///< Below it, a sibling coming next.
    };

    /// \brief Represents a row of a tree view dropped on another, as the game moves the record it shows.
    struct Landing final
    {
        /// The path of the record dropped, or empty when what was dropped is no row of the tree view.
        Str  Source;

        /// The path of the record it was dropped on.
        Str  Target;

        /// Where it lands against the record it was dropped on.
        Spot Where = Spot::Inside;
    };

    /// \brief Represents a tree view, with where its records come from, which branches are open, and its rows laid out.
    struct Hierarchy final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The markup making a row's content from a template, or `nullptr` to show the record's `Name`.
        Ptr<Markup>       Reader   = nullptr;

        /// The name of the template a row's content is made from.
        Str               Template;

        /// The node a row's content is made from when written in place, or empty to use the template's name.
        Unique<JsonValue> Node;

        /// The path of the top list, from the root, as `Scene`.
        Str               Base;

        /// The steps of \ref Base, read once, so the tree view looks its list up without reading the text again.
        Path              Where;

        /// The name of the list each record holds its children in.
        Str               Nested   = "Children";

        /// The style the tree view wears, its parts wearing it followed by theirs.
        Str               Style;

        /// The item list holding the rows.
        Handle            List;

        /// The identifiers of the paths of the records whose branches are open.
        Bag<Digest>       Opened;

        /// The rows, in order, as the tree was last laid out.
        Sequence<Limb>    Limbs;

        /// The model the tree was last laid out from, so one put in its place lays it out again.
        Ptr<Model>        Source   = nullptr;

        /// The model's count of changes when the tree view last looked at its list.
        UInt32            Revision = 0;

        /// The stamp of the newest change to the top list or to anything beneath it when the tree was last laid out.
        UInt32            Deep     = 0;

        /// `true` when the tree is to be laid out again at the next update.
        Bool              Stale    = true;

        /// The path of a record to pick once the tree is next laid out, as one picked before its row existed.
        Str               Wanted;

        /// The kind rows are carried as and accept when dropped on, or empty when they are not dragged.
        Str               Carry;

        /// `true` when a row dropped near another's edge lands beside it rather than in it.
        Bool              Between  = false;

        /// The row showing the marker of where a row carried over it lands, or one naming nothing.
        Handle            Lit;

        /// The drop being reported, as \ref Outline::GetLanding reads it.
        Landing           Last;
    };

    /// \brief Represents a tree view of the records of a model list and, under each open one, its children, indented.
    class Outline final
    {
    public:

        /// The height a tree view starts with, in pixels, since one sized by its rows would make every one.
        static constexpr Real32 kHeight   = 200.0f;

        /// The indent of each level when the theme gives none, in pixels.
        static constexpr Real32 kIndent   = 16.0f;

        /// A branch was opened; `Content` names the record, `Other` the row showing it.
        static constexpr UInt64 kExpand   = Identify("Expand");

        /// A branch was closed; `Content` names the record, `Other` the row showing it.
        static constexpr UInt64 kCollapse = Identify("Collapse");

        /// A row dropped on another; `Content` names its record, `Other` the one carried, \ref GetLanding where.
        static constexpr UInt64 kDrop     = Identify("OutlineDrop");

        /// How far from a row's top or bottom edge a drop lands beside it rather than in it, as a share of its height.
        static constexpr Real32 kEdge     = 0.25f;

    public:

        /// \brief Creates a tree view showing no record.
        ///
        /// \param Parent The element to hold it.
        /// \param Style  The name of the style it wears.
        /// \return The tree view, reporting \ref Signal::kChange, \ref Signal::kActivate and \ref Repeater::kRename.
        static Element Create(Element Parent, Text Style = "Outline");

        /// \brief Creates a tree view from a node, reading `Items`, `Nested`, `ItemTemplate` and `Selected`.
        ///
        /// \param Reader The markup reading the layout, which must outlive the tree view, as rows are made on scroll.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the tree view is read from.
        /// \return The tree view, which reports \ref Signal::kChange when the pick changes.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Shows the records of a list of the model, and the children of each one opened.
        ///
        /// \param Owner  The tree view to show the records in.
        /// \param Source The path of the top list, as `Scene`, read from the root when it opens with `/`.
        /// \param Scope  The path of the record the path is read from, or empty for the root.
        static void SetItems(Element Owner, Text Source, Text Scope = Text());

        /// \brief Names the list each record holds its children in.
        ///
        /// \param Owner The tree view whose records hold children.
        /// \param Key   The name of the list, as `Children`.
        static void SetNested(Element Owner, Text Key);

        /// \brief Fills the content of each row from a template, every row made again.
        ///
        /// \param Owner    The tree view whose rows are filled.
        /// \param Reader   The markup knowing the template, which must outlive the tree view.
        /// \param Template The name of the template.
        static void SetTemplate(Element Owner, Ref<Markup> Reader, Text Template);

        /// \brief Fills the content of each row from a node written in place, every row made again.
        ///
        /// \param Owner  The tree view whose rows are filled.
        /// \param Reader The markup making the node, which must outlive the tree view.
        /// \param Node   The node each row's content is made from, copied by the tree view.
        static void SetTemplate(Element Owner, Ref<Markup> Reader, ConstRef<JsonValue> Node);

        /// \brief Opens or closes the branch of a record, without reporting it.
        ///
        /// \param Owner    The tree view holding the record.
        /// \param Path     The path of the record, as `Scene[0].Children[2]`.
        /// \param Expanded The flag, `true` to show its children, `false` to hide them.
        static void SetExpanded(Element Owner, Text Path, Bool Expanded);

        /// \brief Checks whether the branch of a record is open.
        ///
        /// \param Owner The tree view holding the record.
        /// \param Path  The path of the record, as `Scene[0].Children[2]`.
        /// \return `true` if it is, `false` otherwise.
        static Bool IsExpanded(Element Owner, Text Path);

        /// \brief Lets control and shift pick several rows together.
        ///
        /// \param Owner   The tree view to pick in.
        /// \param Enabled The flag, `true` to pick several, `false` to pick one at a time.
        static void SetMultiple(Element Owner, Bool Enabled);

        /// \brief Picks the row of a record alone, without reporting it, when the record has a row.
        ///
        /// \param Owner The tree view to pick in.
        /// \param Path  The path of the record, or empty to pick none.
        static void Select(Element Owner, Text Path);

        /// \brief Gets the record of the row picked last.
        ///
        /// \param Owner The tree view the pick is read from.
        /// \return The path of the record, or empty when none is picked.
        static Str GetSelected(Element Owner);

        /// \brief Gets the record of every row picked.
        ///
        /// \param Owner The tree view the pick is read from.
        /// \return The paths of the records, in the order the rows stand.
        static Sequence<Str> GetSelection(Element Owner);

        /// \brief Gets how many rows the tree view lays out, each open branch's children included.
        ///
        /// \param Owner The tree view whose rows are counted.
        /// \return The count of rows.
        static UInt32 GetCount(Element Owner);

        /// \brief Gets the record a row shows.
        ///
        /// \param Owner    The tree view the row stands in.
        /// \param Position The position of the row, counted from the top.
        /// \return The path of the record, or empty past the last row.
        static Str GetPath(Element Owner, UInt32 Position);

        /// \brief Gets how deep the record of a row stands.
        ///
        /// \param Owner    The tree view the row stands in.
        /// \param Position The position of the row, counted from the top.
        /// \return The depth, nought for a record of the top list.
        static UInt32 GetDepth(Element Owner, UInt32 Position);

        /// \brief Gets the row showing a record.
        ///
        /// \param Owner The tree view holding the record.
        /// \param Path  The path of the record, as `Scene[0].Children[2]`.
        /// \return The row, or one naming nothing when the record is hidden in a closed branch or too far from view.
        static Element GetRow(Element Owner, Text Path);

        /// \brief Gets the arrow of a row.
        ///
        /// \param Row The row of a tree view, as \ref GetRow gives it.
        /// \return The arrow turning the row's branch, or one naming nothing when the row is gone.
        static Element GetArrow(Element Row);

        /// \brief Gets the item list holding the rows.
        ///
        /// \param Owner The tree view holding the list.
        /// \return The list, or one naming nothing when the element is no tree view.
        static Element GetList(Element Owner);

        /// \brief Lays the tree out again at the next update, as when the game changed it without the model knowing.
        ///
        /// \param Owner The tree view to lay out again.
        static void Refresh(Element Owner);

        /// \brief Lets the pointer drag a row onto another, not its own, reported as \ref kDrop for the game to move.
        ///
        /// \param Owner   The tree view whose rows are dragged.
        /// \param Kind    The kind a row is carried as and accepts, as `Archetype`, or empty to drag none.
        /// \param Between The flag, `true` to land a drop near a row's top or bottom edge beside it, `false` in it.
        static void SetDraggable(Element Owner, Text Kind, Bool Between = false);

        /// \brief Gets where the row being reported as \ref kDrop landed.
        ///
        /// \param Owner The tree view the row was dropped on.
        /// \return The drop, naming the record dropped, the one landed on and where, as the last \ref kDrop reported.
        static Landing GetLanding(Element Owner);

    private:

        /// \brief Runs a tree view, where a click on an arrow turns a branch and its list's events pass on as its own.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event the tree view is offered.
        /// \param Self    The handle of the tree view.
        /// \return `true` if the event opened, closed, moved or was reported again, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Lays the tree out again when its list or a branch changed, then indents rows and shows a drop.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The handle of the tree view.
        /// \return `true` if a row came, went or changed, `false` otherwise.
        static Bool OnRealize(Ref<Interface> Owner, Handle Self);

        /// \brief Fills a row of the list with its indent, its arrow and its content.
        ///
        /// \param Row   The row the list made, to be filled.
        /// \param Scope The path of the row's record.
        static void OnMake(Element Row, Text Scope);

        /// \brief Gathers the rows of a list of the model and of every open branch under it, in order.
        ///
        /// \param Data   The model the records are read from.
        /// \param Own    The tree view's facet, naming the open branches and the list of children.
        /// \param List   The path of the list.
        /// \param Depth  The depth its records stand at.
        /// \param Output The variable receiving the rows.
        static void Walk(ConstRef<Model>     Data,
                         ConstRef<Hierarchy> Own,
                         Text                List,
                         UInt16              Depth,
                         Ref<Sequence<Limb>> Output);

        /// \brief Gets what an open branch is remembered by, its record's key from \ref Model::Mirror, or its path.
        ///
        /// \param Data The model holding the record, or `nullptr` when there is none.
        /// \param Path The path of the record, as `Scene[0].Children[2]`.
        /// \return The digest the branch is held under while open.
        static Digest GetBranch(ConstPtr<Model> Data, Text Path);

        /// \brief Lays the tree out again, keeping every record picked picked while it still has a row.
        ///
        /// \param Owner The tree view to lay out.
        /// \return `true` if a row came, went or changed, `false` otherwise.
        static Bool Lay(Element Owner);

        /// \brief Indents each row and turns its arrow for the record it shows now.
        ///
        /// \param Owner  The tree view whose rows are shaped.
        /// \param Source The theme the indent of a level is read from, or `nullptr` for none.
        /// \return `true` if a row changed, `false` otherwise.
        static Bool Shape(Element Owner, ConstPtr<Theme> Source);

        /// \brief Opens or closes the branch a row shows and reports it.
        ///
        /// \param Owner    The interface it stands in.
        /// \param Root     The tree view holding the row.
        /// \param Position The position of the row.
        /// \param Expanded The flag, `true` to open it, `false` to close it.
        static void Turn(Ref<Interface> Owner, Element Root, UInt32 Position, Bool Expanded);

        /// \brief Picks a row alone, brings it into view and reports it.
        ///
        /// \param Owner    The interface it stands in.
        /// \param Root     The tree view to pick in.
        /// \param Position The position of the row.
        static void Choose(Ref<Interface> Owner, Element Root, UInt32 Position);

        /// \brief Reports a row dropped on another of the tree view, unless dropped on itself or on a row beneath it.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Root    The tree view the row was dropped on.
        /// \param Message The drop, targeting the row dropped on and naming what was carried.
        static void Drop(Ref<Interface> Owner, Element Root, ConstRef<Event> Message);

        /// \brief Shows the marker of where a row carried over the tree view would land, on the row under it.
        ///
        /// \param Owner The interface it stands in.
        /// \param Root  The tree view the row is carried over.
        /// \return `true` if a marker or a row changed, `false` otherwise.
        static Bool Aim(Ref<Interface> Owner, Element Root);

        /// \brief Works out where a drop at a point lands against a row.
        ///
        /// \param Row   The row dropped on.
        /// \param Point The point, in the interface's pixels.
        /// \return Where it lands, beside the row near its edges and in it otherwise.
        static Spot Land(Element Row, Vector2 Point);

        /// \brief Gets the marker a row shows where a drop lands beside it.
        ///
        /// \param Row The row, made while drops land between rows.
        /// \return The marker, the last part of the row.
        static Element GetMarker(Element Row);

        /// \brief Finds the tree view an element stands in.
        ///
        /// \param Inside The element to climb from, as a row or a part of one.
        /// \return The tree view, or one naming nothing.
        static Element Seek(Element Inside);

        /// \brief Finds the row a record has among the rows laid out.
        ///
        /// \param Own  The tree view's facet, holding the rows laid out.
        /// \param Path The path of the record.
        /// \return The position of the row, or below nought when the record has none.
        static SInt32 Find(ConstRef<Hierarchy> Own, Text Path);
    };
}