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

#include "ZyPlugin_UI_Example.Embedded.hpp"
#include <UI.Control/Module.hpp>
#include <UI.Control/Widget/Common.hpp>
#include <UI.Control/Widget/Dialog.hpp>
#include <UI.Control/Widget/Repeater.hpp>
#include <UI.Control/Widget/Label.hpp>
#include <UI.Control/Widget/Listbox.hpp>
#include <UI.Control/Widget/Menu.hpp>
#include <UI.Control/Widget/Spinner.hpp>
#include <UI.Control/Widget/Scroller.hpp>
#include <UI.Control/Widget/Window.hpp>
#include <UI.Core/Element.hpp>
#include <UI.Core/Facet/Visual/Bar.hpp>
#include <UI.Core/Facet/Visual/Caption.hpp>
#include <UI.Core/Facet/Visual/Look.hpp>
#include <UI.Core/Facet/Visual/Picture.hpp>
#include <UI.Core/Facet/Visual/Scroll.hpp>
#include <UI.Core/Interface.hpp>
#include <UI.Data/Lexicon.hpp>
#include <UI.Gesture/Drag.hpp>
#include <UI.Gesture/Tooltip.hpp>
#include <UI.Markup/Markup.hpp>
#include <Zyphryon.Audio/Service.hpp>
#include <Zyphryon.Content/Mount/Disk.hpp>
#include <Zyphryon.Platform/Service.hpp>
#include <Zyphryon.Runtime/Kernel.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents an application driving a game interface, built from code, theme and layout files.
    class Example final : public ZyRuntime::Kernel
    {
    public:

        /// \brief Constructs the application, its interface laid out at the first frame.
        Example()
            : mTime { 0.0f }
        {
        }

    protected:

        /// \see Kernel::OnConfigure(Ref<ZyRuntime::Startup>)
        void OnConfigure(Ref<ZyRuntime::Startup> Startup) override
        {
            Startup.SetWindowTitle("Zyphryon UI");
            Startup.SetWindowWidth(1280);
            Startup.SetWindowHeight(720);
            // A plain target blends as the colours are written, with no conversion on the way.
            Startup.SetGraphicsColorFormat(ZyGraphic::TextureFormat::RGBA8UIntNorm);
        }

        /// \see Kernel::OnInitialize()
        Bool OnInitialize() override
        {
            ConstRetainer<ZyContent::Service> Content = GetService<ZyContent::Service>();
            ZyRegisterEmbedded(* Content);
            Content->AddMount("Resources", Retainer<ZyContent::Disk>::Create(ZY_UI_EXAMPLE_FOLDER));

            mCanvas.Initialize(* this, Colorspace::Linear);
            mFont = Content->Load<ZyRender::Font>("Resources://Font/MedievalSharp-Regular.fnt");
            mDoor = Content->Load<ZyGraphic::Image>("Resources://Image/Door.tex");

            // Every widget wears the look built into the library, in the example's typeface, until its theme is read.
            mMarkup = Unique<Markup>::Create(AddressOf(* Content));
            ControlModule::Declare(* mMarkup);
            mMarkup->SetFont("Body", mFont);

            if (Str Error; !mMarkup->Dress(mTheme, Error))
            {
                LOG_W("The built-in theme cannot be read: {0}", Error);
            }
            mInterface.SetTheme(AddressOf(mTheme));
            // Text fields copy to and paste from the window's clipboard, and the display's scale follows the window.
            Ref<ZyInput::Service>    Input    = * GetService<ZyInput::Service>();
            Ref<ZyPlatform::Service> Platform = * GetService<ZyPlatform::Service>();
            mInterface.Attach(Input, AddressOf(Platform.GetWindow()));
            Input.OnWindowScale.AddMethod<&Example::OnWindowScale>(this);

            // The library names sounds and nothing more; the game decides how each one plays.
            mInterface.SetSoundHook(SoundHook::Create([this](Text Address, Cue Moment, Handle)
            {
                OnSound(Address, Moment);
            }));

            // Bindings, drag and drop, tooltips and context menus run on the interface once the library is installed.
            ControlModule::Declare(mInterface);

            // The game owns the values the interface shows; the interface only points at them.
            Seed();
            Binder::SetModel(mInterface, AddressOf(mModel));
            Scale(Platform);

            // Text written as a key shows the key itself until its language file arrives, then its text.
            mLexicon.SetLanguage("English");
            Binder::SetLexicon(mInterface, AddressOf(mLexicon));

            BuildCard();
            BuildChat();
            BuildBag();
            BuildControls();
            return true;
        }

        /// \see Kernel::OnTick(Real64)
        void OnTick(Real64 Delta) override
        {
            mTime += static_cast<Real32>(Delta);

            Advance(static_cast<Real32>(Delta));

            Ref<ZyPlatform::Service>     Platform = * GetService<ZyPlatform::Service>();
            ConstRef<ZyPlatform::Window> Window   = Platform.GetWindow();

            const Real32 Width  = static_cast<Real32>(Window.GetWidth());
            const Real32 Height = static_cast<Real32>(Window.GetHeight());

            ConstRetainer<ZyGraphic::Service> Graphics = GetService<ZyGraphic::Service>();

            Graphics->Prepare(ZyGraphic::kDisplay, "Interface", ZyGraphic::Viewport(0.0f, 0.0f, Width, Height),
                Color(0.07f, 0.08f, 0.10f, 1.0f), 1.0f, 0);
            {
                mInterface.Update(Vector2(Width, Height), static_cast<Real32>(Delta));

                mCanvas.Begin(static_cast<UInt16>(Width), static_cast<UInt16>(Height));
                mInterface.Draw(mCanvas);
                mCanvas.End();
            }
            Graphics->Commit();
        }

        /// \see Kernel::OnTerminate()
        void OnTerminate() override
        {
            GetService<ZyInput::Service>()->OnWindowScale.RemoveMethod<&Example::OnWindowScale>(this);
            mInterface.Detach(* GetService<ZyInput::Service>());
            mCanvas.Dispose();
        }

    private:

        /// \brief Opens a window-band pane holding a card at a place on screen.
        ///
        /// \param Left  The distance from the left edge of the screen, in pixels.
        /// \param Width The width of the card, in pixels.
        /// \return The card.
        Element OpenCard(Real32 Left, Real32 Width)
        {
            const Element Card(mInterface, mInterface.Open(Window::kLayer));

            Ref<Box> Shape = Card.EditBox();
            Shape.Placement = Position::Absolute;
            Shape.Left      = Length::Pixels(Left);
            Shape.Top       = Length::Pixels(24.0f);
            Shape.Width     = Length::Pixels(Width);
            Shape.Padding   = Edges(16.0f);
            Shape.Gap       = 10.0f;

            Ref<Look> Shade = Card.Add<Look>();
            Shade.Background  = Color(1.0f, 1.0f, 1.0f, 0.06f);
            Shade.Border      = Color(0.23f, 0.25f, 0.31f, 1.0f);
            Shade.BorderWidth = 1.0f;
            Shade.Radius      = 10.0f;
            return Card;
        }

        /// \brief Adds a line of text under an element.
        ///
        /// \param Parent  The element to hold it.
        /// \param Content The text, or the key of a text of the language written `@Key`.
        /// \param Size    The size the text is set at, in pixels.
        /// \param Tint    The colour of the text.
        /// \return The element showing the text.
        Element AddText(Element Parent, Text Content, Real32 Size, Color Tint)
        {
            const Element Result = Parent.Create();

            Ref<Caption> Words = Result.Add<Caption>();
            Words.Content = Content;
            Words.Font    = mFont;
            Words.Size    = Size;
            Words.Tint    = Tint;

            // A key is bound as a layout binds one, so the line follows the language as the layout's text does.
            if (Binder::IsLocalized(Content))
            {
                Binder::Attach(Result, Content, Binder::PutText<&Label::SetText>());
            }
            return Result;
        }

        /// \brief Adds a bar under an element, with a caption laid over it.
        ///
        /// \param Parent  The element to hold it.
        /// \param Fill    The colour of its filled share.
        /// \param Caption Receives the element showing its caption.
        /// \return The bar.
        Element AddBar(Element Parent, Color Fill, Ref<Element> Caption)
        {
            const Element Gauge = Parent.Create();

            Gauge.EditBox().Height = Length::Pixels(18.0f);

            Ref<Look> Shade = Gauge.Add<Look>();
            Shade.Background = Color(0.11f, 0.13f, 0.16f, 1.0f);
            Shade.Radius     = 7.0f;

            Gauge.Add<Bar>().Fill = Fill;

            // The caption stands over the bar, stretched across it, its text centred.
            Caption = AddText(Gauge, "", 13.0f, Color::White());

            Ref<Box> Shape = Caption.EditBox();
            Shape.Placement = Position::Absolute;
            Shape.Left      = Length::Pixels(0.0f);
            Shape.Top       = Length::Pixels(0.0f);
            Shape.Right     = Length::Pixels(0.0f);
            Shape.Bottom    = Length::Pixels(0.0f);

            Caption.Edit<UI::Caption>()->Alignment = Align::Center;
            return Gauge;
        }

        /// \brief Adds a button under an element.
        ///
        /// \param Parent  The element to hold it.
        /// \param Content The text on the button.
        /// \param Fill    The colour of the button.
        void AddButton(Element Parent, Text Content, Color Fill)
        {
            const Element Press = Parent.Create();

            Press.EditBox().Padding = Edges(12.0f, 6.0f, 12.0f, 6.0f);

            Ref<Look> Shade = Press.Add<Look>();
            Shade.Background = Fill;
            Shade.Radius     = 6.0f;

            AddText(Press, Content, 16.0f, Color::White());
        }

        /// \brief Builds the card holding the title, the pickers, a paragraph, the vitals and the actions.
        void BuildCard()
        {
            const Element Card = OpenCard(24.0f, 360.0f);

            // The heading holds the title, then the pickers the layout makes once it arrives.
            const Element Header = Card.Create();
            Ref<Box>      Line   = Header.EditBox();
            Line.Flow  = Direction::Row;
            Line.Items = Align::Center;
            Line.Gap   = 8.0f;
            mHeader = Header;

            const Element Title = AddText(Header, "", 34.0f, Color(0.94f, 0.85f, 0.56f, 1.0f));
            Title.EditBox().Grow = 1.0f;
            Ref<Caption>  Words = * Title.Edit<Caption>();
            Words.Stroke      = Color(0.23f, 0.16f, 0.06f, 1.0f);
            Words.StrokeWidth = 2.0f;
            Words.Shadow      = Color::Black();
            Words.ShadowOffset = Vector2(2.0f, 2.0f);

            // Bound in code as a layout binds it, so typing a name in the character card renames this one.
            Binder::Attach(Title, "{Player.Name}", Binder::PutText<&Label::SetText>());

            AddText(Card, "@Card.About", 15.0f, Color(0.63f, 0.66f, 0.72f, 1.0f));

            AddText(Card, "@Card.Vitals", 20.0f, Color(0.62f, 0.70f, 0.85f, 1.0f));

            // A caption is formatted again only when a whole number it shows changes; its words come from the language.
            Element       HealthText;
            const Element Health = AddBar(Card, Color(0.84f, 0.27f, 0.27f, 1.0f), HealthText);
            Binder::Attach(Health, "{Player.HealthShare}", Binder::Put<Real32, &Example::SetShare>());
            Binder::Attach(HealthText, "@Card.Health", Binder::PutText<&Label::SetText>());

            Element       ManaText;
            const Element Mana = AddBar(Card, Color(0.27f, 0.54f, 0.84f, 1.0f), ManaText);
            Binder::Attach(Mana, "{Player.ManaShare}", Binder::Put<Real32, &Example::SetShare>());
            Binder::Attach(ManaText, "@Card.Mana", Binder::PutText<&Label::SetText>());

            AddText(Card, "@Card.Actions", 20.0f, Color(0.62f, 0.70f, 0.85f, 1.0f));

            const Element Row = Card.Create();
            Ref<Box>      Flow = Row.EditBox();
            Flow.Flow = Direction::Row;
            Flow.Gap  = 8.0f;

            AddButton(Row, "@Card.Attack", Color(0.18f, 0.42f, 0.87f, 1.0f));
            AddButton(Row, "@Card.Flee", Color(0.75f, 0.22f, 0.17f, 1.0f));
            AddButton(Row, "@Card.Cast", Color(0.27f, 0.29f, 0.33f, 1.0f));
        }

        /// \brief Builds the card holding a scrolling chat log, kept at its latest line, and two frames of a sheet.
        void BuildChat()
        {
            const Element Card = OpenCard(408.0f, 460.0f);

            AddText(Card, "@Chat.Title", 20.0f, Color(0.62f, 0.70f, 0.85f, 1.0f));

            const Element Log = Card.Create("Log");
            mLog = Log;
            Ref<Box>      Shape = Log.EditBox();
            Shape.Height  = Length::Pixels(140.0f);
            Shape.Padding = Edges(8.0f);
            Shape.Gap     = 4.0f;

            Ref<Look> Shade = Log.Add<Look>();
            Shade.Background = Color(0.06f, 0.07f, 0.10f, 1.0f);
            Shade.Radius     = 6.0f;

            Ref<Scroll> Roll = Log.Add<Scroll>();
            Roll.Thumb  = Color(0.94f, 0.85f, 0.56f, 0.8f);
            Roll.Follow = true;

            // More lines than the log shows, so it scrolls, staying on the latest one.
            for (UInt32 Line = 0; Line < 30; ++Line)
            {
                Str Content;

                if (Line % 3 == 0)
                {
                    Content.Format<"Guard: line {0} of the log, which scrolls to stay on the latest one.">(Line + 1);
                }
                else if (Line % 3 == 1)
                {
                    Content.Format<"Gatekeeper: the gate opens at dawn, line {0}.">(Line + 1);
                }
                else
                {
                    Content.Format<"Brom: meet me there, line {0}.">(Line + 1);
                }
                AddText(Log, Content, 14.0f, Color(0.78f, 0.80f, 0.83f, 1.0f));
            }

            AddText(Card, "@Chat.Sprites", 20.0f, Color(0.62f, 0.70f, 0.85f, 1.0f));

            const Element Doors = Card.Create();
            Ref<Box>      Row   = Doors.EditBox();
            Row.Flow  = Direction::Row;
            Row.Gap   = 8.0f;
            Row.Items = Align::Center;

            AddDoor(Doors, Rect(0.0f, 0.0f, 0.5f, 1.0f));
            AddDoor(Doors, Rect(0.5f, 0.0f, 1.0f, 1.0f));
        }

        /// \brief Adds one frame of the door sheet under an element.
        ///
        /// \param Parent The element to hold it.
        /// \param Region The part of the sheet shown, in texture coordinates.
        void AddDoor(Element Parent, Rect Region)
        {
            const Element Image = Parent.Create();

            Ref<Picture> Frame = Image.Add<Picture>();
            Frame.Source = mDoor;
            Frame.Region = Region;
        }

        /// \brief Builds the card holding the inventory as an item list of five columns, making only the slots in view.
        void BuildBag()
        {
            const Element Card = OpenCard(892.0f, 300.0f);

            const Element Heading = AddText(Card, "", 20.0f, Color(0.62f, 0.70f, 0.85f, 1.0f));
            Binder::Attach(Heading, "@Inventory.Title", Binder::PutText<&Label::SetText>());

            // Three lines of slots show; the rest of the records are reached by the wheel, their slots handed on.
            const Element Grid = Repeater::Create(Card, "Slots");
            Grid.EditBox().Height = Length::Pixels(156.0f);
            mGrid = Grid;

            Repeater::SetColumns(Grid, 5);
            Repeater::SetItemHeight(Grid, 48.0f);
            Repeater::SetTemplate(Grid, Maker::Create([this](Element Slot, Text Scope) { AddSlot(Slot, Scope); }));
            Repeater::SetItems(Grid, "Inventory");

            // A slot dropped on another swaps the two records, wherever either list shows them.
            Grid.Observe(Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
            {
                return OnSwap(Message, Self);
            }));
        }

        /// \brief Fills one slot of the grid with a count, a badge when rare, a drag payload and a tooltip.
        ///
        /// \param Slot  The slot, which the grid places and its theme dresses.
        /// \param Scope The path of the slot's record.
        void AddSlot(Element Slot, Text Scope)
        {
            const Element Count = Label::Create(Slot, "", "Slots.Count");
            Binder::Attach(Count, "{Count:0}", Binder::PutText<&Label::SetText>(), Reader(), Scope);

            AddBadge(Slot, Scope);

            // Bound like the rest of the slot, so a slot handed to another record carries and explains that one.
            Drag::SetSource(Slot, "Item");
            Drag::SetTarget(Slot, "Item");
            Binder::Attach(Slot, "{Name}", Writer::Create<&Drag::SetPayload>(), Reader(), Scope);
            Binder::Attach(Slot, "{Name} (x{Count:0})", Binder::PutText<&Tooltip::SetText>(), Reader(), Scope);
        }

        /// \brief Adds a badge standing in the top right corner of a slot, shown while the slot's record is rare.
        ///
        /// \param Slot  The slot.
        /// \param Scope The path of the slot's record.
        void AddBadge(Element Slot, Text Scope)
        {
            const Element Badge = Slot.Create();
            Badge.SetStyle("Slots.Badge");

            Ref<Box> Shape = Badge.EditBox();
            Shape.Placement = Position::Absolute;
            Shape.Top       = Length::Pixels(4.0f);
            Shape.Right     = Length::Pixels(4.0f);

            Binder::Attach(Badge, "{!Rare}", Binder::PutState<State::Hidden>(), Reader(), Scope);
        }

        /// \brief Binds the actions of the controls, then reads the languages, the theme and the layout.
        void BuildControls()
        {
            mInterface.Bind("Attack",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnPressed(Message, Self, "Attack");
                }));
            mInterface.Bind("Defend",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnPressed(Message, Self, "Defend");
                }));
            mInterface.Bind("ShowNames",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnToggled(Message, Self, "Show names");
                }));
            mInterface.Bind("Loot",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnToggled(Message, Self, "Loot automatically");
                }));
            mInterface.Bind("Spell",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnPicked(Message, Self, "Spell");
                }));
            mInterface.Bind("Quality",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnPicked(Message, Self, "Quality");
                }));
            mInterface.Bind("Apply",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnPressed(Message, Self, "Apply");
                }));
            mInterface.Bind("Level",
                Handler::Create([this](Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
                {
                    return OnLevel(Owner, Message, Self);
                }));
            mInterface.Bind("Gate",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnPressed(Message, Self, "Open the gate");
                }));
            mInterface.Bind("Bag",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnBag(Message, Self);
                }));
            mInterface.Bind("Editor",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnEditor(Message, Self);
                }));
            mInterface.Bind("Codex",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnCodex(Message, Self);
                }));
            mInterface.Bind("CodexTop",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnCodexTop(Message, Self);
                }));
            mInterface.Bind("Track",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnTrack(Message, Self);
                }));
            mInterface.Bind("Pick",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnPick(Message, Self);
                }));
            mInterface.Bind("Gain",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnGain(Message, Self);
                }));
            mInterface.Bind("Drop",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnDrop(Message, Self);
                }));
            mInterface.Bind("Clear",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnClear(Message, Self);
                }));
            mInterface.Bind("CastSpell",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnSpell(Message, Self, 0);
                }));
            mInterface.Bind("Assign1",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnSpell(Message, Self, 1);
                }));
            mInterface.Bind("Assign2",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnSpell(Message, Self, 2);
                }));
            mInterface.Bind("Assign3",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnSpell(Message, Self, 3);
                }));
            mInterface.Bind("AskQuit",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnAskQuit(Message, Self);
                }));
            mInterface.Bind("Quit",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnPressed(Message, Self, "Farewell, though the example stays open");
                }));
            mInterface.Bind("Stay",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnPressed(Message, Self, "Stay a while longer");
                }));
            mInterface.Bind("Language",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnLanguage(Message, Self);
                }));
            mInterface.Bind("Scale",
                Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
                {
                    return OnScale(Message, Self);
                }));

            ConstRetainer<ZyContent::Service> Content = GetService<ZyContent::Service>();

            // Every language is read up front, so switching is only a matter of which one is spoken.
            Content->Read("Resources://Language/English.lang", ZyJob::Lane::Main,
                ZyContent::Mount::OnRead::Create([this](Filesystem::Result Result, Blob Data)
                {
                    OnWords(Result, Data, "English");
                }));
            Content->Read("Resources://Language/Spanish.lang", ZyJob::Lane::Main,
                ZyContent::Mount::OnRead::Create([this](Filesystem::Result Result, Blob Data)
                {
                    OnWords(Result, Data, "Spanish");
                }));

            Content->Read("Resources://Theme/Example.theme", ZyJob::Lane::Main,
                ZyContent::Mount::OnRead::Create([this](Filesystem::Result Result, Blob Data)
                {
                    OnTheme(Result, Data);
                }));
            Content->Read("Resources://Layout/Controls.ui", ZyJob::Lane::Main,
                ZyContent::Mount::OnRead::Create([this](Filesystem::Result Result, Blob Data)
                {
                    OnLayout(Result, Data);
                }));
        }

        /// \brief Observes the bag's list, so its rows swap with each other and with the grid's slots.
        void ObserveBag()
        {
            mBag.Find("Items").Observe(Handler::Create([this](Ref<Interface>, ConstRef<Event> Message, Handle Self)
            {
                return OnSwap(Message, Self);
            }));
        }

        /// \brief Dresses the interface in its theme file over the library's own, left alone when it cannot be read.
        ///
        /// \param Result The result of the read.
        /// \param Data   The text of the theme.
        void OnTheme(Filesystem::Result Result, ConstRef<Blob> Data)
        {
            Str Error;

            const Text Source(Data.GetData<Char>(), Data.GetSize());

            if (Result != Filesystem::Result::Success || !mMarkup->Dress(mTheme, Source, Error))
            {
                LOG_W("The theme cannot be read: {0}", Error);
            }
        }

        /// \brief Makes the controls card from the layout read from its file.
        ///
        /// \param Result The result of the read.
        /// \param Data   The text of the layout.
        void OnLayout(Filesystem::Result Result, ConstRef<Blob> Data)
        {
            Str Error;

            const Text Source(Data.GetData<Char>(), Data.GetSize());

            if (Result != Filesystem::Result::Success || !mMarkup->Read(Source, Error))
            {
                LOG_W("The layout cannot be read: {0}", Error);
                return;
            }
            mMarkup->Spawn(mInterface, "Controls");
            mMarkup->Spawn(mInterface, "Gallery");
            mMarkup->Spawn(mInterface, "Audio");

            // The pickers of language and scale stand in the character card built in code, beside its title.
            mMarkup->Spawn(mHeader, "Options");

            // The bag waits closed until its button opens it; its rows swap with each other and with the grid's slots.
            mBag = mMarkup->Spawn(mInterface, "Bag");
            Window::Close(mInterface, mBag.GetHandle());
            ObserveBag();

            // The codex waits closed as well; its button opens it, and tab walks into it once it holds the keyboard.
            mCodex = mMarkup->Spawn(mInterface, "Codex");
            Window::Close(mInterface, mCodex.GetHandle());

            // The dialog waits closed too, until the quit button asks.
            mQuit = mMarkup->Spawn(mInterface, "QuitDialog");
            Dialog::Close(mInterface, mQuit.GetHandle());

            // The editor waits closed as well, until its button opens it.
            mEditor = mMarkup->Spawn(mInterface, "Editor");
            Window::Close(mInterface, mEditor.GetHandle());
        }

        /// \brief Gives the lexicon the table of a language read from its file.
        ///
        /// \param Result   The result of the read.
        /// \param Data     The text of the table.
        /// \param Language The name of the language.
        void OnWords(Filesystem::Result Result, ConstRef<Blob> Data, Text Language)
        {
            Str Error;

            const Text Source(Data.GetData<Char>(), Data.GetSize());

            if (Result != Filesystem::Result::Success || !mLexicon.Read(Language, Source, Error))
            {
                LOG_W("The language '{0}' cannot be read: {1}", Language, Error);
            }
        }

        /// \brief Switches the language every text written as a key is read in, from the picker in the character card.
        ///
        /// \param Message The event.
        /// \param Self    The picker.
        /// \return `false` always, so the event bubbles on.
        Bool OnLanguage(ConstRef<Event> Message, Handle Self)
        {
            if (Message.Kind == Signal::kChange && Message.Target == Self)
            {
                mLexicon.SetLanguage(Listbox::GetIndex(Element(mInterface, Self)) == 1 ? "Spanish" : "English");

                Str Line;
                Line.Format<"[Settings] Language: {0}">(mLexicon.GetLanguage());
                Say(Line);
            }
            return false;
        }

        /// \brief Scales the interface over what the display asks for, from the picker in the character card.
        ///
        /// \param Message The event.
        /// \param Self    The picker.
        /// \return `false` always, so the event bubbles on.
        Bool OnScale(ConstRef<Event> Message, Handle Self)
        {
            static constexpr Array<Real32, 3> kScales(1.0f, 1.25f, 1.5f);

            if (Message.Kind == Signal::kChange && Message.Target == Self)
            {
                const SInt32 Picked = Clamp(Listbox::GetIndex(Element(mInterface, Self)), 0, 2);
                mInterface.SetScale(kScales[Picked]);

                Str Line;
                Line.Format<"[Settings] Interface scale: {0}%">(static_cast<UInt32>(kScales[Picked] * 100.0f));
                Say(Line);
            }
            return false;
        }

        /// \brief Takes the display's scale from the monitor holding the middle of the window, as it opens.
        ///
        /// \param Platform The platform service.
        void Scale(Ref<ZyPlatform::Service> Platform)
        {
            ConstRef<ZyPlatform::Window> Window = Platform.GetWindow();

            const SInt32 X = Window.GetX() + static_cast<SInt32>(Window.GetWidth() / 2);
            const SInt32 Y = Window.GetY() + static_cast<SInt32>(Window.GetHeight() / 2);

            if (const ConstPtr<ZyPlatform::Monitor> Found = Platform.GetDisplay().GetMonitor(X, Y))
            {
                OnWindowScale(Found->GetScale());
            }
        }

        /// \brief Takes a new display scale, as the window moves to another DPI, into the interface and the model.
        ///
        /// \param Factor The display's scale.
        /// \return `false` always, so others hear it too.
        Bool OnWindowScale(Real32 Factor)
        {
            mInterface.SetDensity(Factor);
            mModel.SetNumber("Ui.Density", mInterface.GetDensity() * 100.0f);

            LOG_I("The display asks for {0}%, so the interface is drawn at {1}%",
                static_cast<UInt32>(mInterface.GetDensity() * 100.0f),
                static_cast<UInt32>(mInterface.GetPixelRatio() * 100.0f));
            return false;
        }

        /// \brief Plays a sound the theme names through the audio service, loading it the first time it is named.
        ///
        /// \param Address The address of the sound.
        /// \param Moment  The moment of interaction it is played for.
        void OnSound(Text Address, Cue Moment)
        {
            const UInt64 Key = Identify(Address);

            if (const ConstPtr<Retainer<ZyAudio::Sound>> Known = mSounds.Find(Key))
            {
                if ((* Known)->HasCompleted())
                {
                    GetService<ZyAudio::Service>()->Play(ZyAudio::Category::Interface, * Known, 0.8f);
                }
                return;
            }

            LOG_I("The interface asks for sound '{0}' on {1}", Address, ZyEnum::GetName(Moment));
            mSounds.Assign(Key, GetService<ZyContent::Service>()->Load<ZyAudio::Sound>(ZyContent::Uri(Address)));
        }

        /// \brief Writes a line to the chat log.
        ///
        /// \param Content The line.
        void Say(Text Content)
        {
            AddText(mLog, Content, 14.0f, Color(0.94f, 0.85f, 0.56f, 1.0f));
        }

        /// \brief Writes a click on a button to the chat log.
        ///
        /// \param Message The event.
        /// \param Self    The button.
        /// \param Name    The name of the button.
        /// \return `false` always, so the event bubbles on.
        Bool OnPressed(ConstRef<Event> Message, Handle Self, Text Name)
        {
            if (Message.Kind == Signal::kClick && Message.Target == Self)
            {
                Str Line;
                Line.Format<"[You] {0}!">(Name);
                Say(Line);
            }
            return false;
        }

        /// \brief Writes a check box turning to the chat log.
        ///
        /// \param Message The event.
        /// \param Self    The check box.
        /// \param Name    The name of the setting.
        /// \return `false` always, so the event bubbles on.
        Bool OnToggled(ConstRef<Event> Message, Handle Self, Text Name)
        {
            if (Message.Kind == Signal::kChange && Message.Target == Self)
            {
                Str Line;
                const Bool Checked = mInterface.HasState(Self, Widget::Checked());
                Line.Format<"[Settings] {0}: {1}">(Name, Checked ? "on"_Text : "off"_Text);
                Say(Line);
            }
            return false;
        }

        /// \brief Writes a level picked in a number box to the chat log.
        ///
        /// \param Owner   The interface.
        /// \param Message The event.
        /// \param Self    The number box.
        /// \return `false` always, so the event bubbles on.
        Bool OnLevel(Ref<Interface> Owner, ConstRef<Event> Message, Handle Self)
        {
            if (Message.Kind == Signal::kChange && Message.Target == Self)
            {
                Str Line;
                Line.Format<"[Settings] Level: {0}">(static_cast<UInt32>(Spinner::GetValue(Element(Owner, Self))));
                Say(Line);
            }
            return false;
        }

        /// \brief Writes the item picked in a list or a combo box to the chat log.
        ///
        /// \param Message The event.
        /// \param Self    The list or the combo box.
        /// \param Name    The name of the setting.
        /// \return `false` always, so the event bubbles on.
        Bool OnPicked(ConstRef<Event> Message, Handle Self, Text Name)
        {
            if (Message.Kind == Signal::kChange && Message.Target == Self)
            {
                const Element Picked = Listbox::GetSelected(Element(mInterface, Self));

                Str Line;
                Line.Format<"[Settings] {0}: {1}">(Name, Text(Picked.GetFirst().Get<Caption>()->Content));
                Say(Line);
            }
            return false;
        }

        /// \brief Opens the bag window, or closes it when it is open.
        ///
        /// \param Message The event.
        /// \param Self    The button.
        /// \return `false` always, so the event bubbles on.
        Bool OnBag(ConstRef<Event> Message, Handle Self)
        {
            if (Message.Kind == Signal::kClick && Message.Target == Self && mBag.IsAlive())
            {
                if (mBag.HasState(State::Hidden))
                {
                    Window::Show(mInterface, mBag.GetHandle());
                }
                else
                {
                    Window::Close(mInterface, mBag.GetHandle());
                }
            }
            return false;
        }

        /// \brief Opens the editor window, or closes it when it is open.
        ///
        /// \param Message The event.
        /// \param Self    The button.
        /// \return `false` always, so the event bubbles on.
        Bool OnEditor(ConstRef<Event> Message, Handle Self)
        {
            if (Message.Kind == Signal::kClick && Message.Target == Self && mEditor.IsAlive())
            {
                if (mEditor.HasState(State::Hidden))
                {
                    Window::Show(mInterface, mEditor.GetHandle());
                }
                else
                {
                    Window::Close(mInterface, mEditor.GetHandle());
                }
            }
            return false;
        }

        /// \brief Opens the codex window, giving the keyboard to its first control, or closes it when it is open.
        ///
        /// \param Message The event.
        /// \param Self    The button.
        /// \return `false` always, so the event bubbles on.
        Bool OnCodex(ConstRef<Event> Message, Handle Self)
        {
            if (Message.Kind != Signal::kClick || Message.Target != Self || !mCodex.IsAlive())
            {
                return false;
            }

            if (mCodex.HasState(State::Hidden))
            {
                Window::Show(mInterface, mCodex.GetHandle());
                mInterface.Focus(Window::GetClose(mCodex).GetHandle());
            }
            else
            {
                Window::Close(mInterface, mCodex.GetHandle());
            }
            return false;
        }

        /// \brief Scrolls the codex back to its first page, from code.
        ///
        /// \param Message The event.
        /// \param Self    The button.
        /// \return `false` always, so the event bubbles on.
        Bool OnCodexTop(ConstRef<Event> Message, Handle Self)
        {
            if (Message.Kind == Signal::kClick && Message.Target == Self && mCodex.IsAlive())
            {
                Scroller::ScrollTo(mCodex.Find("Pages"), Vector2(0.0f, 0.0f));
            }
            return false;
        }

        /// \brief Writes the beast a codex row's button tracks to the chat log, read from the first label of its row.
        ///
        /// \param Message The event.
        /// \param Self    The button.
        /// \return `false` always, so the event bubbles on.
        Bool OnTrack(ConstRef<Event> Message, Handle Self)
        {
            if (Message.Kind == Signal::kClick && Message.Target == Self)
            {
                const ConstPtr<Caption> Kind = Element(mInterface, Self).GetParent().GetFirst().Get<Caption>();

                Str Line;
                Line.Format<"[Codex] Tracking the {0}.">(Kind ? Text(Kind->Content) : Text("beast"));
                Say(Line);
            }
            return false;
        }

        /// \brief Writes the record picked in the bag to the chat log.
        ///
        /// \param Message The event.
        /// \param Self    The bag's item list.
        /// \return `false` always, so the event bubbles on.
        Bool OnPick(ConstRef<Event> Message, Handle Self)
        {
            if (Message.Kind == Signal::kChange && Message.Target == Self)
            {
                Str Where;
                Where.Format<"Inventory[{0}].Name">(Repeater::GetIndex(Element(mInterface, Self)));

                Str Line;
                Line.Format<"[Bag] Picked {0}.">(Text(mModel.GetText(Where)));
                Say(Line);
            }
            return false;
        }

        /// \brief Adds a record to the end of the inventory, which the bag and the grid show at once.
        ///
        /// \param Message The event.
        /// \param Self    The button.
        /// \return `false` always, so the event bubbles on.
        Bool OnGain(ConstRef<Event> Message, Handle Self)
        {
            if (Message.Kind == Signal::kClick && Message.Target == Self)
            {
                const UInt32 Count = mModel.GetCount("Inventory");

                Str Name;
                Name.Format<"{0}. Found Trinket">(Count + 1);

                const Ptr<Model> Record = mModel.Append("Inventory");
                Record->SetText("Name", Name);
                Record->SetNumber("Count", 1.0);
                Record->SetBool("Rare", true);

                Str Line;
                Line.Format<"[Bag] Looted {0}.">(Text(Name));
                Say(Line);
            }
            return false;
        }

        /// \brief Takes the record picked in the bag out of the inventory, the ones after it moving up.
        ///
        /// \param Message The event.
        /// \param Self    The button.
        /// \return `false` always, so the event bubbles on.
        Bool OnDrop(ConstRef<Event> Message, Handle Self)
        {
            const SInt32 Picked = mBag.IsAlive() ? Repeater::GetIndex(mBag.Find("Items")) : -1;

            if (Message.Kind == Signal::kClick && Message.Target == Self && Picked >= 0)
            {
                Str Where;
                Where.Format<"Inventory[{0}].Name">(Picked);

                Str Line;
                Line.Format<"[Bag] Dropped {0}.">(Text(mModel.GetText(Where)));
                Say(Line);

                mModel.Erase("Inventory", static_cast<UInt32>(Picked));
            }
            return false;
        }

        /// \brief Empties the inventory.
        ///
        /// \param Message The event.
        /// \param Self    The button.
        /// \return `false` always, so the event bubbles on.
        Bool OnClear(ConstRef<Event> Message, Handle Self)
        {
            if (Message.Kind == Signal::kClick && Message.Target == Self)
            {
                mModel.Resize("Inventory", 0);
                Say("[Bag] Emptied the bag.");
            }
            return false;
        }

        /// \brief Swaps the two records a drop joins, a slot or a row dropped on another of the grid or the bag.
        ///
        /// \param Message The event.
        /// \param Self    The item list it bubbled to.
        /// \return `true` if the drop swapped two records, `false` otherwise.
        Bool OnSwap(ConstRef<Event> Message, Handle Self)
        {
            if (Message.Kind != Drag::kDrop)
            {
                return false;
            }

            const SInt32 From = RecordOf(Element(mInterface, Message.Other));
            const SInt32 To   = RecordOf(Element(mInterface, Message.Target));

            if (From < 0 || To < 0 || From == To)
            {
                return false;
            }

            // What the dragged slot carries is its item's name, bound like everything else it shows.
            const Str Carried = Drag::GetPayload(Element(mInterface, Message.Other)).Print();

            Swap(static_cast<UInt32>(From), static_cast<UInt32>(To));

            Str Line;
            Line.Format<"[Bag] Moved {0} to slot {1}.">(Text(Carried), To + 1);
            Say(Line);
            return true;
        }

        /// \brief Finds the record a part of a slot of the grid or a row of the bag shows.
        ///
        /// \param Part The part.
        /// \return The position of the record, or below nought when it stands in neither list.
        SInt32 RecordOf(Element Part)
        {
            const SInt32 Record = Repeater::GetRecord(mGrid, Part);
            return Record >= 0 || !mBag.IsAlive() ? Record : Repeater::GetRecord(mBag.Find("Items"), Part);
        }

        /// \brief Swaps two records of the inventory, value by value.
        ///
        /// \param First  The position of one record.
        /// \param Second The position of the other.
        void Swap(UInt32 First, UInt32 Second)
        {
            const Ptr<Model> One = mModel.GetRecord("Inventory", First);
            const Ptr<Model> Two = mModel.GetRecord("Inventory", Second);

            const Str    Name   = One->GetText("Name");
            const Real64 Count  = One->GetNumber("Count", 0.0);
            const Bool   Rare   = One->GetBool("Rare", false);

            One->SetText("Name", Text(Two->GetText("Name")));
            One->SetNumber("Count", Two->GetNumber("Count", 0.0));
            One->SetBool("Rare", Two->GetBool("Rare", false));
            Two->SetText("Name", Name);
            Two->SetNumber("Count", Count);
            Two->SetBool("Rare", Rare);
        }

        /// \brief Writes what a spell's context menu picked to the chat log.
        ///
        /// \param Message The event.
        /// \param Self    The item picked in the menu.
        /// \param Slot    The slot of the bar the spell goes to, or nought to cast it.
        /// \return `false` always, so the event bubbles on.
        Bool OnSpell(ConstRef<Event> Message, Handle Self, UInt32 Slot)
        {
            if (Message.Kind != Signal::kClick || Message.Target != Self)
            {
                return false;
            }

            const Str Spell(Listbox::GetText(Menu::GetSubject(Element(mInterface, Self))));
            Str       Line;

            if (Spell.IsEmpty())
            {
                Line.Format<"[Spells] Right click a spell to use it.">();
            }
            else if (Slot == 0)
            {
                Line.Format<"[You] Cast {0}!">(Text(Spell));
            }
            else
            {
                Line.Format<"[Bar] {0} goes to slot {1}.">(Text(Spell), Slot);
            }
            Say(Line);
            return false;
        }

        /// \brief Asks whether to quit, in a dialog holding the pointer and the keyboard until it is answered.
        ///
        /// \param Message The event.
        /// \param Self    The button.
        /// \return `false` always, so the event bubbles on.
        Bool OnAskQuit(ConstRef<Event> Message, Handle Self)
        {
            if (Message.Kind == Signal::kClick && Message.Target == Self && mQuit.IsAlive())
            {
                Dialog::Show(mInterface, mQuit.GetHandle());
            }
            return false;
        }

        /// \brief Fills the inventory with five hundred records, named and counted from their position.
        void Stock()
        {
            static constexpr Array<Text, 8> kMaterials("Iron", "Elven", "Rusty", "Gilded",
                                                       "Ancient", "Cursed", "Fine", "Bone");
            static constexpr Array<Text, 10> kKinds("Sword", "Shield", "Potion", "Arrows", "Ring",
                                                    "Helm", "Scroll", "Gem", "Bow", "Cloak");

            for (UInt32 Index = 0; Index < 500; ++Index)
            {
                Str Name;
                Name.Format<"{0}. {1} {2}">(Index + 1, kMaterials[Index % 8], kKinds[(Index / 8) % 10]);

                const Ptr<Model> Record = mModel.Append("Inventory");
                Record->SetText("Name", Name);
                Record->SetNumber("Count", static_cast<Real64>((Index * 7) % 20 + 1));
                Record->SetBool("Rare", Index % 7 == 3);
            }
        }

        /// \brief Gives the editor a scene of nested places and a list of the entities in them.
        void Stage()
        {
            static constexpr Array<Text, 4> kPlaces("Town", "Forest", "Lake", "Ruins");
            static constexpr Array<Text, 5> kRooms("Inn", "Smithy", "Gate", "Cave", "Grove");
            static constexpr Array<Text, 6> kKinds("Guard", "Wolf", "Merchant", "Bandit", "Wraith", "Smith");

            for (UInt32 Index = 0; Index < 4; ++Index)
            {
                const Ptr<Model> Place = mModel.Append("Scene");
                Place->SetText("Name", kPlaces[Index]);

                // Each place holds a room or two, and the first room of the forest holds a nest of its own.
                for (UInt32 Room = 0; Room < (Index % 2) + 2; ++Room)
                {
                    const Ptr<Model> Inner = Place->Append("Children");
                    Inner->SetText("Name", kRooms[(Index + Room) % 5]);

                    if (Index == 1 && Room == 0)
                    {
                        Inner->Append("Children")->SetText("Name", "Nest");
                    }
                }
            }

            for (UInt32 Index = 0; Index < 40; ++Index)
            {
                Str Name;
                Name.Format<"{0} {1}">(kKinds[Index % 6], Index + 1);

                const Ptr<Model> Entity = mModel.Append("Entities");
                Entity->SetText("Name", Name);
                Entity->SetText("Kind", kKinds[Index % 6]);
                Entity->SetNumber("Level", static_cast<Real64>((Index * 7) % 30 + 1));
                Entity->SetNumber("Health", static_cast<Real64>((Index * 37) % 200 + 20));
            }
            mModel.SetText("Editor.Picked", "");
        }

        /// \brief Gives the model the values the interface starts by showing.
        void Seed()
        {
            Stock();
            Stage();

            mModel.SetText("Player.Name", "Aeldur");
            mModel.SetText("Player.Notes",
                "Owes the guard 3 gold.\nMeet Brom at the north gate before dusk, and bring the old key.");
            mModel.SetNumber("Player.Level", 7.0);
            mModel.SetNumber("Player.Experience", 0.42);
            mModel.SetNumber("Player.Gold", 120.0);
            mModel.SetNumber("Player.HealthMax", 250.0);
            mModel.SetNumber("Player.ManaMax", 100.0);
            mModel.SetBool("Settings.ShowNames", true);
            mModel.SetBool("Settings.Loot", false);
            mModel.SetNumber("Settings.Volume", 60.0);
            mModel.SetNumber("Settings.Quality", 4.0);
            mModel.SetBool("Settings.Normal", true);
            mModel.SetNumber("Audio.Music", 0.8);
            mModel.SetNumber("Audio.Effects", 0.6);
            mModel.SetBool("Ui.ShowPurse", true);
            mModel.SetNumber("Ui.Density", 100.0);
        }

        /// \brief Moves the game on, with the vitals rising and falling, gold trickling in and experience filling.
        ///
        /// \param Delta The time since the last frame, in seconds.
        void Advance(Real32 Delta)
        {
            const Real32 Health = Wave(mTime * 0.2f);
            const Real32 Mana   = Wave(mTime * 0.13f + 0.5f);

            // The shares move every frame; the whole numbers only change, and so are shown again, now and then.
            mModel.SetNumber("Player.HealthShare", Health);
            mModel.SetNumber("Player.Health", Floor(Health * 250.0f));
            mModel.SetNumber("Player.ManaShare", Mana);
            mModel.SetNumber("Player.Mana", Floor(Mana * 100.0f));
            mModel.SetNumber("Player.Gold", 120.0 + Floor(mTime * 2.0f));

            Real64 Experience = mModel.GetNumber("Player.Experience") + Delta * 0.05;

            if (Experience >= 1.0)
            {
                const Real64 Level = Min(mModel.GetNumber("Player.Level") + 1.0, 60.0);

                Experience -= 1.0;
                mModel.SetNumber("Player.Level", Level);

                Str Line;
                Line.Format<"[Game] Level up! Now level {0}.">(static_cast<UInt32>(Level));
                Say(Line);
            }
            mModel.SetNumber("Player.Experience", Experience);

            // What the bag costs, shown in it: the rows that exist, against the records it holds.
            if (mBag.IsAlive())
            {
                mModel.SetNumber("Ui.Rows", static_cast<Real64>(Repeater::GetRealized(mBag.Find("Items"))));
            }
        }

        /// \brief Fills a bar to a share.
        ///
        /// \param Gauge The bar.
        /// \param Share The share filled, over nought through one.
        static void SetShare(Element Gauge, Real32 Share)
        {
            Gauge.Edit<Bar>()->Value = Share;
        }

        /// \brief Gets a wave rising from nought to one and falling back, once a second.
        ///
        /// \param Time The time, in seconds.
        /// \return The height of the wave.
        static Real32 Wave(Real32 Time)
        {
            const Real32 Part = Time - static_cast<Real32>(static_cast<SInt32>(Time));
            return Part < 0.5f ? Part * 2.0f : 2.0f - Part * 2.0f;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Canvas                     mCanvas;
        Interface                  mInterface;
        Retainer<ZyRender::Font>   mFont;
        Retainer<ZyGraphic::Image> mDoor;
        Element                    mLog;
        Element                    mBag;
        Element                    mGrid;
        Element                    mQuit;
        Element                    mCodex;
        Element                    mEditor;
        Element                    mHeader;
        Model                      mModel;
        Lexicon                    mLexicon;
        Theme                      mTheme;
        Unique<Markup>             mMarkup;
        Table<UInt64, Retainer<ZyAudio::Sound>> mSounds;
        Real32                     mTime;
    };
}