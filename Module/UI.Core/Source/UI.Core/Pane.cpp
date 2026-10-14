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

#include "Pane.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace ZyPlugin::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pane::Restyle(UInt32 First, UInt32 Last)
    {
        for (UInt32 Row = First; Row < Last; ++Row)
        {
            Marks[Row] |= Dirty::Style;
        }
        Styled = true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pane::Widen(UInt32 Row, UInt32 Count)
    {
        ZY_ASSERT(Row <= GetSize(), "A run can only open within the pane or at its end");

        Columns([this, Row, Count]<typename Type>(Sequence<Type> Pane::* Column)
        {
            WidenColumn(this->*Column, Row, Count);
        });
        Stale = true;
        Moved = true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pane::Narrow(UInt32 Row, UInt32 Count)
    {
        ZY_ASSERT(Row + Count <= GetSize(), "A run can only close within the pane");

        Columns([this, Row, Count]<typename Type>(Sequence<Type> Pane::* Column)
        {
            (this->*Column).Remove(Row, Count);
        });
        Stale = true;
        Moved = true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pane::Rotate(UInt32 First, UInt32 Middle, UInt32 Last)
    {
        ZY_ASSERT(First <= Middle && Middle <= Last && Last <= GetSize(),
                  "The runs must lie within the pane, in order");

        Columns([this, First, Middle, Last]<typename Type>(Sequence<Type> Pane::* Column)
        {
            RotateColumn(this->*Column, First, Middle, Last);
        });
        Stale = true;
        Moved = true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pane::Adopt(UInt32 Row, ConstRef<Pane> Source, UInt32 From, UInt32 Count)
    {
        ZY_ASSERT(AddressOf(Source) != this, "A pane adopts rows of another pane only");
        ZY_ASSERT(From + Count <= Source.GetSize(), "The run copied must lie within the other pane");

        Widen(Row, Count);
        Columns([this, Row, &Source, From, Count]<typename Type>(Sequence<Type> Pane::* Column)
        {
            Copy((this->*Column).GetData() + Row, Count, (Source.*Column).GetData() + From);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Pane::Clear()
    {
        Columns([this]<typename Type>(Sequence<Type> Pane::* Column)
        {
            (this->*Column).Clear();
        });
        Open    = false;
        Stale   = false;
        Whole   = false;
        Styled  = false;
        Moved   = false;
        Touched = false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename Callable>
    void Pane::Columns(AnyRef<Callable> Action)
    {
        Action(&Pane::Slots);
        Action(&Pane::Parents);
        Action(&Pane::Spans);
        Action(&Pane::Depths);
        Action(&Pane::Names);
        Action(&Pane::States);
        Action(&Pane::Marks);
        Action(&Pane::Styles);
        Action(&Pane::Boxes);
        Action(&Pane::Frames);
        Action(&Pane::Sizes);
        Action(&Pane::Rooms);
        Action(&Pane::Laid);
        Action(&Pane::Reaches);
        Action(&Pane::Covers);
        Action(&Pane::Tapes);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename Type>
    void Pane::WidenColumn(Ref<Sequence<Type>> Column, UInt32 Row, UInt32 Count)
    {
        const UInt Size = Column.GetSize();
        Column.Advance(Count);

        // Walked from the back, so no row is overwritten before it has moved.
        const Ptr<Type> Data = Column.GetData();

        for (UInt Index = Size; Index > Row; --Index)
        {
            Data[Index - 1 + Count] = Data[Index - 1];
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename Type>
    void Pane::ReverseColumn(Ref<Sequence<Type>> Column, UInt32 First, UInt32 Last)
    {
        const Ptr<Type> Data = Column.GetData();

        for (; First + 1 < Last; ++First, --Last)
        {
            Swap(Data[First], Data[Last - 1]);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename Type>
    void Pane::RotateColumn(Ref<Sequence<Type>> Column, UInt32 First, UInt32 Middle, UInt32 Last)
    {
        // Reversing each run and then the whole swaps the runs in place, with no room borrowed.
        ReverseColumn(Column, First, Middle);
        ReverseColumn(Column, Middle, Last);
        ReverseColumn(Column, First, Last);
    }
}