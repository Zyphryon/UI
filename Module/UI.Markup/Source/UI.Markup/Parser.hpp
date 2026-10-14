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
#include "UI.Core/Style/Skin.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Provides the reading of the values a theme or a layout writes, from numbers to the parts of a skin.
    class Parser final
    {
    public:

        /// \brief Finds a field of a node.
        ///
        /// \param Node The node of a theme or a layout the field is looked up in.
        /// \param Key  The name of the field.
        /// \return The value, or `nullptr` when the node is not an object or gives no such field.
        static ConstPtr<JsonValue> Find(ConstRef<JsonValue> Node, Text Key);

        /// \brief Parses the text of a theme or a layout, which must be an object.
        ///
        /// \param Source The text of the theme or the layout.
        /// \param Output The variable receiving the document.
        /// \param Error  The variable receiving why it cannot be parsed.
        /// \return `true` if it was parsed, `false` otherwise.
        static Bool Parse(Text Source, Ref<JsonValue> Output, Ref<Str> Error);

        /// \brief Reads a number.
        ///
        /// \param Value  The value written for the field.
        /// \param Output The variable receiving the number.
        /// \return `true` if the value is a number, `false` otherwise.
        static Bool ParseNumber(ConstRef<JsonValue> Value, Ref<Real32> Output);

        /// \brief Reads a flag.
        ///
        /// \param Value  The value written for the field.
        /// \param Output The variable receiving the flag.
        /// \return `true` if the value is a flag, `false` otherwise.
        static Bool ParseBool(ConstRef<JsonValue> Value, Ref<Bool> Output);

        /// \brief Reads one of the values of an enumeration, by its name.
        ///
        /// \param Value  The value written for the field, the name of one of the enumeration's values.
        /// \param Output The variable receiving the value.
        /// \return `true` if the enumeration has a value of that name, `false` otherwise.
        template<IsEnum Type>
        static Bool ParseEnum(ConstRef<JsonValue> Value, Ref<Type> Output)
        {
            constexpr Type kNone = static_cast<Type>(0xFF);

            const Type Found = Value.IsString() ? ZyEnum::Cast(Value.GetString(), kNone) : kNone;

            if (Found == kNone)
            {
                return false;
            }
            Output = Found;
            return true;
        }

        /// \brief Reads a band by the name it was declared with.
        ///
        /// \param Value  The value written for the field, the band's name.
        /// \param Output The variable receiving the band.
        /// \return `true` if a band has that name, `false` otherwise.
        static Bool ParseLayer(ConstRef<JsonValue> Value, Ref<Layer> Output);

        /// \brief Reads one or more states joined by `+`, as `Hover+Checked`.
        ///
        /// \param Source The text naming the states.
        /// \param Output The variable receiving the states.
        /// \return `true` if every one of them is a state, `false` otherwise.
        static Bool ParseStates(Text Source, Ref<State> Output);

        /// \brief Reads a colour written as hex, as a name the theme gives, or as three or four numbers up to one.
        ///
        /// \param Context The theme being read, whose palette names colours.
        /// \param Value   The value written for the field.
        /// \param Output  The variable receiving the colour.
        /// \return `true` if the value is a colour, `false` otherwise.
        static Bool ParseColor(ConstRef<Sheet> Context, ConstRef<JsonValue> Value, Ref<Color> Output);

        /// \brief Reads a length written as a number of pixels, as a share of the room written as `50%`, or as `Auto`.
        ///
        /// \param Value  The value written for the field.
        /// \param Output The variable receiving the length.
        /// \return `true` if the value is a length, `false` otherwise.
        static Bool ParseLength(ConstRef<JsonValue> Value, Ref<Length> Output);

        /// \brief Reads the room on each side of a box, written as one, two or four numbers.
        ///
        /// \param Value  The value written for the field.
        /// \param Output The variable receiving the room.
        /// \return `true` if the value is room on each side, `false` otherwise.
        static Bool ParseEdges(ConstRef<JsonValue> Value, Ref<Edges> Output);

        /// \brief Reads a typeface written as the name of one the theme names, or as the address of one to load.
        ///
        /// \param Context The theme being read, whose typefaces are found by name and whose markup loads the others.
        /// \param Value   The value written for the field.
        /// \param Output  The variable receiving the typeface.
        /// \return `true` if the value is a typeface, `false` otherwise.
        static Bool ParseFont(Ref<Sheet> Context, ConstRef<JsonValue> Value, Ref<Retainer<ZyRender::Font>> Output);

        /// \brief Reads a pair of numbers, written `[x, y]`.
        ///
        /// \param Value  The value written for the field.
        /// \param Output The variable receiving the pair.
        /// \return `true` if the value is two numbers, `false` otherwise.
        static Bool ParsePair(ConstRef<JsonValue> Value, Ref<Vector2> Output);

        /// \brief Reads a scale written as one number for both axes, or as a pair of numbers.
        ///
        /// \param Value  The value written for the field.
        /// \param Output The variable receiving the scale.
        /// \return `true` if the value is a scale, `false` otherwise.
        static Bool ParseScale(ConstRef<JsonValue> Value, Ref<Vector2> Output);

        /// \brief Reads a number that cannot be below nought, as a time or a blur.
        ///
        /// \param Value  The value written for the field.
        /// \param Output The variable receiving the number.
        /// \return `true` if the value is a number of nought or more, `false` otherwise.
        static Bool ParseMeasure(ConstRef<JsonValue> Value, Ref<Real32> Output);

        /// \brief Reads a share of an animation's run, written as a number up to one or as a percentage.
        ///
        /// \param Value  The value written for the field.
        /// \param Output The variable receiving the share, over nought through one.
        /// \return `true` if the value is a share within the run, `false` otherwise.
        static Bool ParseShare(ConstRef<JsonValue> Value, Ref<Real32> Output);

        /// \brief Reads a gradient written as an object giving `From`, `To` and `Angle`, or as two colours.
        ///
        /// \param Context The theme being read, whose palette names colours.
        /// \param Value   The value written for the field.
        /// \param Output  The variable receiving the gradient.
        /// \return `true` if the value is a gradient, `false` otherwise.
        static Bool ParseGradient(ConstRef<Sheet> Context, ConstRef<JsonValue> Value, Ref<Gradient> Output);

        /// \brief Reads a shadow or a glow, written as an object giving any of `Color`, `Offset`, `Blur` and `Spread`.
        ///
        /// \param Context The theme being read, whose palette names colours.
        /// \param Value   The value written for the field.
        /// \param Output  The variable receiving the halo.
        /// \return `true` if the value is a halo, `false` otherwise.
        static Bool ParseHalo(ConstRef<Sheet> Context, ConstRef<JsonValue> Value, Ref<Halo> Output);

        /// \brief Reads a transition written as a number of seconds, or as an object of its fields.
        ///
        /// \param Value  The value written for the field.
        /// \param Output The variable receiving the transition.
        /// \return `true` if the value is a transition, `false` otherwise.
        static Bool ParseTransition(ConstRef<JsonValue> Value, Ref<Transition> Output);

        /// \brief Reads the name of an animation a skin plays, noting it so the theme is checked to define it.
        ///
        /// \param Context The theme being read, which notes the name.
        /// \param Where   The name of the style naming the animation.
        /// \param Key     The name of the field naming the animation.
        /// \param Value   The value written for the field, the name of the animation or empty for none.
        /// \param Output  The variable receiving the identifier of the animation, or zero for none.
        /// \return `true` if the value is a name, `false` otherwise.
        static Bool ParseAnimation(Ref<Sheet>          Context,
                                   Text                Where,
                                   Text                Key,
                                   ConstRef<JsonValue> Value,
                                   Ref<UInt64>         Output);

        /// \brief Reads the source of an image, written as a name the theme gives, an address to load, or empty.
        ///
        /// \param Context The theme being read, whose images are found by name and whose markup loads the others.
        /// \param Value   The value written for the field.
        /// \param Output  The variable receiving the image.
        /// \return `true` if the value is an image, `false` otherwise.
        static Bool ParseSource(Ref<Sheet> Context, ConstRef<JsonValue> Value, Ref<Retainer<ZyGraphic::Image>> Output);

        /// \brief Reads the part of an image shown, written `[u0, v0, u1, v1]` in texture coordinates.
        ///
        /// \param Value  The value written for the field.
        /// \param Output The variable receiving the part.
        /// \return `true` if the value is four numbers, `false` otherwise.
        static Bool ParseRegion(ConstRef<JsonValue> Value, Ref<Rect> Output);

        /// \brief Reads an image a skin lays over the box, written as its source or as an object of its fields.
        ///
        /// \param Context The theme being read, whose images and palette the value may name.
        /// \param Value   The value written for the field.
        /// \param Output  The variable receiving the values given.
        /// \param Bits    The variable receiving the bits of the values given.
        /// \return `true` if the value is an image, `false` otherwise.
        static Bool ParseImage(Ref<Sheet> Context, ConstRef<JsonValue> Value, Ref<Skin> Output, Ref<Skin::Field> Bits);

    private:

        /// The fields a shadow or a glow may give.
        static constexpr Array<Text, 4> kHaloFields { "Color", "Offset", "Blur", "Spread" };

        /// The fields a gradient may give.
        static constexpr Array<Text, 3> kGradientFields { "From", "To", "Angle" };

        /// The fields a transition may give.
        static constexpr Array<Text, 3> kTransitionFields { "Duration", "Delay", "Easing" };

        /// The fields an image written as an object may give.
        static constexpr Array<Text, 4> kImageFields { "Source", "Region", "Slice", "Tint" };

        /// \brief Reads a percentage written as text, as `50%`.
        ///
        /// \param Source The text, which must be a number followed by `%` and nothing else.
        /// \param Output The variable receiving the share it gives, a hundredth of the number.
        /// \return `true` if the whole of the text is a percentage, `false` otherwise.
        static Bool ParsePercent(Text Source, Ref<Real32> Output);

        /// \brief Reads an array holding exactly as many numbers as a buffer does.
        ///
        /// \param Value  The value written for the field.
        /// \param Output The variable receiving the numbers, in the order they are written.
        /// \return `true` if the value is an array of that many numbers, `false` otherwise.
        static Bool ParseNumbers(ConstRef<JsonValue> Value, Span<Real32> Output);

        /// \brief Checks whether an object gives only fields of some names, so a misspelt one is caught.
        ///
        /// \param Node  The object whose fields are checked.
        /// \param Names The names it may give.
        /// \return `true` if it gives no other, `false` otherwise.
        static Bool HasOnly(ConstRef<JsonValue> Node, ConstSpan<Text> Names);

        /// \brief Reads one field of an object, when the object gives it.
        ///
        /// \param Node   The object the field is looked up in.
        /// \param Key    The name of the field.
        /// \param Output The variable receiving the value, left untouched when the object does not give it.
        /// \param Read   The function reading the value.
        /// \return `true` if the object does not give it or gives a value it takes, `false` otherwise.
        template<typename Type, typename Function>
        static Bool Take(ConstRef<JsonValue> Node, Text Key, Ref<Type> Output, Function Read)
        {
            const ConstPtr<JsonValue> Value = Find(Node, Key);
            return !Value || Read(* Value, Output);
        }
    };
}