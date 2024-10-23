#include "sexpresso.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

void pr(const auto& t, const std::string& indent="") {
    if (t.isSexp()){
        for (int i=0; i < t.childCount(); ++i)
          pr(t.getChild(i), indent + " ");
    } else if (t.isNil()) {
        std::cout << indent << "-" << std::endl;
    } else if (t.isString()) {
        std::cout << indent << t.toString() << std::endl;
    } else {
        std::cout << indent << "INVALID" << std::endl;
    }
}

int main(int argc, char** argv)
{
    if (argc < 2) return -1;
    std::cout << "Loading " << argv[1] << std::endl;
    auto fin = std::ifstream(argv[1]);
    std::stringstream buffer;
    buffer << fin.rdbuf();
    std::cout << "Read: " << buffer.str().size() << std::endl;
    auto tree = sexpresso::parse(buffer.str());
    
    for (auto&& args : tree.getChild(0).arguments()) {
        if (args.isSexp() && args.getChild(0).value.str == "footprint") {
            std::cout << args.getChild(1).toString() << std::endl;
        }

    }

    return 0;
}