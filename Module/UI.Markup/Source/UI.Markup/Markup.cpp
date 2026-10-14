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

#include "Facet/Host.hpp"
#include "Interpreter.hpp"
#include "Markup.hpp"
#include "UI.Core/Facet/Input/Control.hpp"
#include "UI.Core/Layers.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Markup::Markup(Ptr<ZyContent::Service> Content)
        : mContent { Content },
          mLayer { Layers::kBase }
    {
        // A node naming no widget is a `Panel`, a plain element until a module teaches the markup its own.
        Register("Panel", Builder::Create<&Markup::Plain>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Markup::Register(Text Widget, Builder Factory)
    {
        mBuilders.Assign(Digest(Identify(Widget)), Move(Factory));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Markup::SetLayer(Layer Default)
    {
        mLayer = Default;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Markup::AddField(Field Reader)
    {
        // A field added again, as by a module declared twice on the markup, is still read once.
        if (!mFields.Contains(Reader))
        {
            mFields.Append(Move(Reader));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Markup::Enlist(Text Module)
    {
        const UInt64 Key = Identify(Module);

        if (mModules.Contains(Key))
        {
            return false;
        }
        mModules.Append(Key);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Markup::AddTheme(Text Source)
    {
        mThemes.Append(Source);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Markup::SetFont(Text Name, Retainer<ZyRender::Font> Font)
    {
        mFonts.Assign(Digest(Identify(Name)), Move(Font));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Markup::Dress(Ref<Theme> Target, Ref<Str> Error)
    {
        return Interpreter::Dress(* this, Target, mThemes, mThemes.GetSize(), mFonts, Error);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Markup::Dress(Ref<Theme> Target, Text Source, Ref<Str> Error)
    {
        // The game's theme is the layer read last, over every layer a module added.
        Sequence<Text> Layers(mThemes);
        Layers.Append(Source);
        return Interpreter::Dress(* this, Target, Layers, mThemes.GetSize(), mFonts, Error);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Markup::Read(Text Source, Ref<Str> Error)
    {
        return Interpreter::Read(mTemplates, Source, Error);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Markup::Spawn(Ref<Tree> Target, Text Template)
    {
        const UInt64              Key   = Identify(Template);
        const ConstPtr<JsonValue> Found = mTemplates.Find(Key);

        if (!Found)
        {
            LOG_W("No template is named '{0}'", Template);
            return Element();
        }

        const JsonValue Expanded = UI::Template::Copy(* Found, JsonValue());

        // Only a named band is the pane's, since an `Image` root reads a number there as the slice it shows.
        Layer Band = mLayer;

        if (const ConstPtr<JsonValue> Named = Parser::Find(Expanded, "Layer"); Named && Named->IsString())
        {
            ReadShape(Expanded, "Layer", Band, Parser::ParseLayer);
        }

        const Element Root(Target, Target.Open(Band));
        Element       Made = Root;

        // A root naming no widget is the pane's own, given what the template gives.
        if (!Parser::Find(Expanded, "Widget"))
        {
            mTemplates.Enter(Key);
            Apply(Root, Expanded);
            mTemplates.Leave();
        }
        else
        {
            // A root naming a widget is made inside the pane, unless it opens a pane of its own, as a window does.
            Made = Expand(Key, Root, Expanded);

            if (!Made.IsAlive() || Made.GetRoot() != Root)
            {
                Root.Destruct();
            }
        }

        return Made;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Markup::Spawn(Element Parent, Text Template)
    {
        const UInt64              Key   = Identify(Template);
        const ConstPtr<JsonValue> Found = mTemplates.Find(Key);

        if (!Found)
        {
            LOG_W("No template is named '{0}'", Template);
            return Element();
        }

        // A template spawned while it is being made, as by a context menu naming it, would be made forever.
        if (mTemplates.IsExpanding(Key))
        {
            LOG_W("The template '{0}' holds itself, so it is left out", Template);
            return Element();
        }

        return Expand(Key, Parent, UI::Template::Copy(* Found, JsonValue()));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Markup::Spawn(Element Parent, Text Template, Text Scope, ConstRef<Path> Within)
    {
        PushScope(Str::Print<"/{0}">(Scope));
        const Element Result = Spawn(Parent, Template);
        PopScope();

        // Made reading the record as it was named, then moved to the record its row shows now, as a list's row is.
        Path From;
        Binder::Locate(Scope, From);

        if (Result.IsAlive() && !From.Steps.IsEmpty())
        {
            Binder::Rescope(Result, From, Within);
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Markup::HasTemplate(Text Template) const
    {
        return mTemplates.Find(Identify(Template)) != nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Markup::Build(Element Parent, ConstRef<JsonValue> Node)
    {
        if (!Node.IsObject())
        {
            LOG_W("A node of the layout is not an object, so it is left out");
            return Element();
        }

        const Text   Widget = GetText(Node, "Widget", "Panel");
        const UInt64 Key    = Identify(Widget);
        Element      Result;

        if (const ConstPtr<Builder> Factory = mBuilders.Find(Digest(Key)))
        {
            Result = Builder(* Factory)(* this, Parent, Node);
        }
        else if (const ConstPtr<JsonValue> Template = mTemplates.Find(Key); Template && !mTemplates.IsExpanding(Key))
        {
            // A template's fields start where it sets them, a parameter reaching only fields naming it, as `"$Title"`.
            Result = Expand(Key, Parent, UI::Template::Copy(* Template, Node));

            // The node's own children are the caller's, so they are made once the template is done and may use it.
            if (Result.IsAlive())
            {
                Nest(Result, Node);
            }
            return Result;
        }
        else
        {
            LOG_W("The layout names '{0}', which is no widget, or a template holding itself, so it is left out",
                  Widget);
            return Element();
        }

        if (Result.IsAlive())
        {
            Apply(Result, Node);
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Markup::Bind(Element Target, ConstRef<JsonValue> Node, Text Key, Writer Write, Reader Read) const
    {
        const ConstPtr<JsonValue> Value = Parser::Find(Node, Key);

        // A key of the lexicon binds as a value does, so it is read again whenever the language changes.
        const Bool Named = Value && Value->IsString();

        if (!Named || (!Binder::IsBound(Value->GetString()) && !Binder::IsLocalized(Value->GetString())))
        {
            return false;
        }
        return Binder::Attach(Target, Value->GetString(), Move(Write), Move(Read), GetScope());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Markup::PushScope(Text Record)
    {
        const Text Outer = GetScope();

        // A scope inside another reads from the record the outer one names, unless it climbs back to the root.
        if (StrStartsWith(Record, "/"))
        {
            mScopes.Append(Str(Record.Slice(1)));
        }
        else if (Outer.IsEmpty())
        {
            mScopes.Append(Str(Record));
        }
        else
        {
            mScopes.Append(Str::Join(Outer, '.', Record));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Markup::PopScope()
    {
        if (!mScopes.IsEmpty())
        {
            mScopes.RemoveLast();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Text Markup::GetScope() const
    {
        return mScopes.IsEmpty() ? Text() : Text(mScopes.GetBack());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Retainer<ZyRender::Font> Markup::LoadFont(Text Path) const
    {
        return mContent ? mContent->Load<ZyRender::Font>(ZyContent::Uri(Path)) : Retainer<ZyRender::Font>();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Retainer<ZyGraphic::Image> Markup::LoadImage(Text Path) const
    {
        if (!mContent || Path.IsEmpty())
        {
            return Retainer<ZyGraphic::Image>();
        }
        return mContent->Load<ZyGraphic::Image>(ZyContent::Uri(Path));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Markup::Apply(Element Target, ConstRef<JsonValue> Node)
    {
        if (const ConstPtr<JsonValue> Name = Parser::Find(Node, "Name"); Name && Name->IsString())
        {
            Target.SetName(Name->GetString());
        }

        if (const ConstPtr<JsonValue> Style = Parser::Find(Node, "Style"); Style && Style->IsString())
        {
            Target.SetStyle(Style->GetString());
        }

        // The layout fields sit on the node itself, as a theme's do on a style, and only those it gives change.
        Ref<Box> Shape = Target.EditBox();
        ReadShape(Node, "Placement", Shape.Placement, Parser::ParseEnum<Position>);
        ReadShape(Node, "Flow",      Shape.Flow,      Parser::ParseEnum<Direction>);
        ReadShape(Node, "Wrap",      Shape.Wrap,      Parser::ParseBool);
        ReadShape(Node, "Align",     Shape.Items,     Parser::ParseEnum<Align>);
        ReadShape(Node, "Justify",   Shape.Content,   Parser::ParseEnum<Justify>);
        ReadShape(Node, "Width",     Shape.Width,     Parser::ParseLength);
        ReadShape(Node, "Height",    Shape.Height,    Parser::ParseLength);
        ReadShape(Node, "Basis",     Shape.Basis,     Parser::ParseLength);
        ReadShape(Node, "Left",      Shape.Left,      Parser::ParseLength);
        ReadShape(Node, "Top",       Shape.Top,       Parser::ParseLength);
        ReadShape(Node, "Right",     Shape.Right,     Parser::ParseLength);
        ReadShape(Node, "Bottom",    Shape.Bottom,    Parser::ParseLength);
        ReadShape(Node, "Margin",    Shape.Margin,    Parser::ParseEdges);
        ReadShape(Node, "Padding",   Shape.Padding,   Parser::ParseEdges);
        ReadShape(Node, "Gap",       Shape.Gap,       Parser::ParseNumber);
        ReadShape(Node, "Grow",      Shape.Grow,      Parser::ParseNumber);
        ReadShape(Node, "Shrink",    Shape.Shrink,    Parser::ParseNumber);

        if (const ConstPtr<JsonValue> Hidden = Parser::Find(Node, "Hidden"); Hidden && Hidden->IsBool())
        {
            Target.SetState(State::Hidden, Hidden->GetBool());
        }

        if (const ConstPtr<JsonValue> Disabled = Parser::Find(Node, "Disabled"); Disabled && Disabled->IsBool())
        {
            Target.SetState(State::Disabled, Disabled->GetBool());
        }

        // Either state may follow a flag of the model instead, as `"Hidden": "{Ui.HideHud}"`.
        Bind(Target, Node, "Hidden", Binder::PutState<State::Hidden>());
        Bind(Target, Node, "Disabled", Binder::PutState<State::Disabled>());

        // A control's action is only a name, so the game binds it whenever it likes, before or after the template.
        if (const ConstPtr<JsonValue> Action = Parser::Find(Node, "Action"); Action && Action->IsString())
        {
            Target.Obtain<Control>().Action = Identify(Action->GetString());
        }

        // Any node may give the fields a module reads, as a tooltip or a menu at a right press.
        for (ConstRef<Field> Reader : mFields)
        {
            Reader(* this, Target, Node);
        }

        // A widget whose children stand in one of its parts, as a window's in its content, names that part.
        Nest(Target, Node);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Markup::Expand(UInt64 Template, Element Parent, ConstRef<JsonValue> Node)
    {
        mTemplates.Enter(Template);
        const Element Result = Build(Parent, Node);
        mTemplates.Leave();
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Markup::Nest(Element Owner, ConstRef<JsonValue> Node)
    {
        if (const ConstPtr<JsonValue> Children = Parser::Find(Node, "Children"); Children && Children->IsArray())
        {
            const Element Content = GetHost(Owner);

            for (ConstRef<JsonValue> Child : Children->GetArray())
            {
                Build(Content, Child);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<JsonValue> Markup::GetField(ConstRef<JsonValue> Node, Text Key)
    {
        return Parser::Find(Node, Key);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Text Markup::GetText(ConstRef<JsonValue> Node, Text Key, Text Default)
    {
        const ConstPtr<JsonValue> Value = Parser::Find(Node, Key);
        return Value && Value->IsString() ? Value->GetString() : Default;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Markup::GetNumber(ConstRef<JsonValue> Node, Text Key, Real32 Default)
    {
        const ConstPtr<JsonValue> Value = Parser::Find(Node, Key);
        return Value && Value->IsNumber() ? Value->GetNumber<Real32>() : Default;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Markup::GetBool(ConstRef<JsonValue> Node, Text Key, Bool Default)
    {
        const ConstPtr<JsonValue> Value = Parser::Find(Node, Key);
        return Value && Value->IsBool() ? Value->GetBool() : Default;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Markup::SetHost(Element Owner, Element Content)
    {
        Owner.Obtain<Host>().Content = Content.GetHandle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Markup::GetHost(Element Owner)
    {
        const ConstPtr<Host> Found = Owner.Get<Host>();
        const Element        Content(Owner.GetTree(), Found ? Found->Content : Handle());
        return Content.IsAlive() ? Content : Owner;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Element Markup::Plain(Ref<Markup>, Element Parent, ConstRef<JsonValue>)
    {
        return Parent.Create();
    }
}