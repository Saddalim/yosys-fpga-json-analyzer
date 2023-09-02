#include <iostream>
#include <ostream>
#include <fstream>
#include <map>
#include <list>
#include <set>

#include "Cell.h"
#include "Port.h"
#include "LogicCell.h"
#include "StringUtils.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::list<LogicCell*> logicCells;

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

    std::map<std::string, size_t> typeCnts;
    std::list<Cell> cells;
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

            Cell& cell = cells.emplace_back(cellCnt, cellData.key(), cellType);
            
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
        dffCell.doForAllInputCells([](const Cell& cell) { if (cell.type != Cell::Type::LUT) std::cerr << "Unexpected cell type " << cell.type << " as input of a DFF: " << cell << " {" << cell.name << "}" << std::endl; return true; } );
    }

    /*
    std::cout << "======================================================\n";
    std::set<cellId_t> visitedCells;
    for (const Cell& cell : cells)
    {
        try
        {
            std::cout << "Start crawl from cell #" << cell.id << std::endl;
            if (! visitedCells.contains(cell.id))
                crawlForward(cell, cells.size());
        }
        catch (std::runtime_error& ex)
        {
            std::cout << "Crawl terminated because: " << ex.what() << std::endl;
            break;
        }
    }
    */
    
    std::cout << "======================================================\n";
    // Build LCs
    std::cout << "Building logic cell hierarchy..." << std::endl;
    parseLogicCellsFromDFFs(cells);
    for (auto& lc : logicCells)
    {
        std::cout << *lc << std::endl;
    }
    
    std::cout << "======================================================\n";
    std::cout << "Looking for longest chain of orphan LUTs...\n";
    std::vector<std::reference_wrapper<Cell>> orphanCells;

    std::copy_if(cells.begin(), cells.end(), std::back_inserter(orphanCells), [](const Cell& cell) { return cell.parentLC == nullptr; });
    orphanCells.shrink_to_fit();
    const cellId_t orphanCellCnt = orphanCells.size();

    std::cout << "Found " << orphanCellCnt << " orphan cells:\n";
    for (const Cell& orphanCell : orphanCells)
    {
        std::cout << orphanCell << std::endl;
    }

    std::cout << "Initializing longest-path-search...\n";
    constexpr cellId_t INF = std::numeric_limits<cellId_t>::max() / 2 - 1;
    std::vector<std::vector<cellId_t>> distances(orphanCellCnt, std::vector<cellId_t>(orphanCellCnt, INF));
    cellId_t repEveryNth = std::max(orphanCellCnt, 100) / 100;

    // Bellman-Ford algorithm
    std::vector<std::vector<cellId_t>> predecessors(orphanCellCnt, std::vector<cellId_t>(orphanCellCnt, INF));

    for (cellId_t sourceCellId = 0; sourceCellId < orphanCellCnt; ++sourceCellId)
    {
        for (cellId_t v = 0; v < orphanCellCnt; ++v)
        {
            distances[sourceCellId][v] = INF;
            predecessors[sourceCellId][v] = Cell::INVALID_ID;
        }

        distances[sourceCellId][sourceCellId] = 0;

        for (cellId_t cycle = 0; cycle < orphanCellCnt - 1; ++cycle)
        {
            for (cellId_t u = 0; u < orphanCellCnt; ++u)
            {
                orphanCells[u].get().doForAllOutputCells([&](const Cell& to){
                    cellId_t v = to.id;
                    if (u == v) return true;
                    if (distances[sourceCellId][u] != INF && distances[sourceCellId][u] + (-1) < distances[sourceCellId][v])
                    {
                        distances[sourceCellId][v] = distances[sourceCellId][u] + (-1);
                        predecessors[sourceCellId][v] = u;
                    }
                    return true;
                });
            }
        }

        // Check for circles
        for (cellId_t cycle = 0; cycle < orphanCellCnt; ++cycle)
        {
            for (cellId_t u = 0; u < orphanCellCnt; ++u)
            {
                orphanCells[u].get().doForAllOutputCells([&](const Cell& to){
                    cellId_t v = to.id;
                    if (u == v) return true;
                    if (distances[sourceCellId][u] == INF || distances[sourceCellId][v] == INF)
                    {
                        // TODO handle component count
                    }
                    else if (distances[sourceCellId][u] + (-1) < distances[sourceCellId][v])
                    {
                        std::cerr << "LUTs form a cycle from node #" << orphanCells[sourceCellId].get().id << ", unable to determine longest path" << std::endl;
                        return false;
                        //predecessors[sourceCellId][v] = u;
                    }
                    return true;
                });
            }
        }

        if (sourceCellId % repEveryNth == 0)
            std::cout << "\r" << (sourceCellId * 100 / orphanCellCnt) << "%" << std::flush;
    }
    std::cout << "\r100%\n";

    cellId_t maxLength = 0;
    std::pair<cellId_t, cellId_t> between;
    cellId_t componentCnt;
    for (cellId_t u = 0; u < orphanCellCnt; ++u)
    {
        for (cellId_t v = 0; v < orphanCellCnt; ++v)
        {
            if (distances[u][v] == INF)
            {
                // TODO handle component count
            }
            else if (maxLength > distances[u][v]) // inverse due to negative weights
            {
                maxLength = distances[u][v];
                between = {u, v};
            }
        }
    }

    //printMatrix(distances, INF);

    // maxLength is the max length between 2 orphans. Full LUT length shall include the hop to the first and from the last orphan LUT
    std::cout << "Longest of the shortest chains is " << (maxLength * -1 + 2) << " LUTs between (excl. first and last cells):\n"
        << orphanCells[between.first] << std::endl << orphanCells[between.second] << std::endl;

    // Floyd-Warshall algorithm
    /*

    for (cellId_t u = 0; u < orphanCellCnt; ++u)
    {
        for (cellId_t v = 0; v < orphanCellCnt; ++v)
        {
            if (u == v)
                distances[u][v] = 0;
            else if (orphanCells[u].get().hasLinkTo(v))
                distances[u][v] = 1;
        }
    }

    //printMatrix(distances, INF);

    std::cout << "Running longest-path-search...\n";

    for (cellId_t k = 0; k < orphanCellCnt; ++k)
    {
        for (cellId_t i = 0; i < orphanCellCnt; ++i)
        {
            for (cellId_t j = 0; j < orphanCellCnt; ++j)
            {
                if (distances[i][k] != INF && distances[k][j] != INF
                    && distances[i][j] > distances[i][k] + distances[k][j])
                {
                    distances[i][j] = distances[i][k] + distances[k][j];
                }
            }
        }
        if (k % repEveryNth == 0)
            std::cout << "\r" << (k * 100 / orphanCellCnt) << "%" << std::flush;
    }
    std::cout << "\r100%\n";

    //printMatrix(distances, INF);
    // determining max chain length and separate paths
    cellId_t maxLength = 0;
    std::pair<cellId_t, cellId_t> between;

    size_t separatePathCnt = 1;
    std::set<cellId_t> groupedCellIds;
    std::set<cellId_t> unreachableCellIds;
    std::list<std::set<cellId_t>> separatePaths;
    std::set<cellId_t>& currentPath = separatePaths.emplace_back();

    for (cellId_t i = 0; i < orphanCellCnt; ++i)
    {
        if (unreachableCellIds.contains(i))
        {
            currentPath = separatePaths.emplace_back();
        }
        groupedCellIds.insert(i);
        for (cellId_t j = 0; j < orphanCellCnt; ++j)
        {
            if (groupedCellIds.contains(j))
                std::cerr << "Szar van a palacsintaban!\n";
            currentPath.insert(j);
            if (distances[i][j] == INF)
            {
                unreachableCellIds.insert(j);
            }
            else if (distances[i][j] > maxLength)
            {
                maxLength = distances[i][j];
                between.first = i;
                between.second = j;
            }
        }
    }

    // maxLength is the max length between 2 orphans. Full LUT length shall include the hop to the first and from the last orphan LUT
    std::cout << "Longest of the shortest chains is " << (maxLength + 2) << " LUTs between (excl. first and last cells):\n"
        << orphanCells[between.first] << std::endl << orphanCells[between.second] << std::endl;
    */
    
}
