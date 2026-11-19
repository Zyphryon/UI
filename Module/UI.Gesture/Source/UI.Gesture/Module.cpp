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
#include "Drag.hpp"
#include "Facet/Draggable.hpp"
#include "Facet/Hint.hpp"
#include "Input.hpp"
#include "Summon.hpp"
#include "Tooltip.hpp"
#include "UI.Core/Layers.hpp"
#include "UI.Gesture/Theme.inl"
#include "UI.Markup/Module.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void GestureModule::Declare(Ref<Interface> Target)
    {
        MarkupModule::Declare(Target);
        Enroll();

        Drag::Declare(Target);
        Tooltip::Declare(Target);
        Summon::Declare(Target);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void GestureModule::Declare(Ref<Markup> Reader)
    {
        if (!Reader.Enlist("Gesture"))
        {
            return;
        }
        Enroll();

        Reader.AddField(Markup::Field::Create<&Drag::Read>());
        Reader.AddField(Markup::Field::Create<&Tooltip::Read>());

        Reader.AddField(Markup::Field::Create<&Input::Read>());

        // The ghost and tooltip wear the built-in theme, which a game's theme overrides field by field.
        Reader.AddTheme(Text(reinterpret_cast<ConstPtr<Char>>(kTheme), sizeof(kTheme)));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void GestureModule::Enroll()
    {
        // The states are named before any theme is read, since a theme names them.
        Drag::Dragging();
        Drag::DropHover();

        Layers::Declare("Tooltip", Tooltip::kLayer);
        Layers::Declare("Drag",    Drag::kLayer);

        // A tooltip or ghost made later from a template reads the record its row shows by then.
        Binder::AddScoped<Hint>(Binder::Scoped::Create<&Tooltip::GetWithin>());
        Binder::AddScoped<Draggable>(Binder::Scoped::Create<&Drag::GetWithin>());
    }
}