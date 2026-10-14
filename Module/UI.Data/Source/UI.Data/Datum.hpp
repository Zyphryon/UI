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

#include "UI.Core/Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    /// \brief Specifies what a value of a model holds.
    enum class Form : UInt8
    {
        None,        ///< Nothing, as a value never set or cleared.
        Bool,        ///< A flag.
        Number,      ///< A number.
        Text,        ///< A text.
        Color,       ///< A colour.
        List,        ///< A list of records, read as a number by its count.
    };

    /// \brief Represents one value of a model, of any form, read as whichever form a widget takes.
    class Datum final
    {
    public:

        /// \brief Constructs a value holding nothing.
        Datum();

        /// \brief Constructs a value holding a flag.
        ///
        /// \param Value The flag, `true` to hold a set flag, `false` to hold a clear one.
        Datum(Bool Value);

        /// \brief Constructs a value holding a number.
        ///
        /// \param Value The number held, read back as a flag, a number or a text as a widget asks.
        Datum(Real64 Value);

        /// \brief Constructs a value holding a single-precision number, held as a double.
        ///
        /// \param Value The number held.
        Datum(Real32 Value);

        /// \brief Constructs a value holding a signed whole number, held as a double.
        ///
        /// \param Value The number held.
        Datum(SInt32 Value);

        /// \brief Constructs a value holding an unsigned whole number, held as a double.
        ///
        /// \param Value The number held.
        Datum(UInt32 Value);

        /// \brief Constructs a value holding a copy of a text.
        ///
        /// \param Value The text copied in.
        Datum(Text Value);

        /// \brief Constructs a value holding text, taking the string it is written in.
        ///
        /// \param Value The string taken, left empty.
        Datum(AnyRef<Str> Value);

        /// \brief Constructs a value holding a text written as a literal, never the flag its pointer converts to.
        ///
        /// \param Literal The literal copied in.
        template<UInt Count>
        ZY_INLINE Datum(const Char (& Literal)[Count])
            : Datum(Text(Literal))
        {
        }

        /// \brief Constructs a value holding a colour.
        ///
        /// \param Value The colour held, read as a flag by whether it shows at all.
        Datum(Color Value);

        /// \brief Gets what the value holds.
        ///
        /// \return The form of what it holds, \ref Form::None for nothing.
        ZY_INLINE Form GetForm() const
        {
            return mForm;
        }

        /// \brief Gets the value as a flag, set for a number other than nought, a text or a list that is not empty.
        ///
        /// \return `true` if the value reads as set, `false` otherwise.
        Bool GetBool() const;

        /// \brief Gets the value as a number, a flag as one or nought, a text read as a number, a list by its count.
        ///
        /// \return The number, nought when the value reads as none.
        Real64 GetNumber() const;

        /// \brief Gets the colour the value holds.
        ///
        /// \return The colour, or transparent when it holds none.
        Color GetColor() const;

        /// \brief Gets the value as one of the types a widget takes, a flag, a number or a colour.
        ///
        /// \return The value read as \p Type.
        template<typename Type>
        Type As() const
        {
            if constexpr (IsAnyOf<Type, Bool>)
            {
                return GetBool();
            }
            else if constexpr (IsReal<Type>)
            {
                return static_cast<Type>(GetNumber());
            }
            else if constexpr (IsUnsigned<Type>)
            {
                const Real64 Number = GetNumber();
                const Bool   Absent = mForm == Form::None || Number < 0.0;
                return Absent ? kMaximum<Type> : static_cast<Type>(::Round(Number));
            }
            else if constexpr (IsIntegral<Type>)
            {
                return mForm == Form::None ? static_cast<Type>(-1) : static_cast<Type>(::Round(GetNumber()));
            }
            else
            {
                return GetColor();
            }
        }

        /// \brief Gets the text the value holds.
        ///
        /// \return The text, or none when the value holds no text.
        ZY_INLINE Text GetText() const
        {
            return mWords;
        }

        /// \brief Writes the value as text.
        ///
        /// \param Decimals The digits a number keeps after the point, or below nought for up to four.
        /// \return The text, with a flag written `true` or `false` and nothing written for none.
        Str Print(SInt32 Decimals = -1) const;

        /// \brief Writes the value as text at the end of a string, so a reused string writes many without allocating.
        ///
        /// \param Output   The receiver of the text, after what it held.
        /// \param Decimals The digits a number keeps after the point, or below nought for up to four.
        void Print(Ref<Str> Output, SInt32 Decimals = -1) const;

        /// \brief Checks whether two values hold the same form and the same content.
        ///
        /// \param Other The value to compare with.
        /// \return `true` if they do, `false` otherwise.
        Bool operator==(ConstRef<Datum> Other) const;

    public:

        /// \brief Constructs a value standing for a list of records.
        ///
        /// \param Count The count of records, which the value reads as a number.
        /// \return The value, of \ref Form::List.
        static Datum Records(UInt32 Count);

    private:

        /// The most digits a number keeps after the point, past which a double holds nothing exact.
        static constexpr SInt32 kMaxDecimals   = 9;

        /// The digits a number written without a format keeps at most, before the zeros ending them are dropped.
        static constexpr UInt   kLooseDecimals = 4;

        /// \brief Writes a number as text at the end of a string.
        ///
        /// \param Output   The receiver of the text, after what it held.
        /// \param Value    The number written.
        /// \param Decimals The digits kept after the point, or below nought for up to four, trailing zeros dropped.
        static void PrintNumber(Ref<Str> Output, Real64 Value, SInt32 Decimals);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Form   mForm;
        Bool   mFlag;
        Real64 mNumber;
        Str    mWords;
        Color  mTint;
    };
}