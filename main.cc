#include <iostream>
#include <fstream>
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
        auto project = data.at("modules").at("top");

        std::cout << "Parsed, found " << project.size() << " nodes" << std::endl;
    }
    catch (std::out_of_range&)
    {
        std::cerr << "Unexpected JSON schema" << std::endl;
        return EXIT_FAILURE;
    }
}
