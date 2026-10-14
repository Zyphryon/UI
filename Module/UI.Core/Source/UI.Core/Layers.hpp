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

#include "Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Provides the bands the interface keeps for itself, and the names layouts know bands by.
    class Layers final
    {
    public:

        /// The band a pane opens in when none is given.
        static constexpr Layer kBase  = Layer(32, Layer::Trait::Cycled);

        /// The band a popup opens in, as a menu or a list dropped from a box.
        static constexpr Layer kPopup = Layer(96, Layer::Trait::Popup);

        /// The band a modal pane opens in, with the backdrop under it.
        static constexpr Layer kModal = Layer(128, Layer::Trait::Modal);

    public:

        /// \brief Names a band, so a layout's `Layer` field can open a pane in it.
        ///
        /// \param Name  The name layouts write to open a pane in the band.
        /// \param Value The band the name stands for.
        static void Declare(Text Name, Layer Value);

        /// \brief Finds a band by its name, in any case.
        ///
        /// \param Name   The name to look for.
        /// \param Output The receiver of the band.
        /// \return `true` if a band has that name, `false` otherwise.
        static Bool Find(Text Name, Ref<Layer> Output);

    private:

        /// \brief Holds one named band.
        struct Entry final
        {
            /// The name the band was declared by, matched in any case.
            Str   Name;

            /// The band the name stands for.
            Layer Value;
        };

        /// \brief Gets the named bands, made with the interface's own the first time they are asked for.
        ///
        /// \return The named bands, the interface's own first and then those declared, in order.
        static Ref<Sequence<Entry>> GetEntries();

        /// \brief Creates the named bands the interface keeps for itself.
        ///
        /// \return The named bands of \ref kBase, \ref kPopup and \ref kModal.
        static Sequence<Entry> Seed();
    };
}