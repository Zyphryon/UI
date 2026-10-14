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
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the state of a splitter, with the least size of each pane and the size each folded one had.
    struct Split final
    {
        /// What a change to the facet must have worked out again.
        static constexpr Dirty kMarks = Dirty::None;

        /// The least size of each pane along the splitter's axis, in pixels, in order.
        Sequence<Real32> Minimums;

        /// The size each pane had before folding, nought for one sharing what is left and below nought while open.
        Sequence<Real32> Folded;

        /// The share of the room each pane keeps, over nought through one, or nought for a size or what is left.
        Sequence<Real32> Shares;

        /// How far into the bar held the press landed, along the axis, in pixels.
        Real32           Grab = 0.0f;

        /// `true` while the pointer holds a bar, between the \ref Signal::kBegin and the \ref Signal::kEnd it reports.
        Bool             Held = false;

        /// The name its sizes are saved under by \ref Splitter::Save, from its layout node, or empty to leave it out.
        Str              Name;
    };

    /// \brief Represents a splitter, panes with a bar between each two that a drag moves and a double click folds.
    class Splitter final
    {
    public:

        /// The thickness of a bar, in pixels.
        static constexpr Real32 kThickness = 6.0f;

        /// The least size a pane is given when none is named, in pixels.
        static constexpr Real32 kMinimum   = 40.0f;

    public:

        /// \brief Creates a splitter holding no pane.
        ///
        /// \param Parent The element to hold it.
        /// \param Flow   The way its panes stand, \ref Direction::Row side by side and \ref Direction::Column stacked.
        /// \param Style  The name of the style it wears.
        /// \return The splitter, reporting \ref Signal::kBegin, \ref Signal::kChange and \ref Signal::kEnd on a drag.
        static Element Create(Element Parent, Direction Flow = Direction::Row, Text Style = "Splitter");

        /// \brief Creates a splitter from a node, reading `Orientation` and `Panes`; its `Name` is the \ref Save name.
        ///
        /// \param Reader The markup reading the layout.
        /// \param Parent The element to hold it.
        /// \param Node   The layout node the splitter is read from.
        /// \return The splitter, holding the panes the node lists.
        static Element Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Adds a pane after the last, with a bar before it when it is not the first.
        ///
        /// \param Owner   The splitter the pane is added to.
        /// \param Size    The size it keeps along the splitter's axis, in pixels, or nought to share what is left.
        /// \param Minimum The least size a drag leaves it, in pixels.
        /// \param Style   The name of the style the splitter wears.
        /// \return The pane, to hold what it shows.
        static Element AddPane(Element Owner, Real32 Size = 0.0f, Real32 Minimum = kMinimum, Text Style = "Splitter");

        /// \brief Adds a pane after the last, keeping a size in pixels, a share of the room, or sharing what is left.
        ///
        /// \param Owner   The splitter the pane is added to.
        /// \param Size    The size along the axis, in pixels or a percent, or from the content to share what is left.
        /// \param Minimum The least size a drag leaves it, in pixels.
        /// \param Style   The name of the style the splitter wears.
        /// \return The pane, to hold what it shows.
        static Element AddPane(Element Owner, Length Size, Real32 Minimum = kMinimum, Text Style = "Splitter");

        /// \brief Gets how many panes a splitter holds.
        ///
        /// \param Owner The splitter to count the panes of.
        /// \return The count of panes, its bars left out.
        static UInt32 GetCount(Element Owner);

        /// \brief Gets a pane of a splitter.
        ///
        /// \param Owner The splitter holding the pane.
        /// \param Index The index of the pane.
        /// \return The pane, or one naming nothing past the last.
        static Element GetPane(Element Owner, UInt32 Index);

        /// \brief Gets the bar standing before a pane.
        ///
        /// \param Owner The splitter holding the bar.
        /// \param Index The index of the pane after the bar, from one.
        /// \return The bar, or one naming nothing for the first pane or past the last.
        static Element GetBar(Element Owner, UInt32 Index);

        /// \brief Gives a pane a size it keeps, or lets it share what is left, opening it when folded.
        ///
        /// \param Owner The splitter holding the pane.
        /// \param Index The index of the pane.
        /// \param Size  The size along the splitter's axis, in pixels, or nought to share what is left.
        static void SetSize(Element Owner, UInt32 Index, Real32 Size);

        /// \brief Gets the size a pane was last laid out at.
        ///
        /// \param Owner The splitter holding the pane.
        /// \param Index The index of the pane.
        /// \return The size along the splitter's axis, in pixels.
        static Real32 GetSize(Element Owner, UInt32 Index);

        /// \brief Gives a pane a share of the room it keeps as the splitter resizes, opening it when folded.
        ///
        /// \param Owner The splitter holding the pane.
        /// \param Index The index of the pane.
        /// \param Share The share along the splitter's axis, over nought through one, or nought to share what is left.
        static void SetShare(Element Owner, UInt32 Index, Real32 Share);

        /// \brief Gets the share of the splitter's room a pane keeps.
        ///
        /// \param Owner The splitter holding the pane.
        /// \param Index The index of the pane.
        /// \return The share, over nought through one, or nought for a pane keeping a size or sharing what is left.
        static Real32 GetShare(Element Owner, UInt32 Index);

        /// \brief Gets the size a pane keeps as it was given, whether it is laid out, folded or hidden.
        ///
        /// \param Owner The splitter holding the pane.
        /// \param Index The index of the pane.
        /// \return The size in pixels, the share as a percent, or one from the content for a pane sharing the rest.
        static Length GetMeasure(Element Owner, UInt32 Index);

        /// \brief Writes the sizes of every named splitter in a part of the tree as JSON, each under its name.
        ///
        /// \param Scope The element whose subtree is walked, itself included, as the root of a workspace.
        /// \return The JSON text, which \ref Load reads back.
        static Str Save(Element Scope);

        /// \brief Reads sizes written by \ref Save back into the named splitters, the rest keeping theirs.
        ///
        /// \param Scope  The element whose subtree is walked, itself included.
        /// \param Source The JSON text \ref Save wrote.
        /// \param Error  The variable receiving why the text cannot be read.
        /// \return `true` if it was read, `false` otherwise.
        static Bool Load(Element Scope, Text Source, Ref<Str> Error);

        /// \brief Folds a pane away to nothing, or opens it again at the size it had.
        ///
        /// \param Owner  The splitter holding the pane.
        /// \param Index  The index of the pane.
        /// \param Folded The flag, `true` to fold it, `false` to open it.
        static void SetFolded(Element Owner, UInt32 Index, Bool Folded);

        /// \brief Checks whether a pane is folded.
        ///
        /// \param Owner The splitter holding the pane.
        /// \param Index The index of the pane.
        /// \return `true` if it is, `false` otherwise.
        static Bool IsFolded(Element Owner, UInt32 Index);

    private:

        /// \brief Runs a splitter, where a press on a bar holds it, a drag moves it, and a double click folds a pane.
        ///
        /// \param Owner   The interface it stands in.
        /// \param Message The event reaching the splitter.
        /// \param Self    The splitter the handler governs.
        /// \return `true` if the event held, moved, let go or folded, `false` otherwise.
        static Bool OnEvent(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self);

        /// \brief Hides each bar that has no shown pane before it or after it, and shows the others.
        ///
        /// \param Owner The interface it stands in.
        /// \param Self  The splitter whose bars are shown or hidden.
        /// \return `true` if a bar was shown or hidden, `false` otherwise.
        static Bool OnRealize(Ref<Interface> Owner, Handle Self);

        /// \brief Moves a bar to the pointer, sharing the room of the panes beside it anew.
        ///
        /// \param Root  The splitter holding the bar.
        /// \param Bar   The bar dragged.
        /// \param Point The pointer, in units of the interface.
        /// \return `true` if the bar moved, `false` if it stood where the pointer is or could go no further.
        static Bool Drag(Element Root, Element Bar, Vector2 Point);

        /// \brief Folds the pane beside a bar that gives way, or opens the one folded there.
        ///
        /// \param Root The splitter holding the bar.
        /// \param Bar  The bar double clicked.
        /// \return `true` if a pane folded or opened, `false` if the bar has no shown pane on one side.
        static Bool Fold(Element Root, Element Bar);

        /// \brief Writes the sizes of a splitter and of every named splitter it holds, each under its name.
        ///
        /// \param Node   The element written, when it is a named splitter, and whose children are walked.
        /// \param Output The object each splitter's sizes are written in.
        static void Gather(Element Node, JsonObject Output);

        /// \brief Reads back the sizes of a splitter and of every named splitter it holds, each from under its name.
        ///
        /// \param Node   The element read into, when it is a named splitter, and whose children are walked.
        /// \param Source The object holding each splitter's sizes.
        static void Scatter(Element Node, ConstRef<JsonValue> Source);

        /// \brief Gives a pane a new size after a drag, kept as a share of the room when it keeps one.
        ///
        /// \param Root  The splitter holding the pane.
        /// \param Index The index of the pane.
        /// \param Size  The size along the splitter's axis, in pixels.
        static void Resize(Element Root, UInt32 Index, Real32 Size);

        /// \brief Lays a pane out to keep a share of the splitter's room.
        ///
        /// \param Pane  The pane to lay out.
        /// \param Share The share, over nought through one.
        static void Spread(Element Pane, Real32 Share);

        /// \brief Gets the room the splitter shares out among its panes, the room a share is measured against.
        ///
        /// \param Root The splitter whose room is measured.
        /// \return The room along its axis inside its padding, in pixels.
        static Real32 GetRoom(Element Root);

        /// \brief Finds the shown panes on either side of a bar.
        ///
        /// \param Root   The splitter holding the bar.
        /// \param Bar    The bar the panes stand beside.
        /// \param Before The variable receiving the index of the nearest shown pane before, or below nought for none.
        /// \param After  The variable receiving the index of the nearest shown pane after it, or below nought for none.
        static void Flank(Element Root, Element Bar, Ref<SInt32> Before, Ref<SInt32> After);

        /// \brief Lays a pane out to keep a size, or to share what is left.
        ///
        /// \param Pane    The pane to lay out.
        /// \param Size    The size, in pixels, or nought to share what is left.
        /// \param Minimum The least size it is laid out at, in pixels.
        static void Shape(Element Pane, Real32 Size, Real32 Minimum);

        /// \brief Checks whether a pane shares what is left rather than keeping a size.
        ///
        /// \param Pane The pane to check.
        /// \return `true` if it does, `false` otherwise.
        static Bool IsSharing(Element Pane);

        /// \brief Checks whether a child of a splitter is one of its bars rather than one of its panes.
        ///
        /// \param Child The child to check.
        /// \return `true` if it is a bar, the only child that answers the pointer, `false` otherwise.
        static Bool IsBar(Element Child);

        /// \brief Gets the extent of a box along the splitter's axis.
        ///
        /// \param Root  The splitter whose axis is read.
        /// \param Frame The box to measure.
        /// \return The width for panes side by side, the height for panes one over another.
        static Real32 Along(Element Root, Rect Frame);

        /// \brief Gets where a box starts along the splitter's axis.
        ///
        /// \param Root  The splitter whose axis is read.
        /// \param Frame The box to measure.
        /// \return The left edge for panes side by side, the top edge for panes one over another.
        static Real32 Start(Element Root, Rect Frame);

        /// \brief Gets where a point stands along the splitter's axis.
        ///
        /// \param Root  The splitter whose axis is read.
        /// \param Point The point to measure.
        /// \return Its distance across for panes side by side, down for panes one over another.
        static Real32 Toward(Element Root, Vector2 Point);
    };
}