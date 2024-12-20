#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <variant>

namespace sexpression {

// Used to denote that a string is really a token
struct token {
    token(const std::string& v) : value(v) {};
    std::string value;
};

#define T(x__) token(#x__)

class consbox;

// Use nil for a null next pointer
struct niltype {};
std::shared_ptr<consbox> nil;

// consbox pointer type and item type
using consboxp = std::shared_ptr<consbox>;
using cons_box_item_type = std::variant<niltype, token, long, double, bool, std::string, consboxp>;

// forward defs
std::ostream& operator<<(std::ostream& s, const consboxp box);
std::ostream& operator<<(std::ostream& s, const cons_box_item_type& item);

// don't construct this all call methods directly
class consbox
{
public:
    consbox() noexcept : _item(nil) {}
    consbox(cons_box_item_type&& val, consboxp next) noexcept : _item(std::move(val)), _next(next) {}

    cons_box_item_type& car() noexcept { return _item; }
    consboxp cdr() noexcept { return _next; }

    std::string toString() const
    {
        std::stringstream buf;
        buf << _item;
        if (_next != nullptr) {
            buf << " " << _next->toString();
        }
        return buf.str();
    }

private:
    cons_box_item_type _item;
    consboxp _next;
    friend consboxp list();
    friend consboxp append(consboxp, consboxp);
};


std::ostream& operator<<(std::ostream& s, const consboxp box)
{
    s << '(' << box->toString() << ')';
    return s;
}

std::ostream& operator<<(std::ostream& s, const cons_box_item_type& item)
{
    if (const long* pval = std::get_if<long>(&item))
        s << *pval;
    else if (const double* pval = std::get_if<double>(&item))
        s << *pval;
    else if (const bool* pval = std::get_if<bool>(&item))
        s << (*pval ? "true" : "false");
    else if (const token* pval = std::get_if<token>(&item))
        s << pval->value;
    else if (const std::string* pval = std::get_if<std::string>(&item))
        s << '"' << *pval << '"';
    else if (const std::shared_ptr<consbox>* pval = std::get_if<std::shared_ptr<consbox>>(&item)) {
        if (*pval != nullptr) {
            s << '(' << (*pval)->toString() << ')';
        }
    } else
        s << "INVALID TYPE!";
    return s;
}

consboxp cons(cons_box_item_type&& a, consboxp rest)
{
    consboxp box = std::make_shared<consbox>(std::move(a), rest);
    return box;
}

template<typename H>
consboxp list(H head)
{
    return cons(head, nil);
}

template<typename H, typename ... T>
consboxp list(H head, T...tail)
{
    return cons(head, list(tail...));
}

consboxp append(consboxp a, consboxp b)
{
    consboxp rval = a;
    while (a->_next != nullptr) a = a->_next;
    a->_next = b;
    return rval;
}

cons_box_item_type car(consboxp b) { return b->car(); }
consboxp cdr(consboxp b) { return b->cdr(); }

consboxp parse(std::istream& in)
{
    std::string token;
    in >> std::ws >> token;
}

}