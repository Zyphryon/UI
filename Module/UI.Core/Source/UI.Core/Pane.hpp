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

#include "Facet/Layout/Box.hpp"
#include "Layers.hpp"
#include "Paint/Canvas.hpp"
#include "States.hpp"
#include "Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents one pane, as a window, a HUD layer, a popup or a tooltip, its elements kept in columns.
    struct Pane final
    {
        /// The parent a pane's root names, since nothing holds it.
        static constexpr UInt32 kNoParent = 0xFFFFFFFF;

        /// The band the pane is drawn in.
        Layer             Band    = Layers::kBase;

        /// The order the pane was last raised in, the higher drawn over the lower within its band.
        UInt32            Raise   = 0;

        /// `true` while the pane holds a root, `false` once it is closed and waits to be opened again.
        Bool              Open    = false;

        /// `true` when something that sizes the pane's elements changed since it was last laid out.
        Bool              Stale   = false;

        /// `true` when the next layout works out every row, not only the rows marked and those they reach.
        Bool              Whole   = false;

        /// `true` when a row was marked to be dressed again since the pane was last dressed.
        Bool              Styled  = false;

        /// `true` when rows moved, came or went since the reach of every row was last worked out.
        Bool              Moved   = false;

        /// `true` when a row was marked to draw differently since the reach of every row was last worked out.
        Bool              Touched = false;

        /// The slot of the element at each row.
        Sequence<UInt32>  Slots;

        /// The slot of the parent of the element at each row, or \ref kNoParent at the root.
        Sequence<UInt32>  Parents;

        /// The number of rows the subtree at each row takes, itself included.
        Sequence<UInt32>  Spans;

        /// The depth of the element at each row, the root being nought.
        Sequence<UInt16>  Depths;

        /// The identifier the element at each row is found by, or zero for none.
        Sequence<UInt64>  Names;

        /// The states the element at each row is in.
        Sequence<State>   States;

        /// What of the element at each row must be worked out again.
        Sequence<Dirty>   Marks;

        /// The identifier of the style the element at each row wears, or zero for none.
        Sequence<UInt64>  Styles;

        /// How the element at each row takes its place and lays out what it holds.
        Sequence<Box>     Boxes;

        /// The box the element at each row was laid out in, in pixels of the target.
        Sequence<Rect>    Frames;

        /// The size the element at each row asks for, its margin left out, as the last layout measured it.
        Sequence<Vector2> Sizes;

        /// The room inside the padding of the element at each row when it was last laid out, which text is fit to.
        Sequence<Vector2> Rooms;

        /// The box the element at each row stood in when it last placed its children.
        Sequence<Rect>    Laid;

        /// The box everything the subtree at each row draws falls within, transformed, in pixels, as last painted.
        Sequence<Rect>    Reaches;

        /// The largest box what the element at each row holds paints opaque, in pixels, or an empty one.
        Sequence<Rect>    Covers;

        /// What the subtree at each row drew when last walked, kept for the rows that bound a replayed part.
        Sequence<Canvas::Tape> Tapes;

        /// \brief Gets how many rows the pane holds.
        ///
        /// \return The count of rows, every element of the pane.
        ZY_INLINE UInt32 GetSize() const
        {
            return static_cast<UInt32>(Slots.GetSize());
        }

        /// \brief Checks whether the element at a row is hidden.
        ///
        /// \param Row The row of the element asked about.
        /// \return `true` if it is, `false` otherwise.
        ZY_INLINE Bool IsHidden(UInt32 Row) const
        {
            return HasBit(States[Row], State::Hidden);
        }

        /// \brief Marks a run of rows to be dressed again before the pane is next drawn.
        ///
        /// \param First The first row of the run.
        /// \param Last  The row one past the last of the run.
        void Restyle(UInt32 First, UInt32 Last);

        /// \brief Opens a run of rows in front of a row, moving the rows from there on behind it.
        ///
        /// \param Row   The row the run starts at, at most the count of rows.
        /// \param Count The number of rows to open, whose fields are left for the caller to fill.
        void Widen(UInt32 Row, UInt32 Count);

        /// \brief Closes a run of rows, moving the rows behind it into its place.
        ///
        /// \param Row   The row the run starts at.
        /// \param Count The number of rows to close.
        void Narrow(UInt32 Row, UInt32 Count);

        /// \brief Swaps two adjacent runs of rows, keeping the order within each.
        ///
        /// \param First  The first row of the leading run.
        /// \param Middle The first row of the trailing run.
        /// \param Last   The row one past the last of the trailing run.
        void Rotate(UInt32 First, UInt32 Middle, UInt32 Last);

        /// \brief Opens a run of rows in front of a row and fills it with a copy of a run of another pane's rows.
        ///
        /// \param Row    The row the run starts at, at most the count of rows.
        /// \param Source The pane the rows are copied from, which must be another pane.
        /// \param From   The first row of the run copied.
        /// \param Count  The number of rows copied.
        void Adopt(UInt32 Row, ConstRef<Pane> Source, UInt32 From, UInt32 Count);

        /// \brief Closes every row, leaving the pane closed.
        void Clear();

    private:

        /// \brief Hands every column of the pane to an action, as a pointer to the member holding it.
        ///
        /// \param Action The callable, taking each column, so every column is reshaped the same way.
        template<typename Callable>
        static void Columns(AnyRef<Callable> Action);

        /// \brief Opens a run of rows in one column in front of a row, moving the rows from there on behind it.
        ///
        /// \param Column The column the run is opened in.
        /// \param Row    The row the run starts at.
        /// \param Count  The number of rows to open.
        template<typename Type>
        static void WidenColumn(Ref<Sequence<Type>> Column, UInt32 Row, UInt32 Count);

        /// \brief Reverses the order of a run of rows in one column.
        ///
        /// \param Column The column the run is reversed in.
        /// \param First  The first row of the run.
        /// \param Last   The row one past the last of the run.
        template<typename Type>
        static void ReverseColumn(Ref<Sequence<Type>> Column, UInt32 First, UInt32 Last);

        /// \brief Swaps two adjacent runs of rows in one column, keeping the order within each.
        ///
        /// \param Column The column the runs are swapped in.
        /// \param First  The first row of the leading run.
        /// \param Middle The first row of the trailing run.
        /// \param Last   The row one past the last of the trailing run.
        template<typename Type>
        static void RotateColumn(Ref<Sequence<Type>> Column, UInt32 First, UInt32 Middle, UInt32 Last);
    };
}