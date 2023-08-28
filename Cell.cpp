#include "Cell.h"

#include "Port.h"
#include "StringUtils.h"

#include <iostream>
#include <iomanip>

void Cell::assignPort(std::string portName, Link& link, Port::Type type)
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

/*static*/ void Cell::crawlForward(const Cell& from, const bool stopOnCircular /* = true */, const size_t maxCellCnt /* = 0 */)
{
    std::set<cellId_t> visitedCells;
    doCrawlForward(from, stopOnCircular, maxCellCnt, visitedCells);
}

/*static*/ void Cell::doCrawlForward(const Cell& from, const bool stopOnCircular, const size_t maxCellCnt, std::set<cellId_t>& visitedCells)
{
    std::cout << " Cell #" << std::setw(2) << from.id << " (" << from.type << ")";
    
    if (visitedCells.contains(from.id))
    {
        std::cerr << "\nCircular network! Found node #" << from.id << " again!\n";
        if (stopOnCircular)
            throw std::runtime_error("Circular network");
        else
            return;
    }

    visitedCells.insert(from.id);
    if (maxCellCnt > 0 && visitedCells.size() >= maxCellCnt)
    {
        std::cout << "\nMax cell count reached, terminating crawl\n";
        throw std::runtime_error("Max cell count reached");
    }

    bool hasNoConnectedOutputs = true;
    for (auto& output : from.outputs)
    {
        for (const Link& link : output.second.links)
        {
            for (auto inputPort : link.outputs)
            {
                hasNoConnectedOutputs = false;
                std::cout << "\n Cell #" << std::setw(2) << from.id << " (" << from.type << ") " << std::setw(2) << output.second.name << " -> " << std::setw(2) << link.id << " -> " << std::setw(2) << inputPort.get().name << " ->";
                doCrawlForward(inputPort.get().cell, stopOnCircular, maxCellCnt, visitedCells);
            }
        }
    }
    if (hasNoConnectedOutputs)
    {
        std::cout << "\nDead-end: #" << from.id << " (" << from.type << ") (" << from.name << ")" << std::endl;
    }
}

/*static*/ Cell::Type Cell::parseType(const std::string& str)
{
    if (str == "SB_LUT4") return Cell::Type::LUT;
    else if (str == "SB_DFF") return Cell::Type::DFF;
    else if (str == "SB_DFFE") return Cell::Type::DFF;
    else if (str == "SB_CARRY") return Cell::Type::Carry;

    return Cell::Type::Unknown;
}

std::ostream& operator<<(std::ostream& os, const Cell& lc)
{
    os << "Cell #" << lc.id << " (" << lc.type 
        << ") IN: [" << StringUtils::join(lc.inputs.cbegin(), lc.inputs.cend(), [](const auto& inputPair) { return inputPair.second; })
        << "]\tOUT: [" << StringUtils::join(lc.outputs.cbegin(), lc.outputs.cend(), [](const auto& inputPair) { return inputPair.second; })
        << "]";

    return os;
}

std::ostream& operator<<(std::ostream& os, Cell::Type type)
{
    os << Cell::typeToStr(type);
    return os;
}
