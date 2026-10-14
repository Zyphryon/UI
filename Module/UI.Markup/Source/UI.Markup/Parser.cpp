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

#include "Markup.hpp"
#include "Parser.hpp"
#include "UI.Core/Layers.hpp"
#include "UI.Core/States.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<JsonValue> Parser::Find(ConstRef<JsonValue> Node, Text Key)
    {
        return Node.IsObject() ? Node.GetObject().Find(Key) : nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::Parse(Text Source, Ref<JsonValue> Output, Ref<Str> Error)
    {
        if (JsonError Problem; !JsonDocument::Parse(Source, Output, Problem))
        {
            Error.Format<"{0}">(Problem);
            return false;
        }

        if (!Output.IsObject())
        {
            Error.Format<"the document is not an object">();
            return false;
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseNumber(ConstRef<JsonValue> Value, Ref<Real32> Output)
    {
        if (!Value.IsNumber())
        {
            return false;
        }
        Output = Value.GetNumber<Real32>();
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseBool(ConstRef<JsonValue> Value, Ref<Bool> Output)
    {
        if (!Value.IsBool())
        {
            return false;
        }
        Output = Value.GetBool();
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseLayer(ConstRef<JsonValue> Value, Ref<Layer> Output)
    {
        return Value.IsString() && Layers::Find(Value.GetString(), Output);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseStates(Text Source, Ref<State> Output)
    {
        // A split makes no word of an empty text and drops a trailing empty one, and neither is a state.
        if (Source.IsEmpty() || StrEndsWith(Source, "+"))
        {
            return false;
        }

        State Result = State::None;
        Bool  Valid  = true;

        StrSplit(Source, '+', [&](Text Word)
        {
            State Bit = State::None;
            Valid     = States::Find(Word, Bit);
            Result   |= Bit;
            return Valid;
        });

        if (Valid)
        {
            Output = Result;
        }
        return Valid;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseColor(ConstRef<Sheet> Context, ConstRef<JsonValue> Value, Ref<Color> Output)
    {
        if (Value.IsArray())
        {
            ConstRef<JsonValue::Array> List = Value.GetArray();

            // A colour and an alpha of its own, as ["Gold", 0.18], the alpha replacing any the colour has.
            if (List.GetSize() == 2 && List[0].IsString())
            {
                Color  Shade;
                Real32 Alpha;

                if (!ParseColor(Context, List[0], Shade) || !ParseNumber(List[1], Alpha))
                {
                    return false;
                }
                Output = Color::WithAlpha(Shade, Alpha);
                return true;
            }

            Array Channels(0.0f, 0.0f, 0.0f, 1.0f);

            const UInt Count = List.GetSize();

            if (Count < 3 || Count > 4 || !ParseNumbers(Value, Span(Channels.GetData(), Count)))
            {
                return false;
            }
            Output = Color(Channels[0], Channels[1], Channels[2], Channels[3]);
            return true;
        }

        if (!Value.IsString())
        {
            return false;
        }

        const Text Source = Value.GetString();

        if (StrStartsWith(Source, "#"))
        {
            UInt       Cursor = 1;
            const Bool Sized  = Source.GetSize() == 7 || Source.GetSize() == 9;

            if (!Sized || StrSkip(Source, Cursor, StrIsHexDigit) != Source.GetSize() - 1)
            {
                return false;
            }
            Output = Color::FromHexadecimal(Source);
            return true;
        }

        if (const ConstPtr<Color> Named = Context.Colors.Find(Digest(Identify(Source))))
        {
            Output = * Named;
            return true;
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseLength(ConstRef<JsonValue> Value, Ref<Length> Output)
    {
        if (Value.IsNumber())
        {
            Output = Length::Pixels(Value.GetNumber<Real32>());
            return true;
        }

        if (!Value.IsString())
        {
            return false;
        }

        if (StrEqualCaseInsensitive(Value.GetString(), "Auto"))
        {
            Output = Length::Auto();
            return true;
        }
        
        const Text Content = StrTrim(Value.GetString());

        if (Content.GetSize() >= 3 && StrEqualCaseInsensitive(Content.Slice(0, 3), "Fit"))
        {
            const Text Most = StrTrim(Content.Slice(3));

            if (Most.IsEmpty())
            {
                Output = Length::Auto();
                return true;
            }

            UInt         Cursor  = 0;
            const Real32 Ceiling = StrExtractNumber<Real32>(Most, Cursor);

            if (Cursor != Most.GetSize() || Ceiling <= 0.0f)
            {
                return false;
            }
            Output = Length::Fit(Ceiling);
            return true;
        }

        if (Real32 Share; ParsePercent(Value.GetString(), Share))
        {
            Output = Length::Percent(Share);
            return true;
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseEdges(ConstRef<JsonValue> Value, Ref<Edges> Output)
    {
        if (Value.IsNumber())
        {
            Output = Edges(Value.GetNumber<Real32>());
            return true;
        }

        if (!Value.IsArray())
        {
            return false;
        }

        const UInt       Size = Value.GetArray().GetSize();
        Array<Real32, 4> Sides;

        if ((Size != 1 && Size != 2 && Size != 4) || !ParseNumbers(Value, Span(Sides.GetData(), Size)))
        {
            return false;
        }

        switch (Size)
        {
        case 1:
            Output = Edges(Sides[0]);
            break;
        case 2:
            Output = Edges(Sides[0], Sides[1], Sides[0], Sides[1]);
            break;
        default:
            Output = Edges(Sides[0], Sides[1], Sides[2], Sides[3]);
            break;
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseFont(Ref<Sheet> Context, ConstRef<JsonValue> Value, Ref<Retainer<ZyRender::Font>> Output)
    {
        if (!Value.IsString())
        {
            return false;
        }

        const Text Source = Value.GetString();

        if (const ConstPtr<Retainer<ZyRender::Font>> Named = Context.Fonts.Find(Digest(Identify(Source))))
        {
            Output = * Named;
            return true;
        }

        if (!StrContains(Source, "://"))
        {
            return false;
        }
        Output = Context.Owner->LoadFont(Source);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParsePair(ConstRef<JsonValue> Value, Ref<Vector2> Output)
    {
        Array<Real32, 2> Axes;

        if (!ParseNumbers(Value, Axes))
        {
            return false;
        }
        Output = Vector2(Axes[0], Axes[1]);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseScale(ConstRef<JsonValue> Value, Ref<Vector2> Output)
    {
        if (Value.IsNumber())
        {
            Output = Vector2(Value.GetNumber<Real32>());
            return true;
        }
        return ParsePair(Value, Output);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseMeasure(ConstRef<JsonValue> Value, Ref<Real32> Output)
    {
        Real32 Number = 0.0f;

        if (!ParseNumber(Value, Number) || Number < 0.0f)
        {
            return false;
        }
        Output = Number;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseShare(ConstRef<JsonValue> Value, Ref<Real32> Output)
    {
        Real32 Share = -1.0f;

        if (Value.IsNumber())
        {
            Share = Value.GetNumber<Real32>();
        }
        else if (Value.IsString() && !ParsePercent(Value.GetString(), Share))
        {
            return false;
        }

        if (Share < 0.0f || Share > 1.0f)
        {
            return false;
        }
        Output = Share;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseGradient(ConstRef<Sheet> Context, ConstRef<JsonValue> Value, Ref<Gradient> Output)
    {
        Gradient Result;

        if (Value.IsArray())
        {
            ConstRef<JsonValue::Array> List = Value.GetArray();

            const Bool Pair = List.GetSize() == 2;

            if (!Pair || !ParseColor(Context, List[0], Result.From) || !ParseColor(Context, List[1], Result.To))
            {
                return false;
            }
        }
        else
        {
            const ConstPtr<JsonValue> From = Find(Value, "From");
            const ConstPtr<JsonValue> To   = Find(Value, "To");

            if (!Value.IsObject() || !HasOnly(Value, kGradientFields) || !From || !To
                || !ParseColor(Context, * From, Result.From) || !ParseColor(Context, * To, Result.To)
                || !Take(Value, "Angle", Result.Angle, ParseNumber))
            {
                return false;
            }
        }
        Output = Result;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseHalo(ConstRef<Sheet> Context, ConstRef<JsonValue> Value, Ref<Halo> Output)
    {
        if (!Value.IsObject() || !HasOnly(Value, kHaloFields))
        {
            return false;
        }

        Halo                      Result;
        const ConstPtr<JsonValue> Tint = Find(Value, "Color");

        Bool Failed = Tint && !ParseColor(Context, * Tint, Result.Tint);
        Failed      = Failed || !Take(Value, "Offset", Result.Offset, ParsePair);
        Failed      = Failed || !Take(Value, "Blur", Result.Blur, ParseMeasure);
        Failed      = Failed || !Take(Value, "Spread", Result.Spread, ParseNumber);

        if (Failed)
        {
            return false;
        }
        Output = Result;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseTransition(ConstRef<JsonValue> Value, Ref<Transition> Output)
    {
        Transition Result;

        if (Value.IsNumber())
        {
            if (!ParseMeasure(Value, Result.Duration))
            {
                return false;
            }
        }
        else if (!Value.IsObject() || !HasOnly(Value, kTransitionFields)
            || !Take(Value, "Duration", Result.Duration, ParseMeasure)
            || !Take(Value, "Delay", Result.Delay, ParseMeasure)
            || !Take(Value, "Easing", Result.Curve, ParseEnum<Easing>))
        {
            return false;
        }
        Output = Result;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseAnimation(Ref<Sheet>          Context,
                                Text                Where,
                                Text                Key,
                                ConstRef<JsonValue> Value,
                                Ref<UInt64>         Output)
    {
        if (!Value.IsString())
        {
            return false;
        }

        const Text Name = Value.GetString();

        if (!Name.IsEmpty())
        {
            Context.Animations.Append(Str(Where), Str(Key), Str(Name), Context.Origin);
        }
        Output = Identify(Name);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseSource(Ref<Sheet> Context, ConstRef<JsonValue> Value, Ref<Retainer<ZyGraphic::Image>> Output)
    {
        if (!Value.IsString())
        {
            return false;
        }

        const Text Source = Value.GetString();

        if (Source.IsEmpty())
        {
            Output = Retainer<ZyGraphic::Image>();
            return true;
        }

        if (const ConstPtr<Retainer<ZyGraphic::Image>> Named = Context.Images.Find(Digest(Identify(Source))))
        {
            Output = * Named;
            return true;
        }

        if (!StrContains(Source, "://"))
        {
            return false;
        }
        Output = Context.Owner->LoadImage(Source);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseRegion(ConstRef<JsonValue> Value, Ref<Rect> Output)
    {
        Array<Real32, 4> Corners;

        if (!ParseNumbers(Value, Corners))
        {
            return false;
        }
        Output = Rect(Corners[0], Corners[1], Corners[2], Corners[3]);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseImage(Ref<Sheet> Context, ConstRef<JsonValue> Value, Ref<Skin> Output, Ref<Skin::Field> Bits)
    {
        if (Value.IsString())
        {
            Bits = Skin::Field::Image;
            return ParseSource(Context, Value, Output.Image);
        }

        if (!Value.IsObject() || !HasOnly(Value, kImageFields))
        {
            return false;
        }

        const ConstPtr<JsonValue> Source = Find(Value, "Source");
        const ConstPtr<JsonValue> Region = Find(Value, "Region");
        const ConstPtr<JsonValue> Slice  = Find(Value, "Slice");
        const ConstPtr<JsonValue> Tint   = Find(Value, "Tint");

        Bool Failed = Source && !ParseSource(Context, * Source, Output.Image);
        Failed      = Failed || (Region && !ParseRegion(* Region, Output.Region));
        Failed      = Failed || (Slice && !ParseEdges(* Slice, Output.Slice));
        Failed      = Failed || (Tint && !ParseColor(Context, * Tint, Output.ImageTint));

        if (Failed)
        {
            return false;
        }

        Bits = SetOrClearBit(Skin::Field::None, Skin::Field::Image,     Source != nullptr);
        Bits = SetOrClearBit(Bits,              Skin::Field::Region,    Region != nullptr);
        Bits = SetOrClearBit(Bits,              Skin::Field::Slice,     Slice  != nullptr);
        Bits = SetOrClearBit(Bits,              Skin::Field::ImageTint, Tint   != nullptr);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParsePercent(Text Source, Ref<Real32> Output)
    {
        if (Source.GetSize() < 2 || !StrEndsWith(Source, "%"))
        {
            return false;
        }

        UInt         Cursor = 0;
        const Text   Digits = Source.Slice(0, Source.GetSize() - 1);
        const Real32 Number = StrExtractNumber<Real32>(Digits, Cursor);

        if (Cursor == 0 || Cursor != Digits.GetSize())
        {
            return false;
        }
        Output = Number / 100.0f;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::ParseNumbers(ConstRef<JsonValue> Value, Span<Real32> Output)
    {
        if (!Value.IsArray() || Value.GetArray().GetSize() != Output.GetSize())
        {
            return false;
        }

        for (UInt Index = 0; Index < Output.GetSize(); ++Index)
        {
            if (!ParseNumber(Value.GetArray()[Index], Output[Index]))
            {
                return false;
            }
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Parser::HasOnly(ConstRef<JsonValue> Node, ConstSpan<Text> Names)
    {
        for (ConstRef<JsonValue::Object::Pair> Entry : Node.GetObject())
        {
            if (!Names.Contains(Entry.First))
            {
                return false;
            }
        }
        return true;
    }
}