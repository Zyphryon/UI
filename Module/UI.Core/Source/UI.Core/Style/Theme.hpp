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

#include "Animation.hpp"
#include "Skin.hpp"
#include "Style.hpp"
#include "UI.Core/Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Represents the styles an interface dresses its elements in, found by the name each element wears.
    class Theme final
    {
    public:

        /// \brief Constructs a theme holding no style.
        Theme();

        /// \brief Defines a style, or starts it over when it was defined before.
        ///
        /// \param Name The name elements wear it by.
        /// \param Base The name of the style it starts from, or empty for none.
        /// \return The style, giving nothing yet.
        Ref<Style> Define(Text Name, Text Base = Text());

        /// \brief Merges a style over the one of its name, as \ref Style::Merge does, or defines it when none has it.
        ///
        /// \param Name The name elements wear it by.
        /// \param Over The style, giving only what it changes.
        void Merge(Text Name, ConstRef<Style> Over);

        /// \brief Takes every style, animation, colour and image of another theme in place of its own.
        ///
        /// \param Other The theme whose contents are taken, left holding no style.
        void Adopt(Ref<Theme> Other);

        /// \brief Works out the skin a style gives an element in some states.
        ///
        /// \param Name    The identifier of the style the element wears.
        /// \param Current The states the element is in, which pick the variants worn.
        /// \param Output  The receiver of the skin, left untouched when no style has the name.
        /// \return `true` if a style has the name, `false` otherwise.
        Bool Resolve(UInt64 Name, State Current, Ref<Skin> Output) const;

        /// \brief Finds a style by the identifier its name hashes to.
        ///
        /// \param Name The identifier of the style to find.
        /// \return The style, or `nullptr` when none has the name.
        ConstPtr<Style> Find(UInt64 Name) const;

        /// \brief Gets the name every style and animation was defined by.
        ///
        /// \return The names, by the identifier each one hashes to.
        ZY_INLINE ConstRef<Table<Digest, Str>> GetNames() const
        {
            return mNames;
        }

        /// \brief Defines an animation, or starts it over when it was defined before.
        ///
        /// \param Name The name skins play it by.
        /// \return The animation, holding no key yet.
        Ref<Animation> Animate(Text Name);

        /// \brief Finds an animation by the identifier its name hashes to.
        ///
        /// \param Name The identifier of the animation to find.
        /// \return The animation, or `nullptr` when none has the name.
        ConstPtr<Animation> FindAnimation(UInt64 Name) const;

        /// \brief Finds the sound a style names for a moment of interaction, falling back to the styles it starts from.
        ///
        /// \param Name   The identifier of the style the element wears.
        /// \param Moment The moment of interaction the sound is played at.
        /// \return The address of the sound, or empty when no style of the chain names one.
        Text FindSound(UInt64 Name, Cue Moment) const;

        /// \brief Gets a number that changes whenever the theme does, so what was worked out from it is redone.
        ///
        /// \return The version, counting every change made to the theme.
        ZY_INLINE UInt32 GetVersion() const
        {
            return mVersion;
        }

    private:

        /// The deepest a chain of styles starting from one another goes before it is taken for a loop.
        static constexpr UInt32 kMaxDepth = 16;

        /// \brief Works out the skin a style gives, the styles it starts from first.
        ///
        /// \param Name    The identifier of the style to gather.
        /// \param Current The states the element is in, which pick the variants worn.
        /// \param Output  The receiver of the values, merged over what the styles before gave.
        /// \param Depth   The number of styles deep the chain already is, which stops a loop at \ref kMaxDepth.
        /// \return `true` if a style has the name, `false` otherwise.
        Bool Gather(UInt64 Name, State Current, Ref<Skin> Output, UInt32 Depth) const;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Table<Digest, Style>     mStyles;
        Table<Digest, Animation> mAnimations;
        Table<Digest, Str>       mNames;
        UInt32                   mVersion;
    };
}