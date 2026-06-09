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

#include <test/libsolidity/util/Common.h>
#include <test/libsolidity/util/StandardJSONOutput.h>

#include <libevmasm/Assembly.h>

#include <liblangutil/Exceptions.h>

#include <libsolutil/JSON.h>
#include <libsolutil/Common.h>
#include <libsolidity/ast/AST.h>
#include <libsolidity/interface/DebugSettings.h>
#include <libsolidity/interface/StandardCompiler.h>
#include <libsolidity/interface/StandardJSONInput.h>

#include <boost/algorithm/string/join.hpp>

#include <range/v3/algorithm.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/concat.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/transform.hpp>

#include <vector>

using namespace solidity;
using namespace solidity::util;
using namespace solidity::frontend::input;

namespace solidity::frontend::test
{

/**
 *
 */
class StandardJSONOutputExt
{
public:
	///
	explicit StandardJSONOutputExt(output::StandardJSONOutput _base):
		m_base(std::move(_base))
	{
		m_compilationOrder = m_base.sources() | ranges::views::transform([](auto const& entry) {
			auto name = entry.first;
			auto source = entry.second;

			if (!source.ast)
				return std::pair{name, std::vector<ContractName>{}};

			auto order = source.ast.value().at("nodes") | ranges::views::filter([](auto const& node) {
				return node.at("nodeType") == "ContractDefinition";
			}) | ranges::views::transform([&](auto const& node) {
				return ContractName{name, node.at("name")};
			}) | ranges::to<std::vector>;

			return std::pair{name, order};
		}) | ranges::to<std::map>();
	}

	///
	std::vector<output::Error> const& errors() const
	{
		return m_base.errors();
	}

	///
	bool success() const
	{
		return !ranges::any_of(errors(), [](auto const& e) {
			return e.type != langutil::Error::Type::Info && e.type != langutil::Error::Type::Warning;
		});
	}

	///
	std::vector<output::Contract const*> const contracts() const;

	///
	output::Contract const* contract(ContractName const& _contractName = {}) const;

private:
	///
	output::StandardJSONOutput m_base;
	///
	std::map<std::string, std::vector<ContractName>> m_compilationOrder;
};

template<typename T>
concept StandardJSONOutputType = std::constructible_from<T, output::StandardJSONOutput>;

/**
 * Provides an interface to the compiler under test.
 */
template<StandardJSONOutputType Output = output::StandardJSONOutput>
class StandardJSONCompiler
{
public:
	StandardJSONCompiler() = default;
	explicit StandardJSONCompiler(boost::filesystem::path _externalCompiler):
		m_externalCompiler(_externalCompiler)
	{}

	/// Takes the current compiler input, requests the compiler under test to compile
    /// and stores its output.
	/// @returns the stored output
	/// @param _input to pass to the compiler
    Output const& compile(StandardJSONInput const& _input)
	{
		if (m_externalCompiler)
		{
			// TODO: Call external compiler via IPC
		}
		else
		{
			auto output = StandardCompiler{}.compile(_input);
			m_output.emplace(StandardJSONOutputExt{std::move(output)});
		}
		return this->output();
	}

    /// @returns the stored output generated during previous compilation.
    Output const& output() const
	{
		solAssert(m_output.has_value(), "No output found. Please compile first.");
		return m_output.value();
	}

private:
	/// If a path is set, this instance will try to call the external compiler via IPC.
	std::optional<boost::filesystem::path> m_externalCompiler;
    /// Last generated output. Will be none before initial compilation.
    std::optional<Output> m_output;
};

}
