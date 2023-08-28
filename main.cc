#include <iostream>
#include <ostream>
#include <fstream>
#include <map>
#include <list>
#include <set>

#include "Cell.h"
#include "Port.h"
#include "LogicCell.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::list<LogicCell*> logicCells;

void parseLogicCells(std::list<Cell>& cells)
{
    std::set<Cell> visitedCells;
    
    for (Cell& cell : cells)
    {
        if (cell.parentLC != nullptr) continue;

        switch (cell.type)
        {
            case Cell::Type::LUT:
                cell.doForAllOutputCells([&](Cell& nextCell) {
                    if (nextCell.type == Cell::Type::DFF)
                    {
                        if (nextCell.parentLC == nullptr)
                        {
                            LogicCell* newLC = new LogicCell();
                            newLC->id = logicCells.size();
                            newLC->lut = &cell;
                            newLC->dff = &nextCell;
                            cell.parentLC = newLC;
                            nextCell.parentLC = newLC;
                            logicCells.push_back(newLC);
                        }
                        else
                        {
                            if (cell.parentLC != nullptr) std::cerr << "WARNING: overwriting parent LC for cell: " << cell << std::endl;
                            cell.parentLC = nextCell.parentLC;
                            if (cell.parentLC->lut != nullptr) std::cerr << "WARNING: overwriting LUT of LC: " << *(cell.parentLC->lut) << " to " << cell << std::endl;
                            cell.parentLC->lut = &cell;
                        }
                    }
                });
                break;
            case Cell::Type::DFF:
                cell.doForAllInputCells([&](Cell& prevCell) {
                    if (prevCell.type == Cell::Type::LUT)
                    {
                        if (prevCell.parentLC == nullptr)
                        {
                            LogicCell* newLC = new LogicCell();
                            newLC->id = logicCells.size();
                            newLC->dff = &cell;
                            newLC->lut = &prevCell;
                            cell.parentLC = newLC;
                            prevCell.parentLC = newLC;
                            logicCells.push_back(newLC);
                        }
                        else
                        {
                            if (cell.parentLC != nullptr) std::cerr << "WARNING: overwriting parent LC for cell: " << cell << std::endl;
                            cell.parentLC = prevCell.parentLC;
                            if (cell.parentLC->dff != nullptr) std::cerr << "WARNING: overwriting DFF of LC: " << *(cell.parentLC->dff) << " to " << cell << std::endl;
                            cell.parentLC->dff = &cell;
                        }
                    }
                });
                cell.doForAllOutputCells([&](Cell& nextCell) {
                    if (nextCell.type == Cell::Type::Carry)
                    {
                        if (nextCell.parentLC == nullptr)
                        {
                            LogicCell* newLC = new LogicCell();
                            newLC->id = logicCells.size();
                            newLC->dff = &cell;
                            newLC->carry = &nextCell;
                            cell.parentLC = newLC;
                            nextCell.parentLC = newLC;
                            logicCells.push_back(newLC);
                        }
                        else
                        {
                            if (cell.parentLC != nullptr) std::cerr << "WARNING: overwriting parent LC for cell: " << cell << std::endl;
                            cell.parentLC = nextCell.parentLC;
                            if (cell.parentLC->dff != nullptr) std::cerr << "WARNING: overwriting DFF of LC: " << *(cell.parentLC->dff) << " to " << cell << std::endl;
                            cell.parentLC->dff = &cell;
                        }
                    }
                });
                break;
            case Cell::Type::Carry:
                cell.doForAllInputCells([&](Cell& prevCell) {
                    if (prevCell.type == Cell::Type::DFF)
                    {
                        if (prevCell.parentLC == nullptr)
                        {
                            LogicCell* newLC = new LogicCell();
                            newLC->id = logicCells.size();
                            newLC->dff = &prevCell;
                            newLC->carry = &cell;
                            cell.parentLC = newLC;
                            prevCell.parentLC = newLC;
                            logicCells.push_back(newLC);
                        }
                        else
                        {
                            if (cell.parentLC != nullptr) std::cerr << "WARNING: overwriting parent LC for cell: " << cell << std::endl;
                            cell.parentLC = prevCell.parentLC;
                            if (cell.parentLC->carry != nullptr) std::cerr << "WARNING: overwriting Carry of LC: " << *(cell.parentLC->carry) << " to " << cell  << std::endl;
                            cell.parentLC->carry = &cell;
                        }
                    }
                });
                break;
        }
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
        std::list<Cell> cells;
        std::map<portId_t, Link> links;
        size_t cellCnt = 0;

        for (const auto& cell : project.at("cells").items())
        {
            std::string cellType = cell.value().at("type");
            std::map<std::string, Port::Type> portTypes;

            if (typeCnts.count(cellType))
                ++typeCnts[cellType];
            else
                typeCnts[cellType] = 1;

            Cell& lc = cells.emplace_back(cellCnt, cell.key(), cellType);
            
            for (const auto& connection : cell.value().at("port_directions").items())
            {
                if (connection.value() == "input")
                    portTypes.emplace(connection.key(), Port::Type::INPUT);
                else if (connection.value() == "output")
                    portTypes.emplace(connection.key(), Port::Type::OUTPUT);
                else
                    throw std::runtime_error("Could not determine type of port" + connection.key() + "of cell #" + std::to_string(lc.id) + ": " + std::string(connection.value()));
            }

            for (const auto& connectionList : cell.value().at("connections").items())
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
        std::cout << "Parsed, found " << cellCnt << " cells of types:" << std::endl;
        for (const auto& typeData : typeCnts)
        {
            std::cout << typeData.first << " : " << typeData.second << '\n';
        }

        std::cout << "======================================================\n";
        // Raw cell data
        std::cout << "Raw cell data:" << std::endl;
        for (Cell& cell : cells)
        {
            std::cout << cell << '\n';
        }

        std::cout << "======================================================\n";
        // Check for DFFs with non-LUT inputs - that would mean some logic madness
        std::cout << "Checking cell hierarchy..." << std::endl;
        for (Cell& dffCell : cells)
        {
            if (dffCell.type != Cell::Type::DFF) continue;
            dffCell.doForAllInputCells([](const Cell& cell) { if (cell.type != Cell::Type::LUT) std::cerr << "Unexpected cell type " << cell.type << " as input of: " << cell << std::endl; } );
        }

        /*
        std::cout << "======================================================\n";
        std::set<cellId_t> visitedCells;
        for (const Cell& cell : cells)
        {
            std::cout << "Start crawl from cell #" << cell.id << std::endl;
            if (! visitedCells.contains(cell.id))
                Cell::doCrawlForward(cell, false, cells.size(), visitedCells);
        }
        */
       
        std::cout << "======================================================\n";
        // Build LCs
        std::cout << "Building logic cell hierarchy..." << std::endl;
        parseLogicCells(cells);
        for (auto& lc : logicCells)
        {
            std::cout << *lc << std::endl;
        }
        std::cout << "======================================================\n";
        std::cout << "Looking for longest chain of orphan LUTs...\n";
        std::cout << "Orphan cells:\n";
        std::list<std::reference_wrapper<Cell>> orphanCells;
        for (Cell& cell : cells)
        {
            if (cell.parentLC != nullptr) continue;
            if (cell.type != Cell::Type::LUT)
            {
                std::cout << "Unexpected orphan cell type: " << cell << std::endl;
                continue;
            }
            orphanCells.push_back(cell);
            std::cout << cell << std::endl;
            std::string chain;
            size_t orphanChainLength = 0;
            cell.crawlForwardUntil(
                [](const Cell& cell) { return cell.parentLC == nullptr; },
                [&] (const Cell& cell) { ++orphanChainLength; chain += std::to_string(cell.id) + " -> "; },
                cells.size()
            );
            std::cout << "\t[" << orphanChainLength << "]: " << chain << std::endl;
        }
        for (auto& orphanCell : orphanCells)
        {
        }
    }
    catch (std::out_of_range&)
    {
        std::cerr << "Unexpected JSON schema" << std::endl;
        return EXIT_FAILURE;
    }
}
