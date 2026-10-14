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
#include "Layout.hpp"
#include "UI.Core/Facet.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Facet/Visual/Caption.hpp"
#include "UI.Core/Facet/Visual/Effect.hpp"
#include "UI.Core/Facet/Visual/Look.hpp"
#include "UI.Core/Facet/Visual/Picture.hpp"
#include "UI.Core/Paint/Canvas.hpp"
#include "UI.Core/Pane.hpp"
#include "UI.Core/Text/Face.hpp"
#include "UI.Core/Tree.hpp"
#include "UI.Core/Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the pass that draws a pane's rows through a canvas, skipping every subtree the clip misses.
    class Painter final
    {
    public:

        /// \brief Represents what a drawer is handed of the element it draws.
        struct Brush final
        {
            /// The pane the element stands in.
            ConstPtr<Pane>   Target  = nullptr;

            /// The row of the element within its pane.
            UInt32           Row     = 0;

            /// The slot of the element, which its facets are looked up by.
            UInt32           Slot    = 0;

            /// The box the element was laid out in, in pixels.
            Rect             Frame;

            /// The box inside the element's padding, in pixels.
            Rect             Inner;

            /// The opacity of the element, together with everything holding it.
            Real32           Opacity = 1.0f;

            /// The radius its corners are rounded to, in pixels, nought when it holds no look.
            Real32           Radius  = 0.0f;

            /// The element's look, or `nullptr` for none.
            ConstPtr<Look>   Shade   = nullptr;

            /// The element's effects, or `nullptr` for none.
            ConstPtr<Effect> Extra   = nullptr;

            /// The largest box what the element holds paints opaque over it, in pixels, or an empty one.
            Rect             Cover;
        };

        /// \brief Defines the code drawing one stage of an element holding a kind of facet.
        using Draw = Delegate<void(Ref<Canvas> Canvas, ConstRef<Brush> Element)>;

    public:

        /// \brief Constructs a painter over the elements of a tree, drawing the interface's own facets.
        ///
        /// \param Owner    The tree whose panes it draws, which must outlive it.
        /// \param Animator The animator that tells whether a hidden element still plays its way out.
        /// \param Layout   The layout, whose tables place the glyphs of text.
        Painter(Ref<Tree> Owner, Ref<Animator> Animator, Ref<Layout> Layout);

        /// \brief Draws every row of a pane whose reach the clip does not miss.
        ///
        /// \param Canvas The canvas to draw through.
        /// \param Target The pane to draw, whose reaches are worked out again first where they changed.
        void Paint(Ref<Canvas> Canvas, Ref<Pane> Target);

        /// \brief Adds a stage of drawing for every element holding a kind of facet.
        ///
        /// \param Order The order the stage is drawn in among every element's stages.
        /// \param Code  The code drawing the stage, handed the element as a \ref Brush.
        template<IsFacet Type>
        void AddDrawer(UInt32 Order, Draw Code)
        {
            // After every stage of the same order, so those run in the order they were added.
            const SInt Found = mDrawers.Find([Order](ConstRef<Drawer> Each)
            {
                return Each.Order > Order;
            });
            const UInt Index = Found < 0 ? mDrawers.GetSize() : static_cast<UInt>(Found);

            mDrawers.Insert(Index, Facets::Index<Type>(), Order, Move(Code));
        }

    private:

        /// \brief Holds one stage of drawing and the kind of facet it draws for.
        struct Drawer final
        {
            /// The index of the kind of facet whose holders it draws.
            UInt32 Facet = 0;

            /// The order it is drawn in among every element's stages.
            UInt32 Order = 0;

            /// The code drawing the stage.
            Draw   Code;
        };

        /// \brief Represents an element whose children or transform are being drawn, and what it set up for them.
        struct Scope final
        {
            /// The row of the element.
            UInt32 Row         = 0;

            /// The row past the last one the element holds.
            UInt32 End         = 0;

            /// The opacity of the element, together with everything holding it.
            Real32 Opacity     = 1.0f;

            /// `true` when it cut what its children draw to its box.
            Bool   Clipped     = false;

            /// `true` when it moved itself and what its children draw by its transform.
            Bool   Transformed = false;

            /// `true` when the element bounds a part being recorded into its tape, which its closing ends.
            Bool              Recording = false;

            /// The tape the recording goes on into once this part's ends, or `nullptr` for none.
            Ptr<Canvas::Tape> Paused    = nullptr;
        };

        /// \brief Represents what the changed children of a row leave for it while reaches are worked out again.
        struct Ripple final
        {
            /// The union of the reach the changed children had, in pixels, meaningful only while \ref Raised.
            Rect Before;

            /// The union of the reach they have now, in pixels, meaningful only while \ref Raised.
            Rect After;

            /// `true` when a child's reach changed.
            Bool Raised = false;
        };

        /// \brief Works out again the reach of every row drawing differently since the last paint.
        ///
        /// \param Target The pane whose reaches are worked out.
        void Trace(Ref<Pane> Target);

        /// \brief Works out again the reach of a row and every row it holds, children first.
        ///
        /// \param Target The pane holding the row.
        /// \param Row    The row whose subtree is worked out again.
        void Retrace(Ref<Pane> Target, UInt32 Row);

        /// \brief Works out the box everything a row and the rows it holds draw falls within, from its children's.
        ///
        /// \param Target The pane, whose children of the row have their reach worked out already.
        /// \param Row    The row of the element.
        /// \param Turned The receiver of `true` when the element's own transform moved the box, `false` otherwise.
        /// \return The box, in pixels, moved by the element's own transform, or an empty one when it is hidden.
        Rect Reach(ConstRef<Pane> Target, UInt32 Row, Ref<Bool> Turned) const;

        /// \brief Draws one element, running every stage of drawing whose kind of facet it holds, in order.
        ///
        /// \param Canvas  The canvas to draw through.
        /// \param Target  The pane holding the element.
        /// \param Row     The row of the element.
        /// \param Opacity The opacity of the element, together with everything holding it.
        void Render(Ref<Canvas> Canvas, ConstRef<Pane> Target, UInt32 Row, Real32 Opacity) const;

        /// \brief Draws anew a part of a replayed pane that draws every frame, as a sketch, where it stood.
        ///
        /// \param Canvas The canvas to draw through.
        /// \param Target The pane being replayed, unchanged since it was recorded.
        /// \param Part   The part the tape noted.
        void Relive(Ref<Canvas> Canvas, ConstRef<Pane> Target, ConstRef<Canvas::Tape::Live> Part) const;

        /// \brief Works out which rows hold a change in their subtree, before the marks telling it are cleared.
        ///
        /// \param Target The pane about to be walked.
        void Survey(ConstRef<Pane> Target);

        /// \brief Checks whether a row bounds a part recorded and replayed whole.
        ///
        /// \param Target The pane holding the row.
        /// \param Row    The row.
        /// \return `true` for the root, an element cutting what it holds to its box, or one holding many rows.
        Bool IsBoundary(ConstRef<Pane> Target, UInt32 Row) const;

        /// \brief Checks whether a boundary's tape draws it as walking it would now.
        ///
        /// \param Canvas  The canvas the part would be drawn through.
        /// \param Target  The pane holding the row.
        /// \param Row     The row bounding the part.
        /// \param Opacity The opacity the part would be drawn at.
        /// \return `true` if nothing in the part changed and it would be drawn under what it was recorded under.
        Bool IsKept(ConstRef<Canvas> Canvas, ConstRef<Pane> Target, UInt32 Row, Real32 Opacity) const;

        /// \brief Draws a boundary's part from its tape, noting it in the tape being recorded around it.
        ///
        /// \param Canvas The canvas to draw through.
        /// \param Target The pane holding the row.
        /// \param Row    The row bounding the part.
        void Replay(Ref<Canvas> Canvas, ConstRef<Pane> Target, UInt32 Row) const;

        /// \brief Draws a pane's rows in order, recording them into the pane's tape as the canvas draws them.
        ///
        /// \param Canvas The canvas to draw through.
        /// \param Target The pane to draw.
        void Walk(Ref<Canvas> Canvas, Ref<Pane> Target);

        /// \brief Works out, for every row of a pane, the largest box what it holds paints opaque, children first.
        ///
        /// \param Target The pane whose covers are worked out.
        /// \param Scale  The pixels of the target one unit covers.
        void Shelter(Ref<Pane> Target, Real32 Scale);

        /// \brief Gets the box an element paints opaque itself, by its fill or by a picture showing no transparency.
        ///
        /// \param Target The pane holding the element.
        /// \param Row    The row of the element.
        /// \param Scale  The pixels of the target one unit covers.
        /// \return The box, in pixels, or an empty one when it lets anything through.
        Rect CoverOf(ConstRef<Pane> Target, UInt32 Row, Real32 Scale) const;

        /// \brief Ends the innermost scope, drawing its scroll thumbs over its children and lifting its clip.
        ///
        /// \param Canvas The canvas to draw through.
        /// \param Target The pane holding the element whose scope ends.
        void Close(Ref<Canvas> Canvas, Ref<Pane> Target);

        /// \brief Draws an element's background, unless an opaque gradient covers it.
        ///
        /// \param Canvas  The canvas to draw through.
        /// \param Element The element whose look is drawn.
        void DrawFill(Ref<Canvas> Canvas, ConstRef<Brush> Element);

        /// \brief Draws the image a theme lays over an element's fill.
        ///
        /// \param Canvas  The canvas to draw through.
        /// \param Element The element whose backdrop is drawn.
        void DrawBackdrop(Ref<Canvas> Canvas, ConstRef<Brush> Element) const;

        /// \brief Draws an element's picture.
        ///
        /// \param Canvas  The canvas to draw through.
        /// \param Element The element whose picture is drawn.
        void DrawPicture(Ref<Canvas> Canvas, ConstRef<Brush> Element) const;

        /// \brief Draws the filled share of an element's bar.
        ///
        /// \param Canvas  The canvas to draw through.
        /// \param Element The element whose bar is drawn.
        void DrawBar(Ref<Canvas> Canvas, ConstRef<Brush> Element) const;

        /// \brief Runs the code a game draws an element with, cut to the element's frame unless it asks otherwise.
        ///
        /// \param Canvas  The canvas to draw through.
        /// \param Element The element whose sketch is drawn.
        void DrawSketch(Ref<Canvas> Canvas, ConstRef<Brush> Element) const;

        /// \brief Draws an element's text, once its typeface has loaded.
        ///
        /// \param Canvas  The canvas to draw through.
        /// \param Element The element whose caption is drawn.
        void DrawCaption(Ref<Canvas> Canvas, ConstRef<Brush> Element) const;

    private:

        /// \brief Fades a colour.
        ///
        /// \param Tint    The colour to fade.
        /// \param Opacity The opacity it is drawn at, over nought through one.
        /// \return The colour with its alpha scaled by the opacity.
        static Color Fade(Color Tint, Real32 Opacity);

        /// \brief Fades both colours of a gradient.
        ///
        /// \param Fill    The gradient to fade.
        /// \param Opacity The opacity it is drawn at, over nought through one.
        /// \return The gradient with the alpha of both colours scaled by the opacity.
        static Gradient Fade(ConstRef<Gradient> Fill, Real32 Opacity);

        /// \brief Builds the transform an effect moves an element by, about the middle of the box it was laid out in.
        ///
        /// \param Frame The box the element was laid out in, in pixels.
        /// \param Extra The effect whose translation, scale and rotation are applied.
        /// \return The transform, in pixels of the target.
        static Matrix3x2 TransformOf(Rect Frame, ConstRef<Effect> Extra);

        /// \brief Draws a halo around a box, grown by its spread before it fades over its blur.
        ///
        /// \param Canvas  The canvas to draw through.
        /// \param Box     The box, already moved by the halo's offset when it is a shadow.
        /// \param Source  The halo, whose tint, spread and blur it is drawn with.
        /// \param Radius  The radius the box's corners are rounded to, in pixels.
        /// \param Opacity The opacity of the element, together with everything holding it.
        /// \param Hole    The box the element's own opaque fill covers whole once drawn over the halo, or an empty one.
        static void Cast(Ref<Canvas> Canvas, Rect Box, ConstRef<Halo> Source, Real32 Radius, Real32 Opacity, Rect Hole);

        /// \brief Draws an image over an element's box, stretched to follow its corners or in nine pieces when sliced.
        ///
        /// \param Canvas  The canvas to draw through.
        /// \param Element The element the image is drawn over.
        /// \param Source  The image, drawn only once it has finished loading.
        /// \param Region  The part of the image to draw, in texture coordinates.
        /// \param Slice   The border kept unstretched on each side, in pixels of the image, or none to stretch it.
        /// \param Tint    The colour the image is multiplied by, before the element's opacity fades it.
        static void Stamp(
            Ref<Canvas>                          Canvas,
            ConstRef<Brush>                      Element,
            ConstRef<Retainer<ZyGraphic::Image>> Source,
            Rect                                 Region,
            ConstRef<Edges>                      Slice,
            Color                                Tint);

        /// \brief Gets the box a picture is drawn over within its element's frame.
        ///
        /// \param Frame The box the element was laid out in, in pixels.
        /// \param Size  The size of the part of the picture shown, in its own pixels, or nought when it is not known.
        /// \param Mode  The way the picture fills the frame.
        /// \return The frame when the picture stretches, else the largest box of its shape in the frame's middle.
        static Rect FitOf(Rect Frame, Vector2 Size, Fit Mode);

        /// \brief Gets the box an element's fill covers whole, so nothing drawn under it before shows through.
        ///
        /// \param Frame   The box the element was laid out in, in pixels.
        /// \param Shade   The element's look, or `nullptr` for none.
        /// \param Extra   The effects it is drawn with, or `nullptr` for none.
        /// \param Opacity The opacity of the element, together with everything holding it.
        /// \param Scale   The pixels of the target one unit covers, which the edge's half pixel of fade is measured in.
        /// \return The box, in pixels, or an empty one when the fill lets anything through.
        static Rect HoleOf(Rect Frame, ConstPtr<Look> Shade, ConstPtr<Effect> Extra, Real32 Opacity, Real32 Scale);

        /// \brief Gets a box with each of its edges moved to the nearest edge of a pixel of the target.
        ///
        /// \param Frame The box, in units.
        /// \param Scale The pixels of the target one unit covers.
        /// \return The box, in units, whose edges fall between pixels, so a thin border covers whole pixels.
        static Rect SnapOf(Rect Frame, Real32 Scale);

        /// \brief Gets the larger of two boxes.
        ///
        /// \param First  The first box, or an empty one.
        /// \param Second The other box, or an empty one.
        /// \return The one covering more, or an empty one when both are.
        static Rect LargerOf(Rect First, Rect Second);

        /// \brief Gets how far a glyph of a caption may stand past its line's box, its outline and shadow included.
        ///
        /// \param Words The caption whose size, outline and shadow are measured.
        /// \return The distance, in pixels, which half the size covers for any overhang and the atlas's padding.
        static Real32 OverhangOf(ConstRef<Caption> Words);

        /// \brief Gets the box everything an element draws of itself falls within, its halos and its text included.
        ///
        /// \param Target The pane holding the element.
        /// \param Row    The row of the element.
        /// \param Extra  The effects it is drawn with, or `nullptr` for none.
        /// \param Words  The text it shows, or `nullptr` for none.
        /// \return The box, in pixels, before its own transform moves it.
        static Rect ReachOf(ConstRef<Pane> Target, UInt32 Row, ConstPtr<Effect> Extra, ConstPtr<Caption> Words);

        /// \brief Places the glyphs of every laid out line of a caption, once, for every paint after to draw from.
        ///
        /// \param Text   The caption, whose typeface must have finished loading.
        /// \param Source The tables of its typeface, which place a glyph as the typeface's own layout does.
        static void Typeset(Ref<Caption> Text, ConstRef<Face> Source);

        /// \brief Draws the text of a caption line by line, placing its glyphs first when its lines or font changed.
        ///
        /// \param Canvas  The canvas to draw through.
        /// \param Text    The caption, whose typeface must have finished loading.
        /// \param Source  The tables of its typeface.
        /// \param Inner   The box inside the padding of the element, in pixels.
        /// \param Opacity The opacity of the element, together with everything holding it.
        static void Write(Ref<Canvas> Canvas, Ref<Caption> Text, ConstRef<Face> Source, Rect Inner, Real32 Opacity);

        /// \brief Checks whether a change to the reach of children of a row can change the row's own reach.
        ///
        /// \param Target The pane holding the row.
        /// \param Row    The row whose children's reach changed.
        /// \param Before The union of the reach the changed children had, in pixels.
        /// \param After  The union of the reach they have now, in pixels.
        /// \return `true` if the row's reach must be worked out again, `false` otherwise.
        static Bool Spills(ConstRef<Pane> Target, UInt32 Row, Rect Before, Rect After);

        /// \brief Draws an element's shadow and glow, leaving out what its fill hides.
        ///
        /// \param Canvas  The canvas to draw through.
        /// \param Element The element whose halos are drawn.
        static void DrawHalos(Ref<Canvas> Canvas, ConstRef<Brush> Element);

        /// \brief Draws an element's gradient over its background.
        ///
        /// \param Canvas  The canvas to draw through.
        /// \param Element The element whose gradient is drawn.
        static void DrawGradient(Ref<Canvas> Canvas, ConstRef<Brush> Element);

        /// \brief Draws the glow just inside an element's edge.
        ///
        /// \param Canvas  The canvas to draw through.
        /// \param Element The element whose inner glow is drawn.
        static void DrawInnerGlow(Ref<Canvas> Canvas, ConstRef<Brush> Element);

        /// \brief Draws an element's border.
        ///
        /// \param Canvas  The canvas to draw through.
        /// \param Element The element whose border is drawn.
        static void DrawBorder(Ref<Canvas> Canvas, ConstRef<Brush> Element);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Tree>        mTree;
        Ref<Animator>    mAnimator;
        Ref<Layout>      mLayout;
        Sequence<Scope>  mScopes;
        Sequence<Ripple> mRipples;
        Sequence<Bool>   mRaised;
        Sequence<Bool>   mDirty;
        Bool             mWhole;
        Sequence<Drawer> mDrawers;
    };
}