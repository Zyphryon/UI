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

#include "Sheet.hpp"
#include "Template.hpp"
#include "UI.Core/Style/Theme.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    class Markup;

    /// \brief Provides the reading of a theme into its styles and animations, and of a layout into its templates.
    class Interpreter final
    {
    public:

        /// \brief Reads the layers of a theme, each over those before it, naming every colour before any style.
        ///
        /// \param Owner   The markup reading the theme, which loads its typefaces and images.
        /// \param Target  The theme, left as it was when any layer cannot be read.
        /// \param Layers  The text of each layer, in the order they are read.
        /// \param Builtin The number of layers, from the first, a module built in, whose mistakes are said to be so.
        /// \param Fonts   The typefaces given from code, found by the identifier of their name, under every layer.
        /// \param Error   The variable receiving why a layer cannot be read.
        /// \return `true` if every layer was read, `false` otherwise.
        static Bool Dress(Ref<Markup>                                       Owner,
                          Ref<Theme>                                        Target,
                          ConstSpan<Text>                                   Layers,
                          UInt                                              Builtin,
                          ConstRef<Table<Digest, Retainer<ZyRender::Font>>> Fonts,
                          Ref<Str>                                          Error);

        /// \brief Reads the templates of a layout, each one replacing a template of the same name once all were read.
        ///
        /// \param Target The templates, left as they were when the layout cannot be read.
        /// \param Source The text of the layout.
        /// \param Error  The variable receiving why the layout cannot be read.
        /// \return `true` if it was read, `false` otherwise.
        static Bool Read(Ref<Template> Target, Text Source, Ref<Str> Error);

    private:

        /// What is put before the first thing wrong with a layer a module built in.
        static constexpr Text kBuiltin  = "the built-in theme: ";

        /// The tables a layer may give, each an object.
        static constexpr Array<Text, 5> kTables { "Colors", "Fonts", "Images", "Styles", "Animations" };

        /// \brief Notes the first thing wrong with a theme, after the origin of the layer being read.
        ///
        /// \param Context The theme being read, which keeps only the first thing wrong.
        /// \param Where   The name of the style or the table it was found in.
        /// \param Key     The name of the field.
        /// \param Why     The reason the field is wrong.
        static void Fail(Ref<Sheet> Context, Text Where, Text Key, Text Why);

        /// \brief Parses one layer of a theme, checking that each table it gives is an object.
        ///
        /// \param Context The theme being read, which notes why the layer cannot be parsed.
        /// \param Source  The text of the layer.
        /// \param Output  The variable receiving the layer.
        static void ParseLayer(Ref<Sheet> Context, Text Source, Ref<JsonValue> Output);

        /// \brief Reads every entry of one table a layer gives, when it gives the table.
        ///
        /// \param Context  The theme being read.
        /// \param Document The layer, parsed and checked to give each table as an object.
        /// \param Table    The name of the table, as `Colors`.
        /// \param Reader   The function reading one entry of the table, by its name and its value.
        static void ReadTable(Ref<Sheet>          Context,
                              ConstRef<JsonValue> Document,
                              Text                Table,
                              void (* Reader)(Ref<Sheet>, Text, ConstRef<JsonValue>));

        /// \brief Reads one field of a skin.
        ///
        /// \param Context The theme being read.
        /// \param Where   The name of the style giving the field.
        /// \param Key     The name of the field.
        /// \param Value   The value written for the field.
        /// \param Output  The variable receiving the value, and the bit of its mask.
        static void ReadField(Ref<Sheet> Context, Text Where, Text Key, ConstRef<JsonValue> Value, Ref<Skin> Output);

        /// \brief Reads every field of a skin.
        ///
        /// \param Context The theme being read.
        /// \param Where   The name of the style giving the fields.
        /// \param Node    The object giving the fields.
        /// \param Output  The variable receiving the values.
        static void ReadSkin(Ref<Sheet> Context, Text Where, ConstRef<JsonValue> Node, Ref<Skin> Output);

        /// \brief Reads one variant of a style, the states it is worn in and the skin it wears.
        ///
        /// \param Context The theme being read.
        /// \param Where   The name of the style giving the variant.
        /// \param Key     The states, joined by `+`.
        /// \param Value   The skin worn in those states.
        /// \param Target  The style the variant belongs to.
        static void ReadVariant(Ref<Sheet>          Context,
                                Text                Where,
                                Text                Key,
                                ConstRef<JsonValue> Value,
                                Ref<Style>          Target);

        /// \brief Reads the sound a style plays at one moment of interaction.
        ///
        /// \param Context The theme being read.
        /// \param Where   The name of the style giving the sound.
        /// \param Key     The name of the moment, as `Click`.
        /// \param Value   The address of the sound, or empty to silence the one its base names.
        /// \param Target  The style the sound belongs to.
        static void ReadSound(Ref<Sheet> Context, Text Where, Text Key, ConstRef<JsonValue> Value, Ref<Style> Target);

        /// \brief Reads one style, with its base, skin, variants and sounds, merging it over the one a layer defined.
        ///
        /// \param Context The theme being read, whose theme the style is merged into.
        /// \param Key     The name of the style.
        /// \param Value   The object giving the style.
        static void ReadStyle(Ref<Sheet> Context, Text Key, ConstRef<JsonValue> Value);

        /// \brief Reads one colour of a theme's palette, which the colours after it may name.
        ///
        /// \param Context The theme being read, whose palette keeps the colour.
        /// \param Key     The name of the colour.
        /// \param Value   The value written for the colour.
        static void ReadColor(Ref<Sheet> Context, Text Key, ConstRef<JsonValue> Value);

        /// \brief Reads one typeface of a theme, loading it, or a `null` slot that code or a later layer fills.
        ///
        /// \param Context The theme being read, which keeps the typeface.
        /// \param Key     The name of the typeface.
        /// \param Value   The address of the typeface, or `null` for a slot, which never empties one already filled.
        static void ReadFont(Ref<Sheet> Context, Text Key, ConstRef<JsonValue> Value);

        /// \brief Reads one image of a theme, loading it, so styles name it as `"Image": "Name"`.
        ///
        /// \param Context The theme being read, which keeps the image.
        /// \param Key     The name of the image.
        /// \param Value   The address of the image.
        static void ReadImage(Ref<Sheet> Context, Text Key, ConstRef<JsonValue> Value);

        /// \brief Reads one field of a key of an animation, which may give any skin field but a style or animation.
        ///
        /// \param Context The theme being read.
        /// \param Where   The name of the animation.
        /// \param Key     The name of the field.
        /// \param Value   The value written for the field.
        /// \param Output  The variable receiving the value, and the bit of its mask.
        static void ReadKeyField(Ref<Sheet>          Context,
                                 Text                Where,
                                 Text                Key,
                                 ConstRef<JsonValue> Value,
                                 Ref<Skin>           Output);

        /// \brief Reads one key of an animation, the share of the run it stands at and the values worn there.
        ///
        /// \param Context The theme being read.
        /// \param Where   The name of the animation.
        /// \param Node    The object giving the key.
        /// \param Target  The animation the key belongs to.
        static void ReadKey(Ref<Sheet> Context, Text Where, ConstRef<JsonValue> Node, Ref<Animation> Target);

        /// \brief Reads one field of an animation.
        ///
        /// \param Context The theme being read.
        /// \param Where   The name of the animation.
        /// \param Key     The name of the field.
        /// \param Value   The value written for the field.
        /// \param Target  The animation the field belongs to.
        static void ReadAnimationField(Ref<Sheet>          Context,
                                       Text                Where,
                                       Text                Key,
                                       ConstRef<JsonValue> Value,
                                       Ref<Animation>      Target);

        /// \brief Reads one animation of a theme, which replaces the one of its name a layer before defined.
        ///
        /// \param Context The theme being read, whose theme the animation is defined in.
        /// \param Key     The name of the animation.
        /// \param Value   The object giving the animation.
        static void ReadAnimation(Ref<Sheet> Context, Text Key, ConstRef<JsonValue> Value);

        /// \brief Checks that every animation a skin names is one some layer defines, wherever it was written.
        ///
        /// \param Context The theme being read, every layer read into its theme.
        static void CheckAnimations(Ref<Sheet> Context);
    };
}