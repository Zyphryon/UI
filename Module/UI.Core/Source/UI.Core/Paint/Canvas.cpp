// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Canvas.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include <Zyphryon.Content/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Canvas::Canvas()
        : mPipeline   { 0 },
          mSolid      { 0 },
          mBehind     { 0 },
          mDepth      { false },
          mTape       { nullptr },
          mKind       { Paint::Colour },
          mTexture    { 0 },
          mSampling   { Filter::Linear },
          mUncut      { false },
          mSampler    { 0 },
          mPoint      { 0 },
          mBlank      { 0 },
          mSheet      { 0 },
          mColorspace { Colorspace::Linear },
          mScale      { 1.0f },
          mTransform  { nullptr },
          mSlot       { 0 },
          mCount      { 0 }
    {
    }

    void Canvas::Initialize(Ref<ZyEngine::Subsystem::Host> Host, Colorspace Space)
    {
        mGraphics   = Host.GetService<ZyGraphic::Service>();
        mColorspace = Space;

        ConstRetainer<ZyContent::Service> Content = Host.GetService<ZyContent::Service>();

        // One technique declares every colour space as a feature, and preloads each of them.
        mTechnique = Content->Load<ZyGraphic::Technique>("Embedded://Technique/UI/Canvas.vfx");

        // The first frame to find the technique loaded resolves the variant, as a callback could outlive the canvas.
        mPipeline = 0;

        mSampler = mGraphics->ObtainSampler(ZyGraphic::Sampler {
            .Filter = ZyGraphic::TextureFilter::Linear
        });

        // Pixel art and tiles are read texel by texel, so a zoomed preview keeps its edges.
        mPoint = mGraphics->ObtainSampler(ZyGraphic::Sampler {
            .Filter = ZyGraphic::TextureFilter::Point
        });

        // A slot no quad of a batch claims still holds a texture of the kind the shader declares there.
        constexpr Array<Byte, 4> kWhite(0xFF, 0xFF, 0xFF, 0xFF);

        Blob Flat    = Blob::Copy<Byte>(kWhite);
        Blob Layered = Blob::Copy<Byte>(kWhite);

        mBlank = mGraphics->CreateTexture(
            ZyGraphic::TextureLayout::Texture2D,
            ZyGraphic::TextureFormat::RGBA8UIntNorm,
            ZyGraphic::Storage::Immutable,
            ZyGraphic::Usage::Sample,
            1,
            1,
            1,
            1,
            Move(Flat));

        mSheet = mGraphics->CreateTexture(
            ZyGraphic::TextureLayout::Texture2DArray,
            ZyGraphic::TextureFormat::RGBA8UIntNorm,
            ZyGraphic::Storage::Immutable,
            ZyGraphic::Usage::Sample,
            1,
            1,
            1,
            1,
            Move(Layered));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Dispose()
    {
        if (mBlank)
        {
            mGraphics->DeleteTexture(mBlank);
            mBlank = 0;
        }

        if (mSheet)
        {
            mGraphics->DeleteTexture(mSheet);
            mSheet = 0;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::ObtainPipeline()
    {
        const Bool Decodes = (mColorspace == Colorspace::sRGB || mColorspace == Colorspace::sRGBUnmanaged);
        const Bool Encodes = (mColorspace == Colorspace::sRGBUnmanaged);

        const ZyGraphic::Technique::Key Decoded = Decodes ? mTechnique->ResolveByName("sRGB") : 0;
        const ZyGraphic::Technique::Key Encoded = Encodes ? mTechnique->ResolveByName("Encode") : 0;

        mPipeline = mTechnique->Obtain(* mGraphics, Decoded | Encoded);

        // Drawing with depth takes two variants more, the opaque pass and the one tested against it.
        const ZyGraphic::Technique::Key Solid  = mDepth ? mTechnique->ResolveByName("Solid") : 0;
        const ZyGraphic::Technique::Key Behind = mDepth ? mTechnique->ResolveByName("Behind") : 0;

        mSolid  = Solid ? mTechnique->Obtain(* mGraphics, Decoded | Encoded | Solid) : 0;
        mBehind = Behind ? mTechnique->Obtain(* mGraphics, Decoded | Encoded | Behind) : 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Canvas::IsSolid(ConstRef<Quad> Drawn)
    {
        const UInt16 Code = Drawn.Shape[3];
        const UInt16 Kind = (Code >> 2u) & 3u;

        // Only a box square to its edges covers all of its quad; a rounded edge, a line or a figure fades.
        if ((Code & 3u) != ZyEnum::Cast(Cut::Plain) || ((Code >> kFormShift) & 3u) != 0u)
        {
            return false;
        }

        const Bool Whole = (Drawn.Tint >> 24u) == 0xFFu && (!(Code & kGradient) || (Drawn.Other >> 24u) == 0xFFu);

        if (Kind == ZyEnum::Cast(Paint::Colour))
        {
            return Whole;
        }
        return Kind != ZyEnum::Cast(Paint::Glyph) && Whole && (Code & kOpaque);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Begin(UInt16 Width, UInt16 Height)
    {
        mBatches.Clear();
        mClips.Clear();
        mTransforms.Clear();
        mMaterials.Clear();
        mAffines.Clear();

        // The first transform of a block stands for none, so a quad square to the target reads it from any block.
        mAffines.Append(kIdentity);

        // The quads are kept from frame to frame, so a frame only grows them past the most any frame drew.
        mCount      = 0;
        mSlot       = 0;
        mViewport   = Rect(0.0f, 0.0f, Width, Height);
        mClip       = mViewport;
        mTransform  = nullptr;
        mProjection = Matrix4x4::CreateOrthographic(0.0f, Width, Height, 0.0f, -1.0f, +1.0f);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::End()
    {
        ZY_PROFILE_SCOPE("UI::Canvas::End");

        ZY_PROFILE_INTEGER("UI::Batches", mBatches.GetSize());
        ZY_PROFILE_INTEGER("UI::Quads",   mCount);

#if defined(ZY_PROFILE_BACKEND_TRACY)
        // The pixels each kind of quad shades, and all of them over the target's, which tells overdraw from the rest.
        Real64 Plain   = 0.0;
        Real64 Shaped  = 0.0;
        Real64 Images  = 0.0;
        Real64 Glyphs  = 0.0;
        Real64 Rounded = 0.0;
        Real64 Shadows = 0.0;
        Real64 Insets  = 0.0;
        Real64 Lines   = 0.0;
        Real64 Figures = 0.0;

        for (UInt Index = 0; Index < mCount; ++Index)
        {
            ConstRef<Quad> Drawn = mQuads[Index];
            const Real64   Area  = static_cast<Real64>(Drawn.Area.GetWidth()) *
                                   static_cast<Real64>(Drawn.Area.GetHeight());
            const UInt16   Kind  = (Drawn.Shape[3] >> 2u) & 3u;

            if (Kind == ZyEnum::Cast(Paint::Glyph))
            {
                Glyphs += Area;
            }
            else if (Kind != ZyEnum::Cast(Paint::Colour))
            {
                Images += Area;
            }
            else if ((Drawn.Shape[3] & 3u) == ZyEnum::Cast(Cut::Plain) && ((Drawn.Shape[3] >> kFormShift) & 3u) == 0u)
            {
                Plain += Area;
            }
            else
            {
                const UInt16 Corners = (Drawn.Shape[3] >> kFormShift) & 3u;
                const UInt16 Edge    = Drawn.Shape[3] & 3u;
                const Bool   Boxed   = Corners == ZyEnum::Cast(Form::Box);

                Shaped  += Area;
                Lines   += Corners == ZyEnum::Cast(Form::Segment) ? Area : 0.0;
                Figures += Corners == ZyEnum::Cast(Form::Polygon) ? Area : 0.0;
                Rounded += Boxed && Edge == ZyEnum::Cast(Cut::Rounded) ? Area : 0.0;
                Shadows += Boxed && Edge == ZyEnum::Cast(Cut::Shadow) ? Area : 0.0;
                Insets  += Boxed && Edge == ZyEnum::Cast(Cut::Inset) ? Area : 0.0;
            }
        }

        const Real64 Screen = Max(static_cast<Real64>(mViewport.GetWidth()) *
                                  static_cast<Real64>(mViewport.GetHeight()), 1.0);

        ZY_PROFILE_REAL("UI::Pixels::Plain",  Plain);
        ZY_PROFILE_REAL("UI::Pixels::Shaped", Shaped);
        ZY_PROFILE_REAL("UI::Pixels::Images", Images);
        ZY_PROFILE_REAL("UI::Pixels::Glyphs", Glyphs);
        ZY_PROFILE_REAL("UI::Shaped::Rounded", Rounded);
        ZY_PROFILE_REAL("UI::Shaped::Shadows", Shadows);
        ZY_PROFILE_REAL("UI::Shaped::Insets",  Insets);
        ZY_PROFILE_REAL("UI::Shaped::Lines",   Lines);
        ZY_PROFILE_REAL("UI::Shaped::Figures", Figures);
        ZY_PROFILE_REAL("UI::Overdraw",       (Plain + Shaped + Images + Glyphs) / Screen);
#endif

        // Nothing is drawn until the technique has uploaded its variants.
        if (mCount == 0 || !mTechnique || !mTechnique->HasCompleted())
        {
            return;
        }

        // The first frame to find the technique loaded resolves its variant; one with no program has nothing to draw.
        if (!mPipeline)
        {
            ObtainPipeline();

            if (!mPipeline)
            {
                return;
            }
        }

        static_assert(sizeof(Quad) == 60, "The quad must match the technique's layout");
        static_assert(sizeof(Material) == 48, "The material must match an element of the shader's array");

        // The quads go up in one copy, and every one reads the same six corners, two triangles across its four.
        constexpr Array<UInt16, 6> kCorners(0, 1, 2, 2, 1, 3);

        // With depth, opaque quads go first nearest first, then the rest in order; a batch is at most two commands.
        const Bool Layered = mDepth && mSolid && mBehind;

        mPasses.Clear();
        mOrdered.Clear();

        if (Layered)
        {
            Arrange();
        }
        else
        {
            for (UInt32 Number = 0; Number < mBatches.GetSize(); ++Number)
            {
                const UInt32 Last = (Number + 1 < mBatches.GetSize()) ? mBatches[Number + 1].Offset : mCount;
                mPasses.Append(Number, mBatches[Number].Offset, Last - mBatches[Number].Offset);
            }
        }

        const ConstSpan<Quad>      Sent  = Layered
            ? ConstSpan(mOrdered.GetData(), mOrdered.GetSize())
            : ConstSpan(mQuads.GetData(), mCount);
        ZyGraphic::Transient<Quad> Quads = mGraphics->AllocateInFlightVertices<Quad>(
            static_cast<UInt32>(Sent.GetSize()));
        Quads.Copy(Sent, 0);

        const ConstSpan         Strip(kCorners.GetData(), kCorners.GetSize());
        const ZyGraphic::Stream Corners = mGraphics->AllocateInFlightIndices<UInt16>(Strip);
        const ZyGraphic::Stream Camera  = mGraphics->AllocateInFlightUniforms<Matrix4x4>(ConstSpan(mProjection));

        // Materials and transforms go up in whole blocks the shader's arrays fill, each batch pointing at its own.
        constexpr UInt32 kMaterialBlock  = kMaterials * sizeof(Material);
        constexpr UInt32 kTransformBlock = kTransforms * sizeof(Affine);

        const UInt32 MaterialSlots  = Max(::Align(static_cast<UInt32>(mMaterials.GetSize()), kMaterials), kMaterials);
        const UInt32 TransformSlots = ::Align(static_cast<UInt32>(mAffines.GetSize()), kTransforms);
        const UInt32 MaterialBytes  = MaterialSlots * static_cast<UInt32>(sizeof(Material));
        const UInt32 TransformBytes = TransformSlots * static_cast<UInt32>(sizeof(Affine));

        ZyGraphic::Transient<Byte> Materials  = mGraphics->AllocateInFlightUniforms<Byte>(MaterialBytes);
        ZyGraphic::Transient<Byte> Transforms = mGraphics->AllocateInFlightUniforms<Byte>(TransformBytes);

        if (!mMaterials.IsEmpty())
        {
            Materials.Copy(ConstSpan(mMaterials.GetData(), mMaterials.GetSize()), 0);
        }
        Transforms.Copy(ConstSpan(mAffines.GetData(), mAffines.GetSize()), 0);

        const ZyGraphic::Stream Instances  = Quads.GetStream();
        const ZyGraphic::Stream Materials0 = Materials.GetStream();
        const ZyGraphic::Stream Transform0 = Transforms.GetStream();

        for (ConstRef<Pass> Step : mPasses)
        {
            ConstRef<Batch> Run = mBatches[Step.Batch];

            if (Step.Count == 0)
            {
                continue;
            }

            Ref<ZyGraphic::Command> Command = mGraphics->AllocateInFlightCommand();

            Command.Scissor  = ZyGraphic::Scissor(
                static_cast<UInt16>(Run.Scissor.GetMinimumX()),
                static_cast<UInt16>(Run.Scissor.GetMinimumY()),
                static_cast<UInt16>(Run.Scissor.GetWidth()),
                static_cast<UInt16>(Run.Scissor.GetHeight()));
            Command.Pipeline = Layered ? (Step.Solid ? mSolid : mBehind) : mPipeline;

            // Each quad is one instance, so a run starts its stream at its first.
            ZyGraphic::Stream Own = Instances;
            Own.Offset += Step.Offset * sizeof(Quad);
            Own.Stride  = sizeof(Quad);
            Command.Vertices.Append(Own);
            Command.Indices = Corners;

            ZyGraphic::Stream Material = Materials0;
            Material.Offset += Run.Materials * kMaterialBlock;
            Material.Stride  = kMaterialBlock;

            ZyGraphic::Stream Transform = Transform0;
            Transform.Offset += Run.Transforms * kTransformBlock;
            Transform.Stride  = kTransformBlock;

            Command.Uniforms[ZyEnum::Cast(ZyGraphic::Frequency::Frame)]    = Camera;
            Command.Uniforms[ZyEnum::Cast(ZyGraphic::Frequency::Material)] = Material;
            Command.Uniforms[ZyEnum::Cast(ZyGraphic::Frequency::Instance)] = Transform;

            Command.Textures.Append(Run.Textures[0] ? Run.Textures[0] : mBlank);
            Command.Textures.Append(Run.Textures[1] ? Run.Textures[1] : mBlank);
            Command.Textures.Append(Run.Textures[2] ? Run.Textures[2] : mSheet);

            // A sampler is bound per texture unit, so the image and the layer each read the way the run chose for them.
            Command.Samplers.Append(mSampler);
            Command.Samplers.Append(Run.Filters[1] == Filter::Point ? mPoint : mSampler);
            Command.Samplers.Append(Run.Filters[2] == Filter::Point ? mPoint : mSampler);

            Command.Parameters =
            {
                .Count     = static_cast<UInt32>(kCorners.GetSize()),
                .Base      = 0,
                .Offset    = 0,
                .Instances = Step.Count
            };
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Arrange()
    {
        // A later quad stands nearer, every one at a depth of its own between nought and one.
        const Real32 Step = 1.0f / static_cast<Real32>(mCount + 1);
        const UInt32 Real = static_cast<UInt32>(mBatches.GetSize());

        // Opaque quads go last batch first, nearest first, so nothing behind is shaded; like batches join a run.
        for (UInt32 Number = Real; Number-- > 0;)
        {
            ConstRef<Batch> Run   = mBatches[Number];
            const UInt32    First = Run.Offset;
            const UInt32    Last  = (Number + 1 < Real) ? mBatches[Number + 1].Offset : mCount;
            const UInt32    Start = static_cast<UInt32>(mOrdered.GetSize());

            Bool Image = false;
            Bool Layer = false;

            for (UInt32 Index = Last; Index-- > First;)
            {
                if (IsSolid(mQuads[Index]))
                {
                    const UInt16 Kind = (mQuads[Index].Shape[3] >> 2u) & 3u;

                    Image = Image || Kind == ZyEnum::Cast(Paint::Image);
                    Layer = Layer || Kind == ZyEnum::Cast(Paint::Layered);

                    Ref<Quad> Kept = mOrdered.Append(mQuads[Index]);
                    Kept.Depth = 1.0f - static_cast<Real32>(Index + 1) * Step;
                }
            }

            const UInt32 Count = static_cast<UInt32>(mOrdered.GetSize()) - Start;

            if (Count == 0)
            {
                continue;
            }

            const Rect   Scissor    = mBatches[Number].Scissor;
            const UInt32 Materials  = mBatches[Number].Materials;
            const UInt32 Transforms = mBatches[Number].Transforms;
            Bool         Joined     = false;

            if (!mPasses.IsEmpty() && mPasses.GetBack().Solid)
            {
                Ref<Pass>       Previous = mPasses.GetBack();
                ConstRef<Batch> Before   = mBatches[Previous.Batch];
                ConstRef<Batch> Now      = mBatches[Number];

                const Bool Placed = Before.Scissor == Scissor && Before.Transforms == Transforms;
                const Bool Images = !Image || !Before.Textures[1]
                    || (Before.Textures[1] == Now.Textures[1] && Before.Filters[1] == Now.Filters[1]);
                const Bool Layers = !Layer || !Before.Textures[2]
                    || (Before.Textures[2] == Now.Textures[2] && Before.Filters[2] == Now.Filters[2]);

                Joined = Placed && Images && Layers;

                if (Joined)
                {
                    Previous.Count += Count;
                }
            }

            if (!Joined)
            {
                mPasses.Append(static_cast<UInt32>(mBatches.GetSize()), Start, Count, true);
                mBatches.Append(Array<ZyGraphic::Object, 3>(), Array<Filter, 3>(), Scissor, Materials, Transforms, 0u);
            }

            // The run takes up the image or layer the batch's opaque quads read, which no batch before it claimed.
            const UInt32 Owner = mPasses.GetBack().Batch;

            if (Image && !mBatches[Owner].Textures[1])
            {
                mBatches[Owner].Textures[1] = mBatches[Number].Textures[1];
                mBatches[Owner].Filters[1]  = mBatches[Number].Filters[1];
            }

            if (Layer && !mBatches[Owner].Textures[2])
            {
                mBatches[Owner].Textures[2] = mBatches[Number].Textures[2];
                mBatches[Owner].Filters[2]  = mBatches[Number].Filters[2];
            }
        }

        // The rest in the order asked, each tested against nearer opaque quads; batches reading alike join their run.
        for (UInt32 Number = 0; Number < Real; ++Number)
        {
            const UInt32 First = mBatches[Number].Offset;
            const UInt32 Last  = (Number + 1 < Real) ? mBatches[Number + 1].Offset : mCount;
            const UInt32 Start = static_cast<UInt32>(mOrdered.GetSize());

            for (UInt32 Index = First; Index < Last; ++Index)
            {
                if (!IsSolid(mQuads[Index]))
                {
                    Ref<Quad> Kept = mOrdered.Append(mQuads[Index]);
                    Kept.Depth = 1.0f - static_cast<Real32>(Index + 1) * Step;
                }
            }

            const UInt32 Count = static_cast<UInt32>(mOrdered.GetSize()) - Start;

            if (Count == 0)
            {
                continue;
            }

            if (!mPasses.IsEmpty())
            {
                Ref<Pass>       Previous = mPasses.GetBack();
                ConstRef<Batch> Before   = mBatches[Previous.Batch];
                ConstRef<Batch> Now      = mBatches[Number];

                const Bool Follows = !Previous.Solid && Previous.Offset + Previous.Count == Start;
                const Bool Reads   = Before.Textures == Now.Textures && Before.Filters == Now.Filters;
                const Bool Placed  = Before.Scissor == Now.Scissor && Before.Materials == Now.Materials
                    && Before.Transforms == Now.Transforms;

                if (Follows && Reads && Placed)
                {
                    Previous.Count += Count;
                    continue;
                }
            }

            mPasses.Append(Number, Start, Count);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::PushClip(Rect Box)
    {
        // The scissor stands square to the target, so a turned box is cut to the square around it.
        if (mTransform)
        {
            Box = Rect::Transform(Box, * mTransform);
        }

        // The cut is in pixels of the target, so the box is scaled as every quad is.
        Box = Box * mScale;

        // The cut only ever narrows, so a box reaching past the one it sits in is held to it.
        mClip = Rect::Intersection(Rect::Floor(Box), mClip);
        mClips.Append(mClip);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::PopClip()
    {
        if (!mClips.IsEmpty())
        {
            mClips.RemoveLast();
        }
        mClip = mClips.IsEmpty() ? mViewport : mClips.GetBack();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::PushTransform(ConstRef<Matrix3x2> Transform)
    {
        // The new one sits inside the last, so a point is moved by it first and by the earlier ones after.
        mTransforms.Append(mTransform ? (* mTransform) * Transform : Transform);
        mTransform = AddressOf(mTransforms.GetBack());
        mSlot      = 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::PopTransform()
    {
        if (!mTransforms.IsEmpty())
        {
            mTransforms.RemoveLast();
        }
        mTransform = mTransforms.IsEmpty() ? nullptr : AddressOf(mTransforms.GetBack());
        mSlot      = 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawShadow(Rect Box, Color Tint, Real32 Radius, Real32 Blur, Rect Hole)
    {
        // Half a pixel of fade is what any edge gets, so no blur still draws an anti-aliased box.
        const Real32 Fade  = Max(Blur, 0.5f);
        const Rect   Whole = Box.Expand(Fade);
        const Steps  Shape = Measure(Cut::Shadow, Paint::Colour, Radius, 0.0f, Fade);

        if (!Open(Paint::Colour, 0))
        {
            return;
        }

        const UInt32 Packed = Tint.ToRGBA8();
        const Rect   Cut    = Rect::Intersection(Hole, Whole);

        // A blur inside the rounded edge is solid, so a large unhidden middle is drawn plain and only the rim fades.
        const Rect Solid = Box.Contract(Fade + Max(Radius, 0.0f));

        // A large hidden part is left out whole, rim and all; the solid middle is split off only when little hides it.
        if (!IsLarge(Solid) || IsLarge(Cut))
        {
            // The quad reaches as far past the box as the fade does, and the shader takes the box back out of it.
            Hollow(Whole, Cut, Packed, Shape);
            return;
        }

        Steps Plain = Shape;
        Plain[3] &= static_cast<UInt16>(~3u);

        Surround(Whole, Whole, Solid, kWhole, Packed, Packed, Shape);
        Pave(Solid, Cut, kWhole, Packed, Packed, Plain);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawInnerGlow(Rect Box, Color Tint, Real32 Radius, Real32 Blur)
    {
        const Real32 Fade  = Max(Blur, 0.5f);
        const Steps  Shape = Measure(Cut::Inset, Paint::Colour, Radius, 0.0f, Fade);

        if (!Open(Paint::Colour, 0))
        {
            return;
        }

        // Nothing deeper than the corner and the fade shows, so a large box leaves its middle out.
        const Real32 Depth = Max(Radius, 0.0f) + Fade + 1.0f / Max(mScale, 1.0e-6f);

        Hollow(Box, Box.Contract(Depth), Tint.ToRGBA8(), Shape);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawGradient(Rect Box, ConstRef<Gradient> Fill, Real32 Radius, Rect Hole)
    {
        // An end fading to nothing keeps the other end's hue, as a blend of premultiplied colours would.
        const Color From = (Fill.From.GetAlpha() > 0.0f)
            ? Fill.From
            : Color::WithAlpha(Fill.To, 0.0f);
        const Color To   = (Fill.To.GetAlpha() > 0.0f)
            ? Fill.To
            : Color::WithAlpha(Fill.From, 0.0f);

        if (!Open(Paint::Colour, 0))
        {
            return;
        }

        // The shader runs the line through the box at the angle, a whole turn spread over 16 bits.
        const Real32 Turn = Wrap(Fill.Angle, 360.0f);

        Steps Shape = Measure(Cut::Plain, Paint::Colour, Radius, 0.0f, 0.0f);
        Shape[2]  = static_cast<UInt16>(static_cast<UInt32>(Turn * (65536.0f / 360.0f)) & 0xFFFFu);
        Shape[3] |= kGradient;

        Pave(Box, Hole, kWhole, From.ToRGBA8(), To.ToRGBA8(), Shape);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawRect(Rect Box, Color Tint, Real32 Radius, Rect Hole)
    {
        const UInt32 Packed = Tint.ToRGBA8();

        if (Open(Paint::Colour, 0))
        {
            Pave(Box, Hole, kWhole, Packed, Packed, Measure(Cut::Plain, Paint::Colour, Radius, 0.0f, 0.0f));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawFrame(Rect Box, Color Tint, Real32 Width, Real32 Radius)
    {
        const Steps Shape = Measure(Cut::Rounded, Paint::Colour, Radius, Width, 0.0f);

        if (!Open(Paint::Colour, 0))
        {
            return;
        }

        // Nothing deeper than the corner, the outline and a pixel of edge shows, so a large box leaves its middle out.
        const Real32 Depth = Max(Radius, 0.0f) + Max(Width, 0.0f) + 1.0f / Max(mScale, 1.0e-6f);

        Hollow(Box, Box.Contract(Depth), Tint.ToRGBA8(), Shape);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawImage(ConstRef<ZyGraphic::Image> Image,
                           Rect                       Box,
                           Rect                       Source,
                           Color                      Tint,
                           Real32                     Radius,
                           UInt16                     Layer,
                           Filter                     Sampling)
    {
        const Surface Texture {
            .Handle = Image.GetHandle(),
            .Layout = Image.GetLayout(),
            .Layer  = Layer,
            .Filter = Sampling
        };
        DrawTexture(Texture, Box, Source, Tint, Radius);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawSliced(ConstRef<ZyGraphic::Image> Image,
                            Rect                       Box,
                            Rect                       Source,
                            Edges                      Slice,
                            Color                      Tint,
                            UInt16                     Layer,
                            Filter                     Sampling)
    {
        const Surface Texture {
            .Handle = Image.GetHandle(),
            .Layout = Image.GetLayout(),
            .Layer  = Layer,
            .Filter = Sampling
        };
        DrawSliced(Texture, Vector2(Image.GetWidth(), Image.GetHeight()), Box, Source, Slice, Tint);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawTexture(ConstRef<Surface> Texture, Rect Box, Rect Source, Color Tint, Real32 Radius)
    {
        const Paint  Kind   = (Texture.Layout == ZyGraphic::TextureLayout::Texture2DArray)
            ? Paint::Layered
            : Paint::Image;
        const UInt32 Packed = Tint.ToRGBA8();

        if (!Open(Kind, Texture.Handle, Texture.Filter))
        {
            return;
        }

        const Steps Shape = ShapeOf(Texture, Kind, Radius);

        // Read upside down, the box's top edge takes the texture's bottom row.
        const Rect Read = Texture.Flip
            ? Rect(Source.GetMinimumX(), Source.GetMaximumY(), Source.GetMaximumX(), Source.GetMinimumY())
            : Source;

        Pave(Box, Rect(), Rect::EncodeNormalized<UInt16>(Read), Packed, Packed, Shape);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawSliced(ConstRef<Surface> Texture, Vector2 Size, Rect Box, Rect Source, Edges Slice, Color Tint)
    {
        const Paint  Kind   = (Texture.Layout == ZyGraphic::TextureLayout::Texture2DArray)
            ? Paint::Layered
            : Paint::Image;
        const UInt32 Packed = Tint.ToRGBA8();
        const Steps  Shape  = ShapeOf(Texture, Kind, 0.0f);

        if (!Open(Kind, Texture.Handle, Texture.Filter))
        {
            return;
        }

        // Read upside down, the bottom border of the texture stands at the top of the box.
        const Real32 Upper = Texture.Flip ? Slice.Bottom : Slice.Top;
        const Real32 Lower = Texture.Flip ? Slice.Top : Slice.Bottom;

        // The border stands on screen at the size it has in the image, halved where the box is too small for both.
        const Real32 Wide = Box.GetWidth();
        const Real32 Tall = Box.GetHeight();

        const Real32 Left   = Min(Slice.Left,  Wide * 0.5f);
        const Real32 Right  = Min(Slice.Right, Wide * 0.5f);
        const Real32 Top    = Min(Upper,       Tall * 0.5f);
        const Real32 Bottom = Min(Lower,       Tall * 0.5f);

        const Real32 PixelX = 1.0f / Max(Size.GetX(), 1.0f);
        const Real32 PixelY = 1.0f / Max(Size.GetY(), 1.0f);

        const Array X(Box.GetMinimumX(), Box.GetMinimumX() + Left, Box.GetMaximumX() - Right, Box.GetMaximumX());
        const Array Y(Box.GetMinimumY(), Box.GetMinimumY() + Top,  Box.GetMaximumY() - Bottom, Box.GetMaximumY());
        const Array U  = {
            Source.GetMinimumX(), Source.GetMinimumX() + Slice.Left * PixelX,
            Source.GetMaximumX() - Slice.Right * PixelX, Source.GetMaximumX() };

        // A flipped row reads the texture mirrored about the middle of the part drawn.
        const Real32 Mirror = Texture.Flip ? Source.GetMinimumY() + Source.GetMaximumY() : 0.0f;
        const Real32 Sign   = Texture.Flip ? -1.0f : 1.0f;

        const Array V(
            Mirror + Sign *  Source.GetMinimumY(),
            Mirror + Sign * (Source.GetMinimumY() + Upper * PixelY),
            Mirror + Sign * (Source.GetMaximumY() - Lower * PixelY),
            Mirror + Sign *  Source.GetMaximumY());

        for (UInt32 Row = 0; Row < 3; ++Row)
        {
            for (UInt32 Column = 0; Column < 3; ++Column)
            {
                const Rect Piece(X[Column], Y[Row], X[Column + 1], Y[Row + 1]);

                // A side the slice leaves no room for has nothing to draw.
                if (!Piece.IsEmpty())
                {
                    const Rect Region(U[Column], V[Row], U[Column + 1], V[Row + 1]);

                    Emit(Piece, Piece, Rect::EncodeNormalized<UInt16>(Region), Packed, Packed, Shape);
                }
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawLine(Vector2 From, Vector2 To, Color Tint, Real32 Width, Cap Ends)
    {
        if (Open(Paint::Colour, 0, Filter::Linear, true))
        {
            Stroke(From, To, PenOf(Tint, Width, Ends));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawPolyline(ConstSpan<Vector2> Points, Color Tint, Real32 Width, Bool Closed, Cap Ends)
    {
        const UInt Count = Points.GetSize();

        if (Count < 2 || !Open(Paint::Colour, 0, Filter::Linear, true))
        {
            return;
        }

        const Pen Brush = PenOf(Tint, Width, Ends);

        // Ends cut square overlap half a pixel at a joint, so the fades of the two lines meeting there leave no seam.
        const Real32 Overlap = (Ends == Cap::Butt) ? 0.5f / Max(mScale, 1.0e-6f) : 0.0f;
        const UInt   Lines   = Closed ? Count : Count - 1;

        for (UInt Index = 0; Index < Lines; ++Index)
        {
            const UInt Next = (Index + 1 < Count) ? Index + 1 : 0;

            const Real32 Lead  = (Closed || Index > 0) ? Overlap : 0.0f;
            const Real32 Trail = (Closed || Index + 1 < Lines) ? Overlap : 0.0f;

            Stroke(Points[Index], Points[Next], Brush, Lead, Trail);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawTriangle(Vector2 First, Vector2 Second, Vector2 Third, Color Tint)
    {
        // A triangle is a figure of four corners whose last two meet, which the shader reads as one edge left out.
        if (Open(Paint::Colour, 0, Filter::Linear, true))
        {
            Fill(First, Second, Third, Third, Tint.ToRGBA8());
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawQuad(Vector2 First, Vector2 Second, Vector2 Third, Vector2 Fourth, Color Tint)
    {
        if (Open(Paint::Colour, 0, Filter::Linear, true))
        {
            Fill(First, Second, Third, Fourth, Tint.ToRGBA8());
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawCircle(Vector2 Center, Real32 Radius, Color Tint)
    {
        // A box rounded by half its side is a circle, so a disc costs what a rounded box does.
        const Real32 Size = Max(Radius, 0.0f);

        DrawRect(Rect(Center, Center).Expand(Size), Tint, Size);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawRing(Vector2 Center, Real32 Radius, Color Tint, Real32 Width)
    {
        // An outline of no width would fill the whole disc, so it draws nothing.
        if (Width <= 0.0f || !Open(Paint::Colour, 0))
        {
            return;
        }

        // An outline is drawn inside its box, so the box reaches half the width past the radius.
        const Real32 Outer = Max(Radius, 0.0f) + Width * 0.5f;
        const Steps  Shape = Measure(Cut::Rounded, Paint::Colour, Outer, Width, 0.0f);

        // Nothing shows within the square the ring's hole holds, so a large ring leaves that square out.
        const Real32 Hole = Max(Outer - Width - 1.0f / Max(mScale, 1.0e-6f), 0.0f) * 0.70710678f;

        Hollow(Rect(Center, Center).Expand(Outer), Rect(Center, Center).Expand(Hole), Tint.ToRGBA8(), Shape);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawArc(Vector2 Center, Real32 Radius, Angle From, Angle To, Color Tint, Real32 Width, Cap Ends)
    {
        const Real32 Start = From.GetRadians();
        const Real32 Sweep = To.GetRadians() - Start;

        if (Sweep == 0.0f || !Open(Paint::Colour, 0, Filter::Linear, true))
        {
            return;
        }

        // A whole turn or more closes on itself, leaving no end open.
        const Bool   Closed = Abs(Sweep) >= kTwoPI<Real32>;
        const Real32 Turn   = Closed ? kTwoPI<Real32> : Sweep;

        Trace(Center, Vector2(Radius, 0.0f), Vector2(0.0f, Radius), Start, Turn, StepsOf(Radius, Turn), Closed,
            PenOf(Tint, Width, Ends), Ends);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawEllipse(Vector2 Center, Vector2 AxisX, Vector2 AxisY, Color Tint, Real32 Width, UInt32 Segments)
    {
        if (!Open(Paint::Colour, 0, Filter::Linear, true))
        {
            return;
        }

        const Real32 Longest = Max(AxisX.GetLength(), AxisY.GetLength());
        const UInt32 Steps   = (Segments > 0) ? Segments : StepsOf(Longest, kTwoPI<Real32>);

        Trace(Center, AxisX, AxisY, 0.0f, kTwoPI<Real32>, Steps, true, PenOf(Tint, Width, Cap::Butt), Cap::Butt);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawText(ConstRef<ZyRender::Font>       Font,
                          Text                           Content,
                          Vector2                        Origin,
                          Real32                         Size,
                          Color                          Tint,
                          ConstRef<ZyRender::FontEffect> Effect)
    {
        Run Line = Prepare(Font, Origin, Size, Tint, Effect);

        Font.Shape(Content, Vector2(), Line);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawGlyphs(ConstRef<ZyRender::Font>             Font,
                            ConstSpan<ZyRender::Font::Placement> Glyphs,
                            Vector2                              Origin,
                            Real32                               Size,
                            Color                                Tint,
                            ConstRef<ZyRender::FontEffect>       Effect)
    {
        Run Line = Prepare(Font, Origin, Size, Tint, Effect);

        for (ConstRef<ZyRender::Font::Placement> Placement : Glyphs)
        {
            Line(Placement);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Rect Canvas::GetPlace(UInt32 Index) const
    {
        ConstRef<Quad> Drawn = mQuads[Index];
        const UInt32   Slot  = (Drawn.Shape[3] >> 5) & (kTransforms - 1);

        if (Slot == 0)
        {
            return Drawn.Area;
        }

        // The quad's transform sits in the block of the batch holding it.
        UInt32 Block = 0;

        for (ConstRef<Batch> Run : mBatches)
        {
            if (Run.Offset <= Index)
            {
                Block = Run.Transforms;
            }
        }

        ConstRef<Affine> Rows = mAffines[Block * kTransforms + Slot];
        return Rect::Transform(Drawn.Area, Matrix3x2(Rows[0], Rows[1], Rows[2], Rows[4], Rows[5], Rows[6]));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Run::operator()(ConstRef<ZyRender::Font::Placement> Placement)
    {
        // Glyphs of one line nearly always share a page, so its texture, its material and its batch are found once.
        if (Placement.Page != Page)
        {
            Page = Placement.Page;

            ConstRetainer<ZyGraphic::Image> Atlas = Font->GetMaterial(Page)->GetImage("Albedo"_Hash);

            Ready = Atlas && Atlas->GetHandle();

            if (Ready)
            {
                const Real32  Distance = Font->GetMetrics().Distance;
                const Vector2 Range(Distance / Atlas->GetWidth(), Distance / Atlas->GetHeight());

                Shape[1] = static_cast<UInt16>(Owner->ObtainMaterial(Range, Effect));
                Ready    = Owner->Open(Paint::Glyph, Atlas->GetHandle());
            }
        }

        if (!Ready)
        {
            return;
        }

        // The font lays glyphs out with y rising, so the top of one on screen is its highest edge in the layout.
        ConstRef<Rect> Local = Placement.Local;
        ConstRef<Rect> Atlas = Placement.Atlas;

        const Rect Box(
            Origin.GetX() + Local.GetMinimumX() * Size,
            Origin.GetY() - Local.GetMaximumY() * Size,
            Origin.GetX() + Local.GetMaximumX() * Size,
            Origin.GetY() - Local.GetMinimumY() * Size);

        const Rect Region(Atlas.GetMinimumX(), Atlas.GetMaximumY(), Atlas.GetMaximumX(), Atlas.GetMinimumY());

        Owner->Emit(Box, Box, Rect::EncodeNormalized<UInt16>(Region), Tint, Tint, Shape);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Canvas::Run Canvas::Prepare(ConstRef<ZyRender::Font>       Font,
                                Vector2                        Origin,
                                Real32                         Size,
                                Color                          Tint,
                                ConstRef<ZyRender::FontEffect> Effect)
    {
        // The shader reads the outline's offset and bias in pixels of the target, so they scale as every length does.
        return Run {
            .Owner  = this,
            .Font   = AddressOf(Font),
            .Effect = ZyRender::FontEffect(Effect.GetOutsetColor(), Effect.GetOutsetOffset() * mScale,
                Effect.GetOutsetWidthRelative(), Effect.GetOutsetWidthAbsolute() * mScale, Effect.GetOutsetBlur(),
                Effect.GetInsetRoundness(), Effect.GetInsetThreshold()),
            .Origin = Vector2(Origin.GetX(), Origin.GetY() + Font.GetMetrics().Ascender * Size),
            .Size   = Size,
            .Tint   = Tint.ToRGBA8(),
            .Shape  = Measure(Cut::Plain, Paint::Glyph, 0.0f, 0.0f, 0.0f)
        };
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Canvas::Open(Paint Kind, ZyGraphic::Object Texture, Filter Sampling, Bool Uncut)
    {
        // A quad cut away entirely costs nothing, not even a batch.
        if (mClip.IsEmpty())
        {
            return false;
        }

        // A turned quad or a figure cannot be cut square on the spot, so only they need the scissor at the clip.
        const Bool   Scissored = Uncut || mTransform;
        const UInt32 Slot      = ObtainTransform();
        const UInt32 Materials = mMaterials.IsEmpty() ? 0 : static_cast<UInt32>(mMaterials.GetSize() - 1) / kMaterials;
        const UInt32 Turns     = static_cast<UInt32>(mAffines.GetSize() - 1) / kTransforms;

        // Each kind of texture has a slot of its own, so a colour takes none and an image only the image's.
        const UInt32 Place = SlotOf(Kind);

        if (!mBatches.IsEmpty())
        {
            Ref<Batch> Last = mBatches.GetBack();

            // A slot claimed already takes the same texture again only when it is read the same way.
            const Bool Same  = Last.Textures[Place] == Texture && Last.Filters[Place] == Sampling;
            const Bool Free  = (Kind == Paint::Colour) || !Last.Textures[Place] || Same;
            const Bool Cut   = Scissored ? (Last.Scissor == mClip) : Last.Scissor.Contains(mClip);
            const Bool Read  = Kind != Paint::Glyph || Last.Materials == Materials;
            const Bool Reads = Read && (Slot == 0 || Last.Transforms == Turns);

            if (Free && Cut && Reads)
            {
                if (Kind != Paint::Colour)
                {
                    Last.Textures[Place] = Texture;
                    Last.Filters[Place]  = Sampling;
                }

                mKind     = Kind;
                mTexture  = Texture;
                mSampling = Sampling;
                mUncut    = Uncut;
                return true;
            }
        }

        Ref<Batch> Fresh = mBatches.Append(Array<ZyGraphic::Object, 3>(),
                                           Array<Filter, 3>(),
                                           Scissored ? mClip : mViewport,
                                           Materials,
                                           Turns,
                                           mCount);

        if (Kind != Paint::Colour)
        {
            Fresh.Textures[Place] = Texture;
            Fresh.Filters[Place]  = Sampling;
        }

        mKind     = Kind;
        mTexture  = Texture;
        mSampling = Sampling;
        mUncut    = Uncut;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Plot(Rect Area, Rect Box, Rect Bounds, UInt32 Tint, UInt16 Code)
    {
        // A figure cannot be cut square on the spot, so one the clip misses entirely is all that is left out.
        if (!IsVisible(Bounds))
        {
            return;
        }

        if (mCount + 1 > mQuads.GetSize())
        {
            mQuads.Advance(Max<UInt>(mQuads.GetSize(), 256));
        }

        Ref<Quad> Drawn = mQuads[mCount++];

        Drawn.Area   = Area;
        Drawn.Box    = Box;
        Drawn.Source = { };
        Drawn.Tint   = Tint;
        Drawn.Other  = Tint;
        Drawn.Shape  = { };

        Drawn.Shape[3] = static_cast<UInt16>(Code | (mSlot << 5));

        if (mTape)
        {
            Note(Drawn);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Stroke(Vector2 From, Vector2 To, ConstRef<Pen> Brush, Real32 Lead, Real32 Trail)
    {
        // The shader cuts the line as a box rounded at its ends, so the code says rounded, a colour and a segment.
        constexpr UInt16 kCode = ZyEnum::Cast(Cut::Rounded) | (ZyEnum::Cast(Form::Segment) << kFormShift);

        if (Lead > 0.0f || Trail > 0.0f)
        {
            const Vector2 Delta  = To - From;
            const Real32  Length = Delta.GetLength();

            if (Length > 0.0f)
            {
                From = From - Delta * (Lead / Length);
                To   = To + Delta * (Trail / Length);
            }
        }

        // The quad grows a pixel past the line on every side, so the bounds the clip is held against grow as much.
        const Real32 Margin = (Brush.Half + Brush.Reach + 1.0f) / Max(mScale, 1.0e-6f);
        const Rect   Bounds = Rect(Vector2::Min(From, To), Vector2::Max(From, To)).Expand(Margin);

        Plot(Rect(From * mScale, To * mScale), Rect(Brush.Half, Brush.Reach, Brush.Radius, 0.0f), Bounds, Brush.Tint,
             kCode);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Trace(Vector2       Center,
                       Vector2       AxisX,
                       Vector2       AxisY,
                       Real32        Start,
                       Real32        Sweep,
                       UInt32        Steps,
                       Bool          Closed,
                       ConstRef<Pen> Brush,
                       Cap           Ends)
    {
        // Each point is turned from the last by one step, costing a sine and a cosine only for the start and step.
        const Real32 Step     = Sweep / static_cast<Real32>(Max(Steps, 1u));
        const Real32 StepCos  = Cosine(Step);
        const Real32 StepSin  = Sine(Step);
        const Real32 Overlap  = (Ends == Cap::Butt) ? 0.5f / Max(mScale, 1.0e-6f) : 0.0f;

        Real32  Across = Cosine(Start);
        Real32  Down   = Sine(Start);
        Vector2 Last   = Center + AxisX * Across + AxisY * Down;

        for (UInt32 Index = 0; Index < Steps; ++Index)
        {
            const Real32 Turned = Across * StepCos - Down * StepSin;
            Down   = Down * StepCos + Across * StepSin;
            Across = Turned;

            const Vector2 Point = Center + AxisX * Across + AxisY * Down;

            // Ends cut square overlap half a pixel at a joint, as a polyline's do.
            const Real32 Lead  = (Closed || Index > 0) ? Overlap : 0.0f;
            const Real32 Trail = (Closed || Index + 1 < Steps) ? Overlap : 0.0f;

            Stroke(Last, Point, Brush, Lead, Trail);
            Last = Point;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Fill(Vector2 First, Vector2 Second, Vector2 Third, Vector2 Fourth, UInt32 Tint)
    {
        // The shader cuts the figure at its edges, so the code says a plain polygon; the inset cut never reaches it.
        constexpr UInt16 kCode = ZyEnum::Cast(Cut::Inset) | (ZyEnum::Cast(Form::Polygon) << kFormShift);

        // The triangles share the line from the second corner to the fourth, so the fourth comes before the third.
        const Rect Bounds = Rect(
            Vector2::Min(Vector2::Min(First, Second), Vector2::Min(Third, Fourth)),
            Vector2::Max(Vector2::Max(First, Second), Vector2::Max(Third, Fourth))).Expand(1.0f / Max(mScale, 1.0e-6f));

        Plot(Rect(First * mScale, Second * mScale), Rect(Fourth * mScale, Third * mScale), Bounds, Tint, kCode);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Emit(Rect Box, Rect Area, ConstRef<Texels> Source, UInt32 Tint, UInt32 Other, Steps Shape)
    {
        // A quad square to the target is cut to the clip here, and the shader reads its box where the cut leaves it.
        const Rect Scaled  = Area * mScale;
        const Rect Clipped = mTransform ? Scaled : Rect::Intersection(Scaled, mClip);

        if (Clipped.IsEmpty())
        {
            return;
        }

        // The quads are grown ahead in large steps, so a quad only writes its own.
        if (mCount + 1 > mQuads.GetSize())
        {
            mQuads.Advance(Max<UInt>(mQuads.GetSize(), 256));
        }

        Ref<Quad> Drawn = mQuads[mCount++];

        Drawn.Area   = Clipped;
        Drawn.Box    = Box * mScale;
        Drawn.Source = Source;
        Drawn.Tint   = Tint;
        Drawn.Other  = Other;
        Drawn.Shape  = Shape;

        Drawn.Shape[3] |= static_cast<UInt16>(mSlot << 5);

        if (mTape)
        {
            Note(Drawn);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Hollow(Rect Box, Rect Inner, UInt32 Tint, Steps Shape)
    {
        // The bands meet the inner part exactly, so leaving it out costs three quads and saves shading all of it.
        if (IsLarge(Inner))
        {
            Surround(Box, Box, Inner, kWhole, Tint, Tint, Shape);
        }
        else
        {
            Emit(Box, Box, kWhole, Tint, Tint, Shape);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Pave(Rect Box, Rect Hole, ConstRef<Texels> Source, UInt32 Tint, UInt32 Other, Steps Shape)
    {
        // Past its corners and a pixel of fading edge a rounded, unoutlined box is solid, so a large one draws plain.
        const Bool   Rounded = (Shape[3] & 3u) == ZyEnum::Cast(Cut::Rounded) && Shape[1] == 0;
        const Real32 Depth   = Shape[0] / (kSubpixel * mScale) + 1.0f / Max(mScale, 1.0e-6f);
        const Rect   Middle  = Rounded ? Box.Contract(Depth) : Box;
        const Rect   Cover   = Rect::Intersection(Hole, Middle);
        const Bool   Hollow  = IsLarge(Cover);

        if (!Hollow && !(Rounded && IsLarge(Middle)))
        {
            Emit(Box, Box, Source, Tint, Other, Shape);
            return;
        }

        Steps Plain = Shape;
        Plain[3] &= static_cast<UInt16>(~3u);

        if (Rounded)
        {
            Surround(Box, Box, Middle, Source, Tint, Other, Shape);
        }

        // A large hole something opaque covers later is left out of the middle as well.
        if (Hollow)
        {
            Surround(Box, Middle, Cover, Source, Tint, Other, Plain);
        }
        else
        {
            Emit(Box, Middle, Source, Tint, Other, Plain);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Surround(Rect             Box,
                          Rect             Outer,
                          Rect             Inner,
                          ConstRef<Texels> Source,
                          UInt32           Tint,
                          UInt32           Other,
                          Steps            Shape)
    {
        // The bands share their edges with each other and with the inner box, so they cover the rest exactly.
        const Rect Upper(Outer.GetMinimumX(), Outer.GetMinimumY(), Outer.GetMaximumX(), Inner.GetMinimumY());
        const Rect Left(Outer.GetMinimumX(), Inner.GetMinimumY(), Inner.GetMinimumX(), Inner.GetMaximumY());
        const Rect Right(Inner.GetMaximumX(), Inner.GetMinimumY(), Outer.GetMaximumX(), Inner.GetMaximumY());
        const Rect Lower(Outer.GetMinimumX(), Inner.GetMaximumY(), Outer.GetMaximumX(), Outer.GetMaximumY());

        Emit(Box, Upper, Source, Tint, Other, Shape);
        Emit(Box, Left, Source, Tint, Other, Shape);
        Emit(Box, Right, Source, Tint, Other, Shape);
        Emit(Box, Lower, Source, Tint, Other, Shape);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Canvas::IsLarge(Rect Middle) const
    {
        return !Middle.IsEmpty() && Middle.GetArea() * mScale * mScale >= kLarge;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Canvas::Steps Canvas::Measure(Cut Edge, Paint Kind, Real32 Radius, Real32 Outline, Real32 Blur) const
    {
        // Lengths go down as whole subpixel steps of the target, so the shader rounds and fades in the pixels seen.
        const Real32 Step = kSubpixel * mScale;

        // An outline thinner than a step keeps one, since nought reads as no outline and would fill the whole box.
        const UInt16 Round = static_cast<UInt16>(Clamp(Radius * Step, 0.0f, 65535.0f));
        const UInt16 Line  = static_cast<UInt16>(Clamp(Outline * Step, (Outline > 0.0f) ? 1.0f : 0.0f, 65535.0f));
        const UInt16 Soft  = static_cast<UInt16>(Clamp(Blur * Step, 0.0f, 65535.0f));

        // A box neither rounded nor outlined covers its whole quad, so it skips the distance altogether.
        const Cut Shaped = (Edge == Cut::Plain && (Round > 0 || Line > 0)) ? Cut::Rounded : Edge;

        const UInt16 Mode = static_cast<UInt16>(ZyEnum::Cast(Shaped) | (ZyEnum::Cast(Kind) << 2));
        return Steps(Round, Line, Soft, Mode);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Canvas::Steps Canvas::ShapeOf(ConstRef<Surface> Texture, Paint Kind, Real32 Radius) const
    {
        Steps Shape = Measure(Cut::Plain, Kind, Radius, 0.0f, 0.0f);

        // A texture's quads carry no blur, so the third step holds the slice an array is read at.
        if (Kind == Paint::Layered)
        {
            Shape[2] = Texture.Layer;
        }

        if (Texture.Opaque)
        {
            Shape[3] |= kOpaque;
        }
        return Shape;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Canvas::Pen Canvas::PenOf(Color Tint, Real32 Width, Cap Ends) const
    {
        // A stroke thinner than a pixel covers one as faintly as it is thin, which keeps a hairline from breaking up.
        const Real32 Pixels = Max(Width, 0.0f) * mScale;
        const Real32 Shown  = Max(Pixels, 1.0f);
        const Color  Faded  = (Pixels < 1.0f) ? Color::WithAlpha(Tint, Tint.GetAlpha() * Pixels) : Tint;
        const Real32 Half   = Shown * 0.5f;

        return Pen {
            .Tint   = Faded.ToRGBA8(),
            .Half   = Half,
            .Reach  = (Ends == Cap::Butt) ? 0.0f : Half,
            .Radius = (Ends == Cap::Round) ? Half : 0.0f
        };
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Canvas::StepsOf(Real32 Radius, Real32 Sweep) const
    {
        // Each line strays from the curve by at most a quarter of a pixel, at its middle.
        const Real32 Pixels = Max(Radius * mScale, 0.5f);
        const Real32 Step   = 2.0f * InvCosine(Max(1.0f - 0.25f / Pixels, -1.0f));
        const Real32 Count  = Ceil(Abs(Sweep) / Max(Step, 1.0e-3f));

        return static_cast<UInt32>(Clamp(Count, 1.0f, 512.0f));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Canvas::ObtainTransform()
    {
        if (!mTransform || mSlot)
        {
            return mSlot;
        }

        // A full block starts the next with the identity, which every quad square to the target reads.
        if (mAffines.GetSize() % kTransforms == 0)
        {
            mAffines.Append(kIdentity);
        }

        // The quads carry their corners scaled, so the move is too, while the turn needs nothing.
        ConstRef<Matrix3x2> Matrix = * mTransform;

        const Vector2 BasisX = Matrix.GetBasisX();
        const Vector2 BasisY = Matrix.GetBasisY();
        const Vector2 Move   = Matrix.GetTranslation() * mScale;

        mAffines.Append(BasisX.GetX(), BasisY.GetX(), Move.GetX(), 0.0f,
                        BasisX.GetY(), BasisY.GetY(), Move.GetY(), 0.0f);

        mSlot = static_cast<UInt32>(mAffines.GetSize() - 1) % kTransforms;
        return mSlot;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Canvas::ObtainMaterial(Vector2 Range, ConstRef<ZyRender::FontEffect> Effect)
    {
        const IntColor8 Outset = Effect.GetOutsetColor();

        const Material Wanted {
            .Range     = Range,
            .Padding   = Vector2(),
            .Tint      = Outset.ToRGBA8(),
            .Offset    = Effect.GetOutsetOffset(),
            .Width     = Effect.GetOutsetWidthRelative(),
            .Bias      = Effect.GetOutsetWidthAbsolute(),
            .Blur      = Effect.GetOutsetBlur(),
            .Roundness = Effect.GetInsetRoundness(),
            .Threshold = Effect.GetInsetThreshold(),
            .Tail      = 0
        };

        return AdoptMaterial(Wanted);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Canvas::AdoptMaterial(ConstRef<Material> Wanted)
    {
        // Text alternates between a few effects at most, so only the last few of the current block are looked through.
        const UInt32 Size   = static_cast<UInt32>(mMaterials.GetSize());
        const UInt32 First  = (Size == 0) ? 0 : (Size - 1) / kMaterials * kMaterials;
        const UInt32 Oldest = Max(First, (Size >= 4) ? Size - 4 : 0);

        for (UInt32 Index = Size; Index > Oldest; --Index)
        {
            ConstRef<Material> Known = mMaterials[Index - 1];

            const ConstPtr<Byte> Seen  = reinterpret_cast<ConstPtr<Byte>>(& Known);
            const ConstPtr<Byte> Asked = reinterpret_cast<ConstPtr<Byte>>(& Wanted);

            if (Compare(Seen, Asked, sizeof(Material)))
            {
                return (Index - 1) % kMaterials;
            }
        }

        mMaterials.Append(Wanted);
        return (Size % kMaterials);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Record(Ptr<Tape> Into)
    {
        mTape = Into;

        if (mTape)
        {
            mTape->Steps.Clear();
            mTape->Lives.Clear();
            mTape->Turns.Clear();
            mTape->Materials.Clear();
            mTape->Scale  = mScale;
            mTape->Clip   = mClip;
            mTape->Turned = mTransform != nullptr;
            mTape->Turn   = mTransform ? * mTransform : Matrix3x2();
            mTape->Valid  = false;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Note(ConstRef<Quad> Drawn)
    {
        Ref<Tape::Step> Kept = mTape->Steps.Append();
        Kept.Drawn    = Drawn;
        Kept.Texture  = mTexture;
        Kept.Clip     = mClip;
        Kept.Kind     = static_cast<UInt8>(mKind);
        Kept.Sampling = mSampling;
        Kept.Uncut    = mUncut;

        // A transform or a material standing as it did for the quad before is kept once, so a run of quads shares it.
        if (mTransform)
        {
            if (mTape->Turns.IsEmpty() || mTape->Turns.GetBack() != * mTransform)
            {
                mTape->Turns.Append(* mTransform);
            }
            Kept.Turn = static_cast<UInt16>(mTape->Turns.GetSize() - 1);
        }

        if (mKind == Paint::Glyph && !mBatches.IsEmpty())
        {
            const UInt32 Global = mBatches.GetBack().Materials * kMaterials + Drawn.Shape[1];

            Array<Byte, 48> Bytes;
            Copy(Bytes.GetData(), sizeof(Material), reinterpret_cast<ConstPtr<Byte>>(AddressOf(mMaterials[Global])));

            if (mTape->Materials.IsEmpty() || mTape->Materials.GetBack() != Bytes)
            {
                mTape->Materials.Append(Bytes);
            }
            Kept.Material = static_cast<UInt16>(mTape->Materials.GetSize() - 1);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ptr<Canvas::Tape> Canvas::Pause(UInt32 Tag, Real32 Opacity)
    {
        const Ptr<Tape> Paused = mTape;

        if (!Paused)
        {
            return nullptr;
        }

        Ref<Tape::Live> Kept = Paused->Lives.Append();
        Kept.At      = static_cast<UInt32>(Paused->Steps.GetSize());
        Kept.Tag     = Tag;
        Kept.Opacity = Opacity;
        Kept.Clip    = mClip;

        if (mTransform)
        {
            if (Paused->Turns.IsEmpty() || Paused->Turns.GetBack() != * mTransform)
            {
                Paused->Turns.Append(* mTransform);
            }
            Kept.Turn = static_cast<UInt16>(Paused->Turns.GetSize() - 1);
        }

        mTape = nullptr;
        return Paused;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Replay(ConstRef<Tape> From, ConstRef<Delegate<void(ConstRef<Tape::Live>)>> OnLive)
    {
        const Rect                Clip      = mClip;
        const ConstPtr<Matrix3x2> Transform = mTransform;
        const UInt32              Slot      = mSlot;
        UInt16                    Turn      = Tape::kNone;
        UInt                      Next      = 0;

        // A transform is taken up as pushing it would, so the quads under it share one slot again.
        const auto Take = [this, &From, &Turn](UInt16 Wanted)
        {
            if (Wanted == Turn)
            {
                return;
            }

            Turn       = Wanted;
            mTransform = nullptr;
            mSlot      = 0;

            if (Turn != Tape::kNone)
            {
                mReplayed  = From.Turns[Turn];
                mTransform = AddressOf(mReplayed);
            }
        };

        // A live part is drawn anew where it stood, under the clip and transform it stood under.
        const auto Relive = [&](UInt Reached)
        {
            while (Next < From.Lives.GetSize() && From.Lives[Next].At <= Reached)
            {
                ConstRef<Tape::Live> Part = From.Lives[Next++];

                mClip = Part.Clip;
                Take(Part.Turn);
                OnLive(Part);

                // The code may have pushed a transform of its own, so the next quad takes its own up again.
                Turn       = Tape::kNone;
                mTransform = nullptr;
                mSlot      = 0;
            }
        };

        for (UInt Index = 0; Index < From.Steps.GetSize(); ++Index)
        {
            Relive(Index);

            ConstRef<Tape::Step> Step = From.Steps[Index];

            mClip = Step.Clip;
            Take(Step.Turn);

            const Paint Kind  = static_cast<Paint>(Step.Kind);
            Steps       Shape = Step.Drawn.Shape;

            if (Kind == Paint::Glyph && Step.Material != Tape::kNone)
            {
                Material Kept;
                Copy(reinterpret_cast<Ptr<Byte>>(AddressOf(Kept)), sizeof(Material),
                     From.Materials[Step.Material].GetData());
                Shape[1] = static_cast<UInt16>(AdoptMaterial(Kept));
            }

            if (!Open(Kind, Step.Texture, Step.Sampling, Step.Uncut))
            {
                continue;
            }

            if (mCount + 1 > mQuads.GetSize())
            {
                mQuads.Advance(Max<UInt>(mQuads.GetSize(), 256));
            }

            Ref<Quad> Drawn = mQuads[mCount++];
            Drawn = Step.Drawn;
            Drawn.Shape    = Shape;
            Drawn.Shape[3] = static_cast<UInt16>((Shape[3] & ~(0xFFu << 5)) | (mSlot << 5));
        }

        Relive(From.Steps.GetSize());

        mClip      = Clip;
        mTransform = Transform;
        mSlot      = Slot;
    }
}