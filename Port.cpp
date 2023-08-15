#include "Port.h"

#include <sstream>

#include "LogicalCell.h"
#include "StringUtils.h"

Port::Port(std::string name, LogicalCell& cell, Link& link, Type type)
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
        os << linksStr << " > " << port.name;
    else
        os << port.name << " > " << linksStr;

    return os;
}
