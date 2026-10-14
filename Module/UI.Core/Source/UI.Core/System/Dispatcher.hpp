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

#include "UI.Core/Event.hpp"
#include "UI.Core/Handle.hpp"
#include "UI.Core/Shortcut.hpp"
#include "UI.Core/Style/Style.hpp"
#include "UI.Core/Style/Theme.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    class Interface;

    /// \brief Defines the code the game runs to play a sound a style names, so the library never depends on audio.
    using SoundHook = Delegate<void(Text Address, Cue Moment, Handle Element)>;

    /// \brief Represents the router of an interface's events, up from their target through every control holding it.
    class Dispatcher final
    {
    public:

        /// \brief Constructs a dispatcher binding no action and playing no sound.
        ///
        /// \param Owner The interface whose events it sends, which handlers are handed, and which must outlive it.
        Dispatcher(Ref<Interface> Owner);

        /// \brief Sets the theme whose styles name the sounds played.
        ///
        /// \param Source The theme, or `nullptr` to play none.
        void SetTheme(ConstPtr<Theme> Source);

        /// \brief Sends an event to its target, and up through every control holding it until one stops it.
        ///
        /// \param Message The event sent, naming the element it starts from.
        /// \return `true` if a handler stopped it, `false` otherwise.
        Bool Dispatch(ConstRef<Event> Message);

        /// \brief Adds code that hears every event before any handler, as the binder storing a changed value does.
        ///
        /// \param Code The code, handed the event and its target as the control; what it returns is not heard.
        void AddListener(Handler Code);

        /// \brief Runs code on the events of every control naming an action, after the control's observers.
        ///
        /// \param Action   The name of the action, which replaces whatever code it ran before.
        /// \param Callback The code, returning `true` to stop the event bubbling further.
        void Bind(Text Action, Handler Callback);

        /// \brief Stops running code on the events of the controls naming an action.
        ///
        /// \param Action The name of the action whose code is dropped.
        void Unbind(Text Action);

        /// \brief Runs a shortcut's code, or else the code bound to its action, as its key goes down.
        ///
        /// \param Entry   The shortcut whose code or action runs.
        /// \param Message The key going down, targeting what the shortcut acts on.
        /// \return `true` if the code took the key, `false` when it declined it or no code is bound.
        Bool Perform(ConstRef<Shortcut> Entry, ConstRef<Event> Message);

        /// \brief Hooks the code that plays the sounds styles name for controls and panes.
        ///
        /// \param Callback The code, or empty to play nothing.
        void SetSoundHook(SoundHook Callback);

        /// \brief Plays the sound an element's style names for a moment, through the game's hook.
        ///
        /// \param Element The element whose style names the sound.
        /// \param Moment  The moment the sound is played for.
        void Sound(Handle Element, Cue Moment);

        /// \brief Gets the root of the modal pane in front, which holds the pointer and the keyboard.
        ///
        /// \return The root, or one naming nothing while no modal pane is shown.
        Handle GetModal() const;

        /// \brief Gets the root of the pane that dims everything under the modal pane in front.
        ///
        /// \return The root, or one naming nothing before the first modal pane was shown.
        ZY_INLINE Handle GetBackdrop() const
        {
            return mBackdrop;
        }

        /// \brief Shows the backdrop under the modal pane in front, or hides it when none is shown.
        void Guard();

        /// \brief Checks whether the root of a pane is a popup, opened by a control it bubbles into.
        ///
        /// \param Root The root of the pane checked.
        /// \return `true` if it is, `false` otherwise.
        Bool IsOwned(Handle Root) const;

        /// \brief Checks whether an element stands beneath another, or is it.
        ///
        /// \param Element  The element whose parents are climbed.
        /// \param Ancestor The element looked for among them.
        /// \return `true` if it does, `false` otherwise.
        Bool IsWithin(Handle Element, Handle Ancestor) const;

        /// \brief Checks whether an element stands beneath another, or is it, climbing from a popup into its opener.
        ///
        /// \param Element  The element whose parents and openers are climbed.
        /// \param Ancestor The element looked for among them.
        /// \return `true` if it does, `false` otherwise.
        Bool IsOpenedFrom(Handle Element, Handle Ancestor) const;

        /// \brief Checks whether a control stands in the tree with nothing holding it, hidden or disabled.
        ///
        /// \param Element The control checked.
        /// \return `true` if it can, `false` otherwise.
        Bool IsUsable(Handle Element) const;

        /// \brief Checks whether an element is alive with neither it nor anything holding it hidden.
        ///
        /// \param Element The element checked.
        /// \return `true` if it is, `false` otherwise.
        Bool IsShown(Handle Element) const;

    private:

        /// \brief Gets the element one step up from another, its parent or, off the root of a popup, what opened it.
        ///
        /// \param Element The element climbed from.
        /// \return The element above it, which may be dead when a popup outlived its opener, or one naming nothing.
        Handle Climb(Handle Element) const;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Interface>         mOwner;
        ConstPtr<Theme>        mTheme;
        Table<Digest, Handler> mActions;
        SoundHook              mSoundHook;
        Handle                 mBackdrop;
        Handle                 mModal;
        Sequence<Handler>      mListeners;
    };
}