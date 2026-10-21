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

#include "Swatch.hpp"
#include "Panel.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Paint/Canvas.hpp"
#include "UI.Core/System/Painter.hpp"
#include "UI.Data/Binder.hpp"
#include "UI.Markup/Markup.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Swatch::Create(Element Parent, Color Value, Text Style)
    {
        const Element Result = Panel::Create(Parent, Style);
        Result.Add<Checker>().Value = Value;
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Swatch::Build(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node)
    {
        const Text  Written = Markup::GetText(Node, "Value");
        const Color Value   = StrStartsWith(Written, "#") ? Color::FromHexadecimal(Written) : Color::White();

        const Element Result = Create(Parent, Value, Markup::GetText(Node, "Style", "Swatch"));
        Reader.Bind(Result, Node, "Value", Binder::Put<Color, &Swatch::SetValue>());
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Swatch::Declare(Ref<Interface> Target)
    {
        if (Target.Find<Checker>())
        {
            return;
        }
        Target.Provide<Checker>();

        // The checker marks the boards drawn, between the shadow and fill so a colour's transparency shows.
        const Ptr<Interface> Owner = AddressOf(Target);

        Target.GetPainter().AddDrawer<Checker>(150, Painter::Draw::Create(
            [Owner](Ref<Canvas> Board, ConstRef<Painter::Brush> Element)
            {
                OnPaint(* Owner, Board, Element);
            }));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Swatch::SetValue(Element Target, Color Value)
    {
        if (const ConstPtr<Checker> Shown = Target.Get<Checker>(); Shown && Shown->Value != Value)
        {
            Target.Edit<Checker>()->Value = Value;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Color Swatch::GetValue(Element Target)
    {
        const ConstPtr<Checker> Shown = Target.Get<Checker>();
        return Shown ? Shown->Value : Color::Transparent();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Swatch::OnPaint(Ref<Interface> Owner, Ref<Canvas> Board, ConstRef<Painter::Brush> Element)
    {
        const Rect              Frame = Element.Frame;
        const Real32            Cell  = Min(Frame.GetWidth(), Frame.GetHeight()) * 0.5f;
        const ConstPtr<Checker> Shown = Owner.Peek<Checker>(Element.Slot);

        if (Cell <= 0.0f || !Shown)
        {
            return;
        }

        // The light squares fill the box, so only the dark ones are drawn over it, each cut to the box at its edges.
        Board.DrawRect(Frame, Color(0.8f, 0.8f, 0.8f, Element.Opacity), Element.Radius);

        const Rect  Inner = Frame.Expand(-Element.Radius * 0.29289322f);
        const Color Dark(0.5f, 0.5f, 0.5f, Element.Opacity);
        UInt32      Row = 0;

        for (Real32 Top = Frame.GetMinimumY(); Top < Frame.GetMaximumY(); Top += Cell, ++Row)
        {
            const Real32 Extra = ((Row & 1) == 0 ? Cell : 0.0f);
            
            for (Real32 Left = Frame.GetMinimumX() + Extra; Left < Frame.GetMaximumX(); Left += Cell * 2.0f)
            {
                const Rect Tile = Rect::Intersection(Rect(Left, Top, Left + Cell, Top + Cell), Inner); 
                
                if (!Tile.IsEmpty())
                {
                    Board.DrawRect(Tile, Dark);
                }
            }
        }

        Board.DrawRect(Frame, Color::Fade(Shown->Value, Element.Opacity), Element.Radius);
    }
}