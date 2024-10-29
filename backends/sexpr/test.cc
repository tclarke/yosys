#include "sexpression.hpp"
#include <iostream>
#include "sexpr_driver.h"

using namespace sexpression;

int main(int argc, char** argv)
{
    Sexpr_Driver driver;
    driver.parse(argv[1]);

    return 0;
}