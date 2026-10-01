// mini_gms_test.cpp - implementation of the mini_gms AST debug printer.
//
// This file implements the helpers declared in mini_gms_test.hpp. It walks
// a parsed mini_gms AST and prints it to std::cout in a syntax-like layout
// so that a human can eyeball the structure. It is not a unit test
// framework; it is a small manual inspection tool.
//
// Printing rules:
//   - var   : printed as null / true / false / number / "string"
//   - vec   : printed as { ... } with one element per line
//   - stu   : printed as { key value ... } with one member per line
//   - vec elements that are themselves vec get a leading ": " to echo the
//     "name : { ... }" source form

#include "mini_gms_test.hpp"
#include <iostream>
#include <variant>

namespace mol
{

    // Recursively prints one value with the given indentation.
    // Scalars are printed inline; vec and stu are printed as multi-line
    // blocks. Null pointers are printed as the word "null".
    void printValue(const ValuePtr &v, int indent)
    {
        std::string pad(indent * 2, ' ');

        if (!v)
        {
            std::cout << "null";
            return;
        }

        if (v->isVar())
        {
            const Var &var = v->asVar();

            if (std::holds_alternative<std::monostate>(var))
            {
                std::cout << "null";
            }
            else if (std::holds_alternative<bool>(var))
            {
                std::cout << (std::get<bool>(var) ? "true" : "false");
            }
            else if (std::holds_alternative<double>(var))
            {
                double d = std::get<double>(var);
                if (d == static_cast<long long>(d))
                    std::cout << static_cast<long long>(d);
                else
                    std::cout << d;
            }
            else
            { // std::string
                std::cout << "\"" << std::get<std::string>(var) << "\"";
            }
        }
        else if (v->isVec())
        {
            std::cout << "{\n";
            for (auto &e : v->asVec())
            {
                std::cout << pad << "  ";
                if (e->isVec())
                    std::cout << ": ";
                printValue(e, indent + 1);
                std::cout << "\n";
            }
            std::cout << pad << "}";
        }
        else
        { // Stu
            std::cout << "{\n";
            for (auto &kv : v->asStu())
            {
                std::cout << pad << "  " << kv.first;
                if (kv.second->isVar() || kv.second->isVec())
                    std::cout << " : ";
                else
                    std::cout << " ";
                printValue(kv.second, indent + 1);
                std::cout << "\n";
            }
            std::cout << pad << "}";
        }
    }

    // Prints every top-level declaration followed by a blank line.
    // The name is printed first, then the value via printValue.
    void dump(const std::vector<std::pair<std::string, ValuePtr>> &decls)
    {
        for (auto &kv : decls)
        {
            std::cout << kv.first << " ";
            printValue(kv.second);
            std::cout << "\n\n";
        }
    }

}