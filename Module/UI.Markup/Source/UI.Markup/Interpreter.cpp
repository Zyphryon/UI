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

#include "Interpreter.hpp"
#include "Markup.hpp"
#include "Parser.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interpreter::Dress(Ref<Markup>                                       Owner,
                            Ref<Theme>                                        Target,
                            ConstSpan<Text>                                   Layers,
                            UInt                                              Builtin,
                            ConstRef<Table<Digest, Retainer<ZyRender::Font>>> Fonts,
                            Ref<Str>                                          Error)
    {
        // Everything is read into a theme of its own, so a mistake in any layer leaves the one in use alone.
        Theme Fresh;
        Sheet Context;
        Context.Owner  = AddressOf(Owner);
        Context.Target = AddressOf(Fresh);
        Context.Fonts  = Fonts;

        Sequence<JsonValue> Documents(Layers.GetSize());

        for (UInt Index = 0; Index < Layers.GetSize() && Context.Error.IsEmpty(); ++Index)
        {
            Context.Origin = (Index < Builtin ? kBuiltin : Text());
            ParseLayer(Context, Layers[Index], Documents.Append());
        }

        // The names of every layer are read before any style, so a colour a later layer gives dresses every style.
        for (UInt Index = 0; Index < Documents.GetSize() && Context.Error.IsEmpty(); ++Index)
        {
            Context.Origin = (Index < Builtin ? kBuiltin : Text());
            ReadTable(Context, Documents[Index], "Colors", ReadColor);
            ReadTable(Context, Documents[Index], "Fonts",  ReadFont);
            ReadTable(Context, Documents[Index], "Images", ReadImage);
        }

        // A layer's animations replace those of their name, and its styles merge over those of theirs, field by field.
        for (UInt Index = 0; Index < Documents.GetSize() && Context.Error.IsEmpty(); ++Index)
        {
            Context.Origin = (Index < Builtin ? kBuiltin : Text());
            ReadTable(Context, Documents[Index], "Animations", ReadAnimation);
            ReadTable(Context, Documents[Index], "Styles",     ReadStyle);
        }

        // A skin may name an animation any layer defines, so the names are checked only once every layer is read.
        CheckAnimations(Context);

        if (!Context.Error.IsEmpty())
        {
            Error = Move(Context.Error);
            return false;
        }

        Target.Adopt(Fresh);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Interpreter::Read(Ref<Template> Target, Text Source, Ref<Str> Error)
    {
        JsonValue Document;

        if (!Parser::Parse(Source, Document, Error))
        {
            return false;
        }

        const ConstPtr<JsonValue> Templates = Parser::Find(Document, "Templates");

        if (!Templates || !Templates->IsObject())
        {
            Error.Format<"the layout gives no 'Templates' object">();
            return false;
        }

        // Every template is checked before any is kept, so a layout with a mistake in it leaves the ones in use alone.
        for (ConstRef<JsonValue::Object::Pair> Entry : Templates->GetObject())
        {
            if (!Entry.Second.IsObject())
            {
                Error.Format<"every template must be an object">();
                return false;
            }
        }

        Templates->GetObject().ForEach([&](ConstRef<Str> Key, ConstRef<JsonValue> Value)
        {
            Target.Assign(Identify(Key), Value);
        });
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interpreter::Fail(Ref<Sheet> Context, Text Where, Text Key, Text Why)
    {
        if (Context.Error.IsEmpty())
        {
            Context.Error.Format<"{0}'{1}', field '{2}': {3}">(Context.Origin, Where, Key, Why);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interpreter::ParseLayer(Ref<Sheet> Context, Text Source, Ref<JsonValue> Output)
    {
        if (Str Why; !Parser::Parse(Source, Output, Why))
        {
            Context.Error.Format<"{0}{1}">(Context.Origin, Why);
            return;
        }

        for (const Text Table : kTables)
        {
            if (const ConstPtr<JsonValue> Found = Parser::Find(Output, Table); Found && !Found->IsObject())
            {
                Context.Error.Format<"{0}'Colors', 'Fonts', 'Images', 'Styles' and 'Animations' must be objects">(
                    Context.Origin);
                return;
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interpreter::ReadTable(Ref<Sheet>          Context,
                                ConstRef<JsonValue> Document,
                                Text                Table,
                                void (* Reader)(Ref<Sheet>, Text, ConstRef<JsonValue>))
    {
        if (const ConstPtr<JsonValue> Found = Parser::Find(Document, Table))
        {
            Found->GetObject().ForEach([&](ConstRef<Str> Key, ConstRef<JsonValue> Value)
            {
                Reader(Context, Key, Value);
            });
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interpreter::ReadField(Ref<Sheet> Context, Text Where, Text Key, ConstRef<JsonValue> Value, Ref<Skin> Output)
    {
        Bool        Valid = false;
        Skin::Field Bit   = Skin::Field::None;

        switch (Identify(Key))
        {
        case Identify("Base"):
        case Identify("When"):
        case Identify("Sounds"):
            return;
        case Identify("Image"):
            Valid = Parser::ParseImage(Context, Value, Output, Bit);
            break;
        case Identify("Background"):
            Valid = Parser::ParseColor(Context, Value, Output.Background);
            Bit   = Skin::Field::Background;
            break;
        case Identify("Border"):
            Valid = Parser::ParseColor(Context, Value, Output.Border);
            Bit   = Skin::Field::Border;
            break;
        case Identify("BorderWidth"):
            Valid = Parser::ParseNumber(Value, Output.BorderWidth);
            Bit   = Skin::Field::BorderWidth;
            break;
        case Identify("Radius"):
            Valid = Parser::ParseNumber(Value, Output.Radius);
            Bit   = Skin::Field::Radius;
            break;
        case Identify("Opacity"):
            Valid = Parser::ParseNumber(Value, Output.Opacity);
            Bit   = Skin::Field::Opacity;
            break;
        case Identify("Tint"):
            Valid = Parser::ParseColor(Context, Value, Output.Tint);
            Bit   = Skin::Field::Tint;
            break;
        case Identify("Size"):
            Valid = Parser::ParseNumber(Value, Output.Size);
            Bit   = Skin::Field::Size;
            break;
        case Identify("Font"):
            Valid = Parser::ParseFont(Context, Value, Output.Font);
            Bit   = Skin::Field::Font;
            break;
        case Identify("Stroke"):
            Valid = Parser::ParseColor(Context, Value, Output.Stroke);
            Bit   = Skin::Field::Stroke;
            break;
        case Identify("StrokeWidth"):
            Valid = Parser::ParseNumber(Value, Output.StrokeWidth);
            Bit   = Skin::Field::StrokeWidth;
            break;
        case Identify("Padding"):
            Valid = Parser::ParseEdges(Value, Output.Padding);
            Bit   = Skin::Field::Padding;
            break;
        case Identify("Width"):
            Valid = Parser::ParseLength(Value, Output.Width);
            Bit   = Skin::Field::Width;
            break;
        case Identify("Height"):
            Valid = Parser::ParseLength(Value, Output.Height);
            Bit   = Skin::Field::Height;
            break;
        case Identify("Fill"):
            Valid = Parser::ParseColor(Context, Value, Output.Fill);
            Bit   = Skin::Field::Fill;
            break;
        case Identify("Thumb"):
            Valid = Parser::ParseColor(Context, Value, Output.Thumb);
            Bit   = Skin::Field::Thumb;
            break;
        case Identify("Alignment"):
            Valid = Parser::ParseEnum(Value, Output.Alignment);
            Bit   = Skin::Field::Alignment;
            break;
        case Identify("Gap"):
            Valid = Parser::ParseNumber(Value, Output.Gap);
            Bit   = Skin::Field::Gap;
            break;
        case Identify("Gradient"):
            Valid = Parser::ParseGradient(Context, Value, Output.Gradient);
            Bit   = Skin::Field::Gradient;
            break;
        case Identify("Shadow"):
            Valid = Parser::ParseHalo(Context, Value, Output.Shadow);
            Bit   = Skin::Field::Shadow;
            break;
        case Identify("Glow"):
            Valid = Parser::ParseHalo(Context, Value, Output.Glow);
            Bit   = Skin::Field::Glow;
            break;
        case Identify("InnerGlow"):
            Valid = Parser::ParseHalo(Context, Value, Output.InnerGlow);
            Bit   = Skin::Field::InnerGlow;
            break;
        case Identify("Scale"):
            Valid = Parser::ParseScale(Value, Output.Scale);
            Bit   = Skin::Field::Scale;
            break;
        case Identify("Rotate"):
            Valid = Parser::ParseNumber(Value, Output.Rotate);
            Bit   = Skin::Field::Rotate;
            break;
        case Identify("Translate"):
            Valid = Parser::ParsePair(Value, Output.Translate);
            Bit   = Skin::Field::Translate;
            break;
        case Identify("Transition"):
            Valid = Parser::ParseTransition(Value, Output.Transition);
            Bit   = Skin::Field::Transition;
            break;
        case Identify("Animation"):
            Valid = Parser::ParseAnimation(Context, Where, Key, Value, Output.Animation);
            Bit   = Skin::Field::Animation;
            break;
        case Identify("Enter"):
            Valid = Parser::ParseAnimation(Context, Where, Key, Value, Output.Enter);
            Bit   = Skin::Field::Enter;
            break;
        case Identify("Exit"):
            Valid = Parser::ParseAnimation(Context, Where, Key, Value, Output.Exit);
            Bit   = Skin::Field::Exit;
            break;
        default:
            Fail(Context, Where, Key, "no skin has such a field");
            return;
        }

        if (!Valid)
        {
            Fail(Context, Where, Key, "the value is not one the field takes");
            return;
        }
        Output.Mask |= Bit;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interpreter::ReadSkin(Ref<Sheet> Context, Text Where, ConstRef<JsonValue> Node, Ref<Skin> Output)
    {
        Node.GetObject().ForEach([&](ConstRef<Str> Key, ConstRef<JsonValue> Value)
        {
            ReadField(Context, Where, Key, Value, Output);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interpreter::ReadVariant(Ref<Sheet>          Context,
                                  Text                Where,
                                  Text                Key,
                                  ConstRef<JsonValue> Value,
                                  Ref<Style>          Target)
    {
        State Bits = State::None;

        if (!Parser::ParseStates(Key, Bits))
        {
            Fail(Context, Where, Key, "the variant names a state there is not");
            return;
        }

        if (!Value.IsObject())
        {
            Fail(Context, Where, Key, "the variant is not an object");
            return;
        }

        // A sound belongs to the moment it plays at, never to a state.
        if (Parser::Find(Value, "Sounds"))
        {
            Fail(Context, Where, "Sounds", "a variant cannot give sounds, which the style gives");
            return;
        }
        ReadSkin(Context, Where, Value, Target.When(Bits));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interpreter::ReadSound(Ref<Sheet> Context, Text Where, Text Key, ConstRef<JsonValue> Value, Ref<Style> Target)
    {
        constexpr Cue kNone = static_cast<Cue>(0xFF);

        const Cue Moment = ZyEnum::Cast(Key, kNone);

        if (Moment == kNone)
        {
            Fail(Context, Where, Key, "no moment of interaction has such a name");
            return;
        }

        if (!Value.IsString())
        {
            Fail(Context, Where, Key, "a sound is the address of one, or empty for none");
            return;
        }
        Target.SetSound(Moment, Value.GetString());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interpreter::ReadStyle(Ref<Sheet> Context, Text Key, ConstRef<JsonValue> Value)
    {
        if (!Value.IsObject())
        {
            Fail(Context, "Styles", Key, "the style is not an object");
            return;
        }

        // Read on its own, so it gives only what the layer writes when it is merged over the style a layer before gave.
        Style Found;
        Found.Base = Identify(Markup::GetText(Value, "Base"));
        ReadSkin(Context, Key, Value, Found.Normal);

        if (const ConstPtr<JsonValue> Variants = Parser::Find(Value, "When"); Variants && !Variants->IsObject())
        {
            Fail(Context, Key, "When", "the variants are not an object");
        }
        else if (Variants)
        {
            Variants->GetObject().ForEach([&](ConstRef<Str> When, ConstRef<JsonValue> Variant)
            {
                ReadVariant(Context, Key, When, Variant, Found);
            });
        }

        if (const ConstPtr<JsonValue> Sounds = Parser::Find(Value, "Sounds"); Sounds && !Sounds->IsObject())
        {
            Fail(Context, Key, "Sounds", "the sounds are not an object");
        }
        else if (Sounds)
        {
            Sounds->GetObject().ForEach([&](ConstRef<Str> Moment, ConstRef<JsonValue> Entry)
            {
                ReadSound(Context, Key, Moment, Entry, Found);
            });
        }
        Context.Target->Merge(Key, Found);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interpreter::ReadColor(Ref<Sheet> Context, Text Key, ConstRef<JsonValue> Value)
    {
        if (Color Found; Parser::ParseColor(Context, Value, Found))
        {
            Context.Colors.Assign(Digest(Identify(Key)), Found);
        }
        else
        {
            Fail(Context, "Colors", Key, "the value is not a colour");
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interpreter::ReadFont(Ref<Sheet> Context, Text Key, ConstRef<JsonValue> Value)
    {
        const Digest Name = Digest(Identify(Key));

        if (Value.IsString())
        {
            Context.Fonts.Assign(Name, Context.Owner->LoadFont(Value.GetString()));
        }
        else if (Value.IsNull())
        {
            // A slot holds none until code or a later layer fills it, and never empties one filled already.
            Context.Fonts.FindOrInsert(Name);
        }
        else
        {
            Fail(Context, "Fonts", Key, "the value is not the address of a typeface, or null for a slot");
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interpreter::ReadImage(Ref<Sheet> Context, Text Key, ConstRef<JsonValue> Value)
    {
        if (Value.IsString() && !Value.GetString().IsEmpty())
        {
            Context.Images.Assign(Digest(Identify(Key)), Context.Owner->LoadImage(Value.GetString()));
        }
        else
        {
            Fail(Context, "Images", Key, "the value is not the address of an image");
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interpreter::ReadKeyField(Ref<Sheet>          Context,
                                   Text                Where,
                                   Text                Key,
                                   ConstRef<JsonValue> Value,
                                   Ref<Skin>           Output)
    {
        switch (Identify(Key))
        {
        case Identify("At"):
            return;
        case Identify("Base"):
        case Identify("When"):
        case Identify("Sounds"):
        case Identify("Transition"):
        case Identify("Animation"):
        case Identify("Enter"):
        case Identify("Exit"):
            Fail(Context, Where, Key, "a key of an animation cannot give this field");
            return;
        default:
            ReadField(Context, Where, Key, Value, Output);
            return;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interpreter::ReadKey(Ref<Sheet> Context, Text Where, ConstRef<JsonValue> Node, Ref<Animation> Target)
    {
        if (!Node.IsObject())
        {
            Fail(Context, Where, "Keys", "a key is not an object");
            return;
        }

        Real32 Share = 0.0f;

        if (const ConstPtr<JsonValue> At = Parser::Find(Node, "At"); !At || !Parser::ParseShare(* At, Share))
        {
            Fail(Context, Where, "At", "a key must stand at a share over nought through one, as 0.5 or \"50%\"");
            return;
        }

        Ref<Skin> Values = Target.At(Share);
        Node.GetObject().ForEach([&](ConstRef<Str> Key, ConstRef<JsonValue> Value)
        {
            ReadKeyField(Context, Where, Key, Value, Values);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interpreter::ReadAnimationField(Ref<Sheet>          Context,
                                         Text                Where,
                                         Text                Key,
                                         ConstRef<JsonValue> Value,
                                         Ref<Animation>      Target)
    {
        Bool Valid = false;

        switch (Identify(Key))
        {
        case Identify("Duration"):
            Valid = Parser::ParseMeasure(Value, Target.Duration) && Target.Duration > 0.0f;
            break;
        case Identify("Repeat"):
            if (Real32 Count = 0.0f; Parser::ParseMeasure(Value, Count) && Count == Floor(Count))
            {
                Target.Repeat = static_cast<UInt32>(Count);
                Valid         = true;
            }
            break;
        case Identify("Alternate"):
            Valid = Parser::ParseBool(Value, Target.Alternate);
            break;
        case Identify("Easing"):
            Valid = Parser::ParseEnum(Value, Target.Curve);
            break;
        case Identify("Keys"):
            if (Value.IsArray())
            {
                for (ConstRef<JsonValue> Entry : Value.GetArray())
                {
                    ReadKey(Context, Where, Entry, Target);
                }
                Valid = true;
            }
            break;
        default:
            Fail(Context, Where, Key, "no animation has such a field");
            return;
        }

        if (!Valid)
        {
            Fail(Context, Where, Key, "the value is not one the field takes");
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interpreter::ReadAnimation(Ref<Sheet> Context, Text Key, ConstRef<JsonValue> Value)
    {
        if (!Value.IsObject())
        {
            Fail(Context, "Animations", Key, "the animation is not an object");
            return;
        }

        Ref<Animation> Found = Context.Target->Animate(Key);
        Value.GetObject().ForEach([&](ConstRef<Str> Field, ConstRef<JsonValue> Entry)
        {
            ReadAnimationField(Context, Key, Field, Entry, Found);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Interpreter::CheckAnimations(Ref<Sheet> Context)
    {
        for (ConstRef<Sheet::Reference> Entry : Context.Animations)
        {
            if (!Context.Target->FindAnimation(Identify(Entry.Name)))
            {
                // The mistake belongs to the layer naming the animation, not the one read last.
                Context.Origin = Entry.Origin;
                Fail(Context, Entry.Where, Entry.Key, Str::Print<"no animation is named '{0}'">(Entry.Name));
            }
        }
    }
}