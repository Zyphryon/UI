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

#include "UI.Core/Types.hpp"
#include <Zyphryon.Graphic/Image.hpp>
#include <Zyphryon.Graphic/Service.hpp>
#include <Zyphryon.Graphic/Technique.hpp>
#include <Zyphryon.Math/Angle.hpp>
#include <Zyphryon.Math/Matrix3x2.hpp>
#include <Zyphryon.Math/Matrix4x4.hpp>
#include <Zyphryon.Render/Font/Font.hpp>
#include <Zyphryon.Render/Font/FontEffect.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the batcher an interface is drawn through, turned into as few engine commands as it can.
    class Canvas final
    {
    public:

        /// \brief Specifies how a quad's edge is cut, in the lowest two bits of its code.
        enum class Cut : UInt16
        {
            Plain,       ///< Covering the whole of its quad, with no distance worked out.
            Rounded,     ///< Cut at a rounded edge, and to the band of its outline when it has one.
            Shadow,      ///< Fading out across the blur on either side of the edge, the quad grown by the blur.
            Inset,       ///< Solid at the edge and fading out inward over the blur, with nothing outside.
        };

        /// \brief Specifies what a quad is filled with, in the two bits above its \ref Cut.
        enum class Paint : UInt16
        {
            Colour,      ///< Its colour alone, sampling nothing.
            Image,       ///< A plain texture, from the image slot.
            Layered,     ///< One slice of an array texture, from the layer slot, the slice in its shape's third step.
            Glyph,       ///< A multi-channel distance field, from the atlas slot, the way a font stores its glyphs.
        };

        /// \brief Specifies where a quad's corners stand, in bits 13 and 14 of its code.
        enum class Form : UInt16
        {
            Box,         ///< At the corners of its part drawn, square to its transform.
            Segment,     ///< Around the line between the two points of its area, as wide as its box tells.
            Polygon,     ///< Just outside a convex figure whose four corners are its area's two and its box's two.
        };

        /// \brief Specifies how a texture is read between its texels.
        enum class Filter : UInt8
        {
            Linear,      ///< Blending the texels nearest, as a photo or a scaled interface reads best.
            Point,       ///< Taking the nearest texel alone, so pixel art and tiles stay sharp at any zoom.
        };

        /// \brief Specifies how the ends of a line are drawn.
        enum class Cap : UInt8
        {
            Butt,        ///< Cut square at each point.
            Round,       ///< Rounded by a half circle past each point.
            Square,      ///< Cut square half the width past each point.
        };

        /// \brief Represents one quad in 60 bytes, laid out as the technique's per-instance layout reads it.
        struct Quad final
        {
            /// The part of the box drawn, in pixels of the target before the quad's transform.
            Rect             Area;

            /// The box the quad rounds, fades and maps its texture over, in the same pixels, holding the part drawn.
            Rect             Box;

            /// The texture coordinates of the box's left, top, right and bottom edges, over the whole range of 16 bits.
            Array<UInt16, 4> Source;

            /// The colour, as four bytes of red, green, blue and alpha.
            UInt32           Tint;

            /// The colour a gradient ends at, packed the same way.
            UInt32           Other;

            /// The radius, material, blur, angle or slice, and the code holding the transform, form and opacity.
            Array<UInt16, 4> Shape;

            /// How far the quad stands, from nought nearest to one, which only a canvas drawing with depth sets.
            Real32           Depth = 0.0f;
        };

        /// \brief Represents what one pane drew, each quad with what it was drawn under, to draw it again unwalked.
        struct Tape final
        {
            /// \brief Represents one quad recorded, with the batch, clip, transform and material it was drawn under.
            struct Step final
            {
                /// The quad as it was emitted, its transform slot and material slot worked out again on replay.
                Quad              Drawn;

                /// The texture the quad's batch read.
                ZyGraphic::Object Texture  = 0;

                /// The clip the quad was drawn under, in pixels.
                Rect              Clip;

                /// The index of the transform the quad was drawn under, or \ref kNone for none.
                UInt16            Turn     = 0xFFFF;

                /// The index of the glyph material the quad read, or \ref kNone for none.
                UInt16            Material = 0xFFFF;

                /// The kind of paint the quad's batch was opened for.
                UInt8             Kind     = 0;

                /// The way the quad's texture is read.
                Filter            Sampling = Filter::Linear;

                /// `true` when the quad's batch was opened to stand at the clip with the scissor.
                Bool              Uncut    = false;
            };

            /// \brief Represents a part of the pane drawn anew every frame, as a sketch, which a replay hands back.
            struct Live final
            {
                /// The count of steps recorded before it, where the replay draws it.
                UInt32 At      = 0;

                /// The tag the recorder gave it, as the row of the element drawing it.
                UInt32 Tag     = 0;

                /// The opacity it was drawn at.
                Real32 Opacity = 1.0f;

                /// The clip it was drawn under, in pixels.
                Rect   Clip;

                /// The index of the transform it was drawn under, or \ref kNone for none.
                UInt16 Turn    = 0xFFFF;
            };

            /// The index a step holds when it was drawn under no transform or reads no material.
            static constexpr UInt16 kNone = 0xFFFF;

            /// The parts drawn anew every frame, in the order they were drawn.
            Sequence<Live>      Lives;

            /// The quads recorded, in the order they were drawn.
            Sequence<Step>      Steps;

            /// The transforms the quads were drawn under, each once in a row.
            Sequence<Matrix3x2> Turns;

            /// The glyph materials the quads read, each once in a row.
            Sequence<Array<Byte, 48>> Materials;

            /// The scale the pane was drawn at, which must stand again for the tape to be replayed.
            Real32              Scale = 0.0f;

            /// The clip the pane was drawn under, which must stand again for the tape to be replayed.
            Rect                Clip;

            /// The opacity the part was drawn at, which must stand again for the tape to be replayed.
            Real32              Opacity = 1.0f;

            /// The transform the part was drawn under, which must stand again for the tape to be replayed.
            Matrix3x2           Turn;

            /// `true` when the part was drawn under a transform.
            Bool                Turned  = false;

            /// `true` when the tape holds the pane as it was last drawn, and nothing in it draws anew each frame.
            Bool                Valid = false;
        };

        /// \brief Represents a texture of the graphics service drawn as it stands, as a target or an array slice.
        struct Surface final
        {
            /// The texture, as the graphics service made it, or nought for none.
            ZyGraphic::Object        Handle = 0;

            /// The shape of the texture, where an array is read at its \ref Layer and any other as a plain texture.
            ZyGraphic::TextureLayout Layout = ZyGraphic::TextureLayout::Texture2D;

            /// The slice of an array read.
            UInt16                   Layer  = 0;

            /// How it is read between its texels.
            Filter                   Filter = Filter::Linear;

            /// `true` to show its colour alone with its alpha taken as whole, as a target whose alpha holds data.
            Bool                     Opaque = false;

            /// `true` to read it upside down, its last row at the top.
            Bool                     Flip   = false;
        };

        /// The bit of a quad's code set when its colour runs from its tint to its other colour across the box.
        static constexpr UInt16 kGradient  = 16;

        /// The bit of a quad's code set when the texture it reads shows its colour alone, its alpha taken as whole.
        static constexpr UInt16 kOpaque    = 0x8000;

        /// The bit a quad's code holds its \ref Form from.
        static constexpr UInt16 kFormShift = 13;

    public:

        /// \brief Constructs a canvas that draws nothing until it is initialized.
        Canvas();

        /// \brief Initializes the canvas with the specified host.
        ///
        /// \param Host  The engine subsystem host used to access graphics and content services.
        /// \param Space The colour space the target starts at, which \ref SetColorspace changes afterwards.
        void Initialize(Ref<ZyEngine::Subsystem::Host> Host, Colorspace Space = Colorspace::Linear);

        /// \brief Releases the textures the canvas made for itself.
        void Dispose();

        /// \brief Sets the colour space the target stores, which every draw is encoded for from here on.
        ///
        /// \param Space The colour space of the target.
        ZY_INLINE void SetColorspace(Colorspace Space)
        {
            if (mColorspace != Space)
            {
                mColorspace = Space;

                if (mTechnique && mTechnique->HasCompleted())
                {
                    ObtainPipeline();
                }
            }
        }

        /// \brief Draws every opaque quad first, nearest first, keeping its depth, so nothing under it is shaded.
        ///
        /// \param Enabled The flag, `true` to draw with depth, `false` to draw every quad in order as it was asked.
        ZY_INLINE void SetDepth(Bool Enabled)
        {
            if (mDepth != Enabled)
            {
                mDepth = Enabled;

                if (mTechnique && mTechnique->HasCompleted())
                {
                    ObtainPipeline();
                }
            }
        }

        /// \brief Starts or stops recording what is drawn into a tape, which is emptied when the recording starts.
        ///
        /// \param Into The tape to record into, or `nullptr` to stop recording.
        void Record(Ptr<Tape> Into);

        /// \brief Notes a part drawn anew every frame into the tape being recorded, and stops recording while it draws.
        ///
        /// \param Tag     The tag the replay hands back, as the row of the element drawing it.
        /// \param Opacity The opacity it is drawn at.
        /// \return The tape the recording goes on into once \ref Resume is called, or `nullptr` when none is recorded.
        Ptr<Tape> Pause(UInt32 Tag, Real32 Opacity);

        /// \brief Goes on recording into a tape \ref Pause stopped.
        ///
        /// \param Into The tape \ref Pause gave back.
        ZY_INLINE void Resume(Ptr<Tape> Into)
        {
            mTape = Into;
        }

        /// \brief Draws again what a tape recorded, as it was drawn, through the batches of this frame.
        ///
        /// \param From   The tape to replay.
        /// \param OnLive The code drawing a part anew, handed what the tape noted of it.
        void Replay(ConstRef<Tape> From, ConstRef<Delegate<void(ConstRef<Tape::Live>)>> OnLive);

        /// \brief Gets the transform quads are moved by now.
        ///
        /// \return The transform, or `nullptr` for none.
        ZY_INLINE ConstPtr<Matrix3x2> GetTransform() const
        {
            return mTransform;
        }

        /// \brief Gets the clip quads are cut to now.
        ///
        /// \return The clip, in pixels.
        ZY_INLINE Rect GetClip() const
        {
            return mClip;
        }

        /// \brief Checks whether the canvas draws with depth.
        ///
        /// \return `true` if it does, `false` otherwise.
        ZY_INLINE Bool HasDepth() const
        {
            return mDepth;
        }

        /// \brief Checks whether a quad covers every pixel of its area opaque, to draw it in the first pass.
        ///
        /// \param Drawn The quad.
        /// \return `true` for a plain box of an opaque colour or an opaque image shown whole, `false` otherwise.
        static Bool IsSolid(ConstRef<Quad> Drawn);

        /// \brief Gets the colour space the canvas encodes its draws for.
        ///
        /// \return The colour space of the target.
        ZY_INLINE Colorspace GetColorspace() const
        {
            return mColorspace;
        }

        /// \brief Opens a frame of drawing over a target of the given size, forgetting the last frame's draws.
        ///
        /// \param Width  The width of the target, in pixels.
        /// \param Height The height of the target, in pixels.
        void Begin(UInt16 Width, UInt16 Height);

        /// \brief Uploads what the frame drew and hands the engine one command per batch.
        void End();

        /// \brief Cuts everything drawn after this to a box, within whatever it was already cut to.
        ///
        /// \param Box The box to cut to, in pixels.
        void PushClip(Rect Box);

        /// \brief Returns to the cut that stood before the last \ref PushClip.
        void PopClip();

        /// \brief Moves everything drawn until the matching \ref PopTransform, inside the transforms pushed before.
        ///
        /// \param Transform The transform, in pixels of the target.
        void PushTransform(ConstRef<Matrix3x2> Transform);

        /// \brief Returns to the transform that stood before the last \ref PushTransform.
        void PopTransform();

        /// \brief Checks whether any of a box would be seen, within the current clip and under the current transform.
        ///
        /// \param Box The box, in the units drawn in, before the current transform moves it.
        /// \return `true` if some of it lies within the clip, `false` otherwise.
        ZY_INLINE Bool IsVisible(Rect Box) const
        {
            if (mTransform)
            {
                Box = Rect::Transform(Box, * mTransform);
            }

            // Seen where the two overlap by more than an edge, which a clip cut to nothing never does.
            return !Rect::Intersection(Box * mScale, mClip).IsEmpty();
        }

        /// \brief Sets how many pixels of the target one unit of what is drawn covers.
        ///
        /// \param Factor The ratio, where one draws a unit to the pixel.
        ZY_INLINE void SetScale(Real32 Factor)
        {
            mScale = Factor;
            mSlot  = 0;
        }

        /// \brief Gets how many pixels of the target one unit of what is drawn covers.
        ///
        /// \return The ratio, where one draws a unit to the pixel.
        ZY_INLINE Real32 GetScale() const
        {
            return mScale;
        }

        /// \brief Draws a soft rounded box whose edge fades out over a blur, as a shadow or a glow is drawn.
        ///
        /// \param Box    The box at the middle of the fade, in pixels, already moved and grown as the shadow is.
        /// \param Tint   The colour at its most solid.
        /// \param Radius The radius its corners are rounded to, in pixels.
        /// \param Blur   The distance the edge fades out over on either side of the box, in pixels.
        /// \param Hole   The box an opaque fill drawn next covers whole, in pixels, or an empty one for none.
        void DrawShadow(Rect Box, Color Tint, Real32 Radius, Real32 Blur, Rect Hole = Rect());

        /// \brief Draws a glow just inside a rounded box, solid at its edge and fading out inward.
        ///
        /// \param Box    The box, in pixels.
        /// \param Tint   The colour at its edge.
        /// \param Radius The radius its corners are rounded to, in pixels.
        /// \param Blur   The distance the glow fades out over inward, in pixels.
        void DrawInnerGlow(Rect Box, Color Tint, Real32 Radius, Real32 Blur);

        /// \brief Fills a box with a gradient.
        ///
        /// \param Box    The box to fill, in pixels.
        /// \param Fill   The gradient, running between its two colours at its angle.
        /// \param Radius The radius its corners are rounded to, in pixels.
        /// \param Hole   The box inside it that something opaque drawn after covers, or an empty one.
        void DrawGradient(Rect Box, ConstRef<Gradient> Fill, Real32 Radius = 0.0f, Rect Hole = Rect());

        /// \brief Fills a box with a colour.
        ///
        /// \param Box    The box to fill, in pixels.
        /// \param Tint   The colour to fill it with.
        /// \param Radius The radius its corners are rounded to, in pixels.
        /// \param Hole   The box an opaque fill drawn later covers whole, in pixels, or an empty one for none.
        void DrawRect(Rect Box, Color Tint, Real32 Radius = 0.0f, Rect Hole = Rect());

        /// \brief Draws the outline of a box.
        ///
        /// \param Box    The box to outline, in pixels.
        /// \param Tint   The colour of the outline.
        /// \param Width  The thickness of the outline, drawn inside the box, in pixels.
        /// \param Radius The radius the box's corners are rounded to, in pixels.
        void DrawFrame(Rect Box, Color Tint, Real32 Width, Real32 Radius = 0.0f);

        /// \brief Draws part of an image stretched over a box.
        ///
        /// \param Image    The image to draw, which must have finished loading.
        /// \param Box      The box to draw it over, in pixels.
        /// \param Source   The part of the image to draw, in texture coordinates.
        /// \param Tint     The colour the image is multiplied by.
        /// \param Radius   The radius the box's corners are rounded to, in pixels.
        /// \param Layer    The slice read when the image is an array, ignored otherwise.
        /// \param Sampling The way the image is read between its texels.
        void DrawImage(ConstRef<ZyGraphic::Image> Image,
                       Rect                       Box,
                       Rect                       Source,
                       Color                      Tint,
                       Real32                     Radius = 0.0f,
                       UInt16                     Layer = 0,
                       Filter                     Sampling = Filter::Linear);

        /// \brief Draws part of an image over a box in nine pieces, keeping its border unstretched.
        ///
        /// \param Image    The image to draw, which must have finished loading.
        /// \param Box      The box to draw it over, in pixels.
        /// \param Source   The part of the image to draw, in texture coordinates.
        /// \param Slice    The border kept unstretched on each side, in pixels of the image.
        /// \param Tint     The colour the image is multiplied by.
        /// \param Layer    The slice read when the image is an array, ignored otherwise.
        /// \param Sampling The way the image is read between its texels.
        void DrawSliced(ConstRef<ZyGraphic::Image> Image,
                        Rect                       Box,
                        Rect                       Source,
                        Edges                      Slice,
                        Color                      Tint,
                        UInt16                     Layer = 0,
                        Filter                     Sampling = Filter::Linear);

        /// \brief Draws part of any texture of the graphics service stretched over a box, as a render target is shown.
        ///
        /// \param Texture The texture and how it is read, which must have been made by the graphics service.
        /// \param Box     The box to draw it over, in pixels.
        /// \param Source  The part of the texture to draw, in texture coordinates, its top row first unless it flips.
        /// \param Tint    The colour the texture is multiplied by.
        /// \param Radius  The radius the box's corners are rounded to, in pixels.
        void DrawTexture(ConstRef<Surface> Texture, Rect Box, Rect Source, Color Tint, Real32 Radius = 0.0f);

        /// \brief Draws part of any texture of the graphics service over a box in nine pieces, its border unstretched.
        ///
        /// \param Texture The texture and how it is read, which must have been made by the graphics service.
        /// \param Size    The size of the texture, in its own pixels, which the border is measured against.
        /// \param Box     The box to draw it over, in pixels.
        /// \param Source  The part of the texture to draw, in texture coordinates.
        /// \param Slice   The border kept unstretched on each side, in pixels of the texture.
        /// \param Tint    The colour the texture is multiplied by.
        void DrawSliced(ConstRef<Surface> Texture, Vector2 Size, Rect Box, Rect Source, Edges Slice, Color Tint);

        /// \brief Draws a straight line of any angle, its edges smoothed, as one quad.
        ///
        /// \param From  The point the line starts at, in pixels.
        /// \param To    The point the line ends at, in pixels.
        /// \param Tint  The colour of the line.
        /// \param Width The thickness of the line, in pixels.
        /// \param Ends  The way both ends are drawn.
        void DrawLine(Vector2 From, Vector2 To, Color Tint, Real32 Width = 1.0f, Cap Ends = Cap::Butt);

        /// \brief Draws lines joining a run of points, one quad for each, overlapping where they meet.
        ///
        /// \param Points The points, in pixels, in the order the lines join them.
        /// \param Tint   The colour of the lines.
        /// \param Width  The thickness of the lines, in pixels.
        /// \param Closed The flag, `true` to join the last point back to the first, `false` to leave the run open.
        /// \param Ends   The way the ends of every line are drawn.
        void DrawPolyline(ConstSpan<Vector2> Points,
                          Color              Tint,
                          Real32             Width = 1.0f,
                          Bool               Closed = false,
                          Cap                Ends = Cap::Butt);

        /// \brief Fills a triangle, its edges smoothed, as one quad.
        ///
        /// \param First  The first corner, in pixels.
        /// \param Second The second corner, in pixels.
        /// \param Third  The third corner, in pixels.
        /// \param Tint   The colour to fill it with.
        void DrawTriangle(Vector2 First, Vector2 Second, Vector2 Third, Color Tint);

        /// \brief Fills a convex figure of four corners as one quad.
        ///
        /// \param First  The first corner, in pixels.
        /// \param Second The corner after it around the figure, either way round.
        /// \param Third  The corner after that.
        /// \param Fourth The last corner, beside the first.
        /// \param Tint   The colour to fill it with.
        void DrawQuad(Vector2 First, Vector2 Second, Vector2 Third, Vector2 Fourth, Color Tint);

        /// \brief Fills a circle, as a box rounded whole.
        ///
        /// \param Center The middle of the circle, in pixels.
        /// \param Radius The radius, in pixels, rounded as a corner is up to 8191 pixels of the target.
        /// \param Tint   The colour to fill it with.
        void DrawCircle(Vector2 Center, Real32 Radius, Color Tint);

        /// \brief Draws the outline of a circle, centred on its radius, as a box rounded whole.
        ///
        /// \param Center The middle of the circle, in pixels.
        /// \param Radius The radius the middle of the outline runs along, in pixels.
        /// \param Tint   The colour of the outline.
        /// \param Width  The thickness of the outline, in pixels.
        void DrawRing(Vector2 Center, Real32 Radius, Color Tint, Real32 Width = 1.0f);

        /// \brief Draws part of the outline of a circle as short lines, as the handle of a turn is.
        ///
        /// \param Center The middle of the circle, in pixels.
        /// \param Radius The radius, in pixels.
        /// \param From   The angle the arc starts at, nought pointing right and a quarter turn pointing down.
        /// \param To     The angle the arc ends at, which may run either way from the start.
        /// \param Tint   The colour of the arc.
        /// \param Width  The thickness of the arc, in pixels.
        /// \param Ends   The way the two ends of the arc are drawn.
        void DrawArc(Vector2 Center,
                     Real32  Radius,
                     Angle   From,
                     Angle   To,
                     Color   Tint,
                     Real32  Width = 1.0f,
                     Cap     Ends = Cap::Butt);

        /// \brief Draws the outline of an ellipse spanned by two axes as short lines, as a projected circle.
        ///
        /// \param Center   The middle of the ellipse, in pixels.
        /// \param AxisX    The offset from the middle to the point the ellipse starts at, in pixels.
        /// \param AxisY    The offset to the point a quarter of the way round, in pixels.
        /// \param Tint     The colour of the outline.
        /// \param Width    The thickness of the outline, in pixels.
        /// \param Segments The count of lines, or nought for as many as keep it smooth at its size.
        void DrawEllipse(Vector2 Center,
                         Vector2 AxisX,
                         Vector2 AxisY,
                         Color   Tint,
                         Real32  Width = 1.0f,
                         UInt32  Segments = 0);

        /// \brief Draws text with its first line's top at a point.
        ///
        /// \param Font    The typeface to set the text in, which must have finished loading.
        /// \param Content The text to draw.
        /// \param Origin  The top left corner of the text, in pixels.
        /// \param Size    The size to set the text at, in pixels.
        /// \param Tint    The colour to draw the text in.
        /// \param Effect  The outline the text is drawn with, its offset and bias in pixels, scaled as every length is.
        void DrawText(ConstRef<ZyRender::Font>       Font,
                      Text                           Content,
                      Vector2                        Origin,
                      Real32                         Size,
                      Color                          Tint,
                      ConstRef<ZyRender::FontEffect> Effect = ZyRender::FontEffect());

        /// \brief Draws a line of glyphs the font already placed, as \ref DrawText would, its top at a point.
        ///
        /// \param Font   The typeface that placed the glyphs, which must have finished loading.
        /// \param Glyphs The glyphs, where the font placed them, in font units from the start of the line.
        /// \param Origin The top left corner of the line, in pixels.
        /// \param Size   The size to set the text at, in pixels.
        /// \param Tint   The colour to draw the text in.
        /// \param Effect The outline the text is drawn with, its offset and bias in pixels, scaled as every length is.
        void DrawGlyphs(ConstRef<ZyRender::Font>             Font,
                        ConstSpan<ZyRender::Font::Placement> Glyphs,
                        Vector2                              Origin,
                        Real32                               Size,
                        Color                                Tint,
                        ConstRef<ZyRender::FontEffect>       Effect = ZyRender::FontEffect());

        /// \brief Gets the quads drawn since \ref Begin, as \ref End uploads them.
        ///
        /// \return The quads, in the order they were drawn.
        ZY_INLINE ConstSpan<Quad> GetQuads() const
        {
            return ConstSpan(mQuads.GetData(), mCount);
        }

        /// \brief Gets the box a quad covers on the target, once its transform has moved it.
        ///
        /// \param Index The quad, in the order drawn.
        /// \return The box around its part drawn, in pixels of the target.
        Rect GetPlace(UInt32 Index) const;

        /// \brief Gets the count of batches drawn since \ref Begin, each one command once \ref End runs.
        ///
        /// \return The count of batches.
        ZY_INLINE UInt32 GetBatchCount() const
        {
            return static_cast<UInt32>(mBatches.GetSize());
        }

        /// \brief Gets how a batch reads the texture of the slot a kind of paint takes.
        ///
        /// \param Index The batch, in the order drawn.
        /// \param Kind  The paint whose slot is asked about, where a colour or a glyph is always read linearly.
        /// \return The filter the slot's sampler reads with.
        ZY_INLINE Filter GetFilter(UInt32 Index, Paint Kind) const
        {
            return mBatches[Index].Filters[SlotOf(Kind)];
        }

    private:

        /// \brief Defines the texture coordinates of a quad's edges, over 16 bits.
        using Texels = Array<UInt16, 4>;

        /// \brief Defines a type alias for the shape a quad carries, as \ref Quad::Shape lays it out.
        using Steps = Array<UInt16, 4>;

        /// \brief Defines a type alias for two rows of a transform, as the shader dots a point with each.
        using Affine = Array<Real32, 8>;

        /// The texture coordinates of a whole texture.
        static constexpr Texels kWhole      = { 0, 0, 0xFFFF, 0xFFFF };

        /// The transform that moves nothing, which starts every block of transforms.
        static constexpr Affine kIdentity   = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };

        /// The steps a pixel is split into where a quad carries a length as a whole number, so 16 bits reach 8191.
        static constexpr Real32 kSubpixel   = 8.0f;

        /// The materials one block holds, as the shader declares its array.
        static constexpr UInt32 kMaterials  = 256;

        /// The transforms one block holds, the first always the identity, as the shader declares its array.
        static constexpr UInt32 kTransforms = 256;

        /// The pixels a middle must cover before the three more quads that split it off cost less than shading it.
        static constexpr Real32 kLarge      = 16384.0f;

        /// \brief Represents what a glyph run reads besides its atlas, as the shader's array element.
        struct Material final
        {
            /// The width of the atlas's distance range, in texture coordinates.
            Vector2 Range;

            /// Unused, so the effect starts on the boundary the block aligns it to.
            Vector2 Padding;

            /// The colour of the outline, packed with red in the lowest byte.
            UInt32  Tint      = 0;

            /// How far the outline is pushed past the stroke, in pixels of the target.
            Real32  Offset    = 0.0f;

            /// The width of the outline as a share of the distance range.
            Real32  Width     = 0.0f;

            /// The width added to the outline, in pixels of the target.
            Real32  Bias      = 0.0f;

            /// How much of the outline fades out, from nought to one.
            Real32  Blur      = 0.0f;

            /// The blend from the sharp distance at nought to the smooth one at one.
            Real32  Roundness = 0.0f;

            /// The distance the stroke's edge sits at.
            Real32  Threshold = 0.0f;

            /// Unused, so each element ends on the boundary the block aligns the next to.
            UInt32  Tail      = 0;
        };

        /// \brief Represents a run of quads drawn as one command.
        struct Batch final
        {
            /// The textures of the atlas, image and layer slots, nought where no quad of the run claimed one.
            Array<ZyGraphic::Object, 3> Textures;

            /// How the texture of each slot is read, which the run's first quad to claim the slot chose.
            Array<Filter, 3>            Filters;

            /// The box the run is cut to, in pixels, which only turned quads and figures need narrower than the target.
            Rect                        Scissor;

            /// The block of materials the run's glyphs read.
            UInt32                      Materials;

            /// The block of transforms the run's quads read.
            UInt32                      Transforms;

            /// The first quad of the run, which runs on to the next batch's first or to the last quad drawn.
            UInt32                      Offset;
        };

        /// \brief Represents a run of quads one command draws, a batch's whole or the share of it one pass takes.
        struct Pass final
        {
            /// The batch whose textures, clip, materials and transforms the run reads.
            UInt32 Batch  = 0;

            /// The first quad of the run, among the quads sent.
            UInt32 Offset = 0;

            /// The count of quads of the run.
            UInt32 Count  = 0;

            /// `true` when the run draws opaque quads nearest first, keeping their depth.
            Bool   Solid  = false;
        };

        /// \brief Represents a line of text being drawn, which the font hands each glyph as it places it.
        struct Run final
        {
            /// The canvas the line is drawn through.
            Ptr<Canvas>              Owner = nullptr;

            /// The typeface the line is set in.
            ConstPtr<ZyRender::Font> Font  = nullptr;

            /// The outline the line is drawn with, its lengths already scaled.
            ZyRender::FontEffect     Effect;

            /// The left of the line and its baseline, in the units drawn in.
            Vector2                  Origin;

            /// The size the line is set at, in the units drawn in.
            Real32                   Size  = 0.0f;

            /// The colour, packed.
            UInt32                   Tint  = 0;

            /// The shape every glyph of the page carries, its material included.
            Steps                    Shape = { };

            /// The atlas page of the last glyph, or the maximum before the first.
            UInt32                   Page  = kMaximum<UInt32>;

            /// `true` while the page's atlas has loaded and the clip lets something through.
            Bool                     Ready = false;

            /// \brief Draws one glyph where the font placed it.
            ///
            /// \param Placement The place the font gave the glyph, in font units, and where its atlas holds it.
            void operator()(ConstRef<ZyRender::Font::Placement> Placement);
        };

        /// \brief Resolves the technique variant the canvas draws with under the current colour space.
        void ObtainPipeline();

        /// \brief Finds a glyph material among the last few of the current block, or adds it.
        ///
        /// \param Wanted The material.
        /// \return The slot of the material within its block.
        UInt32 AdoptMaterial(ConstRef<Material> Wanted);

        /// \brief Records the quad just emitted into the tape, with the batch, clip, transform and material.
        ///
        /// \param Drawn The quad.
        void Note(ConstRef<Quad> Drawn);

        /// \brief Orders the frame's quads for drawing with depth and lists the runs that draw them.
        void Arrange();

        /// \brief Sets a line of text up to be handed its glyphs.
        ///
        /// \param Font   The typeface the line is set in.
        /// \param Origin The top left corner of the line, in pixels.
        /// \param Size   The size to set the text at, in pixels.
        /// \param Tint   The colour to draw the text in.
        /// \param Effect The outline the text is drawn with, before it is scaled.
        /// \return The line, ready for its first glyph.
        Run Prepare(ConstRef<ZyRender::Font>       Font,
                    Vector2                        Origin,
                    Real32                         Size,
                    Color                          Tint,
                    ConstRef<ZyRender::FontEffect> Effect);

        /// \brief Represents how the lines of a stroke are drawn, in pixels of the target.
        struct Pen final
        {
            /// The colour, packed, faded by how much thinner than a pixel the stroke was asked to be.
            UInt32 Tint   = 0;

            /// Half the thickness, never under half a pixel.
            Real32 Half   = 0.0f;

            /// How far each end stands past its point.
            Real32 Reach  = 0.0f;

            /// The radius each end is rounded to.
            Real32 Radius = 0.0f;
        };

        /// \brief Makes the last batch the one the next quads land in, opening a new one when it must.
        ///
        /// \param Kind     The paint the quads are filled with, which names the slot their texture takes.
        /// \param Texture  The texture the quads sample, or nought for a colour.
        /// \param Sampling The way the texture is read between its texels.
        /// \param Uncut    The flag, `true` when the quads cannot be cut to the clip on the spot, `false` otherwise.
        /// \return `true` if the quads can be drawn, `false` otherwise.
        Bool Open(Paint Kind, ZyGraphic::Object Texture, Filter Sampling = Filter::Linear, Bool Uncut = false);

        /// \brief Writes the quad of a figure into the last batch \ref Open made ready, unless the clip misses it.
        ///
        /// \param Area   The figure's first two points, in pixels of the target before the transform.
        /// \param Box    The figure's last two points, or what a segment carries there, in the same pixels.
        /// \param Bounds The box the figure covers, its smoothed edge included, in the units drawn in.
        /// \param Tint   The colour, packed.
        /// \param Code   The code, its transform still left out.
        void Plot(Rect Area, Rect Box, Rect Bounds, UInt32 Tint, UInt16 Code);

        /// \brief Writes one line between two points into the last batch \ref Open made ready.
        ///
        /// \param From  The point the line starts at, in the units drawn in.
        /// \param To    The point the line ends at, in the same units.
        /// \param Brush The way the line is drawn.
        /// \param Lead  The length the line is drawn on past its start, in the same units, as a joint overlaps.
        /// \param Trail The length the line is drawn on past its end, in the same units.
        void Stroke(Vector2 From, Vector2 To, ConstRef<Pen> Brush, Real32 Lead = 0.0f, Real32 Trail = 0.0f);

        /// \brief Writes the lines around part of an ellipse into the last batch \ref Open made ready.
        ///
        /// \param Center The middle of the ellipse, in the units drawn in.
        /// \param AxisX  The offset to the point at an angle of nought, in the same units.
        /// \param AxisY  The offset to the point a quarter turn round, in the same units.
        /// \param Start  The angle the run starts at, in radians.
        /// \param Sweep  The angle the run turns through, in radians, either way round.
        /// \param Steps  The count of lines.
        /// \param Closed The flag, `true` when the run goes the whole way round, so no end of it is left open.
        /// \param Brush  The way the lines are drawn.
        /// \param Ends   The way the open ends are drawn.
        void Trace(Vector2       Center,
                   Vector2       AxisX,
                   Vector2       AxisY,
                   Real32        Start,
                   Real32        Sweep,
                   UInt32        Steps,
                   Bool          Closed,
                   ConstRef<Pen> Brush,
                   Cap           Ends);

        /// \brief Writes a convex figure of four corners into the last batch \ref Open made ready.
        ///
        /// \param First  The first corner, in the units drawn in.
        /// \param Second The corner after it around the figure.
        /// \param Third  The corner after that.
        /// \param Fourth The last corner, the third again for a triangle.
        /// \param Tint   The colour, packed.
        void Fill(Vector2 First, Vector2 Second, Vector2 Third, Vector2 Fourth, UInt32 Tint);

        /// \brief Gets the shape a quad of a texture carries, its slice and whether its alpha counts included.
        ///
        /// \param Texture The texture and how it is read.
        /// \param Kind    The paint the texture fills its quads with.
        /// \param Radius  The radius the box's corners are rounded to, in the units drawn in.
        /// \return The shape, its code still without the transform.
        Steps ShapeOf(ConstRef<Surface> Texture, Paint Kind, Real32 Radius) const;

        /// \brief Gets how a stroke of a thickness is drawn at the current scale.
        ///
        /// \param Tint  The colour of the stroke.
        /// \param Width The thickness, in the units drawn in.
        /// \param Ends  The way the ends of its lines are drawn.
        /// \return The pen, in pixels of the target.
        Pen PenOf(Color Tint, Real32 Width, Cap Ends) const;

        /// \brief Gets the count of lines that keeps part of an ellipse smooth at the current scale.
        ///
        /// \param Radius The longer of its two radii, in the units drawn in.
        /// \param Sweep  The angle the part turns through, in radians.
        /// \return The count, at least one and at most 512.
        UInt32 StepsOf(Real32 Radius, Real32 Sweep) const;

        /// \brief Writes one quad into the last batch \ref Open made ready, cut to the clip when square to the target.
        ///
        /// \param Box    The box, in the units drawn in, its minimum corner being the top left.
        /// \param Area   The part of the box to draw, in the same units.
        /// \param Source The texture coordinates of the box's edges.
        /// \param Tint   The colour, packed.
        /// \param Other  The colour a gradient ends at, packed.
        /// \param Shape  The shape, its code still without the transform.
        void Emit(Rect Box, Rect Area, ConstRef<Texels> Source, UInt32 Tint, UInt32 Other, Steps Shape);

        /// \brief Writes a box of one colour into the last batch, leaving a large inner part out.
        ///
        /// \param Box   The box, in the units drawn in.
        /// \param Inner The part nothing shows in, within the box, in the same units, drawn over when it is small.
        /// \param Tint  The colour, packed.
        /// \param Shape The shape, its code still without the transform.
        void Hollow(Rect Box, Rect Inner, UInt32 Tint, Steps Shape);

        /// \brief Writes a box into the last batch, leaving a large covered hole out.
        ///
        /// \param Box    The box, in the units drawn in.
        /// \param Hole   The part an opaque fill drawn later covers whole, in the same units, or an empty one.
        /// \param Source The texture coordinates of the box's edges.
        /// \param Tint   The colour, packed.
        /// \param Other  The colour a gradient ends at, packed.
        /// \param Shape  The shape, its code still without the transform.
        void Pave(Rect Box, Rect Hole, ConstRef<Texels> Source, UInt32 Tint, UInt32 Other, Steps Shape);

        /// \brief Writes the four bands between two boxes, one within the other, each reading the whole of a box.
        ///
        /// \param Box    The box every band reads, in the units drawn in.
        /// \param Outer  The outer edge of the bands, within the box, in the same units.
        /// \param Inner  The part left out, within the outer edge, in the same units.
        /// \param Source The texture coordinates of the box's edges.
        /// \param Tint   The colour, packed.
        /// \param Other  The colour a gradient ends at, packed.
        /// \param Shape  The shape, its code still without the transform.
        void Surround(Rect             Box,
                      Rect             Outer,
                      Rect             Inner,
                      ConstRef<Texels> Source,
                      UInt32           Tint,
                      UInt32           Other,
                      Steps            Shape);

        /// \brief Checks whether a part of a box is worth the bands drawn around it to leave it out or draw it plain.
        ///
        /// \param Middle The part, in the units drawn in.
        /// \return `true` if it covers at least \ref kLarge pixels of the target, `false` otherwise.
        Bool IsLarge(Rect Middle) const;

        /// \brief Gets the shape a quad of a box carries, its lengths turned to steps at the current scale.
        ///
        /// \param Edge    The way the edge is cut.
        /// \param Kind    The paint the quad is filled with.
        /// \param Radius  The radius of the corners, in the units drawn in.
        /// \param Outline The thickness of the outline, in the units drawn in, or nought for none.
        /// \param Blur    The distance a soft edge fades over, in the units drawn in.
        /// \return The shape, its code still without the transform.
        Steps Measure(Cut Edge, Paint Kind, Real32 Radius, Real32 Outline, Real32 Blur) const;

        /// \brief Gets the transform the next quads read, adding the current one to the frame's when it has none yet.
        ///
        /// \return The transform's place in its block, nought for none.
        UInt32 ObtainTransform();

        /// \brief Finds the material a glyph run reads, adding it when no recent one matches.
        ///
        /// \param Range  The width of the atlas's distance range, in texture coordinates.
        /// \param Effect The outline the text is drawn with.
        /// \return The material's place in its block.
        UInt32 ObtainMaterial(Vector2 Range, ConstRef<ZyRender::FontEffect> Effect);

    public:

        /// \brief Gets the slot of a batch the texture of a kind of paint takes.
        ///
        /// \param Kind The paint, where a colour shares the glyph's slot as it claims none.
        /// \return The slot, nought for the atlas, one for the image and two for the layer.
        ZY_INLINE static constexpr UInt32 SlotOf(Paint Kind)
        {
            return (Kind == Paint::Image) ? 1 : (Kind == Paint::Layered) ? 2 : 0;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Retainer<ZyGraphic::Service>   mGraphics;
        Retainer<ZyGraphic::Technique> mTechnique;
        ZyGraphic::Object              mPipeline;
        ZyGraphic::Object              mSolid;
        ZyGraphic::Object              mBehind;
        Bool                           mDepth;
        Ptr<Tape>                      mTape;
        Paint                          mKind;
        ZyGraphic::Object              mTexture;
        Filter                         mSampling;
        Bool                           mUncut;
        Matrix3x2                      mReplayed;
        ZyGraphic::Object              mSampler;
        ZyGraphic::Object              mPoint;
        ZyGraphic::Object              mBlank;
        ZyGraphic::Object              mSheet;
        Colorspace                     mColorspace;
        Real32                         mScale;
        Matrix4x4                      mProjection;
        Rect                           mViewport;
        Rect                           mClip;
        ConstPtr<Matrix3x2>            mTransform;
        UInt32                         mSlot;
        Sequence<Rect>                 mClips;
        Sequence<Matrix3x2>            mTransforms;
        Sequence<Quad>                 mQuads;
        UInt32                         mCount;
        Sequence<Batch>                mBatches;
        Sequence<Quad>                 mOrdered;
        Sequence<Pass>                 mPasses;
        Sequence<Material>             mMaterials;
        Sequence<Affine>               mAffines;
    };
}