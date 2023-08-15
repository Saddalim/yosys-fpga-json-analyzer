#include <iostream>
#include <ostream>
#include <fstream>
#include <map>
#include <list>
#include <set>

#include "LogicalCell.h"
#include "Port.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::set<cellId_t> visitedCells;

void crawlBack(LogicalCell& from)
{
    std::cout << " Cell #" << std::setw(2) << from.id;
    
    if (visitedCells.contains(from.id))
    {
        std::cerr << "\nCircular network! Found node #" << from.id << " again!\n";
        throw std::runtime_error("Circular network");
    }

    visitedCells.insert(from.id);

    bool hasNoInputs = true;
    for (auto& input : std::as_const(from.inputs))
    {
        for (const Link& link : input.second.links)
        {
            if (link.input != nullptr)
            {
                hasNoInputs = false;
                std::cout << "\n Cell #" << std::setw(2) << from.id << " " << std::setw(2) << input.second.name << " <- " << std::setw(2) << link.id << " <- " << std::setw(2) << link.input->name << " <-";
                crawlBack(link.input->cell);
            }
        }
    }
    if (hasNoInputs)
    {
        std::cout << "\nPotential root cell: #" << from.id << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Invalid number of arguments" << std::endl;
        return EXIT_FAILURE;
    }

    std::string fileName = argv[1];
    std::cout << "Opening file: " << fileName << std::endl;
    std::ifstream file(fileName);

    std::cout << "Parsing JSON..." << std::endl;
    json data = json::parse(file);

    try
    {
        const auto& project = data.at("modules").at("top");
        std::map<std::string, size_t> typeCnts;
        std::list<LogicalCell> cells;
        std::map<portId_t, Link> links;
        size_t cellCnt = 0;

        for (const auto& cell : project.at("cells"))
        {
            std::string cellType = cell.at("type");
            std::map<std::string, Port::Type> portTypes;

            if (typeCnts.count(cellType))
                ++typeCnts[cellType];
            else
                typeCnts[cellType] = 1;

            LogicalCell& lc = cells.emplace_back(cellCnt, cellType);
            
            for (const auto& connection : cell.at("port_directions").items())
            {
                if (connection.value() == "input")
                    portTypes.emplace(connection.key(), Port::Type::INPUT);
                else if (connection.value() == "output")
                    portTypes.emplace(connection.key(), Port::Type::OUTPUT);
                else
                    throw std::runtime_error("Could not determine type of port" + connection.key() + "of cell #" + std::to_string(lc.id) + ": " + std::string(connection.value()));
            }

            for (const auto& connectionList : cell.at("connections").items())
            {
                std::string portId = connectionList.key();
                for (const auto& remotePort : connectionList.value())
                {
                    switch (remotePort.type())
                    {
                        case json::value_t::string:
                            // not connected
                            break;
                        case json::value_t::number_integer:
                        case json::value_t::number_unsigned:
                            {
                                Link& link = links.try_emplace(remotePort, remotePort).first->second;
                                lc.assignPort(portId, link, portTypes.at(portId));
                            }
                            break;
                        default:
                            throw std::runtime_error("Invalid port connection in node #" + std::to_string(lc.id) + " port " + portId + ": " + std::string(remotePort));
                    }
                }
            }

            ++cellCnt;            
        }

        // Cell counts
        std::cout << "Parsed, found " << cellCnt << " cells:" << std::endl;
        for (const auto& typeData : typeCnts)
        {
            std::cout << typeData.first << " : " << typeData.second << '\n';
        }

        std::cout << "======================================================\n";
        // Raw cell data
        for (LogicalCell& cell : cells)
        {
            std::cout << cell << '\n';
        }

        std::cout << "======================================================\n";
        // Try to find root link
        crawlBack(cells.front());
        
    }
    catch (std::out_of_range&)
    {
        std::cerr << "Unexpected JSON schema" << std::endl;
        return EXIT_FAILURE;
    }
}
