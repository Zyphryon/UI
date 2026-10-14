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

#include "Dispatcher.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Input/Control.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Pane.hpp"
#include "UI.Core/Shortcut.hpp"
#include "UI.Core/Style/Theme.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Dispatcher::Dispatcher(Ref<Interface> Owner)
        : mOwner { Owner },
          mTheme { nullptr }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dispatcher::SetTheme(ConstPtr<Theme> Source)
    {
        mTheme = Source;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dispatcher::Dispatch(ConstRef<Event> Message)
    {
        Bool Stopped = false;

        // What a module listens for is heard first, as a widget's new value reaches the model before any handler.
        for (ConstRef<Handler> Listener : mListeners)
        {
            Listener(mOwner, Message, Message.Target);
        }

        if (Message.Kind == Signal::kChange)
        {
            Sound(Message.Target, Cue::Change);
        }

        // Changes the handlers make to the shape of the tree wait, so the climb never loses its way.
        mOwner.Enter();

        for (Handle Current = Message.Target, Next; Current.IsValid(); Current = Next)
        {
            const ConstPtr<Control> Found = mOwner.Get<Control>(Current);

            // Off the root of a popup, the event carries on into whatever opened it.
            Next = Climb(Current);

            if (!Found)
            {
                continue;
            }

            // Copied, since a handler may add a control and move the store under this one.
            const Handler           Behaviour = Found->Behaviour;
            const Sequence<Handler> Observers = Found->Observers;
            const UInt64            Action    = Found->Action;

            Stopped = !Behaviour.IsEmpty() && Behaviour(mOwner, Message, Current);

            for (UInt Index = 0; !Stopped && Index < Observers.GetSize(); ++Index)
            {
                Stopped = Observers[Index](mOwner, Message, Current);
            }

            if (const ConstPtr<Handler> Bound = Action ? mActions.Find(Digest(Action)) : nullptr; !Stopped && Bound)
            {
                Stopped = Handler(* Bound)(mOwner, Message, Current);
            }

            if (Stopped || !mOwner.IsAlive(Current))
            {
                break;
            }
        }

        mOwner.Leave();
        return Stopped;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dispatcher::AddListener(Handler Code)
    {
        mListeners.Append(Move(Code));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dispatcher::Bind(Text Action, Handler Callback)
    {
        mActions.Assign(Digest(Identify(Action)), Move(Callback));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dispatcher::Unbind(Text Action)
    {
        mActions.Erase(Digest(Identify(Action)));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dispatcher::Perform(ConstRef<Shortcut> Entry, ConstRef<Event> Message)
    {
        // Copied, since the code may add or remove shortcuts and move the one it was handed.
        Handler Code = Entry.Callback;

        if (Code.IsEmpty() && Entry.Action != 0)
        {
            if (const ConstPtr<Handler> Bound = mActions.Find(Digest(Entry.Action)))
            {
                Code = * Bound;
            }
        }
        return !Code.IsEmpty() && Code(mOwner, Message, Message.Target);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dispatcher::SetSoundHook(SoundHook Callback)
    {
        mSoundHook = Move(Callback);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dispatcher::Sound(Handle Element, Cue Moment)
    {
        if (mSoundHook.IsEmpty() || !mTheme || !mOwner.IsAlive(Element))
        {
            return;
        }

        // Copied, since the hook may change the theme and drop the text it was handed.
        const Str Address(mTheme->FindSound(mOwner.GetStyle(Element), Moment));

        if (!Address.IsEmpty())
        {
            mSoundHook(Address, Moment, Element);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Dispatcher::GetModal() const
    {
        const ConstSpan Order = mOwner.GetOrder();

        // The front-most one shown wins; the backdrop and the popups opened from a modal pane are no modal panes.
        for (UInt Index = Order.GetSize(); Index-- > 0;)
        {
            ConstRef<Pane> Target = mOwner.GetPane(Order[Index]);
            const Handle   Root   = mOwner.GetHandle(Target.Slots[0]);

            if (Target.Band.Has(Layer::Trait::Modal) && !Target.IsHidden(0) && Root != mBackdrop && !IsOwned(Root))
            {
                return Root;
            }
        }
        return Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dispatcher::Guard()
    {
        const Handle Top = GetModal();

        if (!Top.IsValid())
        {
            if (mOwner.IsAlive(mBackdrop))
            {
                mOwner.SetState(mBackdrop, State::Hidden, true);
            }
            mModal = Handle();
            return;
        }

        // The backdrop covers the target, dressed by the theme as `Modal.Backdrop`, made the first time it is needed.
        if (!mOwner.IsAlive(mBackdrop))
        {
            mBackdrop = mOwner.Open(Layers::kModal);
            mModal    = Handle();
            mOwner.SetStyle(mBackdrop, Identify("Modal.Backdrop"));

            Ref<Box> Shape = mOwner.EditBox(mBackdrop);
            Shape.Placement = Position::Absolute;
            Shape.Left      = Length::Pixels(0.0f);
            Shape.Top       = Length::Pixels(0.0f);
            Shape.Right     = Length::Pixels(0.0f);
            Shape.Bottom    = Length::Pixels(0.0f);
        }

        mOwner.SetState(mBackdrop, State::Hidden, false);

        // It stands just under the modal pane in front, so every other modal pane is dimmed with the rest.
        if (Top != mModal)
        {
            mOwner.Raise(mBackdrop);
            mOwner.Raise(Top);
            mModal = Top;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dispatcher::IsOwned(Handle Root) const
    {
        const ConstPtr<Control> Found = mOwner.Get<Control>(Root);
        return Found && Found->Owner.IsValid();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dispatcher::IsWithin(Handle Element, Handle Ancestor) const
    {
        for (Handle Current = Element; Current.IsValid(); Current = mOwner.GetParent(Current))
        {
            if (Current == Ancestor)
            {
                return true;
            }
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dispatcher::IsOpenedFrom(Handle Element, Handle Ancestor) const
    {
        for (Handle Current = Element; mOwner.IsAlive(Current); Current = Climb(Current))
        {
            if (Current == Ancestor)
            {
                return true;
            }
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dispatcher::IsUsable(Handle Element) const
    {
        for (Handle Current = Element; Current.IsValid(); Current = Climb(Current))
        {
            if (!mOwner.IsAlive(Current) || HasAnyBit(mOwner.GetStates(Current), State::Hidden | State::Disabled))
            {
                return false;
            }
        }
        return Element.IsValid() && !mOwner.IsPending(Element);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dispatcher::IsShown(Handle Element) const
    {
        for (Handle Current = Element; Current.IsValid(); Current = mOwner.GetParent(Current))
        {
            if (!mOwner.IsAlive(Current) || mOwner.HasState(Current, State::Hidden))
            {
                return false;
            }
        }
        return Element.IsValid();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Dispatcher::Climb(Handle Element) const
    {
        if (const Handle Parent = mOwner.GetParent(Element); Parent.IsValid())
        {
            return Parent;
        }

        const ConstPtr<Control> Found = mOwner.Get<Control>(Element);
        return Found ? Found->Owner : Handle();
    }
}