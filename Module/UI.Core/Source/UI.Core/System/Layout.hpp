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

#include "Animator.hpp"
#include "UI.Core/Facet.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Facet/Visual/Caption.hpp"
#include "UI.Core/Pane.hpp"
#include "UI.Core/Text/Face.hpp"
#include "UI.Core/Tree.hpp"
#include "UI.Core/Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the pass that lays out a pane's rows, measuring children first and arranging parents first.
    class Layout final
    {
    public:

        /// The most times a layout runs over while wrapped text settles to the room it was given.
        static constexpr UInt32 kPasses = 3;

        /// \brief Defines the code measuring what an element holding a kind of facet shows.
        using Measurer = Delegate<Bool(Ref<Tree> Owner, ConstRef<Pane> Target, UInt32 Row, Ref<Vector2> Content)>;

    public:

        /// \brief Constructs a layout over a target of no size, over the elements of a tree.
        ///
        /// \param Owner    The tree whose panes it lays out, which must outlive it.
        /// \param Animator The animator that tells whether a hidden element still plays its way out.
        Layout(Ref<Tree> Owner, Ref<Animator> Animator);

        /// \brief Adds what measures the content of every element holding a kind of facet.
        ///
        /// \param Code The code measuring the content, handed the element's pane and row.
        template<IsFacet Type>
        void AddMeasurer(Measurer Code)
        {
            mProbes.Append(Facets::Index<Type>(), Move(Code));
        }

        /// \brief Sets the size of the target the panes are laid out over.
        ///
        /// \param Size The size of the target, in units of the interface.
        ZY_INLINE void SetSize(Vector2 Size)
        {
            mSize = Size;
        }

        /// \brief Gets the size of the target the panes were last laid out over.
        ///
        /// \return The size, in units of the interface, which is the target's pixels over the pixel ratio.
        ZY_INLINE Vector2 GetSize() const
        {
            return mSize;
        }

        /// \brief Lays out a pane, whole when it asks for it and otherwise only the rows marked and those they reach.
        ///
        /// \param Target The pane to lay out, whose marks tell which rows changed.
        void Run(Ref<Pane> Target);

        /// \brief Lays out the text a box held back while it was out of view, as the box comes into view.
        ///
        /// \param Target The pane holding the box.
        /// \param Row    The row of the box coming into view.
        /// \return `true` if text was laid out, `false` otherwise.
        Bool Catch(Ref<Pane> Target, UInt32 Row);

        /// \brief Gets the tables text set in a typeface is measured with, building them the first time.
        ///
        /// \param Font The typeface, which must have finished loading.
        /// \return The tables, which stand until the next \ref Forget.
        ConstRef<Face> FaceOf(ConstRef<Retainer<ZyRender::Font>> Font);

        /// \brief Lets go of the tables of every typeface nothing but the tables holds any more.
        void Forget();

    private:

        /// The marks asking for a row to be measured again.
        static constexpr Dirty kAsked  = Dirty::Measure | Dirty::Resize;

        /// The marks asking for a row, or one it holds, to be arranged again.
        static constexpr Dirty kSought = Dirty::Arrange | Dirty::Below;

        /// \brief Holds one measurer and the kind of facet it measures.
        struct Probe final
        {
            /// The index of the kind of facet whose holders it measures.
            UInt32   Facet = 0;

            /// The code measuring the content of those holders.
            Measurer Code;
        };

        /// \brief Lays out every row of a pane.
        ///
        /// \param Target The pane to lay out whole.
        void Overhaul(Ref<Pane> Target);

        /// \brief Lays out only the rows of a pane that changed, and those a change of size reaches.
        ///
        /// \param Target The pane whose marked rows are laid out.
        void Amend(Ref<Pane> Target);

        /// \brief Measures again every row marked for it, children first, marking the parent of each that changed size.
        ///
        /// \param Target The pane whose marked rows are measured.
        void Survey(Ref<Pane> Target);

        /// \brief Measures one row again and marks it to be arranged.
        ///
        /// \param Target The pane holding the row.
        /// \param Row    The row to measure.
        /// \param Bits   The marks of the row, with what its children asked of it.
        /// \return \ref Dirty::Resize when its parent must be measured again, \ref Dirty::None otherwise.
        Dirty Refit(Ref<Pane> Target, UInt32 Row, Dirty Bits);

        /// \brief Marks every row fit to a new room to be measured again, and forgets them.
        ///
        /// \param Target The pane holding the rows.
        void Requeue(Ref<Pane> Target);

        /// \brief Checks whether a row's text may wait to be measured, as alone in a pixel-sized box out of view.
        ///
        /// \param Target The pane holding the row.
        /// \param Row    The row whose text is asked about.
        /// \return `true` if it may wait, `false` otherwise.
        Bool Defers(ConstRef<Pane> Target, UInt32 Row) const;

        /// \brief Arranges every drawn row of a run that was measured again or moved, walking only into such rows.
        ///
        /// \param Target The pane holding the run.
        /// \param First  The first row of the run.
        /// \param Last   The row past the run.
        /// \return `true` if the room some content was fit to changed, so the layout must run over, `false` otherwise.
        Bool Sweep(Ref<Pane> Target, UInt32 First, UInt32 Last);

        /// \brief Measures the size an element asks for, from the sizes its children asked for.
        ///
        /// \param Target The pane holding the element.
        /// \param Row    The row of the element.
        /// \return `true` if a font or an image it measures has not loaded yet, `false` otherwise.
        Bool Measure(Ref<Pane> Target, UInt32 Row);

        /// \brief Places the children of an element within the box it was laid out in.
        ///
        /// \param Target The pane holding the element.
        /// \param Row    The row of the element.
        /// \return `true` if the room its content was fit to changed, so the layout must run over, `false` otherwise.
        Bool Arrange(Ref<Pane> Target, UInt32 Row);

        /// \brief Breaks the text of a caption into lines for a width, ellipsizing it as it asks.
        ///
        /// \param Words The caption, whose typeface has finished loading.
        /// \param Width The width the lines may take, in pixels, or nought for any.
        void Typeset(Ref<Caption> Words, Real32 Width);

        /// \brief Measures how wide the content of a scrolling element asks to be, as it lays its flowing children out.
        ///
        /// \param Target The pane holding the element.
        /// \param Row    The row of the element.
        /// \return The width, in pixels.
        Real32 Breadth(Ref<Pane> Target, UInt32 Row) const;

    private:

        /// \brief Checks whether a box is sized in pixels on both axes, so its content never changes its size.
        ///
        /// \param Shape The box of the element.
        /// \return `true` if it is, `false` otherwise.
        static Bool IsFixed(ConstRef<Box> Shape);

        /// \brief Gets the margin an element takes along the axis its parent lays it out on.
        ///
        /// \param Row    The flag, `true` to lay out in a row, `false` in a column.
        /// \param Margin The margin of the element.
        /// \return The margin on both sides along the axis, in pixels.
        static Real32 MainOf(Bool Row, ConstRef<Edges> Margin);

        /// \brief Gets the margin an element takes across the axis its parent lays it out on.
        ///
        /// \param Row    The flag, `true` to lay out in a row, `false` in a column.
        /// \param Margin The margin of the element.
        /// \return The margin on both sides across the axis, in pixels.
        static Real32 SideOf(Bool Row, ConstRef<Edges> Margin);

        /// \brief Gets the size an element starts from along its parent's axis, before it grows or shrinks.
        ///
        /// \param Row      The flag, `true` to lay out in a row, `false` in a column.
        /// \param Shape    The box of the element.
        /// \param Measured The size the element asked for, in pixels.
        /// \param Room     The room along the axis, which a share is taken of, in pixels.
        /// \return The size, in pixels.
        static Real32 BaseOf(Bool Row, ConstRef<Box> Shape, Vector2 Measured, Real32 Room);

        /// \brief Places an element standing over its parent along one axis, by its distance from either edge.
        ///
        /// \param Lead    The distance from the near edge of the parent.
        /// \param Trail   The distance from the far edge of the parent.
        /// \param Size    The size the element sets.
        /// \param Minimum The near edge of the parent, in pixels.
        /// \param Maximum The far edge of the parent, in pixels.
        /// \param Wanted  The size the element asked for, in pixels.
        /// \param Before  The margin on the near side of the element, in pixels.
        /// \param After   The margin on the far side of the element, in pixels.
        /// \param Start   The receiver of where the element starts, in pixels.
        /// \param End     The receiver of where the element ends, in pixels.
        static void SpanOf(
            ConstRef<Length> Lead,
            ConstRef<Length> Trail,
            ConstRef<Length> Size,
            Real32           Minimum,
            Real32           Maximum,
            Real32           Wanted,
            Real32           Before,
            Real32           After,
            Ref<Real32>      Start,
            Ref<Real32>      End);

        /// \brief Measures the pixels a picture shows, as the interface's own measurer.
        ///
        /// \param Owner   The tree holding the picture.
        /// \param Target  The pane holding the element.
        /// \param Row     The row of the element.
        /// \param Content The receiver of the size the picture asks for, once its image has loaded.
        /// \return `true` if its image has not loaded yet, `false` otherwise.
        static Bool MeasurePicture(Ref<Tree> Owner, ConstRef<Pane> Target, UInt32 Row, Ref<Vector2> Content);

        /// \brief Places an element that stands over its parent, by its distance from the parent's edges.
        ///
        /// \param Target The pane holding the element.
        /// \param Row    The row of the element.
        /// \param Parent The box of the parent, in pixels.
        static void Place(Ref<Pane> Target, UInt32 Row, Rect Parent);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Tree>        mTree;
        Ref<Animator>    mAnimator;
        Vector2          mSize;
        Sequence<UInt32> mFits;
        Sequence<UInt32> mWaiting;
        Sequence<Dirty>  mClimb;
        Sequence<Face>   mFaces;
        Sequence<Probe>  mProbes;
    };
}