#include "Port.h"

#include <sstream>

#include "Cell.h"
#include "StringUtils.h"

#define PRINT_PORT_DESTINATION_CELLS 1

Port::Port(std::string name, Cell& cell, Link& link, Type type)
    : name(name)
    , cell(cell)
    , type(type)
{}

std::ostream& operator<<(std::ostream& os, const Port& port)
{
    std::string linksStr;
    if (port.links.size() == 0)
    {
        linksStr = "-";
    }
    else if (port.links.size() == 1)
    {
        linksStr = std::to_string(port.links.front().get().id);
    }
    else
    {
        linksStr = StringUtils::join(port.links.cbegin(), port.links.cend(), [&](const auto& link){ return link.get().id; });
    }

    if (port.type == Port::Type::INPUT)
    {
#ifdef PRINT_PORT_DESTINATION_CELLS
        os << "[" << StringUtils::join(port.links.begin(), port.links.end(), [](const auto& link) {
                return link.get().input == nullptr ? "X" : std::to_string(link.get().input->cell.id) + " (" + Cell::typeToStr(link.get().input->cell.type) + ")";
            }) << "] > ";
#endif
        os << linksStr << " > " << port.name;
    }
    else
    {
        os << port.name << " > " << linksStr;
#ifdef PRINT_PORT_DESTINATION_CELLS
        os << " > [" << StringUtils::join(port.links.begin(), port.links.end(), [](const auto& link) { 
                return StringUtils::join(link.get().outputs.begin(), link.get().outputs.end(), [](const auto& output) { return std::to_string(output.get().cell.id) + " (" + Cell::typeToStr(output.get().cell.type) + ")"; });
            }) << "]";
#endif
    }

    return os;
}
