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

#include "Interface.hpp"
#include "Element.hpp"
#include "Event.hpp"
#include "Facet/Layout/Virtual.hpp"
#include "Paint/Canvas.hpp"
#include "Pane.hpp"
#include "Shortcut.hpp"
#include "Style/Theme.hpp"
#include "System/Dispatcher.hpp"
#include "System/Dresser.hpp"
#include "System/Layout.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Interface::Interface()
        : mDensity    { 1.0f },
          mScale      { 1.0f },
          mRatio      { 1.0f },
          mAnimator   { * this },
          mDresser    { * this, mAnimator },
          mLayout     { * this, mAnimator },
          mPainter    { * this, mAnimator, mLayout },
          mDispatcher { * this },
          mPointer    { * this, mLayout, mDispatcher },
          mFocus      { * this, mDispatcher, mPointer },
          mInput      { nullptr },
          mSurface    { nullptr },
          mStaging    { false }
    {
        // A popup made for an element that died since goes with it, before its bindings are shown again.
        AddStage(Phase::Prepare, 10, Stage::Create<&Interface::OnPrune>());

        // A list making only the rows in view makes them for where it was scrolled since the last layout.
        AddStage(Phase::Prepare, 30, Stage::Create<&Interface::OnRealize>());

        // The pointer's own clock moves on, which tells a double click from two clicks.
        AddStage(Phase::Prepare, 40, Stage::Create<&Interface::OnTick>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::Update(Vector2 Size, Real32 Delta)
    {
        ZY_PROFILE_SCOPE("UI::Interface::Update");

        // A larger pixel ratio leaves the panes less room, and the pointer's pixel is another point of the layout.
        const Real32 Ratio    = GetPixelRatio();
        const Bool   Rescaled = (Ratio != mRatio);
        mRatio = Ratio;

        if (Rescaled)
        {
            mPointer.Rescale(Ratio);
        }

        const Vector2 Room    = Size / Ratio;
        const Bool    Resized = (Room != mLayout.GetSize());
        mLayout.SetSize(Room);

        // A theme that changed dresses every element again.
        const Bool Restyled = mDresser.Revise();

        // What modules prepare runs first, then the backdrop of a modal pane is made before anything is dressed.
        Run(Phase::Prepare, Delta);
        mDispatcher.Guard();

        if (Restyled)
        {
            Restyle();
        }

        for (const UInt16 Index : GetOrder())
        {
            mDresser.Dress(Access(Index));
        }

        for (ConstRef<Dresser::Chime> Entry : mDresser.Drain())
        {
            mDispatcher.Sound(Entry.Element, Entry.Moment);
        }

        // A move a restyle just started takes this frame's time too; an exit that ends here lays its pane out at once.
        mAnimator.Tick(Delta);

        // A pane may be sized as a share of the target, so a new size lays all of it out again.
        if (Resized)
        {
            Relayout();
        }
        Reflow();

        // Rows made for the room a layout just gave are dressed and laid out at once, so none shows up a frame late.
        for (UInt32 Pass = 0; Pass < Layout::kPasses && Realize(); ++Pass)
        {
            for (const UInt16 Index : GetOrder())
            {
                mDresser.Dress(Access(Index));
            }
            Reflow();
        }

        // An element asked into view before it was laid out is scrolled to now, and the scroll laid out at once.
        if (mFocus.Settle())
        {
            Reflow();
        }

        // Only a layout tells how large a tooltip or a menu came out, so one past an edge is moved back after it.
        if (mPointer.Fence())
        {
            Reflow();
        }
        mLayout.Forget();

        // What the pointer rests on moved under it with the new ratio, so the hover follows it there.
        if (Rescaled)
        {
            mPointer.Repick();
        }

        // What modules settle runs once every pane is laid out.
        Run(Phase::Settle, Delta);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::AddStage(Phase When, UInt32 Order, Stage Code)
    {
        // A stage added while the stages run waits, so the walk over them never shifts under it.
        if (mStaging)
        {
            mWaiting.Append(When, Order, ::Move(Code));
            return;
        }

        // After every stage of the same phase and order, so those run in the order they were added.
        const SInt Found = mStages.Find([When, Order](ConstRef<Entry> Each)
        {
            return Each.When > When || (Each.When == When && Each.Order > Order);
        });
        mStages.Insert(Found < 0 ? mStages.GetSize() : static_cast<UInt>(Found), When, Order, ::Move(Code));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::Draw(Ref<Canvas> Canvas)
    {
        ZY_PROFILE_SCOPE("UI::Interface::Draw");

        // The canvas scales every length to the target's pixels, so corners, blurs and glyphs are not stretched.
        const Real32 Previous = Canvas.GetScale();
        Canvas.SetScale(Previous * mRatio);

        for (const UInt16 Index : GetOrder())
        {
            mPainter.Paint(Canvas, Access(Index));
        }

        Canvas.SetScale(Previous);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::Relayout()
    {
        for (const UInt16 Index : GetOrder())
        {
            Ref<Pane> Target = Access(Index);
            Target.Whole = true;
            Target.Stale = true;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::SetScale(Real32 Factor)
    {
        mScale = Max(Factor, kMinimumScale);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::SetDensity(Real32 Factor)
    {
        mDensity = Max(Factor, kMinimumScale);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::SetTheme(ConstPtr<Theme> Source)
    {
        mAnimator.SetTheme(Source);
        mDresser.SetTheme(Source);
        mDispatcher.SetTheme(Source);

        // Whatever wore the last theme wears this one, or nothing.
        Restyle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::Copy(Text Content)
    {
        if (mSurface)
        {
            mSurface->SetClipboard(Content);
        }
        else
        {
            mClipboard = Content;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Text Interface::Paste()
    {
        if (mSurface)
        {
            // Text another program copied may end its lines in carriage returns, which no text here holds.
            const Str Copied = mSurface->GetClipboard();
            mClipboard.Clear();

            for (const Char Character : Text(Copied))
            {
                if (Character != '\r')
                {
                    mClipboard.Append(Character);
                }
            }
        }
        return mClipboard;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::Attach(Ref<ZyInput::Service> Input, Ptr<ZyPlatform::Window> Surface)
    {
        mInput   = AddressOf(Input);
        mSurface = Surface;

        Input.OnKeyDown.AddMethod<&Interface::OnKeyDown>(this);
        Input.OnKeyRepeat.AddMethod<&Interface::OnKeyRepeat>(this);
        Input.OnKeyType.AddMethod<&Interface::OnKeyType>(this);
        Input.OnMouseDown.AddMethod<&Interface::OnMouseDown>(this);
        Input.OnMouseUp.AddMethod<&Interface::OnMouseUp>(this);
        Input.OnMouseMove.AddMethod<&Interface::OnMouseMove>(this);
        Input.OnMouseScroll.AddMethod<&Interface::OnMouseScroll>(this);
        Input.OnWindowScale.AddMethod<&Interface::OnWindowScale>(this);
        Input.OnWindowFocus.AddMethod<&Interface::OnWindowFocus>(this);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::Detach(Ref<ZyInput::Service> Input)
    {
        Input.OnKeyDown.RemoveMethod<&Interface::OnKeyDown>(this);
        Input.OnKeyRepeat.RemoveMethod<&Interface::OnKeyRepeat>(this);
        Input.OnKeyType.RemoveMethod<&Interface::OnKeyType>(this);
        Input.OnMouseDown.RemoveMethod<&Interface::OnMouseDown>(this);
        Input.OnMouseUp.RemoveMethod<&Interface::OnMouseUp>(this);
        Input.OnMouseMove.RemoveMethod<&Interface::OnMouseMove>(this);
        Input.OnMouseScroll.RemoveMethod<&Interface::OnMouseScroll>(this);
        Input.OnWindowScale.RemoveMethod<&Interface::OnWindowScale>(this);
        Input.OnWindowFocus.RemoveMethod<&Interface::OnWindowFocus>(this);

        mInput   = nullptr;
        mSurface = nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interface::Dispatch(ConstRef<Event> Message)
    {
        return mDispatcher.Dispatch(Message);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::Bind(Text Action, Handler Callback)
    {
        mDispatcher.Bind(Action, ::Move(Callback));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::Unbind(Text Action)
    {
        mDispatcher.Unbind(Action);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::OnClick(Text Action, Reaction Code)
    {
        React(Action, Signal::kClick, ::Move(Code));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::OnChange(Text Action, Reaction Code)
    {
        React(Action, Signal::kChange, ::Move(Code));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::React(Text Action, UInt64 Kind, Reaction Code)
    {
        // What bubbles up from a control's parts, and every other kind of event, passes on untouched.
        Bind(Action, Handler::Create([Kind, Code = ::Move(Code)](Ref<Interface> Owner,
                                                                 ConstRef<Event> Message,
                                                                 Handle Self)
        {
            if (Message.Kind != Kind || Message.Target != Self)
            {
                return false;
            }

            // The event goes on bubbling, as a menu or a dialog holding the control waits for it to close.
            Code(Element(Owner, Self));
            return false;
        }));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::SetSoundHook(SoundHook Callback)
    {
        mDispatcher.SetSoundHook(::Move(Callback));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::Focus(Handle Element)
    {
        mFocus.Set(Element);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interface::NextFocus()
    {
        return mFocus.NextFocus();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interface::PreviousFocus()
    {
        return mFocus.PreviousFocus();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interface::Navigate(Heading Way)
    {
        return mFocus.Navigate(Way);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interface::Activate()
    {
        return mFocus.Activate();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interface::Cancel()
    {
        return mFocus.Cancel();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::Reveal(Handle Element)
    {
        mFocus.Reveal(Element);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::Summon(Handle Popup, Vector2 At, Handle Subject)
    {
        mPointer.Summon(Popup, At, Subject);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::Confine(Handle Root, Real32 Gap)
    {
        mPointer.Confine(Root, Gap);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Interface::GetModal() const
    {
        return mDispatcher.GetModal();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Vector2 Interface::GetPointer(Handle Element) const
    {
        return mPointer.GetPosition() - GetFrame(Element).GetPosition();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::AddShortcut(ConstRef<Shortcut> Entry, Handle Owner)
    {
        mFocus.AddShortcut(Entry, Owner);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::RemoveShortcut(ZyInput::Key Key, ZyInput::Modifier Modifiers, Handle Owner)
    {
        mFocus.RemoveShortcut(Key, Modifiers, Owner);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interface::OnMouseMove(Real32 X, Real32 Y, Real32 DeltaX, Real32 DeltaY)
    {
        return mPointer.OnMouseMove(X, Y, DeltaX, DeltaY, mRatio);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interface::OnMouseDown(ZyInput::Button Button)
    {
        return mPointer.OnMouseDown(Button);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interface::OnMouseUp(ZyInput::Button Button)
    {
        return mPointer.OnMouseUp(Button);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interface::OnMouseScroll(Real32 DeltaX, Real32 DeltaY)
    {
        return mPointer.OnMouseScroll(DeltaX, DeltaY, HasBit(GetModifiers(), ZyInput::Modifier::Shift));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interface::OnKeyDown(ZyInput::Key Key)
    {
        return mFocus.OnKeyDown(Key, GetModifiers());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interface::OnKeyRepeat(ZyInput::Key Key)
    {
        return mFocus.OnKeyDown(Key, GetModifiers(), true);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interface::OnKeyType(Text Content)
    {
        return mFocus.OnKeyType(Content);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interface::OnWindowScale(Real32 Scale)
    {
        SetDensity(Scale);
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interface::OnWindowFocus(Bool Focused)
    {
        // A button let go of while another window holds the keyboard never comes up here, so none is kept held.
        if (!Focused)
        {
            mPointer.Abandon();
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interface::Realize()
    {
        // Taken first, since one may make another that realizes or destroy one that did; the list is reused.
        mRealizing.Clear();
        GetFacets<Virtual>().ForEach([this](UInt32 Slot, Ref<Virtual>)
        {
            mRealizing.Append(GetHandle(Slot));
        });

        Bool Changed = false;

        for (UInt Index = 0; Index < mRealizing.GetSize(); ++Index)
        {
            const Handle Element = mRealizing[Index];
            const ConstPtr<Virtual> Found = Get<Virtual>(Element);

            if (Found && !Found->Realize.IsEmpty() && !IsPending(Element))
            {
                // Copied, since the code may add a facet of the kind and move the store under it.
                const Realizer Code = Found->Realize;
                Changed = Code(* this, Element) || Changed;
            }
        }
        return Changed;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::Run(Phase When, Real32 Delta)
    {
        mStaging = true;

        for (ConstRef<Entry> Each : mStages)
        {
            if (Each.When == When)
            {
                Each.Code(* this, Delta);
            }
        }

        mStaging = false;

        for (Ref<Entry> Each : mWaiting)
        {
            AddStage(Each.When, Each.Order, ::Move(Each.Code));
        }
        mWaiting.Clear();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::Reflow()
    {
        for (const UInt16 Index : GetOrder())
        {
            Ref<Pane> Target = Access(Index);

            if (Target.Stale)
            {
                mLayout.Run(Target);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::Restyle()
    {
        for (const UInt16 Index : GetOrder())
        {
            Ref<Pane> Target = Access(Index);
            Target.Restyle(0, Target.GetSize());
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::OnPrune(Ref<Interface> Owner, Real32)
    {
        Owner.mPointer.Prune();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::OnRealize(Ref<Interface> Owner, Real32)
    {
        Owner.Realize();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interface::OnTick(Ref<Interface> Owner, Real32 Delta)
    {
        Owner.mPointer.Tick(Delta);
    }
}