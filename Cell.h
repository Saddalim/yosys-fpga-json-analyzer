#pragma once

#include <ostream>
#include <string>
#include <iostream>
#include <map>
#include <set>
#include <list>
#include <functional>

#include "Port.h"

typedef int cellId_t;

class LogicCell;

struct Cell
{
    enum class Type
    {
        Unknown, LUT, DFF, Carry
    };

    static std::string typeToStr(Type type)
    {
        switch (type)
        {
            case Cell::Type::LUT: return "LUT";
            case Cell::Type::DFF: return "DFF";
            case Cell::Type::Carry: return "Carry";
        }
        return "???";
    }

    static constexpr cellId_t INVALID_ID = -1; 

    cellId_t id = INVALID_ID;
    std::string name;
    Type type;
    std::map<std::string, Port> inputs;
    std::map<std::string, Port> outputs;
    LogicCell* parentLC;

    void assignPort(std::string portName, Link& link, Port::Type type);


    static void crawlForward(const Cell& from, const bool stopOnCircular = true, const size_t maxCellCnt = 0);
    static void doCrawlForward(const Cell& from, const bool stopOnCircular, const size_t maxCellCnt, std::set<cellId_t>& visitedCells);
    static Type parseType(const std::string& str);
    
    template <typename Func>
    void doForAllInputCells(Func func) const
    {
        for (auto& input : inputs)
        {
            for (auto& link : input.second.links)
            {
                if (link.get().input != nullptr)
                    func(link.get().input->cell);
            }
        }
    }

    template <typename Func>
    void doForAllOutputCells(Func func) const
    {
        for (auto& output : outputs)
        {
            for (auto& link : output.second.links)
            {
                for (auto& port : link.get().outputs)
                {
                    func(port.get().cell);
                }
            }
        }
    }

    template <typename Predicate, typename Callback>
    void crawlForwardUntil(Predicate continuePredicate, Callback callback, const size_t maxDepth)
    {
        if (! continuePredicate(*this)) return;
        callback(*this);
        // TODO better to reuse, but this fails to link :(
        //auto f1 = std::bind(&Cell::crawlForwardUntil<Predicate, Callback, const size_t>, this, continuePredicate, callback, maxDepth - 1);
        
        for (auto& output : outputs)
        {
            for (auto& link : output.second.links)
            {
                for (auto& port : link.get().outputs)
                {
                    port.get().cell.crawlForwardUntil(continuePredicate, callback, maxDepth - 1);
                }
            }
        }
    }

    Cell() = delete;
    Cell(cellId_t id, std::string name, Type type) : id(id), name(name), type(type), parentLC(nullptr) {}
    Cell(cellId_t id, std::string name, std::string typeStr) : Cell(id, name, parseType(typeStr)) {}
};

std::ostream& operator<<(std::ostream& os, const Cell& lc);
std::ostream& operator<<(std::ostream& os, Cell::Type type);
