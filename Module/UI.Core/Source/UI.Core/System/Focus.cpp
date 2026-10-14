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

#include "Focus.hpp"
#include "Dispatcher.hpp"
#include "Pointer.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Event.hpp"
#include "UI.Core/Facet/Input/Control.hpp"
#include "UI.Core/Facet/Input/Keymap.hpp"
#include "UI.Core/Facet/Visual/Scroll.hpp"
#include "UI.Core/Pane.hpp"
#include "UI.Core/Shortcut.hpp"
#include "UI.Core/Tree.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Focus::Focus(Ref<Tree> Owner, Ref<Dispatcher> Dispatcher, Ref<Pointer> Pointer)
        : mTree       { Owner },
          mDispatcher { Dispatcher },
          mPointer    { Pointer }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Focus::Set(Handle Element)
    {
        if (Element == mFocused)
        {
            return;
        }

        const Handle Previous = mFocused;
        mFocused = Element;

        if (mTree.IsAlive(Previous))
        {
            mTree.SetState(Previous, State::Focus, false);
            mDispatcher.Dispatch(Event(Signal::kBlur, Previous));
        }

        if (mTree.IsAlive(Element))
        {
            mTree.SetState(Element, State::Focus, true);
            mDispatcher.Sound(Element, Cue::Focus);
            mDispatcher.Dispatch(Event(Signal::kFocus, Element));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::NextFocus()
    {
        return Cycle(true);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::PreviousFocus()
    {
        return Cycle(false);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::Navigate(Heading Way)
    {
        // A pad has no keyboard to start from, so the first move with nothing focused picks the first control.
        if (!mDispatcher.IsUsable(mFocused) && !mDispatcher.GetModal().IsValid())
        {
            return Cycle(true);
        }
        return Press(KeyOf(Way), ZyInput::Modifier::None);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::Activate()
    {
        if (!mDispatcher.IsUsable(mFocused) && !mDispatcher.GetModal().IsValid())
        {
            return false;
        }
        return Press(ZyInput::Key::Enter, ZyInput::Modifier::None);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::Cancel()
    {
        return Press(ZyInput::Key::Escape, ZyInput::Modifier::None);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Focus::Reveal(Handle Element)
    {
        if (!mTree.IsAlive(Element) || mTree.IsPending(Element))
        {
            return;
        }

        // Each scroll moves the box as the next one out sees it, so an element deep in nested scrolls comes out whole.
        Rect Box = mTree.GetFrame(Element);

        for (Handle Current = mTree.GetParent(Element); Current.IsValid(); Current = mTree.GetParent(Current))
        {
            const ConstPtr<Scroll> Found = mTree.Get<Scroll>(Current);

            if (!Found)
            {
                continue;
            }

            const Vector2 Corner = mTree.GetBox(Current).Padding.Inset(mTree.GetFrame(Current)).GetPosition();
            const Rect    Seen(Corner, Corner + Found->View);

            const Real32 Down   = Nudge(Box.GetMinimumY(), Box.GetMaximumY(), Seen.GetMinimumY(), Seen.GetMaximumY());
            const Real32 Across = Found->Horizontal
                ? Nudge(Box.GetMinimumX(), Box.GetMaximumX(), Seen.GetMinimumX(), Seen.GetMaximumX())
                : 0.0f;

            const Real32 Offset = Clamp(Found->Offset + Down, 0.0f, Max(Found->Overflow - Found->View.GetY(), 0.0f));
            const Real32 OffsetX = Clamp(Found->OffsetX + Across, 0.0f,
                                         Max(Found->OverflowX - Found->View.GetX(), 0.0f));

            // Edited only when it moves, since an edit lays the pane out again.
            if (Offset != Found->Offset || OffsetX != Found->OffsetX)
            {
                const Ptr<Scroll> Moved = mTree.Edit<Scroll>(Current);
                Box = Box + Vector2(Moved->OffsetX - OffsetX, Moved->Offset - Offset);

                Moved->Offset  = Offset;
                Moved->OffsetX = OffsetX;
                Moved->Follow  = false;
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::IsTyping() const
    {
        const ConstPtr<Control> Found = mDispatcher.IsUsable(mFocused) ? mTree.Get<Control>(mFocused) : nullptr;
        return Found && Found->Typing;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Focus::AddShortcut(ConstRef<Shortcut> Entry, Handle Owner)
    {
        if (!Owner.IsValid())
        {
            mShortcuts.Append(Entry);
        }
        else if (mTree.IsAlive(Owner))
        {
            mTree.Obtain<Keymap>(Owner).Entries.Append(Entry);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Focus::RemoveShortcut(ZyInput::Key Key, ZyInput::Modifier Modifiers, Handle Owner)
    {
        const auto Chord = [Key, Modifiers](ConstRef<Shortcut> Each)
        {
            return Each.Key == Key && Each.Modifiers == Modifiers;
        };

        if (!Owner.IsValid())
        {
            mShortcuts.RemoveSomeIf(Chord);
        }
        else if (const Ptr<Keymap> Found = mTree.IsAlive(Owner) ? mTree.Get<Keymap>(Owner) : nullptr)
        {
            Found->Entries.RemoveSomeIf(Chord);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::OnKeyDown(ZyInput::Key Key, ZyInput::Modifier Modifiers, Bool Repeat)
    {
        // A module's gesture may take the key first, as escape letting go of what is carried does, then a shortcut may.
        return mPointer.OnKey(Key) || Answer(Key, Modifiers, Repeat) || Deliver(Key, Modifiers);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::Press(ZyInput::Key Key, ZyInput::Modifier Modifiers)
    {
        return mPointer.OnKey(Key) || Deliver(Key, Modifiers);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::Answer(ZyInput::Key Key, ZyInput::Modifier Modifiers, Bool Repeat)
    {
        // Only shift, control and alt make a chord, so a held super key changes nothing.
        const ZyInput::Modifier Chord = Modifiers &
            (ZyInput::Modifier::Shift | ZyInput::Modifier::Control | ZyInput::Modifier::Alt);

        // The elements are taken first, since a shortcut's code may give an element shortcuts or destroy one.
        Sequence<Handle> Owners;
        mTree.GetFacets<Keymap>().ForEach([this, &Owners](UInt32 Slot, Ref<Keymap>)
        {
            Owners.Append(mTree.GetHandle(Slot));
        });

        // A shortcut kept for where the pointer or the keyboard is goes before one that runs anywhere.
        for (UInt32 Pass = 0; Pass < 2; ++Pass)
        {
            for (const Handle Owner : Owners)
            {
                const Bool Placed = (Pass == 0);

                // Found again for every entry, since the code of the last may have moved the store or the element.
                for (UInt Index = 0; ; ++Index)
                {
                    const ConstPtr<Keymap> Found = mTree.Get<Keymap>(Owner);

                    if (!Found || Index >= Found->Entries.GetSize())
                    {
                        break;
                    }

                    ConstRef<Shortcut> Entry = Found->Entries[Index];

                    const Bool Stands = (Entry.Where != Shortcut::Scope::Anywhere) == Placed;

                    if (Stands && Answers(Entry, Owner, Key, Chord, Repeat) && Perform(Entry, Owner, Key, Modifiers))
                    {
                        return true;
                    }
                }
            }
        }

        // The interface's own run anywhere, but never behind a modal pane.
        for (UInt Index = 0; Index < mShortcuts.GetSize(); ++Index)
        {
            ConstRef<Shortcut> Entry = mShortcuts[Index];

            if (Answers(Entry, Handle(), Key, Chord, Repeat) && Perform(Entry, Handle(), Key, Modifiers))
            {
                return true;
            }
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::Answers(ConstRef<Shortcut> Entry,
                        Handle             Owner,
                        ZyInput::Key       Key,
                        ZyInput::Modifier  Modifiers,
                        Bool               Repeat) const
    {
        const Bool Differs  = Entry.Key != Key || Entry.Modifiers != Modifiers;
        const Bool Declined = (Repeat && !Entry.Repeat) || (!Entry.Typing && IsTyping());

        if (Differs || Declined)
        {
            return false;
        }

        // A modal pane keeps every shortcut behind it from running, its own and those of the popups it opened aside.
        const Handle Modal = mDispatcher.GetModal();

        if (!Owner.IsValid())
        {
            return !Modal.IsValid();
        }

        if (!mDispatcher.IsUsable(Owner) || (Modal.IsValid() && !mDispatcher.IsOpenedFrom(Owner, Modal)))
        {
            return false;
        }

        switch (Entry.Where)
        {
        case Shortcut::Scope::Hover:
            return mDispatcher.IsWithin(mPointer.GetOver(), Owner);
        case Shortcut::Scope::Focus:
            return mDispatcher.IsUsable(mFocused) && mDispatcher.IsOpenedFrom(mFocused, Owner);
        default:
            return true;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::Perform(Shortcut Entry, Handle Owner, ZyInput::Key Key, ZyInput::Modifier Modifiers)
    {
        // As an undo does, a shortcut may first drop the caret of a half typed edit, which commits it as leaving does.
        if (Entry.Blur && IsTyping())
        {
            Set(Handle());
        }

        Event Message(Signal::kKey, Owner.IsValid() ? Owner : mFocused);
        Message.Key       = Key;
        Message.Modifiers = Modifiers;
        Message.Position  = mPointer.GetPosition();
        return mDispatcher.Perform(Entry, Message);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::Deliver(ZyInput::Key Key, ZyInput::Modifier Modifiers)
    {
        const Bool Backward = HasBit(Modifiers, ZyInput::Modifier::Shift);

        // A modal pane holds the keyboard, so a control outside it hears nothing and the pane's root hears it instead.
        Handle       Target = mFocused;
        const Handle Modal  = mDispatcher.GetModal();

        if (Modal.IsValid() && !mDispatcher.IsOpenedFrom(mFocused, Modal))
        {
            if (!mDispatcher.IsUsable(Modal) || !mTree.Has<Control>(Modal))
            {
                return true;
            }
            Target = Modal;
        }
        else if (!mDispatcher.IsUsable(mFocused))
        {
            // With nothing focused, tab starts moving the keyboard and escape closes a popup; the rest reach the game.
            Set(Handle());
            return (Key == ZyInput::Key::Tab && Cycle(!Backward)) || (Key == ZyInput::Key::Escape && Retreat());
        }

        Event Message(Signal::kKey, Target);
        Message.Key       = Key;
        Message.Modifiers = Modifiers;

        if (!mDispatcher.Dispatch(Message))
        {
            Steer(Key, Modifiers, Target);
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::OnKeyType(Text Content)
    {
        // Typing reaches no control a modal pane stands over.
        if (const Handle Modal = mDispatcher.GetModal(); Modal.IsValid() && !mDispatcher.IsOpenedFrom(mFocused, Modal))
        {
            return true;
        }

        if (!mDispatcher.IsUsable(mFocused))
        {
            Set(Handle());
            return false;
        }

        Event Message(Signal::kType, mFocused);
        Message.Content = Content;
        mDispatcher.Dispatch(Message);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::Steer(ZyInput::Key Key, ZyInput::Modifier Modifiers, Handle Target)
    {
        // Under a modal pane the keyboard may stand behind it; then only moving into the pane or closing it is done.
        const Bool Held = (Target == mFocused);

        switch (Key)
        {
        case ZyInput::Key::Tab:
            return Cycle(!HasBit(Modifiers, ZyInput::Modifier::Shift));
        case ZyInput::Key::Up:
            return Held ? Travel(Heading::Up) : Cycle(true);
        case ZyInput::Key::Down:
            return Held ? Travel(Heading::Down) : Cycle(true);
        case ZyInput::Key::Left:
            return Held ? Travel(Heading::Left) : Cycle(true);
        case ZyInput::Key::Right:
            return Held ? Travel(Heading::Right) : Cycle(true);
        case ZyInput::Key::Enter:
        case ZyInput::Key::Space:
            return Held && Trigger(mFocused);
        case ZyInput::Key::Escape:
            return Retreat();
        case ZyInput::Key::PageUp:
        case ZyInput::Key::PageDown:
        case ZyInput::Key::Home:
        case ZyInput::Key::End:
            return Held && Page(Key);
        default:
            return false;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Focus::Arena()
    {
        if (const Handle Modal = mDispatcher.GetModal(); Modal.IsValid())
        {
            return Modal;
        }

        if (mDispatcher.IsUsable(mFocused))
        {
            return mTree.GetRoot(mFocused);
        }

        // With nothing holding the keyboard, the pane in front that holds anything to take it is the one moved in.
        const ConstSpan  Order = mTree.GetOrder();
        Sequence<Handle> Found;

        for (UInt Index = Order.GetSize(); Index-- > 0;)
        {
            ConstRef<Pane> Target = mTree.GetPane(Order[Index]);
            const Handle   Root   = mTree.GetHandle(Target.Slots[0]);

            // Tooltips, ghosts and the backdrop never take the keyboard.
            if (Target.Band.Has(Layer::Trait::Overlay) || Root == mDispatcher.GetBackdrop() || Target.IsHidden(0))
            {
                continue;
            }

            Gather(Root, Found);

            if (!Found.IsEmpty())
            {
                return Root;
            }
        }
        return Handle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Focus::Gather(Handle Root, Ref<Sequence<Handle>> Output) const
    {
        if (!mTree.IsAlive(Root) || mTree.IsPending(Root))
        {
            return;
        }

        // The pane is found by its root, since a handle names a slot and not the pane holding it.
        ConstPtr<Pane> Found = nullptr;

        for (const UInt16 Index : mTree.GetOrder())
        {
            if (mTree.GetHandle(mTree.GetPane(Index).Slots[0]) == Root)
            {
                Found = AddressOf(mTree.GetPane(Index));
            }
        }

        if (!Found)
        {
            return;
        }

        // Preorder is the order the layout places them in, every flow running left to right and top to bottom.
        ConstRef<Pane>   Target = * Found;
        Sequence<UInt32> Rows;

        for (UInt32 Row = 1; Row < Target.GetSize(); ++Row)
        {
            if (HasAnyBit(Target.States[Row], State::Hidden | State::Disabled))
            {
                Row += Target.Spans[Row] - 1;
                continue;
            }

            if (const ConstPtr<Control> Gate = mTree.Peek<Control>(Target.Slots[Row]); Gate && Gate->Focusable)
            {
                Rows.Append(Row);
            }
        }

        for (UInt Index = 0; Index < Rows.GetSize(); ++Index)
        {
            const UInt32 Row      = Rows[Index];
            const Bool   Shelters = Index + 1 < Rows.GetSize() && Rows[Index + 1] < Row + Target.Spans[Row];

            if (const Handle Element = mTree.GetHandle(Target.Slots[Row]); !Shelters && mDispatcher.IsUsable(Element))
            {
                Output.Append(Element);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::Cycle(Bool Forward)
    {
        Sequence<Handle> Found;
        Gather(Arena(), Found);

        if (Found.IsEmpty())
        {
            return false;
        }

        const SInt Count    = static_cast<SInt>(Found.GetSize());
        const SInt Position = Found.Find(mFocused);
        const SInt Step     = Forward ? 1 : -1;

        // A keyboard standing outside the pane enters it at its first control, or its last going backward.
        const SInt Entry    = Forward ? 0 : Count - 1;
        const UInt Next     = static_cast<UInt>(Position < 0 ? Entry : Wrap(Position + Step, Count));

        Set(Found[Next]);
        Reveal(Found[Next]);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::Travel(Heading Way)
    {
        if (!mDispatcher.IsUsable(mFocused) || mTree.IsPending(mFocused))
        {
            return Cycle(true);
        }

        const Rect       From = mTree.GetFrame(mFocused);
        Sequence<Handle> Found;
        Gather(Arena(), Found);

        Handle Best = Toward(Found, From, Way);

        // With nothing that way in its pane, the move may cross into a window or the hud, never out of a modal.
        if (!Best.IsValid() && !mDispatcher.GetModal().IsValid())
        {
            const Handle Home = mTree.GetRoot(mFocused);
            Found.Clear();

            for (const UInt16 Index : mTree.GetOrder())
            {
                ConstRef<Pane> Target = mTree.GetPane(Index);
                const Handle   Root   = mTree.GetHandle(Target.Slots[0]);
                const Bool     Open   = Target.Band.Has(Layer::Trait::Cycled);

                if (Open && Root != Home && !Target.IsHidden(0))
                {
                    Gather(Root, Found);
                }
            }
            Best = Toward(Found, From, Way);
        }

        if (!Best.IsValid())
        {
            return false;
        }

        Set(Best);
        Reveal(Best);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Handle Focus::Toward(ConstRef<Sequence<Handle>> Candidates, Rect From, Heading Way) const
    {
        Handle Best;
        Real32 Score = 0.0f;

        for (const Handle Candidate : Candidates)
        {
            if (Candidate == mFocused)
            {
                continue;
            }

            Real32     Along  = 0.0f;
            Real32     Across = 0.0f;
            const Bool Ahead  = Gauge(From, mTree.GetFrame(Candidate), Way, Along, Across);

            // Only what stands past the edge counts; one in line beats any off it, where straying weighs double.
            const Real32 Weight = Along + Across * 2.0f + (Across > 0.0f ? kBeam : 0.0f);

            if (Ahead && (!Best.IsValid() || Weight < Score))
            {
                Best  = Candidate;
                Score = Weight;
            }
        }
        return Best;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::Trigger(Handle Element)
    {
        if (!mDispatcher.IsUsable(Element) || !mTree.Has<Control>(Element))
        {
            return false;
        }

        // The pointer's press, release and click over the control's middle, so every widget answers as to a click.
        Event Message(Signal::kPress, Element);
        Message.Position = mTree.GetFrame(Element).GetCenter();

        mTree.SetState(Element, State::Press, true);
        mDispatcher.Sound(Element, Cue::Press);
        mDispatcher.Dispatch(Message);

        if (!mTree.IsAlive(Element))
        {
            return true;
        }

        mTree.SetState(Element, State::Press, false);
        Message.Kind = Signal::kRelease;
        mDispatcher.Dispatch(Message);

        if (mTree.IsAlive(Element))
        {
            mDispatcher.Sound(Element, Cue::Click);
            Message.Kind = Signal::kClick;
            mDispatcher.Dispatch(Message);
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::Retreat()
    {
        const ConstSpan Order = mTree.GetOrder();

        for (UInt Index = Order.GetSize(); Index-- > 0;)
        {
            ConstRef<Pane> Target = mTree.GetPane(Order[Index]);
            const Handle   Root   = mTree.GetHandle(Target.Slots[0]);

            // Tooltips and ghosts stand over everything, but escape never closes them.
            if (Target.Band.Has(Layer::Trait::Overlay) || Root == mDispatcher.GetBackdrop() || Target.IsHidden(0))
            {
                continue;
            }

            // Only a popup, or a modal pane no popup stands over, is closed; a window in front is left alone.
            const Bool Popped = Target.Band.Has(Layer::Trait::Popup) || mDispatcher.IsOwned(Root) ||
                               mPointer.IsSummoned(Root);

            if (!Popped && !Target.Band.Has(Layer::Trait::Modal))
            {
                return false;
            }

            const ConstPtr<Control> Found  = mTree.Get<Control>(Root);
            const Handle            Opener = Found ? Found->Owner : Handle();
            const Bool              Living = mTree.IsAlive(mFocused) && !mTree.IsPending(mFocused);
            const Bool              Inside = Living && mDispatcher.IsWithin(mFocused, Root);

            mTree.SetState(Root, State::Hidden, true);

            // The keyboard held in what closed goes back to what opened it.
            if (Inside)
            {
                Set(mDispatcher.IsUsable(Opener) ? Opener : Handle());
            }
            return true;
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::Page(ZyInput::Key Key)
    {
        for (Handle Current = mFocused; Current.IsValid(); Current = mTree.GetParent(Current))
        {
            const ConstPtr<Scroll> Found = mTree.Get<Scroll>(Current);

            if (!Found)
            {
                continue;
            }

            const Real32 View  = Found->View.GetY();
            const Real32 Limit = Max(Found->Overflow - View, 0.0f);
            Real32       Offset;

            switch (Key)
            {
            case ZyInput::Key::PageUp:
                Offset = Found->Offset - View;
                break;
            case ZyInput::Key::PageDown:
                Offset = Found->Offset + View;
                break;
            case ZyInput::Key::Home:
                Offset = 0.0f;
                break;
            default:
                Offset = Limit;
                break;
            }

            // Edited only when it moves, since an edit lays the pane out again; the end keeps following a log.
            if (Offset = Clamp(Offset, 0.0f, Limit); Offset != Found->Offset)
            {
                const Ptr<Scroll> Moved = mTree.Edit<Scroll>(Current);
                Moved->Offset = Offset;
                Moved->Follow = Moved->Follow && Key == ZyInput::Key::End;
            }
            return true;
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ZyInput::Key Focus::KeyOf(Heading Way)
    {
        switch (Way)
        {
        case Heading::Up:
            return ZyInput::Key::Up;
        case Heading::Down:
            return ZyInput::Key::Down;
        case Heading::Left:
            return ZyInput::Key::Left;
        default:
            return ZyInput::Key::Right;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Focus::Gauge(Rect From, Rect To, Heading Way, Ref<Real32> Along, Ref<Real32> Across)
    {
        const Vector2 Gap      = To.GetCenter() - From.GetCenter();
        const Vector2 Middle   = To.GetCenter();
        const Bool    Vertical = (Way == Heading::Up || Way == Heading::Down);

        // Boxes overlapping across the way moved, as buttons over a wide field, stand in line whatever their sizes.
        const Bool Overlap = Vertical
            ? To.GetMinimumX() < From.GetMaximumX() && To.GetMaximumX() > From.GetMinimumX()
            : To.GetMinimumY() < From.GetMaximumY() && To.GetMaximumY() > From.GetMinimumY();

        Across = Overlap ? 0.0f : Abs(Vertical ? Gap.GetX() : Gap.GetY());

        switch (Way)
        {
        case Heading::Up:
            Along = Max(From.GetMinimumY() - To.GetMaximumY(), 0.0f);
            return Middle.GetY() < From.GetMinimumY() + 0.5f;
        case Heading::Down:
            Along = Max(To.GetMinimumY() - From.GetMaximumY(), 0.0f);
            return Middle.GetY() > From.GetMaximumY() - 0.5f;
        case Heading::Left:
            Along = Max(From.GetMinimumX() - To.GetMaximumX(), 0.0f);
            return Middle.GetX() < From.GetMinimumX() + 0.5f;
        default:
            Along = Max(To.GetMinimumX() - From.GetMaximumX(), 0.0f);
            return Middle.GetX() > From.GetMaximumX() - 0.5f;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Focus::Nudge(Real32 Near, Real32 Far, Real32 ViewNear, Real32 ViewFar)
    {
        // Past the far edge it is brought up to it, and its near edge wins when it stands longer than the view.
        const Real32 Past = Max(Far - ViewFar, 0.0f);
        return (Near - Past < ViewNear) ? Near - ViewNear : Past;
    }
}