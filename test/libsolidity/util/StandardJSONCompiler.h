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
#include <libsolidity/util/SoltestErrors.h>

#include <boost/asio.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/process/v2.hpp>
#include <boost/process/v2/process.hpp>
#include <boost/process/v2/stdio.hpp>

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
		m_compilationOrder = m_base.sources | ranges::views::transform([](auto const& entry) {
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
		return m_base.errors;
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
	explicit StandardJSONCompiler(boost::filesystem::path _compilerPath):
		m_compilerPath(_compilerPath)
	{}

	/// Takes the current compiler input, requests the compiler under test to compile
    /// and stores its output.
	/// @returns the stored output
	/// @param _input to pass to the compiler
    Output const& compile(StandardJSONInput const& _input)
	{
		if (m_compilerPath)
		{
			namespace bp = boost::process::v2;
			namespace asio = boost::asio;

			asio::io_context ioCtx;
			asio::readable_pipe stdoutPipe{ioCtx};
			asio::writable_pipe stdinPipe{ioCtx};

			boost::system::error_code error;
			bp::process child{
				ioCtx,
				*m_compilerPath,
				{"--standard-json"},
				bp::process_stdio{stdinPipe, stdoutPipe, {}}
			};
			soltestAssert(child.running(), "Failed to launch the external compiler '" + m_compilerPath->string() + "'.");

			Json jsonInput = _input;
			asio::write(stdinPipe, asio::buffer(jsonInput.dump()), error);
			stdinPipe.close();
			soltestAssert(!error, "Error writing to stdin.");

			std::string output;
			asio::read(stdoutPipe, asio::dynamic_buffer(output), error);
			// error::eof is expected — the child closed its end.
			soltestAssert(!error || error == asio::error::eof, "Error reading from stdout.");


			child.wait();
			soltestAssert(
				child.exit_code() == 0,
				"External compiler exited unexpectedly with code '" + std::to_string(child.exit_code()) + "'"
			);

			while (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
				output.pop_back();

			auto json = Json::parse(output);
			auto deserialized = json.get<output::StandardJSONOutput>();
			m_output.emplace(Output{std::move(deserialized)});
		}
		else
		{
			auto json = StandardCompiler{}.compile(_input);
			auto deserialized = json.get<output::StandardJSONOutput>();
			m_output.emplace(Output{std::move(deserialized)});
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
	std::optional<boost::filesystem::path> m_compilerPath;
    /// Last generated output. Will be none before initial compilation.
    std::optional<Output> m_output;
};

}
