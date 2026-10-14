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
    /// \brief Represents one step of a path through a model, naming a value or a record of a list.
    struct Step final
    {
        /// The index a step naming a value rather than a record holds.
        static constexpr UInt32 kLeaf = kMaximum<UInt32>;

        /// The identifier of the value's name, dots and all, as `Player.Gold`.
        UInt64 Key;

        /// The record of the list the step goes into, or \ref kLeaf for the value the path ends on.
        UInt32 Index;

        /// \brief Checks whether two steps name the same value and the same record.
        ///
        /// \param Other The step compared with this one.
        /// \return `true` if they do, `false` otherwise.
        ZY_INLINE constexpr Bool operator==(ConstRef<Step> Other) const = default;
    };
}