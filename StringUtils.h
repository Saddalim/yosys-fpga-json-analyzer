#pragma once

#include <string>
#include <sstream>

namespace StringUtils
{
    template <typename It, typename MapFunc>
    std::string join(It begin, It end, MapFunc func, std::string delimiter = ", ")
    {
        if (begin == end) return "";

        It it = begin;
        std::stringstream ss;
        ss << func(*it++);
        for (; it != end; ++it)
        {
            ss << delimiter << func(*it);
        }
        return ss.str();
    }

    template <typename It>
    std::string join(It begin, It end, std::string delimiter = ", ")
    {
        return join(begin, end, [](const auto& val){ return val; }, delimiter);
    }
}
