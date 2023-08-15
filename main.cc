#include <iostream>
#include <ostream>
#include <fstream>
#include <map>
#include <list>

#include <format>

#include "LogicalCell.h"
#include "Port.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Invalid number of arguments";
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
                    throw std::runtime_error(std::format("Could not determine type of port {} of cell #{}: {}", connection.key(), std::to_string(lc.id), std::string(connection.value())));
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
                            throw std::runtime_error(std::format("Invalid port connection in node #{} port {}: {}", lc.id, portId, std::string(remotePort)));
                    }
                }
            }

            ++cellCnt;            
        }

        std::cout << "Parsed, found " << cellCnt << " cells:" << std::endl;
        for (const auto& typeData : typeCnts)
        {
            std::cout << typeData.first << " : " << typeData.second << '\n';
        }
        for (auto& cell : cells)
        {
            std::cout << cell << '\n';
        }
    }
    catch (std::out_of_range&)
    {
        std::cerr << "Unexpected JSON schema" << std::endl;
        return EXIT_FAILURE;
    }
}
