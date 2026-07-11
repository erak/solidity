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

#include <liblangutil/Exceptions.h>
#include <libsolidity/interface/StandardJSONInput.h>

#include <optional>

namespace solidity::frontend::test
{

using namespace solidity::frontend::input;
using namespace output;

template<typename T>
concept Compiler = requires(T t, StandardJSONInput const& input) {
	{ t.compile(input) } -> std::same_as<StandardJSONOutput>;
};

template<typename T>
concept StandardJSONOutputType = std::constructible_from<T, StandardJSONOutput>;

/**
 * Provides an interface to the compiler under test.
 *
 * @tparam C A type satisfying the Compiler concept — provides `compile(StandardJSONInput const&) -> StandardJSONOutput`.
 * @tparam Output A type satisfying StandardJSONOutputType — constructible from StandardJSONOutput.
 */
template<Compiler C, StandardJSONOutputType Output = StandardJSONOutput>
class StandardJSONCompiler
{
public:
	template<typename... Args>
	explicit StandardJSONCompiler(Args&&... args):
		m_compiler(std::forward<Args>(args)...)
	{}

	/// Takes the current compiler input, requests the compiler under test to compile
	/// and stores its output.
	/// @returns the stored output
	/// @param _input to pass to the compiler
	Output const& compile(StandardJSONInput const& _input)
	{
		m_output.emplace(Output{m_compiler.compile(_input)});
		return this->output();
	}

	/// @returns the stored output generated during previous compilation.
	Output const& output() const
	{
		solAssert(m_output.has_value(), "No output found. Please compile first.");
		return m_output.value();
	}

private:
	C m_compiler;
	std::optional<Output> m_output;
};

}