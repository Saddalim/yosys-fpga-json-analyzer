#pragma once

#include <string>
#include <list>
#include <functional>

struct Cell;

typedef int portId_t;

struct Link;

struct Port
{
    static constexpr portId_t INVALID_ID = -1;

    enum class Type { INPUT, OUTPUT };

    std::string name;
    Cell& cell;
    Type type;
    std::list<std::reference_wrapper<Link>> links;

    Port() = delete;
    Port(std::string name, Cell& cell, Link& link, Type type);
};

std::ostream& operator<<(std::ostream& os, const Port& port);

struct Link
{
    portId_t id;
    Port* input;
    std::list<std::reference_wrapper<Port>> outputs;

    Link() = delete;
    Link(portId_t id) : id(id), input(nullptr) {}
};
