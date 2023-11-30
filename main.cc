#include <iostream>
#include <ostream>
#include <fstream>
#include <map>
#include <list>
#include <set>
#include <algorithm>
#include <limits>

#include "Cell.h"
#include "Port.h"
#include "LogicCell.h"
#include "StringUtils.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

size_t histogramHeight = std::numeric_limits<size_t>::max();
size_t histogramWidth = 30;

std::list<LogicCell*> logicCells;

enum class CellVisitState
{
    NOT_VISITED, IN_PROGRESS, VISITED
};
struct CellVisitData
{
    CellVisitState state = CellVisitState::NOT_VISITED;
    std::list<cellId_t> longestPath;
};

void parseLogicCellsFromLUTs(std::list<Cell>& cells)
{
    for (Cell& cell : cells)
    {
        if (cell.type != Cell::Type::LUT) continue;
        if (cell.parentLC != nullptr) continue;

        LogicCell* lc = new LogicCell();
        lc->id = logicCells.size();
        lc->lut = &cell;
        cell.parentLC = lc;

        cell.doForAllOutputCells([&](Cell& nextCell) {
            if (nextCell.type != Cell::Type::DFF) return true;

            lc->dff = &nextCell;
            nextCell.parentLC = lc;

            nextCell.doForAllOutputCells([&](Cell& nextNextCell) {
                if (nextNextCell.type != Cell::Type::Carry) return true;

                lc->carry = &nextNextCell;
                nextNextCell.parentLC = lc;
                return false;
            });
            return false;
        });

        logicCells.push_back(lc);
    }
}

void parseLogicCellsFromDFFs(std::list<Cell>& cells)
{
    for (Cell& cell : cells)
    {
        if (cell.type != Cell::Type::DFF) continue;
        if (cell.parentLC != nullptr) continue;

        LogicCell* lc = new LogicCell();
        lc->id = logicCells.size();
        lc->dff = &cell;
        cell.parentLC = lc;

        cell.doForAllInputCells([&](Cell& prevCell) {
            if (prevCell.type != Cell::Type::LUT) return true;

            lc->lut = &prevCell;
            prevCell.parentLC = lc;
            return false;
        });
        cell.doForAllOutputCells([&](Cell& nextCell) {
            if (nextCell.type != Cell::Type::Carry) return true;

            lc->carry = &nextCell;
            nextCell.parentLC = lc;
            return false;
        });

        logicCells.push_back(lc);
    }
}

template<typename Mx>
void printMatrix(const Mx& matrix, size_t invalidValue)
{
    for (const auto& row : matrix)
        std::cout << StringUtils::join(row.begin(), row.end(), [&](auto& val){ return val == invalidValue ? "X" : std::to_string(val); }, std::string("\t")) << "\n";
    std::cout << std::endl;
}

