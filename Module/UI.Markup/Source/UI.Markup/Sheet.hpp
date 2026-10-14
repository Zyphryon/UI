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

#include "UI.Core/Types.hpp"
#include <Zyphryon.Graphic/Image.hpp>
#include <Zyphryon.Render/Font/Font.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    class Markup;
    class Theme;

    /// \brief Represents what a theme names while it is read, and the first thing wrong with it.
    struct Sheet final
    {
        /// \brief Represents an animation a skin names, which the theme must define once it is read whole.
        struct Reference final
        {
            /// The name of the style naming it.
            Str  Where;

            /// The name of the field naming it.
            Str  Key;

            /// The name of the animation.
            Str  Name;

            /// The origin of the layer naming it, as \ref Sheet::Origin gave it then.
            Text Origin;
        };

        /// The markup reading the theme, which loads its typefaces and images.
        Ptr<Markup>                               Owner  = nullptr;

        /// The theme the animations and styles of every layer are read into, kept apart from the one in use.
        Ptr<Theme>                                Target = nullptr;

        /// The colours every layer names, found by the identifier of their name.
        Table<Digest, Color>                      Colors;

        /// The typefaces every layer names, or a slot one declares holding none, found by the identifier of their name.
        Table<Digest, Retainer<ZyRender::Font>>   Fonts;

        /// The images every layer names, found by the identifier of their name.
        Table<Digest, Retainer<ZyGraphic::Image>> Images;

        /// The animations the skins name, checked once every animation is read, wherever it was written.
        Sequence<Reference>                       Animations;

        /// What is put before the first thing wrong with the layer being read, empty for a game's own layer.
        Text                                      Origin;

        /// The first thing wrong with the theme, or empty while nothing is.
        Str                                       Error;
    };
}