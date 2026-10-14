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

#include "Module.hpp"
#include "UI.Control/Theme.inl"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Layers.hpp"
#include "UI.Gesture/Module.hpp"
#include "UI.Markup/Markup.hpp"
#include "Widget/Button.hpp"
#include "Widget/Checkbox.hpp"
#include "Widget/Combobox.hpp"
#include "Widget/Common.hpp"
#include "Widget/Dialog.hpp"
#include "Widget/Dock.hpp"
#include "Widget/Dropdown.hpp"
#include "Widget/Expander.hpp"
#include "Widget/Grid.hpp"
#include "Widget/Image.hpp"
#include "Widget/Label.hpp"
#include "Widget/Listbox.hpp"
#include "Widget/Menu.hpp"
#include "Widget/Menubar.hpp"
#include "Widget/Outline.hpp"
#include "Widget/Panel.hpp"
#include "Widget/Progress.hpp"
#include "Widget/Radio.hpp"
#include "Widget/Repeater.hpp"
#include "Widget/Scalar.hpp"
#include "Widget/Scrollbar.hpp"
#include "Widget/Scroller.hpp"
#include "Widget/Slider.hpp"
#include "Widget/Spinner.hpp"
#include "Widget/Splitter.hpp"
#include "Widget/Swatch.hpp"
#include "Widget/Tabs.hpp"
#include "Widget/Textbox.hpp"
#include "Widget/Window.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void ControlModule::Declare(Ref<Interface> Target)
    {
        GestureModule::Declare(Target);
        Enroll();

        // A swatch's board is drawn by the painter, under the colour it shows.
        Swatch::Declare(Target);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void ControlModule::Declare(Ref<Markup> Reader)
    {
        // Taught once, so a builder or a band a game gave the markup since stays when the library is declared again.
        if (!Reader.Enlist("Control"))
        {
            return;
        }
        GestureModule::Declare(Reader);
        Enroll();

        // A template opens in a window unless it says, and any node may open a menu at a right press.
        Reader.SetLayer(Window::kLayer);
        Reader.AddField(Markup::Field::Create<&Menu::Read>());

        Reader.Register("Panel",     Markup::Builder::Create<&Panel::Build>());
        Reader.Register("Label",     Markup::Builder::Create<&Label::Build>());
        Reader.Register("Image",     Markup::Builder::Create<&Image::Build>());
        Reader.Register("Button",    Markup::Builder::Create<&Button::Build>());
        Reader.Register("Checkbox",  Markup::Builder::Create<&Checkbox::Build>());
        Reader.Register("Slider",    Markup::Builder::Create<&Slider::Build>());
        Reader.Register("Listbox",   Markup::Builder::Create<&Listbox::Build>());
        Reader.Register("Repeater",  Markup::Builder::Create<&Repeater::Build>());
        Reader.Register("Combobox",  Markup::Builder::Create<&Combobox::Build>());
        Reader.Register("Radio",     Markup::Builder::Create<&Radio::Build>());
        Reader.Register("Progress",  Markup::Builder::Create<&Progress::Build>());
        Reader.Register("Tabs",      Markup::Builder::Create<&Tabs::Build>());
        Reader.Register("Expander",  Markup::Builder::Create<&Expander::Build>());
        Reader.Register("Window",    Markup::Builder::Create<&Window::Build>());
        Reader.Register("Spinner",   Markup::Builder::Create<&Spinner::Build>());
        Reader.Register("Scalar", Markup::Builder::Create<&Scalar::Build>());
        Reader.Register("Swatch",    Markup::Builder::Create<&Swatch::Build>());
        Reader.Register("Textbox",   Markup::Builder::Create<&Textbox::Build>());
        Reader.Register("Menu",      Markup::Builder::Create<&Menu::Build>());
        Reader.Register("Menubar",   Markup::Builder::Create<&Menubar::Build>());
        Reader.Register("Dropdown", Markup::Builder::Create<&Dropdown::Build>());
        Reader.Register("Dialog",    Markup::Builder::Create<&Dialog::Build>());
        Reader.Register("Scrollbar", Markup::Builder::Create<&Scrollbar::Build>());
        Reader.Register("Scroller",  Markup::Builder::Create<&Scroller::Build>());
        Reader.Register("Splitter",  Markup::Builder::Create<&Splitter::Build>());
        Reader.Register("Grid",      Markup::Builder::Create<&Grid::Build>());
        Reader.Register("Outline",   Markup::Builder::Create<&Outline::Build>());
        Reader.Register("Dock",      Markup::Builder::Create<&Dock::Build>());

        // Every widget wears the theme built into the library, read over the gesture module's.
        Reader.AddTheme(Text(reinterpret_cast<ConstPtr<Char>>(kTheme), sizeof(kTheme)));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void ControlModule::Enroll()
    {
        // The states are named before any theme is read, since a theme names them.
        Widget::Selected();
        Widget::Checked();
        Repeater::Marked();

        Layers::Declare("Window", Window::kLayer);
    }
}