std::list<cellId_t> crawlBackward(const Cell& cell, std::map<cellId_t, CellVisitData>& data, std::list<cellId_t> currentPath)
{
    if (! data.contains(cell.id))
    {
        data[cell.id] = CellVisitData();
    }
    CellVisitData& cellData = data.at(cell.id);
    currentPath.push_back(cell.id);

    if (cellData.state == CellVisitState::NOT_VISITED)
    {
        cellData.state = CellVisitState::IN_PROGRESS;

        size_t maxDepth = 0;
        std::list<cellId_t> longestPath;
        cell.doForAllInputCells([&](const Cell& prevCell) {
            if (prevCell.type == Cell::Type::DFF || prevCell.type == Cell::Type::Carry) return true;

            std::list<cellId_t> pathOnward = crawlBackward(prevCell, data, currentPath);
            size_t pathLengthOnward = pathOnward.size();
            if (pathOnward.size() > longestPath.size())
            {
                longestPath = std::move(pathOnward);
            }
            return true;
        });

        cellData.state = CellVisitState::VISITED;
        cellData.longestPath = longestPath;

        return longestPath;
    }
    else if (cellData.state == CellVisitState::IN_PROGRESS)
    {
        // circle!
        std::cout << "Circle in LUT graph at cell #" << cell.id << '\n';
        return {};
    }
    else if (cellData.state == CellVisitState::VISITED)
    {
        currentPath.emplace_back(cell.id);
        return currentPath;
    }

    std::cout << "Crawl found cell in unexpected state: " << (int)cellData.state << '\n';
    throw std::runtime_error("");
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Invalid number of arguments" << std::endl;
        return EXIT_FAILURE;
    }

    if (argc >= 3)
    {
        histogramHeight = std::stol(argv[2]);
    }

    std::cout << "Histogram height: " << (histogramHeight == std::numeric_limits<size_t>::max() ? "FULL" : std::to_string(histogramHeight)) << '\n';

    std::string fileName = argv[1];
    std::cout << "Opening file: " << fileName << std::endl;
    std::ifstream file(fileName);

    std::cout << "Parsing JSON..." << std::endl;
    json data = json::parse(file);

    std::map<std::string, size_t> typeCnts;
    std::map<cellId_t, Cell> cells;
    std::map<portId_t, Link> links;
    size_t cellCnt = 0;

    try
    {
        const auto& project = data.at("modules").at("top");

        for (const auto& cellData : project.at("cells").items())
        {
            std::string cellType = cellData.value().at("type");
            std::map<std::string, Port::Type> portTypes;

            if (typeCnts.count(cellType))
                ++typeCnts[cellType];
            else
                typeCnts[cellType] = 1;

            //Cell& cell = cells.emplace(cellCnt, {cellCnt, cellData.key(), cellType}).first->second;
            Cell& cell = cells.emplace(std::piecewise_construct, std::forward_as_tuple(cellCnt), std::forward_as_tuple(cellCnt, cellData.key(), cellType)).first->second;

            cell.verilogSrc = cellData.value().at("attributes").at("src");
            
            for (const auto& connection : cellData.value().at("port_directions").items())
            {
                if (connection.value() == "input")
                    portTypes.emplace(connection.key(), Port::Type::INPUT);
                else if (connection.value() == "output")
                    portTypes.emplace(connection.key(), Port::Type::OUTPUT);
                else
                    throw std::runtime_error("Could not determine type of port" + connection.key() + "of cell #" + std::to_string(cell.id) + ": " + std::string(connection.value()));
            }

            for (const auto& connectionList : cellData.value().at("connections").items())
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
                                cell.assignPort(portId, link, portTypes.at(portId));
                            }
                            break;
                        default:
                            throw std::runtime_error("Invalid port connection in node #" + std::to_string(cell.id) + " port " + portId + ": " + std::string(remotePort));
                    }
                }
            }

            ++cellCnt;            
        }

    }
    catch (std::out_of_range&)
    {
        std::cerr << "Unexpected JSON schema" << std::endl;
        return EXIT_FAILURE;
    }

    // Cell counts
    std::cout << "Parsed, found " << cellCnt << " cells of types:" << std::endl;
    for (const auto& typeData : typeCnts)
    {
        std::cout << typeData.first << " : " << typeData.second << '\n';
    }

    /*
    std::cout << "======================================================\n";
    // Raw cell data
    std::cout << "Raw cell data:" << std::endl;
    for (Cell& cell : cells)
    {
        std::cout << cell << '\n';
    }
    */

    std::cout << "======================================================\n";
    std::map<cellId_t, CellVisitData> cellVisitStates;
    std::vector<std::pair<cellId_t, CellVisitData>> cellData;
    std::map<size_t, size_t> histogramData;
    size_t maxCnt = 0;
    for (auto& cellPair : cells)
    {
        Cell& cell = cellPair.second;
        if (cell.type != Cell::Type::DFF && cell.type != Cell::Type::RAM) continue;
        std::list<cellId_t> longestPath;
        crawlBackward(cell, cellVisitStates, longestPath);
        cellData.emplace_back(cell.id, cellVisitStates.at(cell.id));

        size_t pathLength = cellVisitStates.at(cell.id).longestPath.size();
        if (pathLength > 0)
        {
            if (! histogramData.contains(pathLength))
            {
                histogramData[pathLength] = 1;
            }
            else
            {
                ++histogramData[pathLength];
            }
            maxCnt = std::max(maxCnt, histogramData[pathLength]);
        }
    }

    cellData.shrink_to_fit();


    std::sort(cellData.begin(), cellData.end(), [](auto& a, auto& b) { return a.second.longestPath.size() > b.second.longestPath.size(); });

    size_t topListSize = std::min((size_t)10, cellData.size());
    if (topListSize == 0)
    {
        std::cout << "Found no routes?!\n";
        return EXIT_SUCCESS;
    }

    std::cout << "The " << topListSize << " longest paths:\n";
    for (size_t i = 0; i < topListSize; ++i)
    {
        const Cell& cell = cells.at(cellData[i].first);
        std::cout << "#" << std::setw(2) << (i + 1) << ".: len: " << std::setw(3) << cellData[i].second.longestPath.size() << ", name: " << std::setw(50) << cell.name << ", src: " << cell.verilogSrc << '\n';
        for (cellId_t pathCellId : cellData[i].second.longestPath)
        {
            const Cell& pathCell = cells.at(pathCellId);
            std::cout << '\t' << pathCell.name << " (" << pathCell.verilogSrc << ")\n";
        }
        std::cout << '\n';
    }

    // Histogram
    std::cout << "\nRoute length histogram (distribution):\n\n";

    histogramHeight = std::min(histogramHeight, histogramData.size());
    size_t lenCatSize = histogramData.size() / histogramHeight;

    size_t fromLen = histogramData.begin()->first;
    size_t toLen = fromLen;
    size_t accuCnt = 0;
    size_t sumCnt = 0;

    std::cout << "  Length | Count\n---------+--------------------------------------\n";

    for (auto& histogramDatum : histogramData)
    {
        if (fromLen == toLen) fromLen = histogramDatum.first;
        sumCnt += histogramDatum.second;
        ++accuCnt;

        if (accuCnt % lenCatSize == 0 || histogramDatum == *(--histogramData.end()))
        {
            toLen = histogramDatum.first;
            size_t barWidth = (sumCnt * histogramWidth) / maxCnt;

            std::cout << " " 
                << std::setw(3) << std::right << fromLen 
                << "-"
                << std::setw(3) << std::left << toLen
                << " | " 
                << std::setw(3) << sumCnt 
                << " ";

            for (size_t i = 0; i < barWidth; ++i) std::cout << "*";
            std::cout << '\n';

            fromLen = toLen;

            accuCnt = 0;
            sumCnt = 0;
        }
    }

    std::cout << "\nDone\n";
}
