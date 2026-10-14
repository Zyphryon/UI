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

#include "Image.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Facet/Visual/Picture.hpp"
#include "UI.Core/Paint/Canvas.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Binder.hpp"
#include "UI.Data/Datum.hpp"
#include "UI.Data/Link.hpp"
#include "UI.Markup/Markup.hpp"
#include "UI.Markup/Parser.hpp"
#include "UI.Markup/Sheet.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Image::Create(Element Parent, Retainer<ZyGraphic::Image> Source, Text Style)
    {
        const Element Result = Parent.Create();
        Result.Add<Picture>().Source = Move(Source);
        Result.SetStyle(Style);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Image::CreateView(Element Parent, ZyGraphic::Object Texture, ZyGraphic::TextureLayout Layout, Text Style)
    {
        const Element Result = Parent.Create();

        Ref<Picture> Shown = Result.Add<Picture>();
        Shown.Texture = Texture;
        Shown.Layout  = Layout;

        Result.SetStyle(Style);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Image::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        // An address bound to a value or to the lexicon names no picture until the binding shows it.
        const Text Address = Markup::GetText(Node, "Source");
        const Bool Deferred = Binder::IsBound(Address) || Binder::IsLocalized(Address);

        const Element Result = Create(Parent, Deferred ? Retainer<ZyGraphic::Image>() : Reader.LoadImage(Address),
            Markup::GetText(Node, "Style"));

        Ref<Picture> Shown = * Result.Get<Picture>();

        const ConstPtr<JsonValue> Region = Markup::GetField(Node, "Region");

        const Bool Written = Region && !Binder::IsBound(Markup::GetText(Node, "Region"));

        if (Written && !Parser::ParseRegion(* Region, Shown.Region))
        {
            LOG_W("An image gives 'Region' a value it cannot take, so it shows the whole picture");
        }

        // A colour written out reads as a theme's would, though a layout names no palette.
        const ConstPtr<JsonValue> Tint = Markup::GetField(Node, "Tint");

        if (Tint && !Binder::IsBound(Markup::GetText(Node, "Tint")))
        {
            Sheet Plain;

            if (!Parser::ParseColor(Plain, * Tint, Shown.Tint))
            {
                LOG_W("An image gives 'Tint' a value it cannot take, so it shows the picture as it is");
            }
        }

        const Real32 Layer = Markup::GetNumber(Node, "Layer", 0.0f);

        Shown.Layer  = static_cast<UInt16>(Clamp(Layer, 0.0f, 65535.0f));
        Shown.Filter = ZyEnum::Cast(Markup::GetText(Node, "Filter"), Canvas::Filter::Linear);
        Shown.Fit    = ZyEnum::Cast(Markup::GetText(Node, "Fit"), Fit::Stretch);
        Shown.Opaque = Markup::GetBool(Node, "Opaque");
        Shown.Flip   = Markup::GetBool(Node, "Flip");

        // The markup outlives every element it builds, so a binding may load through it later.
        const Ptr<Markup> Loader = AddressOf(Reader);

        Reader.Bind(Result, Node, "Source", Writer([Loader](Element Target, ConstRef<Datum> Value)
        {
            SetAddress(Target, * Loader, Value.GetText());
        }));

        // A bound region is a text of four numbers, as `0 0 0.5 1`, so a row of a list shows its own frame of a sheet.
        Reader.Bind(Result, Node, "Region", Writer([](Element Target, ConstRef<Datum> Value)
        {
            const Text Content = Value.GetText();
            Array      Corners(0.0f, 0.0f, 1.0f, 1.0f);
            UInt       Cursor  = 0;

            for (Ref<Real32> Corner : Corners)
            {
                while (Cursor < Content.GetSize() && (StrIsSpace(Content[Cursor]) || Content[Cursor] == ','))
                {
                    ++Cursor;
                }
                Corner = StrExtractNumber<Real32>(Content, Cursor);
            }
            SetRegion(Target, Rect(Corners[0], Corners[1], Corners[2], Corners[3]));
        }));
        Reader.Bind(Result, Node, "Layer", Binder::Put<UInt32, &Image::SetLayer>());
        Reader.Bind(Result, Node, "Tint", Binder::Put<Color, &Image::SetTint>());

        // A list mixing art whose alpha is coverage with art whose alpha carries data, as a height, picks per row.
        Reader.Bind(Result, Node, "Opaque", Binder::Put<Bool, &Image::SetOpaque>());
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Image::SetSource(Element Target, Retainer<ZyGraphic::Image> Source)
    {
        // A new picture may be of another size, so the image is measured again.
        if (const Ptr<Picture> Shown = Target.Get<Picture>(); Shown && Shown->Source != Source)
        {
            Shown->Source = Move(Source);
            Target.GetTree().Mark(Target.GetHandle(), Picture::kMarks);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Image::SetAddress(Element Target, ConstRef<Markup> Reader, Text Address)
    {
        const ConstPtr<Picture> Shown = Target.Get<Picture>();

        if (!Shown)
        {
            return;
        }

        // The shown picture is held against the address first, so a row moved to a same-picture record loads nothing.
        const Bool Held = Shown->Source ? Shown->Source->GetKey().GetUrl() == Address : Address.IsEmpty();

        if (!Held)
        {
            SetSource(Target, Reader.LoadImage(Address));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Image::SetTexture(Element Target, ZyGraphic::Object Texture, ZyGraphic::TextureLayout Layout)
    {
        // A target keeps no size of its own to measure, so a new one only draws differently.
        const Ptr<Picture> Shown = Target.Get<Picture>();

        if (Shown && (Shown->Texture != Texture || Shown->Layout != Layout))
        {
            Shown->Texture = Texture;
            Shown->Layout  = Layout;
            Target.GetTree().Mark(Target.GetHandle(), Dirty::Paint);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Image::SetLayer(Element Target, UInt32 Layer)
    {
        // A bound value holding none reads as the largest number, which shows the first slice.
        const UInt16 Slice = (Layer == kMaximum<UInt32>) ? 0 : static_cast<UInt16>(Min(Layer, 0xFFFFu));

        if (const Ptr<Picture> Shown = Target.Get<Picture>(); Shown && Shown->Layer != Slice)
        {
            Shown->Layer = Slice;
            Target.GetTree().Mark(Target.GetHandle(), Dirty::Paint);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Image::SetRegion(Element Target, Rect Region)
    {
        // The image asks for the pixels of the part it shows, so another part measures it again.
        if (const Ptr<Picture> Shown = Target.Get<Picture>(); Shown && Shown->Region != Region)
        {
            Shown->Region = Region;
            Target.GetTree().Mark(Target.GetHandle(), Picture::kMarks);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Image::SetTint(Element Target, Color Tint)
    {
        if (const Ptr<Picture> Shown = Target.Get<Picture>(); Shown && Shown->Tint != Tint)
        {
            Shown->Tint = Tint;
            Target.GetTree().Mark(Target.GetHandle(), Dirty::Paint);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Image::SetFilter(Element Target, Canvas::Filter Filter)
    {
        if (const Ptr<Picture> Shown = Target.Get<Picture>(); Shown && Shown->Filter != Filter)
        {
            Shown->Filter = Filter;
            Target.GetTree().Mark(Target.GetHandle(), Dirty::Paint);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Image::SetFit(Element Target, Fit Mode)
    {
        if (const Ptr<Picture> Shown = Target.Get<Picture>(); Shown && Shown->Fit != Mode)
        {
            Shown->Fit = Mode;
            Target.GetTree().Mark(Target.GetHandle(), Dirty::Paint);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Image::SetOpaque(Element Target, Bool Opaque)
    {
        if (const Ptr<Picture> Shown = Target.Get<Picture>(); Shown && Shown->Opaque != Opaque)
        {
            Shown->Opaque = Opaque;
            Target.GetTree().Mark(Target.GetHandle(), Dirty::Paint);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Image::SetFlip(Element Target, Bool Flip)
    {
        if (const Ptr<Picture> Shown = Target.Get<Picture>(); Shown && Shown->Flip != Flip)
        {
            Shown->Flip = Flip;
            Target.GetTree().Mark(Target.GetHandle(), Dirty::Paint);
        }
    }
}