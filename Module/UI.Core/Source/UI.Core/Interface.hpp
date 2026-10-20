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

#include "Element.hpp"
#include "Event.hpp"
#include "Handle.hpp"
#include "Paint/Canvas.hpp"
#include "Shortcut.hpp"
#include "Style/Theme.hpp"
#include "System/Animator.hpp"
#include "System/Dispatcher.hpp"
#include "System/Dresser.hpp"
#include "System/Focus.hpp"
#include "System/Layout.hpp"
#include "System/Painter.hpp"
#include "System/Pointer.hpp"
#include "Text/Face.hpp"
#include "Tree.hpp"
#include "Types.hpp"
#include <Zyphryon.Input/Service.hpp>
#include <Zyphryon.Platform/Window.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the code an action runs on a control's own click or change, handed that control.
    using Reaction = Delegate<void(Element Control)>;

    /// \brief Represents one user interface, its elements laid out over a target and drawn through a canvas.
    class Interface : public Tree
    {
    public:

        /// \brief Specifies when in an update a stage runs.
        enum class Phase : UInt8
        {
            Prepare,  ///< Before anything is dressed or laid out, once the game's changes reached the tree.
            Settle,   ///< After every pane is laid out, before the next draw.
        };

        /// \brief Defines the code a module runs at one phase of every update, handed the interface and the delta.
        using Stage = Delegate<void(Ref<Interface> Owner, Real32 Delta)>;

    public:

        /// \brief Constructs an interface holding no pane, drawn one pixel to the unit.
        Interface();

        /// \brief Shows changed bound values and texts, realizes, animates, dresses and lays out every changed pane.
        ///
        /// \param Size  The size of the target, in pixels, which holds its size over the pixel ratio in units.
        /// \param Delta The time since the last update, in seconds, where nought moves nothing.
        void Update(Vector2 Size, Real32 Delta = 0.0f);

        /// \brief Adds code that runs at a phase of every update, after the stages of that phase of a lower order.
        ///
        /// \param When  The phase the stage runs at.
        /// \param Order The order among the stages of the phase, ties running in the order they were added.
        /// \param Code  The code run at every update.
        void AddStage(Phase When, UInt32 Order, Stage Code);

        /// \brief Gets the pass drawing the panes, to add stages of drawing to it.
        ///
        /// \return The painter, which draws every pane through the canvas \ref Draw is handed.
        ZY_INLINE Ref<Painter> GetPainter()
        {
            return mPainter;
        }

        /// \brief Gets the pass laying the panes out, to add measurers to it.
        ///
        /// \return The layout, which sizes and places the elements of every changed pane.
        ZY_INLINE Ref<Layout> GetLayout()
        {
            return mLayout;
        }

        /// \brief Gets the pass moving the elements' looks, to add appliers to it.
        ///
        /// \return The animator, which eases every element's look toward what its skins give.
        ZY_INLINE Ref<Animator> GetAnimator()
        {
            return mAnimator;
        }

        /// \brief Gets the pointer, to add hooks to it and read what it rests on.
        ///
        /// \return The pointer, which hovers, presses, drags and drops over the panes.
        ZY_INLINE Ref<Pointer> GetCursor()
        {
            return mPointer;
        }

        /// \brief Gets what routes events, to ask whether an element can be used or stands within another.
        ///
        /// \return The dispatcher, which bubbles every event up from the control it targets.
        ZY_INLINE Ref<Dispatcher> GetDispatcher()
        {
            return mDispatcher;
        }

        /// \brief Draws every pane, back to front, at the pixel ratio the last update laid them out at.
        ///
        /// \param Canvas The canvas to draw through, between its \ref Canvas::Begin and \ref Canvas::End.
        void Draw(Ref<Canvas> Canvas);

        /// \brief Lays every pane out whole at the next update, as a new target size does.
        void Relayout();

        /// \brief Sets how much larger the interface is drawn than the display's scale asks, as a player's choice.
        ///
        /// \param Factor The scale, where one draws at the display's own.
        void SetScale(Real32 Factor);

        /// \brief Gets how much larger the interface is drawn than the display's scale asks.
        ///
        /// \return The scale, where one draws at the display's own.
        ZY_INLINE Real32 GetScale() const
        {
            return mScale;
        }

        /// \brief Sets the scale the display asks for, as its DPI gives it, which the game's own scale multiplies.
        ///
        /// \param Factor The scale, where one is a pixel to the unit, as at 96 DPI.
        void SetDensity(Real32 Factor);

        /// \brief Gets the scale the display asks for.
        ///
        /// \return The scale, where one is a pixel to the unit.
        ZY_INLINE Real32 GetDensity() const
        {
            return mDensity;
        }

        /// \brief Gets how many pixels of the target one unit of the interface covers.
        ///
        /// \return The ratio, which is the display's scale times the game's.
        ZY_INLINE Real32 GetPixelRatio() const
        {
            return mDensity * mScale;
        }

        /// \brief Dresses every element wearing a style from a theme, which must outlive the interface's use of it.
        ///
        /// \param Source The theme, or `nullptr` to dress nothing.
        void SetTheme(ConstPtr<Theme> Source);

        /// \brief Gets the theme every element wearing a style is dressed from.
        ///
        /// \return The theme, or `nullptr` for none.
        ZY_INLINE ConstPtr<Theme> GetTheme() const
        {
            return mDresser.GetTheme();
        }

        /// \brief Puts text on the clipboard of the window attached, or on the interface's own with none.
        ///
        /// \param Content The text to put on the clipboard.
        void Copy(Text Content);

        /// \brief Gets the text on the clipboard of the window attached, or on the interface's own with none.
        ///
        /// \return The text, without carriage returns, valid until the clipboard is next written or read.
        Text Paste();

        /// \brief Gets the tables text set in a typeface is measured with, as a text field places its caret by them.
        ///
        /// \param Font The typeface, which must have finished loading.
        /// \return The tables, valid until the next update.
        ZY_INLINE ConstRef<Face> GetFace(ConstRef<Retainer<ZyRender::Font>> Font)
        {
            return mLayout.FaceOf(Font);
        }

        /// \brief Gets the modifier keys held, as the input service attached reads them.
        ///
        /// \return The modifier keys held, or none with no input service attached.
        ZY_INLINE ZyInput::Modifier GetModifiers() const
        {
            return mInput ? mInput->GetModifiers() : ZyInput::Modifier::None;
        }

        /// \brief Checks whether a key is held down, as the input service attached reads it.
        ///
        /// \param Key The key asked about.
        /// \return `true` while it is held, `false` otherwise or with no input service attached.
        ZY_INLINE Bool IsKeyHeld(ZyInput::Key Key) const
        {
            return mInput && mInput->IsKeyHeld(Key);
        }

        /// \brief Checks whether a button of the pointer is held down, whatever took its press.
        ///
        /// \param Button The button asked about.
        /// \return `true` while it is held, `false` otherwise.
        ZY_INLINE Bool IsButtonHeld(ZyInput::Button Button) const
        {
            return mPointer.IsHeld(Button);
        }

        /// \brief Checks whether a text field holds the keyboard, as a control taking typed text does.
        ///
        /// \return `true` if one does, `false` otherwise.
        ZY_INLINE Bool IsTyping() const
        {
            return mFocus.IsTyping();
        }

        /// \brief Adds a shortcut, run before the control holding the keyboard hears its key.
        ///
        /// \param Entry The shortcut.
        /// \param Owner The element keeping it, or one naming nothing to keep it for the interface.
        void AddShortcut(ConstRef<Shortcut> Entry, Handle Owner = Handle());

        /// \brief Removes every shortcut of a chord the interface or an element keeps.
        ///
        /// \param Key       The key of the chord.
        /// \param Modifiers The modifier keys of the chord.
        /// \param Owner     The element keeping them, or one naming nothing for the interface's own.
        void RemoveShortcut(ZyInput::Key Key, ZyInput::Modifier Modifiers, Handle Owner = Handle());

        /// \brief Hooks the interface to the input service, hearing the pointer and keyboard first.
        ///
        /// \param Input   The input service whose pointer, keyboard and window events the interface hears.
        /// \param Surface The window text fields copy to and paste from, or `nullptr` for its own clipboard.
        void Attach(Ref<ZyInput::Service> Input, Ptr<ZyPlatform::Window> Surface = nullptr);

        /// \brief Unhooks the interface from the input service and the window.
        ///
        /// \param Input The input service it was hooked to.
        void Detach(Ref<ZyInput::Service> Input);

        /// \brief Sends an event to its target, and up through every control holding it until one stops it.
        ///
        /// \param Message The event to send, naming the control it targets.
        /// \return `true` if a handler stopped it, `false` otherwise.
        Bool Dispatch(ConstRef<Event> Message);

        /// \brief Runs code on the events of every control naming an action, after the control's observers.
        ///
        /// \param Action   The name of the action, which replaces whatever code it ran before.
        /// \param Callback The code, returning `true` to stop the event bubbling further.
        void Bind(Text Action, Handler Callback);

        /// \brief Stops running code on the events of the controls naming an action.
        ///
        /// \param Action The name of the action.
        void Unbind(Text Action);

        /// \brief Stops running code on the events of the controls naming any action.
        void UnbindAll();

        /// \brief Runs code when a control naming an action is clicked.
        ///
        /// \param Action The name of the action, which replaces whatever code it ran before.
        /// \param Code   The code, handed the control clicked; the click bubbles on, so a menu holding it closes.
        void OnClick(Text Action, Reaction Code);

        /// \brief Runs code when a control naming an action reports a change of its own value, hearing no other event.
        ///
        /// \param Action The name of the action, which replaces whatever code it ran before.
        /// \param Code   The code, handed the control changed; the change bubbles on.
        void OnChange(Text Action, Reaction Code);

        /// \brief Hooks the code that plays the sounds styles name, as a control is hovered, pressed or a pane shown.
        ///
        /// \param Callback The code, or empty to play nothing.
        void SetSoundHook(SoundHook Callback);

        /// \brief Moves the keyboard to a control, or takes it away.
        ///
        /// \param Element The control, or one naming nothing.
        void Focus(Handle Element);

        /// \brief Moves the keyboard to the next control in layout order, wrapping around, as the tab key does.
        ///
        /// \return `true` if a control took the keyboard, `false` otherwise.
        Bool NextFocus();

        /// \brief Moves the keyboard to the control before the one holding it, wrapping around, as shift and tab do.
        ///
        /// \return `true` if a control took the keyboard, `false` otherwise.
        Bool PreviousFocus();

        /// \brief Moves the keyboard toward the nearest control on screen one way, as an arrow key or a pad does.
        ///
        /// \param Way The way to move.
        /// \return `true` if a control took the move or the keyboard, `false` otherwise.
        Bool Navigate(Heading Way);

        /// \brief Uses the control holding the keyboard as a click would, as enter, space or a pad's confirm do.
        ///
        /// \return `true` if a control holds the keyboard, `false` otherwise.
        Bool Activate();

        /// \brief Backs out as escape does, letting go of a drag or closing the popup or modal pane in front.
        ///
        /// \return `true` if anything heard it or closed, `false` otherwise.
        Bool Cancel();

        /// \brief Scrolls every scrolling element holding an element so the element stands in its view.
        ///
        /// \param Element The element to bring into view.
        void Reveal(Handle Element);

        /// \brief Gets the element the pointer rests on.
        ///
        /// \return The element, or one naming nothing when the pointer passes through to the world.
        ZY_INLINE Handle GetOver() const
        {
            return mPointer.GetOver();
        }

        /// \brief Gets the control holding the keyboard.
        ///
        /// \return The control, or one naming nothing.
        ZY_INLINE Handle GetFocused() const
        {
            return mFocus.GetFocused();
        }

        /// \brief Gets the size of the target the panes were last laid out over.
        ///
        /// \return The size, in units of the interface, which is the target's pixels over the pixel ratio.
        ZY_INLINE Vector2 GetSize() const
        {
            return mLayout.GetSize();
        }

        /// \brief Gets where the pointer is.
        ///
        /// \return The point, in units of the interface.
        ZY_INLINE Vector2 GetPointer() const
        {
            return mPointer.GetPosition();
        }

        /// \brief Gets where the pointer is from the top left corner of an element, as a view maps it to what it shows.
        ///
        /// \param Element The element measured from, as last laid out.
        /// \return The point, in units of the interface.
        Vector2 GetPointer(Handle Element) const;

        /// \brief Opens a popup at a point, over every pane of its band and kept on screen, telling it its subject.
        ///
        /// \param Popup   The root of the popup.
        /// \param At      The point its top left corner opens at, in pixels of the target.
        /// \param Subject The element it was opened over, or one naming nothing.
        void Summon(Handle Popup, Vector2 At, Handle Subject = Handle());

        /// \brief Keeps the root of a pane on screen, moving it back inside the target whenever it overruns an edge.
        ///
        /// \param Root The root, which must be placed by its distance from the target's top left corner.
        /// \param Gap  The gap it keeps from the pointer, standing above it instead of off the bottom, or nought.
        void Confine(Handle Root, Real32 Gap = 0.0f);

        /// \brief Gets the control the left button went down on and has not come up from yet.
        ///
        /// \return The control, or one naming nothing while the button is up or the press became a drag.
        ZY_INLINE Handle GetPressed() const
        {
            return mPointer.GetPressed();
        }

        /// \brief Gets the control a button went down on and has not come up from yet.
        ///
        /// \param Button The button, where any but the left one holds only a control taking every button.
        /// \return The control, or one naming nothing while the button is up or holds nothing.
        ZY_INLINE Handle GetPressed(ZyInput::Button Button) const
        {
            return mPointer.GetPressed(Button);
        }

        /// \brief Gets the root of the modal pane in front, which holds the pointer and the keyboard.
        ///
        /// \return The root, or one naming nothing while no modal pane is shown.
        Handle GetModal() const;

        /// \brief Moves the pointer.
        ///
        /// \param X      The pointer's horizontal position, in pixels.
        /// \param Y      The pointer's vertical position, in pixels.
        /// \param DeltaX The horizontal movement since the last event, in pixels.
        /// \param DeltaY The vertical movement since the last event, in pixels.
        /// \return `true` if the pointer rests on the interface, `false` otherwise.
        Bool OnMouseMove(Real32 X, Real32 Y, Real32 DeltaX, Real32 DeltaY);

        /// \brief Presses a pointer button, where the left one may start a drag and the right one opens a context menu.
        ///
        /// \param Button The button pressed.
        /// \return `true` if the press lands on the interface, `false` otherwise.
        Bool OnMouseDown(ZyInput::Button Button);

        /// \brief Releases a pointer button, clicking the control it went down on or dropping what the pointer carries.
        ///
        /// \param Button The button released.
        /// \return `true` if the press had landed on the interface, `false` otherwise.
        Bool OnMouseUp(ZyInput::Button Button);

        /// \brief Turns the wheel, scrolling the innermost scrolling element under the pointer, sideways with shift.
        ///
        /// \param DeltaX The horizontal turn, in notches.
        /// \param DeltaY The vertical turn, in notches.
        /// \return `true` if the pointer rests on the interface, `false` otherwise.
        Bool OnMouseScroll(Real32 DeltaX, Real32 DeltaY);

        /// \brief Presses a key, which the shortcuts hear first, then the control holding the keyboard.
        ///
        /// \param Key The key pressed.
        /// \return `true` if the key was taken or moved the keyboard, `false` otherwise.
        Bool OnKeyDown(ZyInput::Key Key);

        /// \brief Repeats a key held down, which the control holding the keyboard hears as another press.
        ///
        /// \param Key The key held down.
        /// \return `true` if the key was taken or moved the keyboard, `false` otherwise.
        Bool OnKeyRepeat(ZyInput::Key Key);

        /// \brief Types text, which the control holding the keyboard hears, unless a modal pane stands over it.
        ///
        /// \param Content The text typed.
        /// \return `true` if a control or a modal pane holds the keyboard, `false` otherwise.
        Bool OnKeyType(Text Content);

        /// \brief Takes the display's scale the window now stands at, as when it moves to a monitor of another DPI.
        ///
        /// \param Scale The scale, where one is a pixel to the unit.
        /// \return `false` always, so the game hears it too.
        Bool OnWindowScale(Real32 Scale);

        /// \brief Takes the window gaining or losing the keyboard.
        ///
        /// \param Focused The flag, `true` when the window gained the keyboard, `false` when it lost it.
        /// \return `false` always, so the game hears it too.
        Bool OnWindowFocus(Bool Focused);

    private:

        /// The smallest the display's scale or the game's may be, so a slip never divides the target by nought.
        static constexpr Real32 kMinimumScale = 0.1f;

        /// \brief Holds one stage and when it runs.
        struct Entry final
        {
            /// The phase it runs at.
            Phase  When  = Phase::Prepare;

            /// Its order among the stages of the phase.
            UInt32 Order = 0;

            /// The code run at that phase.
            Stage  Code;
        };

        /// \brief Lets every element that makes what it holds itself make it for the room the last layout gave it.
        ///
        /// \return `true` if any of them changed the tree, `false` otherwise.
        Bool Realize();

        /// \brief Runs every stage of a phase, by order, then takes in the stages added while they ran.
        ///
        /// \param When  The phase whose stages run.
        /// \param Delta The time since the last update, in seconds.
        void Run(Phase When, Real32 Delta);

        /// \brief Lays out again every pane something changed in since it was last laid out.
        void Reflow();

        /// \brief Marks every element of every pane to be dressed again, as a new theme asks.
        void Restyle();

    private:

        /// \brief Runs code on one kind of event a control naming an action reports of itself.
        ///
        /// \param Action The name of the action.
        /// \param Kind   The kind of event heard.
        /// \param Code   The code, handed the control.
        void React(Text Action, UInt64 Kind, Reaction Code);

        /// \brief Destroys every popup whose opener was destroyed, as the first stage before dressing.
        ///
        /// \param Owner The interface the stage runs for.
        static void OnPrune(Ref<Interface> Owner, Real32);

        /// \brief Makes the rows of every list making only the rows in view, as a stage before dressing.
        ///
        /// \param Owner The interface the stage runs for.
        static void OnRealize(Ref<Interface> Owner, Real32);

        /// \brief Moves the pointer's clock on, as the last of the interface's own stages before dressing.
        ///
        /// \param Owner The interface the stage runs for.
        /// \param Delta The time since the last update, in seconds.
        static void OnTick(Ref<Interface> Owner, Real32 Delta);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Real32                  mDensity;
        Real32                  mScale;
        Real32                  mRatio;
        Animator                mAnimator;
        Dresser                 mDresser;
        Layout                  mLayout;
        Painter                 mPainter;
        Dispatcher              mDispatcher;
        Pointer                 mPointer;
        UI::Focus               mFocus;
        Ptr<ZyInput::Service>   mInput;
        Ptr<ZyPlatform::Window> mSurface;
        Str                     mClipboard;
        Sequence<Entry>         mStages;
        Sequence<Entry>         mWaiting;
        Sequence<Handle>        mRealizing;
        Bool                    mStaging;
    };
}