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

#include "Animator.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Facet/Style/Motion.hpp"
#include "UI.Core/Facet/Visual/Backdrop.hpp"
#include "UI.Core/Facet/Visual/Bar.hpp"
#include "UI.Core/Facet/Visual/Caption.hpp"
#include "UI.Core/Facet/Visual/Effect.hpp"
#include "UI.Core/Facet/Visual/Look.hpp"
#include "UI.Core/Facet/Visual/Scroll.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Pane.hpp"
#include "UI.Core/Style/Animation.hpp"
#include "UI.Core/Style/Appearance.hpp"
#include "UI.Core/Style/Keyframe.hpp"
#include "UI.Core/Style/Skin.hpp"
#include "UI.Core/Style/Theme.hpp"
#include "UI.Core/Tree.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Animator::Animator(Ref<Interface> Owner)
        : mOwner { Owner },
          mClock { 0.0 }
    {
        // The interface's own facets wear a skin in this order, a module's after them.
        AddApplier(Applier(Read::Create<&Animator::ReadLook>(), Write::Create<&Animator::WriteLook>(), Fit()));
        AddApplier(Applier(Read::Create<&Animator::ReadCaption>(), Write::Create<&Animator::WriteCaption>(),
            Fit::Create<&Animator::FitCaption>()));
        AddApplier(Applier(Read::Create<&Animator::ReadBar>(), Write::Create<&Animator::WriteBar>(), Fit()));
        AddApplier(Applier(Read::Create<&Animator::ReadScroll>(), Write::Create<&Animator::WriteScroll>(), Fit()));
        AddApplier(Applier(Read::Create<&Animator::ReadEffect>(), Write::Create<&Animator::WriteEffect>(), Fit()));
        AddApplier(Applier(Read::Create<&Animator::ReadBackdrop>(), Write::Create<&Animator::WriteBackdrop>(),
            Fit::Create<&Animator::FitBackdrop>()));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::Stir(Ref<Pane> Target, UInt32 Row, ConstRef<Skin> Values, Ref<Motion> State, Bool Dressed)
    {
        const Handle Element = mOwner.GetHandle(Target.Slots[Row]);

        // A skin giving what the last one gave starts no transition, and leaves one that runs to run on.
        const Bool Changed = Lay(State.Goal, Values) != Skin::Field::None;

        // An element already shown moves from its own values to the new ones; one dressed the first time snaps.
        if (!Dressed || Values.Transition.Duration <= 0.0f)
        {
            State.Own      = State.Goal;
            State.Shifting = false;
        }
        else if (Changed)
        {
            State.From     = State.Own;
            State.Clock    = Playback(Values.Transition.Duration);
            State.Curve    = Values.Transition.Curve;
            State.Shifting = true;

            // The delay is waited out as the time before nought, through which the transition stands at its start.
            State.Clock.Seek(-Values.Transition.Delay);

            // Only what starts apart from where it ends is eased, the rest already standing at its end.
            State.Moving = Apart(State.From, State.Goal) & Motion::kMoving;
        }

        // A new name starts the loop over, and none stops it.
        if (Values.Animation != State.Loop)
        {
            State.Loop      = Values.Animation;
            State.LoopClock = Playback();
        }

        // Only the element's own hidden state plays it in or out; what it holds goes along with it.
        if (const Bool Hidden = Target.IsHidden(Row); Hidden != State.Hidden)
        {
            const UInt64 Once = Hidden ? Values.Exit : Values.Enter;

            State.Hidden    = Hidden;
            State.Once      = Seek(Once) ? Once : 0;
            State.OnceClock = Playback();
            State.Leaving   = Hidden && State.Once != 0;
        }

        // Each animation runs as the theme times it now, so one a new theme retimes plays on from where it stands.
        if (const ConstPtr<Animation> Loop = Seek(State.Loop))
        {
            State.LoopClock.SetDuration(Loop->Duration);
            State.LoopClock.SetRepeat(Loop->Alternate ? Repeat::Mirror : Repeat::Loop);
            State.LoopClock.SetLaps(Loop->Repeat);
        }

        if (const ConstPtr<Animation> Once = Seek(State.Once))
        {
            State.OnceClock.SetDuration(Once->Duration);
        }

        Compose(State);
        Apply(Element, State.Shown, State.Touched);

        if (!State.Listed && (State.Shifting || State.Loop != 0 || State.Once != 0))
        {
            mMoving.Append(Element);
            State.Listed = true;
        }

        // Listed, its clocks run on from where they stood, so one held while off the list never skips ahead.
        if (State.Listed)
        {
            Resume(State.Clock);
            Resume(State.LoopClock);
            Resume(State.OnceClock);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::Tick(Real32 Delta)
    {
        ZY_PROFILE_SCOPE("UI::Animator::Tick");

        // Every clock of every element runs against this one, which moves on only here.
        mClock += Delta;

        mMoving.RemoveFastSomeIf([this](Handle Element)
        {
            const Ptr<Motion> State = mOwner.Get<Motion>(Element);

            if (State && Advance(Element, * State))
            {
                return false;
            }

            // One let go holds its clocks where they stand, so listed again it plays on rather than skipping ahead.
            if (State)
            {
                State->Listed = false;
                State->Clock.Pause();
                State->LoopClock.Pause();
                State->OnceClock.Pause();
            }
            return true;
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Appearance Animator::Capture(UInt32 Slot) const
    {
        Appearance Result;

        for (ConstRef<Applier> Each : mAppliers)
        {
            if (!Each.Capture.IsEmpty())
            {
                Each.Capture(mOwner, Slot, Result);
            }
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::Apply(Handle Element, ConstRef<Appearance> Values, Skin::Field Given)
    {
        for (ConstRef<Applier> Each : mAppliers)
        {
            if (!Each.Apply.IsEmpty())
            {
                Each.Apply(mOwner, Element, Values, Given);
            }
        }

        // A pane holding what was written is drawn anew, rather than replayed as it was.
        mOwner.Mark(Element, Dirty::Paint);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Animator::Tailor(Handle Element, ConstRef<Skin> Values)
    {
        Bool Sized = false;

        for (ConstRef<Applier> Each : mAppliers)
        {
            if (!Each.Fitting.IsEmpty() && Each.Fitting(mOwner, Element, Values))
            {
                Sized = true;
            }
        }
        return Sized;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::AddApplier(Applier Entry)
    {
        mAppliers.Append(Move(Entry));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Animator::IsDrawn(ConstRef<Pane> Target, UInt32 Row) const
    {
        if (!Target.IsHidden(Row))
        {
            return true;
        }

        // Hidden, it still stands while it plays its way out, though the pointer passes through it.
        const ConstPtr<Motion> State = mOwner.Peek<Motion>(Target.Slots[Row]);
        return State && State->Leaving;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Animator::Advance(Handle Element, Ref<Motion> State)
    {
        // Each clock has run only while the element was listed, so it stands as far on as it was played.
        State.Clock.Advance(mClock);
        State.LoopClock.Advance(mClock);
        State.OnceClock.Advance(mClock);

        if (State.Shifting)
        {
            // The share holds at nought while the delay is waited out, and the transition ends once its time has run.
            State.Shifting = !State.Clock.IsComplete();

            if (State.Shifting)
            {
                Blend(State.Own, State.From, State.Goal, Ease(State.Curve, State.Clock.GetProgress()), State.Moving);
            }
            else
            {
                State.Own = State.Goal;
            }
        }

        const ConstPtr<Animation> Loop = Seek(State.Loop);

        // An animation played once ends there, and one played on the way out lets the element go for real.
        const ConstPtr<Animation> Once = Seek(State.Once);

        if (State.Once != 0 && (!Once || State.OnceClock.IsComplete()))
        {
            if (State.Leaving)
            {
                State.Leaving = false;
                mOwner.Mark(Element, Dirty::Measure);
            }
            State.Once = 0;
        }

        Compose(State);
        Apply(Element, State.Shown, State.Touched);

        // An endless loop of no length never ends its clock, so only one that takes some time plays.
        const Bool Looping = Loop && Loop->Duration > 0.0f && !State.LoopClock.IsComplete();
        return State.Shifting || Looping || State.Once != 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::Compose(Ref<Motion> State)
    {
        State.Shown = State.Own;

        // A loop that ran its count is gone, as an ended CSS animation leaves the element's own values.
        if (const ConstPtr<Animation> Loop = Seek(State.Loop); Loop && Loop->Duration > 0.0f)
        {
            if (!State.LoopClock.IsComplete())
            {
                Overlay(* Loop, State.LoopClock.GetProgress(), State.Own, State.Shown, State.Touched);
            }
        }

        // The way in or out plays over the loop, a single run forward.
        if (const ConstPtr<Animation> Once = Seek(State.Once); Once && Once->Duration > 0.0f)
        {
            const Appearance Beneath = State.Shown;
            Overlay(* Once, State.OnceClock.GetProgress(), Beneath, State.Shown, State.Touched);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::Overlay(ConstRef<Animation>  Clip,
                           Real32               Share,
                           ConstRef<Appearance> Beneath,
                           Ref<Appearance>      Output,
                           Ref<Skin::Field>     Touched)
    {
        if (Clip.Keys.IsEmpty())
        {
            return;
        }

        // Each key is lifted over the values beneath once, so every value it leaves out is the element's own.
        mLifted.Clear();

        for (ConstRef<Keyframe> Key : Clip.Keys)
        {
            Lay(mLifted.Append(Beneath), Key.Values);
            Touched |= Key.Values.Mask & Motion::kMoving;
        }

        for (UInt64 Bits = static_cast<UInt64>(Motion::kMoving); Bits != 0; Bits &= Bits - 1)
        {
            Tween(Clip, Share, static_cast<Skin::Field>(Bits & (~Bits + 1ull)), Beneath, Output);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::Tween(ConstRef<Animation>  Clip,
                         Real32               Share,
                         Skin::Field          Bit,
                         ConstRef<Appearance> Beneath,
                         Ref<Appearance>      Output) const
    {
        constexpr UInt kNone = kMaximum<UInt>;

        UInt Before = kNone;
        UInt After  = kNone;

        // The keys around the share that give the value; past the first or the last, the values beneath stand in.
        for (UInt Index = 0; Index < Clip.Keys.GetSize(); ++Index)
        {
            ConstRef<Keyframe> Key = Clip.Keys[Index];

            if (!Key.Values.Has(Bit))
            {
                continue;
            }

            if (Key.At <= Share)
            {
                Before = Index;
            }
            else if (After == kNone)
            {
                After = Index;
            }
        }

        if (Before == kNone && After == kNone)
        {
            return;
        }

        const Real32 Start = Before == kNone ? 0.0f : Clip.Keys[Before].At;
        const Real32 End   = After  == kNone ? 1.0f : Clip.Keys[After].At;
        const Real32 Local = End > Start ? Remap(Share, Start, End, 0.0f, 1.0f) : 1.0f;

        ConstRef<Appearance> From = Before == kNone ? Beneath : mLifted[Before];
        ConstRef<Appearance> To   = After  == kNone ? Beneath : mLifted[After];

        Blend(Output, From, To, Ease(Clip.Curve, Local), Bit);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<Animation> Animator::Seek(UInt64 Name) const
    {
        const ConstPtr<Theme> Source = mOwner.GetTheme();
        return (Source && Name != 0) ? Source->FindAnimation(Name) : nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::Resume(Ref<Playback> Clock) const
    {
        // Brought to the time now first, so it plays on from there and not from wherever it was last moved to.
        Clock.Advance(mClock);
        Clock.Play();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Skin::Field Animator::Lay(Ref<Appearance> Output, ConstRef<Skin> Values)
    {
        using Field = Skin::Field;

        Field Changed = Field::None;

        Fields([&Output, &Values, &Changed]<typename Type>(Field Bit, Type Appearance::* Shown, Type Skin::* Given)
        {
            if (Values.Has(Bit) && Differs(Output.*Shown, Values.*Given))
            {
                Output.*Shown = Values.*Given;
                Changed      |= Bit;
            }
        });
        return Changed;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename Callable>
    void Animator::Fields(AnyRef<Callable> Action)
    {
        using Field = Skin::Field;

        Action(Field::Background,  &Appearance::Background,  &Skin::Background);
        Action(Field::Border,      &Appearance::Border,      &Skin::Border);
        Action(Field::BorderWidth, &Appearance::BorderWidth, &Skin::BorderWidth);
        Action(Field::Radius,      &Appearance::Radius,      &Skin::Radius);
        Action(Field::Opacity,     &Appearance::Opacity,     &Skin::Opacity);
        Action(Field::Tint,        &Appearance::Tint,        &Skin::Tint);
        Action(Field::Stroke,      &Appearance::Stroke,      &Skin::Stroke);
        Action(Field::StrokeWidth, &Appearance::StrokeWidth, &Skin::StrokeWidth);
        Action(Field::Fill,        &Appearance::Fill,        &Skin::Fill);
        Action(Field::Thumb,       &Appearance::Thumb,       &Skin::Thumb);
        Action(Field::Gradient,    &Appearance::Gradient,    &Skin::Gradient);
        Action(Field::Shadow,      &Appearance::Shadow,      &Skin::Shadow);
        Action(Field::Glow,        &Appearance::Glow,        &Skin::Glow);
        Action(Field::InnerGlow,   &Appearance::InnerGlow,   &Skin::InnerGlow);
        Action(Field::Scale,       &Appearance::Scale,       &Skin::Scale);
        Action(Field::Rotate,      &Appearance::Rotate,      &Skin::Rotate);
        Action(Field::Translate,   &Appearance::Translate,   &Skin::Translate);
        Action(Field::ImageTint,   &Appearance::ImageTint,   &Skin::ImageTint);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Color Animator::Mix(Color From, Color To, Real32 Share)
    {
        // An end showing nothing keeps the other end's hue, as a blend of premultiplied colours would.
        const Color Start = (From.GetAlpha() > 0.0f) ? From : Color::WithAlpha(To, 0.0f);
        const Color End   = (To.GetAlpha() > 0.0f) ? To : Color::WithAlpha(From, 0.0f);

        return Color::Lerp(Start, End, Share);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Halo Animator::Mix(ConstRef<Halo> From, ConstRef<Halo> To, Real32 Share)
    {
        Halo Result;
        Result.Tint   = Mix(From.Tint, To.Tint, Share);
        Result.Offset = ::Lerp(From.Offset, To.Offset, Share);
        Result.Blur   = ::Lerp(From.Blur, To.Blur, Share);
        Result.Spread = ::Lerp(From.Spread, To.Spread, Share);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Gradient Animator::Mix(ConstRef<Gradient> From, ConstRef<Gradient> To, Real32 Share)
    {
        Gradient Result;
        Result.From  = Mix(From.From, To.From, Share);
        Result.To    = Mix(From.To, To.To, Share);
        Result.Angle = ::Lerp(From.Angle, To.Angle, Share);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename Type>
    Type Animator::Mix(ConstRef<Type> From, ConstRef<Type> To, Real32 Share)
    {
        return ::Lerp(From, To, Share);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::Blend(Ref<Appearance>      Output,
                         ConstRef<Appearance> From,
                         ConstRef<Appearance> To,
                         Real32               Share,
                         Skin::Field          Mask)
    {
        using Field = Skin::Field;

        Fields([&Output, &From, &To, Share, Mask]<typename Type>(Field Bit, Type Appearance::* Shown, Type Skin::*)
        {
            if (HasBit(Mask, Bit))
            {
                Output.*Shown = Mix(From.*Shown, To.*Shown, Share);
            }
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename Type>
    Bool Animator::Differs(ConstRef<Type> First, ConstRef<Type> Second)
    {
        const ConstPtr<Byte> Left  = reinterpret_cast<ConstPtr<Byte>>(AddressOf(First));
        const ConstPtr<Byte> Right = reinterpret_cast<ConstPtr<Byte>>(AddressOf(Second));
        return !Compare(Left, Right, sizeof(Type));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Skin::Field Animator::Apart(ConstRef<Appearance> From, ConstRef<Appearance> To)
    {
        using Field = Skin::Field;

        Field Result = Field::None;

        Fields([&From, &To, &Result]<typename Type>(Field Bit, Type Appearance::* Shown, Type Skin::*)
        {
            Result |= Differs(From.*Shown, To.*Shown) ? Bit : Field::None;
        });
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename Type>
    Ptr<Type> Animator::Obtain(Ref<Tree> Owner, Handle Element, Bool Needed)
    {
        // A facet is added only for what a skin gives, so the common element holds none.
        const Ptr<Type> Found = Owner.Peek<Type>(Element.GetSlot());
        return (Found || !Needed) ? Found : AddressOf(Owner.Add<Type>(Element));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::ReadLook(Ref<Tree> Owner, UInt32 Slot, Ref<Appearance> Output)
    {
        if (const ConstPtr<Look> Shade = Owner.Peek<Look>(Slot))
        {
            Output.Background  = Shade->Background;
            Output.Border      = Shade->Border;
            Output.BorderWidth = Shade->BorderWidth;
            Output.Radius      = Shade->Radius;
            Output.Opacity     = Shade->Opacity;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::WriteLook(Ref<Tree> Owner, Handle Element, ConstRef<Appearance> Values, Skin::Field Given)
    {
        if (const Ptr<Look> Shade = Obtain<Look>(Owner, Element, HasAnyBit(Given, Motion::kShading)))
        {
            const Bool Solid = Shade->Opacity >= 1.0f;
            const Bool Fills = Shade->Background.IsOpaque();

            Shade->Background  = Values.Background;
            Shade->Border      = Values.Border;
            Shade->BorderWidth = Values.BorderWidth;
            Shade->Radius      = Values.Radius;
            Shade->Opacity     = Values.Opacity;

            // What it hides of what holds it is worked out again only when it turns opaque or stops being so.
            if (Solid != (Shade->Opacity >= 1.0f) || Fills != Shade->Background.IsOpaque())
            {
                Owner.Mark(Element, Dirty::Paint);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::ReadCaption(Ref<Tree> Owner, UInt32 Slot, Ref<Appearance> Output)
    {
        if (const ConstPtr<Caption> Text = Owner.Peek<Caption>(Slot))
        {
            Output.Tint        = Text->Tint;
            Output.Stroke      = Text->Stroke;
            Output.StrokeWidth = Text->StrokeWidth;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::WriteCaption(Ref<Tree> Owner, Handle Element, ConstRef<Appearance> Values, Skin::Field)
    {
        // Written in place, since editing a caption would lay the pane out again for a change of colour.
        if (const Ptr<Caption> Text = Owner.Peek<Caption>(Element.GetSlot()))
        {
            Text->Tint        = Values.Tint;
            Text->Stroke      = Values.Stroke;
            Text->StrokeWidth = Values.StrokeWidth;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::ReadBar(Ref<Tree> Owner, UInt32 Slot, Ref<Appearance> Output)
    {
        if (const ConstPtr<Bar> Fill = Owner.Peek<Bar>(Slot))
        {
            Output.Fill        = Fill->Fill;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::WriteBar(Ref<Tree> Owner, Handle Element, ConstRef<Appearance> Values, Skin::Field)
    {
        if (const Ptr<Bar> Fill = Owner.Peek<Bar>(Element.GetSlot()))
        {
            Fill->Fill = Values.Fill;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::ReadScroll(Ref<Tree> Owner, UInt32 Slot, Ref<Appearance> Output)
    {
        if (const ConstPtr<Scroll> Roll = Owner.Peek<Scroll>(Slot))
        {
            Output.Thumb       = Roll->Thumb;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::WriteScroll(Ref<Tree> Owner, Handle Element, ConstRef<Appearance> Values, Skin::Field)
    {
        if (const Ptr<Scroll> Roll = Owner.Peek<Scroll>(Element.GetSlot()))
        {
            Roll->Thumb = Values.Thumb;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::ReadEffect(Ref<Tree> Owner, UInt32 Slot, Ref<Appearance> Output)
    {
        if (const ConstPtr<Effect> Extra = Owner.Peek<Effect>(Slot))
        {
            Output.Gradient    = Extra->Fill;
            Output.Shadow      = Extra->Shadow;
            Output.Glow        = Extra->Glow;
            Output.InnerGlow   = Extra->InnerGlow;
            Output.Scale       = Extra->Scale;
            Output.Rotate      = Extra->Rotate;
            Output.Translate   = Extra->Translate;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::WriteEffect(Ref<Tree> Owner, Handle Element, ConstRef<Appearance> Values, Skin::Field Given)
    {
        if (const Ptr<Effect> Extra = Obtain<Effect>(Owner, Element, HasAnyBit(Given, Motion::kEffects)))
        {
            const Bool Moved = Extra->IsTransformed();
            const Bool Fills = Extra->Fill.From.IsOpaque() && Extra->Fill.To.IsOpaque();

            Extra->Fill      = Values.Gradient;
            Extra->Shadow    = Values.Shadow;
            Extra->Glow      = Values.Glow;
            Extra->InnerGlow = Values.InnerGlow;
            Extra->Scale     = Values.Scale;
            Extra->Rotate    = Values.Rotate;
            Extra->Translate = Values.Translate;

            // What it hides of its holder is redone only when a transform starts or ends, or the fill opacity changes.
            if (Moved != Extra->IsTransformed() || Fills != (Extra->Fill.From.IsOpaque() && Extra->Fill.To.IsOpaque()))
            {
                Owner.Mark(Element, Dirty::Paint);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::ReadBackdrop(Ref<Tree> Owner, UInt32 Slot, Ref<Appearance> Output)
    {
        if (const ConstPtr<Backdrop> Layer = Owner.Peek<Backdrop>(Slot))
        {
            Output.ImageTint   = Layer->Tint;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Animator::WriteBackdrop(Ref<Tree> Owner, Handle Element, ConstRef<Appearance> Values, Skin::Field Given)
    {
        if (const Ptr<Backdrop> Layer = Obtain<Backdrop>(Owner, Element, HasBit(Given, Skin::Field::ImageTint)))
        {
            Layer->Tint = Values.ImageTint;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Animator::FitCaption(Ref<Tree> Owner, Handle Element, ConstRef<Skin> Values)
    {
        const Ptr<Caption> Text = Owner.Peek<Caption>(Element.GetSlot());

        if (!Text)
        {
            return false;
        }

        // A new size or typeface changes what the text measures, the rest only how it is drawn.
        Bool Sized = false;

        if (Values.Has(Skin::Field::Size) && Text->Size != Values.Size)
        {
            Text->Size = Values.Size;
            Sized      = true;
        }

        if (Values.Has(Skin::Field::Font) && Text->Font != Values.Font)
        {
            Text->Font = Values.Font;
            Sized      = true;
        }

        if (Values.Has(Skin::Field::Alignment))
        {
            Text->Alignment = Values.Alignment;
        }
        return Sized;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Animator::FitBackdrop(Ref<Tree> Owner, Handle Element, ConstRef<Skin> Values)
    {
        // The image is only drawn, never measured, so swapping it never lays the pane out again.
        if (!HasAnyBit(Values.Mask, Motion::kImaging))
        {
            return false;
        }

        const Ptr<Backdrop> Layer = Obtain<Backdrop>(Owner, Element, true);

        if (Values.Has(Skin::Field::Image))
        {
            Layer->Source = Values.Image;
        }

        if (Values.Has(Skin::Field::Region))
        {
            Layer->Region = Values.Region;
        }

        if (Values.Has(Skin::Field::Slice))
        {
            Layer->Slice  = Values.Slice;
        }
        return false;
    }
}