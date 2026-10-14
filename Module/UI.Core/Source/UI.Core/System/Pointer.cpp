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

#include "Pointer.hpp"
#include "Dispatcher.hpp"
#include "Layout.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Input/Control.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Facet/Visual/Backdrop.hpp"
#include "UI.Core/Facet/Visual/Effect.hpp"
#include "UI.Core/Facet/Visual/Look.hpp"
#include "UI.Core/Facet/Visual/Scroll.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Layers.hpp"
#include "UI.Core/Pane.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Pointer::Pointer(Ref<Interface> Owner, Ref<Layout> Layout, Ref<Dispatcher> Dispatcher)
        : mOwner      { Owner },
          mLayout     { Layout },
          mDispatcher { Dispatcher },
          mGrab       { 0.0f },
          mBlocked    { false },
          mClock      { 0.0f }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<auto Part, typename... Arguments>
    Bool Pointer::Offer(Arguments... Values)
    {
        for (ConstRef<Hook> Each : mHooks)
        {
            if (!(Each.*Part).IsEmpty() && (Each.*Part)(mOwner, Values...))
            {
                return true;
            }
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<auto Part, typename... Arguments>
    void Pointer::Notify(Arguments... Values)
    {
        for (ConstRef<Hook> Each : mHooks)
        {
            if (!(Each.*Part).IsEmpty())
            {
                (Each.*Part)(mOwner, Values...);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pointer::Rescale(Real32 Ratio)
    {
        mPointer = mCursor / Ratio;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Pointer::OnMouseMove(Real32 X, Real32 Y, Real32 DeltaX, Real32 DeltaY, Real32 Ratio)
    {
        // The pixel is kept too, so a new ratio finds the point of the layout the pointer stands on now.
        mCursor  = Vector2(X, Y);
        mPointer = mCursor / Ratio;

        // A thumb held follows the pointer, and nothing else hears the move.
        if (mGrip.IsValid())
        {
            Slide();
            return true;
        }

        // A module's gesture may take the move, as a drag carrying something does.
        if (Offer<&Hook::Move>())
        {
            return true;
        }

        // The control each button went down on hears the pointer move until that button comes up, wherever it goes.
        for (UInt Index = 0; Index < kButtons; ++Index)
        {
            if (const Handle Pressed = mPressed[Index]; mOwner.IsAlive(Pressed))
            {
                Event Message(Signal::kDrag, Pressed);
                Message.Button    = static_cast<ZyInput::Button>(Index);
                Message.Modifiers = mOwner.GetModifiers();
                Message.Position  = mPointer;
                Message.Delta     = Vector2(DeltaX, DeltaY) / Ratio;
                mDispatcher.Dispatch(Message);
            }
        }

        Repick();

        // Every control asking for hover events under the pointer hears it move, the ones it just came over too.
        for (UInt Index = 0; Index < mHovered.GetSize(); ++Index)
        {
            if (const Handle Hovered = mHovered[Index]; Hovers(Hovered))
            {
                Tell(Signal::kMove, Hovered);
            }
        }

        Bool Holding = false;

        for (UInt Index = 0; Index < kButtons; ++Index)
        {
            Holding = Holding || mOwner.IsAlive(mPressed[Index]);
        }
        return mOver.IsValid() || mBlocked || Holding;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Pointer::OnMouseDown(ZyInput::Button Button)
    {
        const UInt Index = ZyEnum::Cast(Button);

        // The button is down from here, whatever takes its press.
        mHeld.Set(Index);

        // A module's gesture may take the press first, as another button letting a drag go does.
        if (Offer<&Hook::Press>(Button))
        {
            return true;
        }

        Repick();
        Dismiss();

        // A press on a scrolling element's track takes its thumb, pressing no control.
        if (Button == ZyInput::Button::Left && mOver.IsValid() && Grip(mOver))
        {
            return true;
        }

        Notify<&Hook::Pressed>(Button);

        if (!mOver.IsValid())
        {
            // Under a modal pane, a press outside it lands on the interface and changes nothing.
            if (mBlocked)
            {
                return true;
            }

            mOwner.Focus(Handle());
            return false;
        }

        const Handle Target = Nearest(mOver, false);

        mOwner.Focus(Nearest(mOver, true));

        // A press soon after the last of its button, on the same control and about the same place, counts on from it.
        Ref<Streak> Run   = mStreaks[Index];
        const Bool  Close = mPointer.GetDistance(Run.At) < kThreshold;
        const Bool  Again = (Target == Run.On) && mClock - Run.Time <= kRepeat && Close;

        Run.Count = Again ? Run.Count + 1 : 1;
        Run.Time  = mClock;
        Run.On    = Target;
        Run.At    = mPointer;

        // Moving the focus runs code that may destroy what was pressed.
        Bool Stopped = false;

        if (Target.IsValid() && mOwner.IsAlive(Target) && !mOwner.IsPending(Target))
        {
            // Only the left button lights a control; another holds and counts only on one taking every button.
            const Bool Left  = (Button == ZyInput::Button::Left);
            const Bool Holds = Left || TakesAny(Target);

            if (Holds)
            {
                mPressed[Index] = Target;
            }

            if (Left)
            {
                mOwner.SetState(Target, State::Press, true);
                mDispatcher.Sound(Target, Cue::Press);
            }

            Event Message(Signal::kPress, Target);
            Message.Button    = Button;
            Message.Modifiers = mOwner.GetModifiers();
            Message.Position  = mPointer;
            Message.Count     = Holds ? Run.Count : 1;
            Stopped = mDispatcher.Dispatch(Message);
        }

        Notify<&Hook::Offered>(Button, Stopped);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Pointer::OnMouseUp(ZyInput::Button Button)
    {
        const UInt Index = ZyEnum::Cast(Button);
        mHeld.Reset(Index);

        if (Button == ZyInput::Button::Left && mGrip.IsValid())
        {
            mGrip = Handle();
            return true;
        }

        // A pane may have closed under a still pointer since the press, so what it rests on is found again.
        Repick();

        // A module's gesture may take the release, as letting go of what is carried does.
        if (Offer<&Hook::Release>(Button))
        {
            return true;
        }

        const Handle Pressed = mPressed[Index];

        if (!Pressed.IsValid())
        {
            return mOver.IsValid() || mBlocked;
        }

        mPressed[Index] = Handle();

        if (mOwner.IsAlive(Pressed))
        {
            const Bool Left = (Button == ZyInput::Button::Left);

            if (Left)
            {
                mOwner.SetState(Pressed, State::Press, false);
            }

            Event Message(Signal::kRelease, Pressed);
            Message.Button    = Button;
            Message.Modifiers = mOwner.GetModifiers();
            Message.Position  = mPointer;
            mDispatcher.Dispatch(Message);

            // A release away from where it went down takes the click back, as does one on a control hidden since.
            if (mDispatcher.IsUsable(Pressed) && mDispatcher.IsWithin(mOver, Pressed))
            {
                if (Left)
                {
                    mDispatcher.Sound(Pressed, Cue::Click);
                }

                Message.Kind = Signal::kClick;
                mDispatcher.Dispatch(Message);
            }
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pointer::Abandon()
    {
        mGrip = Handle();

        for (UInt Index = 0; Index < kButtons; ++Index)
        {
            const Handle Pressed = mPressed[Index];
            mPressed[Index] = Handle();
            mHeld.Reset(Index);

            if (!mOwner.IsAlive(Pressed))
            {
                continue;
            }

            if (Index == ZyEnum::Cast(ZyInput::Button::Left))
            {
                mOwner.SetState(Pressed, State::Press, false);
            }

            // A button let go of while the window was away came up nowhere, so it clicks nothing.
            Event Message(Signal::kRelease, Pressed);
            Message.Button   = static_cast<ZyInput::Button>(Index);
            Message.Position = mPointer;
            mDispatcher.Dispatch(Message);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Pointer::OnMouseScroll(Real32 DeltaX, Real32 DeltaY, Bool Shift)
    {
        // A pane may have opened or closed under a still pointer, so what it rests on is found again.
        Repick();
        Notify<&Hook::Wheel>();

        if (!mOver.IsValid())
        {
            Notify<&Hook::Turned>(false);
            return mBlocked;
        }

        Event Message(Signal::kWheel, mOver);
        Message.Modifiers = mOwner.GetModifiers();
        Message.Position  = mPointer;
        Message.Delta     = Vector2(DeltaX, DeltaY);

        const Handle Target  = Nearest(mOver, false);
        const Bool   Stopped = Target.IsValid() && mDispatcher.Dispatch(Message);

        // What no control took scrolls the innermost scrolling element; with shift held the wheel turns sideways.
        if (!Stopped)
        {
            const Bool Sideways = Shift && DeltaX == 0.0f;
            Wheel(mOver, Sideways ? -DeltaY : DeltaX, Sideways ? 0.0f : DeltaY);
        }

        Notify<&Hook::Turned>(Stopped);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pointer::Summon(Handle Popup, Vector2 At, Handle Subject)
    {
        if (!mOwner.IsAlive(Popup))
        {
            return;
        }

        Ref<Box> Shape = mOwner.EditBox(Popup);
        Shape.Placement = Position::Absolute;
        Shape.Left      = Length::Pixels(At.GetX());
        Shape.Top       = Length::Pixels(At.GetY());

        // A popup opened from a modal pane must stand over it, so it takes that band rather than its own.
        const ConstPtr<Control> Found  = mOwner.Get<Control>(Popup);
        const Handle            Caller = Found ? Found->Owner : Handle();
        const Bool              Above  = mOwner.IsAlive(Caller) && mOwner.GetBand(Caller).Order > Layers::kPopup.Order;
        const Layer             Band   = Above ? mOwner.GetBand(Caller) : Layers::kPopup;

        if (mOwner.GetBand(Popup) != Band)
        {
            mOwner.SetBand(Popup, Band);
        }

        mOwner.SetState(Popup, State::Hidden, false);
        mOwner.Raise(Popup);
        Confine(Popup);

        if (!mSummoned.Contains(Popup))
        {
            mSummoned.Append(Popup);
        }

        Event Message(Signal::kOpen, Popup);
        Message.Other    = Subject;
        Message.Position = At;
        mDispatcher.Dispatch(Message);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pointer::Confine(Handle Root, Real32 Gap)
    {
        if (!mOwner.IsAlive(Root))
        {
            return;
        }

        const Bool Kept = mConfined.Contains([Root](ConstRef<Fenced> Each)
        {
            return Each.Root == Root;
        });

        if (!Kept)
        {
            mConfined.Append(Root, Gap);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Pointer::IsSummoned(Handle Root) const
    {
        return mSummoned.Contains(Root);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pointer::Repick()
    {
        mOver = Pick(mPointer);

        // Every control holding what the pointer rests on is hovered, and each one it left is not.
        mReached.Clear();

        for (Handle Current = mOver; Current.IsValid(); Current = mOwner.GetParent(Current))
        {
            if (mOwner.Has<Control>(Current))
            {
                mReached.Append(Current);
            }
        }

        for (const Handle Left : mHovered)
        {
            if (!mReached.Contains(Left) && mOwner.IsAlive(Left) && !mOwner.IsPending(Left))
            {
                mOwner.SetState(Left, State::Hover, false);
            }
        }

        for (const Handle Entered : mReached)
        {
            if (!mHovered.Contains(Entered))
            {
                mOwner.SetState(Entered, State::Hover, true);

                // A control that cannot be used is still lit, but makes no sound.
                if (mDispatcher.IsUsable(Entered))
                {
                    mDispatcher.Sound(Entered, Cue::Hover);
                }
            }
        }

        // Swapped rather than moved, so the next finding fills the storage of this one and allocates nothing.
        ::Swap(mHovered, mReached);
        Announce();
        Notify<&Hook::Picked>();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pointer::Prune()
    {
        // A destruction held back by a walk would leave the popup standing, so none is pruned until the walk ends.
        if (mOwner.IsWalking())
        {
            return;
        }

        // One pane at a time, since destroying one changes the order and may orphan the submenus opened from it.
        for (Bool Pruned = true; Pruned;)
        {
            Pruned = false;

            for (const UInt16 Index : mOwner.GetOrder())
            {
                const Handle            Root   = mOwner.GetHandle(mOwner.GetPane(Index).Slots[0]);
                const ConstPtr<Control> Opener = mOwner.Get<Control>(Root);

                if (Opener && Opener->Owner.IsValid() && !mOwner.IsAlive(Opener->Owner))
                {
                    mOwner.Destroy(Root);
                    Pruned = true;
                    break;
                }
            }
        }

        // A summoned popup that died is no longer one to close.
        mSummoned.RemoveFastSomeIf([this](Handle Root)
        {
            return !mOwner.IsAlive(Root);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pointer::Tick(Real32 Delta)
    {
        // The pointer keeps its own time, which tells a double click from two clicks.
        mClock += Delta;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Pointer::Fence()
    {
        Bool Moved = false;

        mConfined.RemoveFastSomeIf([this](ConstRef<Fenced> Each)
        {
            return !mOwner.IsAlive(Each.Root);
        });

        for (ConstRef<Fenced> Each : mConfined)
        {
            const Handle Root = Each.Root;

            if (mOwner.IsPending(Root) || mOwner.HasState(Root, State::Hidden))
            {
                continue;
            }

            const Rect    Frame  = mOwner.GetFrame(Root);
            const Vector2 Room   = Vector2::Max(mLayout.GetSize() - Frame.GetSize(), Vector2());
            const Vector2 Corner = Vector2::Clamp(Frame.GetPosition(), Vector2(), Room);
            const Real32  Left   = Corner.GetX();
            Real32        Top    = Corner.GetY();

            // One kept a gap from the pointer, as a tooltip, stands above it instead of running off the bottom.
            if (Each.Gap > 0.0f && Frame.GetMaximumY() > mLayout.GetSize().GetY())
            {
                Top = Max(mPointer.GetY() - Each.Gap * 0.5f - Frame.GetHeight(), 0.0f);
            }

            if (Vector2(Left, Top).GetDistanceChebyshev(Frame.GetPosition()) < 0.5f)
            {
                continue;
            }

            Ref<Box> Shape = mOwner.EditBox(Root);
            Shape.Left = Length::Pixels(Left);
            Shape.Top  = Length::Pixels(Top);
            Moved      = true;
        }

        return Moved;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pointer::AddHook(Hook Entry)
    {
        mHooks.Append(Move(Entry));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Pointer::OnKey(ZyInput::Key Key)
    {
        return Offer<&Hook::Key>(Key);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pointer::Unpress()
    {
        Ref<Handle> Pressed = mPressed[ZyEnum::Cast(ZyInput::Button::Left)];

        if (mOwner.IsAlive(Pressed))
        {
            mOwner.SetState(Pressed, State::Press, false);
        }
        Pressed = Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Pointer::Nearest(Handle Element, Bool Focusable) const
    {
        for (Handle Current = Element; Current.IsValid(); Current = mOwner.GetParent(Current))
        {
            const ConstPtr<Control> Found = mOwner.Get<Control>(Current);

            if (Found && (!Focusable || Found->Focusable))
            {
                return mDispatcher.IsUsable(Current) ? Current : Handle();
            }
        }
        return Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Pointer::Pick(Vector2 Point)
    {
        const ConstSpan Order = mOwner.GetOrder();

        mBlocked = false;

        // Panes are tried front to back; tooltips, ghosts and the backdrop of a modal pane are only shown, never hit.
        for (UInt Index = Order.GetSize(); Index-- > 0;)
        {
            ConstRef<Pane> Target = mOwner.GetPane(Order[Index]);
            UInt32         Found  = Pane::kNoParent;
            const Handle   Root   = mOwner.GetHandle(Target.Slots[0]);

            if (Target.Band.Has(Layer::Trait::Overlay) || Root == mDispatcher.GetBackdrop())
            {
                continue;
            }

            for (UInt32 Row = 0; Row < Target.GetSize(); ++Row)
            {
                const UInt32         Slot  = Target.Slots[Row];
                const ConstPtr<Look> Shade = mOwner.Peek<Look>(Slot);
                const Bool           Cuts  = (Shade && Shade->Clip) || mOwner.Peek<Scroll>(Slot);
                const Bool           Over  = Target.Frames[Row].Contains(Point);

                // Hidden or faded out, it is not there; cutting what it holds, nothing of it is outside its box.
                if (Target.IsHidden(Row) || (Shade && Shade->Opacity <= 0.0f) || (Cuts && !Over))
                {
                    Row += Target.Spans[Row] - 1;
                    continue;
                }

                // Only what draws, or holds a kind of facet that answers the pointer, stops it.
                const ConstPtr<Effect>   Extra = mOwner.Peek<Effect>(Slot);
                const ConstPtr<Backdrop> Layer = mOwner.Peek<Backdrop>(Slot);
                const ConstPtr<Scroll>   Roll  = mOwner.Peek<Scroll>(Slot);
                const Real32             Top   = Target.Frames[Row].GetMinimumY() + Target.Boxes[Row].Padding.Top;
                const Bool               Solid = mOwner.IsInteractive(Slot)
                    || (Shade && (Shade->Background.GetAlpha() > 0.0f || Shade->BorderWidth > 0.0f))
                    || (Extra && Extra->Fill.IsVisible())
                    || (Layer && Layer->Source)
                    || (Roll && Roll->GetTrack(Target.Frames[Row], Top).Contains(Point));

                if (Over && Solid)
                {
                    Found = Row;
                }
            }

            if (Found != Pane::kNoParent)
            {
                return mOwner.GetHandle(Target.Slots[Found]);
            }

            // A modal pane holds the pointer: whatever stands behind it is out of reach.
            if (Target.Band.Has(Layer::Trait::Modal) && !Target.IsHidden(0) && !mDispatcher.IsOwned(Root))
            {
                mBlocked = true;
                return Handle();
            }
        }
        return Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pointer::Dismiss()
    {
        const Handle Over = mOver.IsValid() ? mOwner.GetRoot(mOver) : Handle();

        for (const UInt16 Index : mOwner.GetOrder())
        {
            ConstRef<Pane> Target = mOwner.GetPane(Index);
            const Handle   Root   = mOwner.GetHandle(Target.Slots[0]);

            // A popup opened from a modal pane stands in its band, and is closed the same.
            if ((!Target.Band.Has(Layer::Trait::Popup) && !mDispatcher.IsOwned(Root)) || Target.IsHidden(0))
            {
                continue;
            }

            const ConstPtr<Control> Opener   = mOwner.Get<Control>(Root);
            const Bool              Summoned = mSummoned.Contains(Root);

            // A press on the popup, or on the opener of one not summoned at the pointer, is left to them to handle.
            const Bool Beyond = !Summoned && Opener && mOwner.IsAlive(Opener->Owner) &&
                               mDispatcher.IsOpenedFrom(mOver, Opener->Owner);
            const Bool Kept   = Root == Over || mDispatcher.IsOpenedFrom(mOver, Root) || Beyond;

            if (!Kept)
            {
                mOwner.SetState(Root, State::Hidden, true);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Pointer::TakesAny(Handle Target) const
    {
        const ConstPtr<Control> Found = mOwner.Get<Control>(Target);
        return Found && Found->AnyButton;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Pointer::Hovers(Handle Target) const
    {
        const ConstPtr<Control> Found = mOwner.Get<Control>(Target);
        return Found && Found->Hover && !mOwner.IsPending(Target);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pointer::Tell(UInt64 Kind, Handle Target)
    {
        Event Message(Kind, Target);
        Message.Modifiers = mOwner.GetModifiers();
        Message.Position  = mPointer;
        mDispatcher.Dispatch(Message);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pointer::Announce()
    {
        // Walked by index, since a handler may find what the pointer rests on again and refill both lists.
        for (UInt Index = 0; Index < mReached.GetSize(); ++Index)
        {
            if (const Handle Left = mReached[Index]; Hovers(Left) && !mHovered.Contains(Left))
            {
                Tell(Signal::kLeave, Left);
            }
        }

        for (UInt Index = 0; Index < mHovered.GetSize(); ++Index)
        {
            if (const Handle Entered = mHovered[Index]; Hovers(Entered) && !mReached.Contains(Entered))
            {
                Tell(Signal::kEnter, Entered);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pointer::Wheel(Handle Element, Real32 Across, Real32 Down)
    {
        for (Handle Current = Element; Current.IsValid(); Current = mOwner.GetParent(Current))
        {
            const ConstPtr<Scroll> Found = mOwner.Get<Scroll>(Current);

            // A turn sideways passes over what scrolls only up and down, to the scroll holding it.
            if (!Found || (Down == 0.0f && !Found->Horizontal))
            {
                continue;
            }

            const Real32 Limit  = Max(Found->Overflow - Found->View.GetY(), 0.0f);
            const Real32 LimitX = Max(Found->OverflowX - Found->View.GetX(), 0.0f);

            // One with nowhere to go either way, as a text field holding a short line, passes the turn on.
            if (Limit <= 0.0f && (!Found->Horizontal || LimitX <= 0.0f))
            {
                continue;
            }

            const Real32 Offset  = Clamp(Found->Offset - Down * kNotch, 0.0f, Limit);
            const Real32 OffsetX = Found->Horizontal ? Clamp(Found->OffsetX + Across * kNotch, 0.0f, LimitX)
                                                     : Found->OffsetX;

            // The innermost one takes the turn, and stops following its end once moved up.
            const Ptr<Scroll> Moved = mOwner.Edit<Scroll>(Current);
            Moved->Offset  = Offset;
            Moved->OffsetX = OffsetX;
            Moved->Follow  = Moved->Follow && Down <= 0.0f;
            return;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Pointer::Grip(Handle Element)
    {
        for (Handle Current = Element; Current.IsValid(); Current = mOwner.GetParent(Current))
        {
            const ConstPtr<Scroll> Found = mOwner.Get<Scroll>(Current);

            if (!Found)
            {
                continue;
            }

            const Rect   Frame = mOwner.GetFrame(Current);
            const Real32 Top   = GetRoomTop(Current);

            if (!Found->GetTrack(Frame, Top).Contains(mPointer))
            {
                continue;
            }

            // A press on the thumb keeps the place it was taken by; one beside it takes the thumb by its middle.
            const Rect Knob = Found->GetThumb(Frame, Top);
            const Bool Held = mPointer.GetY() >= Knob.GetMinimumY() && mPointer.GetY() <= Knob.GetMaximumY();

            mGrip = Current;
            mGrab = Held ? mPointer.GetY() - Knob.GetMinimumY() : Knob.GetHeight() * 0.5f;

            if (!Held)
            {
                Slide();
            }
            return true;
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pointer::Slide()
    {
        const ConstPtr<Scroll> Found = mOwner.IsAlive(mGrip) ? mOwner.Get<Scroll>(mGrip) : nullptr;

        if (!Found)
        {
            mGrip = Handle();
            return;
        }

        const Real32 Top    = GetRoomTop(mGrip);
        const Real32 Room   = Found->View.GetY();
        const Real32 Limit  = Max(Found->Overflow - Room, 0.0f);
        const Real32 Travel = Room - Found->GetThumb(mOwner.GetFrame(mGrip), Top).GetHeight();

        if (Travel <= 0.0f || Limit <= 0.0f)
        {
            return;
        }

        // The thumb's top goes where the pointer is less the place it was taken by, held within the track.
        const Real32 Share = Clamp((mPointer.GetY() - mGrab - Top) / Travel, 0.0f, 1.0f);

        const Ptr<Scroll> Moved = mOwner.Edit<Scroll>(mGrip);
        Moved->Offset = Share * Limit;
        Moved->Follow = Moved->Follow && Share >= 1.0f;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Pointer::GetRoomTop(Handle Element) const
    {
        return mOwner.GetFrame(Element).GetMinimumY() + mOwner.GetBox(Element).Padding.Top;
    }
}