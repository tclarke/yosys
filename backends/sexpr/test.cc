#include "sexpression.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace sexpression;

int main()
{
    std::cout << cons(token("foo"), cons("bar", cons(1, cons(2.345, nil)))) << std::endl;
    
    auto foo = cons(cons(1, cons(2, nil)), cons("ack", nil));
    std::cout << foo << '\n' << car(foo) << '\n' << cdr(foo) << std::endl;
    auto bar = cons(token("foo"), cons("bar", nil));
    std::cout << bar << '\n' << car(bar) << '\n' << cdr(bar) << std::endl;

    std::cout << list(1L, 2L, 3L, 4L) << std::endl;

    return 0;
}