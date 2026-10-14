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
#include "UI.Core/Facet/Visual/Look.hpp"
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
        Result.Add<Checker>();
        Result.Obtain<Look>().Background = Value;
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
        Target.GetPainter().AddDrawer<Checker>(150, Painter::Draw::Create<&Swatch::OnPaint>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Swatch::SetValue(Element Target, Color Value)
    {
        if (Target.Obtain<Look>().Background != Value)
        {
            Target.Edit<Look>()->Background = Value;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Color Swatch::GetValue(Element Target)
    {
        const ConstPtr<Look> Shade = Target.Get<Look>();
        return Shade ? Shade->Background : Color::Transparent();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Swatch::OnPaint(Ref<Canvas> Board, ConstRef<Painter::Brush> Element)
    {
        const Rect   Frame = Element.Frame;
        const Real32 Cell  = Min(Frame.GetWidth(), Frame.GetHeight()) * 0.5f;

        if (Cell <= 0.0f)
        {
            return;
        }

        // The light squares fill the box, so only the dark ones are drawn over it, each cut to the box at its edges.
        Board.DrawRect(Frame, Color(0.8f, 0.8f, 0.8f, Element.Opacity), Element.Radius);

        const Color Dark(0.5f, 0.5f, 0.5f, Element.Opacity);
        UInt32      Row = 0;

        for (Real32 Top = Frame.GetMinimumY(); Top < Frame.GetMaximumY(); Top += Cell, ++Row)
        {
            const Real32 Bottom = Min(Top + Cell, Frame.GetMaximumY());

            for (Real32 Left = Frame.GetMinimumX() + ((Row & 1) == 0 ? Cell : 0.0f); Left < Frame.GetMaximumX();
                 Left += Cell * 2.0f)
            {
                Board.DrawRect(Rect(Left, Top, Min(Left + Cell, Frame.GetMaximumX()), Bottom), Dark);
            }
        }
    }
}