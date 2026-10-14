// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the templates a layout gives, and the ones being made, so a template holding itself is caught.
    class Template final
    {
    public:

        /// \brief Keeps a template, replacing one of the same name.
        ///
        /// \param Name The identifier of the template's name.
        /// \param Node The template's root node, which is copied.
        void Assign(UInt64 Name, ConstRef<JsonValue> Node);

        /// \brief Finds a template.
        ///
        /// \param Name The identifier of the template's name.
        /// \return The template, or `nullptr` when none has the name.
        ConstPtr<JsonValue> Find(UInt64 Name) const;

        /// \brief Checks whether a template is being made already, further up, so making it again would never end.
        ///
        /// \param Name The identifier of the template's name.
        /// \return `true` if it is, `false` otherwise.
        Bool IsExpanding(UInt64 Name) const;

        /// \brief Notes that a template is being made, until the matching \ref Leave.
        ///
        /// \param Name The identifier of the template's name.
        void Enter(UInt64 Name);

        /// \brief Notes that the template entered last is made.
        void Leave();

    public:

        /// \brief Copies a template as a node using it makes it, with the parameters and the node's fields.
        ///
        /// \param Template The template's root node.
        /// \param Node     The node using it, or one that is not an object to take every default.
        /// \return The copy, ready to be built.
        static JsonValue Copy(ConstRef<JsonValue> Template, ConstRef<JsonValue> Node);

    private:

        /// The field of a template that holds its parameters.
        static constexpr Symbol kParameters  = "Parameters";

        /// The fields of a node using a template that are never copied into it, as its widget, parameters and children.
        static constexpr Array<Text, 3> kOwnFields { "Widget", "Parameters", "Children" };

        /// \brief Gives the parameters of a template the values a node using it gives, or their defaults.
        ///
        /// \param Target     The value of the copy of the template, every string `$Name` in it being replaced.
        /// \param Parameters The parameters the template declares, each with its default.
        /// \param Node       The node using the template, whose fields give the parameters.
        static void Substitute(Ref<JsonValue> Target, ConstRef<JsonValue> Parameters, ConstRef<JsonValue> Node);

        /// \brief Gives a copy of a template one field of the node using it.
        ///
        /// \param Target     The copy of the template.
        /// \param Parameters The parameters the template declares, which reach only where the template names them.
        /// \param Key        The name of the field.
        /// \param Value      The value, left out for the node's children and its widget, which is the template.
        static void Overlay(Ref<JsonValue>      Target,
                            ConstRef<JsonValue> Parameters,
                            ConstRef<Str>       Key,
                            ConstRef<JsonValue> Value);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Table<Digest, JsonValue> mTemplates;
        Sequence<UInt64>         mExpanding;
    };
}