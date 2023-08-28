#pragma once

#include "Cell.h"

#include <ostream>
#include <string>

typedef int lcId_t;

struct LogicCell
{
    static constexpr cellId_t INVALID_ID = -1;

    lcId_t id = INVALID_ID;
    Cell* lut = nullptr;
    Cell* dff = nullptr;
    Cell* carry = nullptr;
};

std::string subcellIdToStr(Cell* cell)
{
    return cell == nullptr ? "X" : std::to_string(cell->id);
}

std::ostream& operator<<(std::ostream& os, const LogicCell& lc)
{
    os << "LC #" << lc.id << " [LUT: " << subcellIdToStr(lc.lut) << ", DFF: " << subcellIdToStr(lc.dff) << ", CARRY: " << subcellIdToStr(lc.carry) << "]";
    return os;
}
