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
#include "UI.Core/Facet/Input/Control.hpp"
#include "UI.Core/Facet/Layout/Box.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Pane.hpp"
#include "UI.Core/Style/Theme.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Dispatcher::Dispatcher(Ref<Interface> Owner)
        : mOwner { Owner }
    {
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

    void Dispatcher::UnbindAll()
    {
        mActions.Clear();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Dispatcher::IsBound(Text Action) const
    {
        return mActions.Contains(Digest(Identify(Action)));
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

    void Dispatcher::Attach(Ref<ZyEngine::Subsystem::Host> Host)
    {
        mAudio   = Host.GetService<ZyAudio::Service>();
        mContent = Host.GetService<ZyContent::Service>();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Dispatcher::Sound(Handle Element, Cue Moment)
    {
        const ConstPtr<Theme> Source = mOwner.GetTheme();

        if (!mAudio || !mContent || !Source || !mOwner.IsAlive(Element))
        {
            return;
        }

        const Text Address = Source->FindSound(mOwner.GetStyle(Element), Moment);

        if (Address.IsEmpty())
        {
            return;
        }

        // A sound is loaded the first time a style asks for it and kept, so only that time waits for it to land.
        Ref<Retainer<ZyAudio::Sound>> Found = mSounds.FindOrInsert(Digest(Identify(Address)));

        if (!Found)
        {
            Found = mContent->Load<ZyAudio::Sound>(ZyContent::Uri(Address));
        }

        // One still on its way plays when it lands, holding the service and the sound rather than the dispatcher.
        ZyContent::Service::Callback Ring = [Audio = mAudio, Sound = Found](Ref<ZyContent::Resource>)
        {
            if (Sound->HasCompleted())
            {
                Audio->Play(ZyAudio::Category::Interface, Sound, 1.0f);
            }
        };

        if (mContent->Await(Found, Move(Ring)) && Found->HasCompleted())
        {
            mAudio->Play(ZyAudio::Category::Interface, Found, 1.0f);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Dispatcher::GetModal() const
    {
        const ConstSpan Order = mOwner.GetOrder();

        // The front-most one shown wins, and neither the backdrop nor a modal pane's popups count.
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

        // The backdrop covers the target, dressed as `Modal.Backdrop`, and is made the first time it is needed.
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
        return Element.IsValid();
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