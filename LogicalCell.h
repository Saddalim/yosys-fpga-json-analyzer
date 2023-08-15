#pragma once

#include <ostream>
#include <string>
#include <iostream>
#include <map>
#include <list>
#include <functional>

#include "Port.h"

typedef int cellId_t;

struct LogicalCell
{
    static constexpr cellId_t INVALID_ID = -1; 

    cellId_t id = INVALID_ID;
    std::string type;
    std::map<std::string, Port> inputs;
    std::map<std::string, Port> outputs;

    void assignPort(std::string portName, Link& link, Port::Type type);

    LogicalCell() = delete;
    LogicalCell(cellId_t id, std::string type) : id(id), type(type) {}

};

std::ostream& operator<<(std::ostream& os, const LogicalCell& lc);
