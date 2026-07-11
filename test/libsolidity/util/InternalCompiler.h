/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <test/libsolidity/util/StandardJSONOutput.h>

#include <libsolidity/interface/StandardCompiler.h>
#include <libsolidity/interface/StandardJSONInput.h>

namespace solidity::frontend::test
{

using namespace solidity::frontend::input;

/**
 * Compiler implementation that delegates to the in-process StandardCompiler.
 */
class InternalCompiler
{
public:
	InternalCompiler() = default;

	/// Compiles the given input using the in-process StandardCompiler and
	/// @returns the deserialized StandardJSONOutput.
	StandardJSONOutput compile(StandardJSONInput const& _input);
};

inline StandardJSONOutput InternalCompiler::compile(StandardJSONInput const& _input)
{
	Json jsonInput = _input;
	Json jsonOutput = StandardCompiler{}.compile(jsonInput);
	return jsonOutput.get<StandardJSONOutput>();
}

}