#include "LogicalCell.h"

#include "Port.h"
#include "StringUtils.h"

void LogicalCell::assignPort(std::string portName, Link& link, Port::Type type)
{
    if (type == Port::Type::INPUT)
    {
        auto itInput = inputs.find(portName);
        if (itInput == inputs.end())
        {
            Port& port = inputs.emplace(std::piecewise_construct, std::forward_as_tuple(portName), std::forward_as_tuple(portName, *this, link, type)).first->second;
            port.links.push_back(link);
            link.outputs.push_back(port);
        }
        else
        {
            std::cerr << "WARNING: ignoring duplicated input " << portName << " of cell #" << id << " already set to link #" << link.id << ", instead of link #" << itInput->second.links.front().get().id << ". Possible config error?" << std::endl;
        }
    }
    else if (type == Port::Type::OUTPUT)
    {
        Port& port = outputs.try_emplace(portName, portName, *this, link, type).first->second;
        port.links.push_back(link);
        if (link.input == nullptr)
        {
            link.input = &port;
        }
        else
        {
            std::cerr << "WARNING: link#" << link.id << " already has a connected input port: " << link.input->name << ", cannot add port " << portName << " as input" << std::endl;
        }
    }
    else
    {
        throw std::runtime_error("Invalid port type");
    }
}

std::ostream& operator<<(std::ostream& os, const LogicalCell& lc)
{
    os << "Cell #" << lc.id << " (" << lc.type 
        << ") IN: [" << StringUtils::join(lc.inputs.cbegin(), lc.inputs.cend(), [](const auto& inputPair) { return inputPair.second; })
        << "] OUT: [" << StringUtils::join(lc.outputs.cbegin(), lc.outputs.cend(), [](const auto& inputPair) { return inputPair.second; })
        << "]";

    return os;
}
