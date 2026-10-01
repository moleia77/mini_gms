// mini_gms_test.hpp - declarations for the mini_gms AST debug printer.
//
// This header is used by the test / demo side of the project. It declares
// two helpers that walk a parsed mini_gms AST and print it in a readable
// form. They are meant for manual inspection and small test programs, not
// for production use.
//
// Both helpers operate on the raw AST returned by mol::parse, i.e. a list
// of top-level (name, ValuePtr) pairs.

#pragma once

#include "mini_gms.hpp"
#include <string>

namespace mol
{

	// Recursively prints a single AST value to std::cout.
	// indent controls the number of leading spaces per nesting level.
	// Handles all three node kinds: var, vec, and stu.
	void printValue(const ValuePtr &v, int indent = 0);

	// Prints every top-level declaration in the given AST, one per block.
	// Calls printValue internally for each declaration value.
	void dump(const std::vector<std::pair<std::string, ValuePtr>> &decls);

}