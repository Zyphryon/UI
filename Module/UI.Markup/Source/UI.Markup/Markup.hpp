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

#include "Parser.hpp"
#include "Template.hpp"
#include "UI.Core/Element.hpp"
#include "UI.Core/Interface.hpp"
#include "UI.Core/Types.hpp"
#include "UI.Data/Binder.hpp"
#include <Zyphryon.Content/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Provides the reading of interfaces written as data, themes from `.theme` and templates from `.ui` files.
    class Markup final
    {
    public:

        /// \brief Represents the code creating a widget out of one node, its own parts included.
        using Builder = Delegate<Element(Ref<Markup>, Element, ConstRef<JsonValue>)>;

        /// \brief Represents the code reading a field any node may give into the element created from it.
        using Field   = Delegate<void(Ref<Markup>, Element, ConstRef<JsonValue>)>;

    public:

        /// \brief Constructs a markup knowing no template and no widget but a plain `Panel`.
        ///
        /// \param Content The content service typefaces and images named in the data are loaded through.
        explicit Markup(Ptr<ZyContent::Service> Content);

        /// \brief Teaches the markup a widget, or replaces how it creates one it knew.
        ///
        /// \param Widget  The name nodes create it by.
        /// \param Factory The builder creating the widget out of a node.
        void Register(Text Widget, Builder Factory);

        /// \brief Changes the band a template opens in when it names none.
        ///
        /// \param Default The band templates naming none open in.
        void SetLayer(Layer Default);

        /// \brief Teaches the markup a field any node may give, ignoring a reader it already has.
        ///
        /// \param Reader The code reading it, run on every node in the order the readers were added.
        void AddField(Field Reader);

        /// \brief Records that a module was declared on the markup.
        ///
        /// \param Module The name of the module.
        /// \return `true` the first time the module is named, `false` after.
        Bool Enlist(Text Module);

        /// \brief Adds a layer every theme the markup dresses is read from, over the layers added before it.
        ///
        /// \param Source The text of the layer, which must outlive the markup, as the bytes a module is built with do.
        void AddTheme(Text Source);

        /// \brief Gives a typeface by name under every layer, filling the slot a layer declares as `null`.
        ///
        /// \param Name The name styles set their text in it by, as `Body`.
        /// \param Font The typeface, or none to set their text in none.
        void SetFont(Text Name, Retainer<ZyRender::Font> Font);

        /// \brief Dresses a theme in the layers added alone, which replace every style of it only once all were read.
        ///
        /// \param Target The theme, left as it was when a layer cannot be read.
        /// \param Error  The variable receiving why a layer cannot be read, saying so when a module built it in.
        /// \return `true` if every layer was read, `false` otherwise.
        Bool Dress(Ref<Theme> Target, Ref<Str> Error);

        /// \brief Dresses a theme in the layers added and a game's theme read over them, naming only declared states.
        ///
        /// \param Target The theme, left as it was when a layer cannot be read.
        /// \param Source The text of the game's theme, whose names dress every layer, its styles merging over theirs.
        /// \param Error  The variable receiving why a layer cannot be read, saying so when a module built it in.
        /// \return `true` if every layer was read, `false` otherwise.
        Bool Dress(Ref<Theme> Target, Text Source, Ref<Str> Error);

        /// \brief Reads the templates of a layout, replacing those of the same name only once all were read.
        ///
        /// \param Source The text of the layout.
        /// \param Error  The variable receiving why the layout cannot be read.
        /// \return `true` if it was read, `false` otherwise.
        Bool Read(Text Source, Ref<Str> Error);

        /// \brief Creates a template in a pane of its own, at the layer the template names.
        ///
        /// \param Target   The tree to open the pane in.
        /// \param Template The name of the template.
        /// \return The root of the pane, or one naming nothing when no template has the name.
        Element Spawn(Ref<Tree> Target, Text Template);

        /// \brief Creates a template under an element.
        ///
        /// \param Parent   The element to hold the template's root.
        /// \param Template The name of the template.
        /// \return The template's root, or one naming nothing when no template has the name.
        Element Spawn(Element Parent, Text Template);

        /// \brief Creates a template under an element, its bindings reading the record a scope named.
        ///
        /// \param Parent   The element to hold the template's root.
        /// \param Template The name of the template.
        /// \param Scope    The scope the template was named in, as \ref GetScope gave it, or empty for the root.
        /// \param Within   The steps of that scope's record now, as \ref Binder::Rescope keeps them.
        /// \return The template's root, or one naming nothing when no template has the name.
        Element Spawn(Element Parent, Text Template, Text Scope, ConstRef<Path> Within);

        /// \brief Checks whether a template of a name was read.
        ///
        /// \param Template The name of the template.
        /// \return `true` if one was, `false` otherwise.
        Bool HasTemplate(Text Template) const;

        /// \brief Creates the element one node describes and everything it holds.
        ///
        /// \param Parent The element to hold the new one.
        /// \param Node   The node of the layout describing the element.
        /// \return The element, or one naming nothing when the node names no widget the markup knows.
        Element Build(Element Parent, ConstRef<JsonValue> Node);

        /// \brief Binds a field of a node to the tree's model, as `"{Player.Gold}"`.
        ///
        /// \param Target The element created from the node, which shows the value.
        /// \param Node   The node of the layout giving the field.
        /// \param Key    The name of the field.
        /// \param Write  The code showing the value on the element.
        /// \param Read   The code reading the element's value back, or empty for a field that only shows.
        /// \return `true` if the field binds, `false` otherwise.
        Bool Bind(Element Target, ConstRef<JsonValue> Node, Text Key, Writer Write, Reader Read = Reader()) const;

        /// \brief Opens a scope, so the bindings of the nodes built until it is closed read from one record.
        ///
        /// \param Record The path of the record, as `Party[2]`, read from the open scope unless it starts with `/`.
        void PushScope(Text Record);

        /// \brief Closes the scope opened last.
        void PopScope();

        /// \brief Gets the path of the record the bindings created now read from.
        ///
        /// \return The path, or empty for the root.
        Text GetScope() const;

        /// \brief Loads a typeface named in the data.
        ///
        /// \param Path The address of the typeface.
        /// \return The typeface, loading in the background.
        Retainer<ZyRender::Font> LoadFont(Text Path) const;

        /// \brief Loads an image named in the data.
        ///
        /// \param Path The address of the image.
        /// \return The image, loading in the background.
        Retainer<ZyGraphic::Image> LoadImage(Text Path) const;

    private:

        /// \brief Gives an element what every node may have, as a name, a style, a layout, states and children.
        ///
        /// \param Target The element created from the node.
        /// \param Node   The node of the layout giving the fields.
        void Apply(Element Target, ConstRef<JsonValue> Node);

        /// \brief Builds a node from a template that is being expanded, keeping it on the stack of those being built.
        ///
        /// \param Template The identifier of the template's name.
        /// \param Parent   The element to hold the template's root.
        /// \param Node     The expanded copy of the template.
        /// \return The element, or one naming nothing when it cannot be built.
        Element Expand(UInt64 Template, Element Parent, ConstRef<JsonValue> Node);

        /// \brief Builds the `Children` a node gives into the part of an element they go in, as a window's content.
        ///
        /// \param Owner The element created from the node.
        /// \param Node  The node of the layout giving the children.
        void Nest(Element Owner, ConstRef<JsonValue> Node);

    public:

        /// \brief Gets the value a node gives a field, of whatever kind, as `Find` takes only a `Symbol`.
        ///
        /// \param Node The node of the layout giving the field.
        /// \param Key  The name of the field.
        /// \return The value, or `nullptr` when the node is not an object or gives no such field.
        static ConstPtr<JsonValue> GetField(ConstRef<JsonValue> Node, Text Key);

        /// \brief Gets the text a node gives a field.
        ///
        /// \param Node    The node of the layout giving the field.
        /// \param Key     The name of the field.
        /// \param Default The text when the node gives none.
        /// \return The text, or the default when the node gives none or gives a value of another kind.
        static Text GetText(ConstRef<JsonValue> Node, Text Key, Text Default = Text());

        /// \brief Gets the number a node gives a field.
        ///
        /// \param Node    The node of the layout giving the field.
        /// \param Key     The name of the field.
        /// \param Default The number when the node gives none.
        /// \return The number, or the default when the node gives none or gives a value of another kind.
        static Real32 GetNumber(ConstRef<JsonValue> Node, Text Key, Real32 Default = 0.0f);

        /// \brief Gets the flag a node gives a field.
        ///
        /// \param Node    The node of the layout giving the field.
        /// \param Key     The name of the field.
        /// \param Default The flag when the node gives none.
        /// \return The flag, or the default when the node gives none or gives a value of another kind.
        static Bool GetBool(ConstRef<JsonValue> Node, Text Key, Bool Default = false);

        /// \brief Names the part of an element the children a layout gives it go in, as a window's content panel.
        ///
        /// \param Owner   The element the layout gives the children to.
        /// \param Content The part of it they go in.
        static void SetHost(Element Owner, Element Content);

        /// \brief Gets where the children a layout gives an element go.
        ///
        /// \param Owner The element the layout gives the children to.
        /// \return The part it names, or the element itself when it names none.
        static Element GetHost(Element Owner);

    private:

        /// \brief Reads one field of a node's layout when the node gives it, warning when the value is not taken.
        ///
        /// \param Node   The node of the layout giving the field.
        /// \param Key    The name of the field.
        /// \param Output The variable receiving the value, left as it was when the value is not one it takes.
        /// \param Read   The function reading the value.
        template<typename Type, typename Function>
        static void ReadShape(ConstRef<JsonValue> Node, Text Key, Ref<Type> Output, Function Read)
        {
            if (const ConstPtr<JsonValue> Value = Parser::Find(Node, Key); Value && !Read(* Value, Output))
            {
                LOG_W("The layout gives '{0}' a value it cannot take, so it is left as it was", Key);
            }
        }

        /// \brief Builds a node naming no widget as an element of the node's fields alone.
        ///
        /// \param Reader The markup building the node.
        /// \param Parent The element to hold the new one.
        /// \param Node   The node of the layout, whose fields the markup applies afterwards.
        /// \return The plain element made for the node.
        static Element Plain(Ref<Markup> Reader, Element Parent, ConstRef<JsonValue> Node);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ptr<ZyContent::Service>                 mContent;
        Table<Digest, Builder>                  mBuilders;
        Template                                mTemplates;
        Sequence<Str>                           mScopes;
        Layer                                   mLayer;
        Sequence<Field>                         mFields;
        Sequence<UInt64>                        mModules;
        Sequence<Text>                          mThemes;
        Table<Digest, Retainer<ZyRender::Font>> mFonts;
    };
}