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
#include <test/libsolidity/util/StandardJSONOutputParser.h>

#include <liblangutil/Exceptions.h>

#include <libsolutil/JSON.h>
#include <libsolidity/interface/StandardCompiler.h>
#include <libsolidity/interface/StandardJSONInput.h>
#include <libsolidity/util/SoltestErrors.h>

#include <boost/asio.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/process/v2.hpp>
#include <boost/process/v2/process.hpp>
#include <boost/process/v2/stdio.hpp>

#include <optional>

namespace solidity::frontend::test
{

using namespace solidity::frontend::input;
using namespace output;

template<typename T>
concept StandardJSONOutputType = std::constructible_from<T, StandardJSONOutput>;

/**
 * Provides an interface to the compiler under test.
 */
template<StandardJSONOutputType Output = StandardJSONOutput>
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
	Output const& compile(StandardJSONInput const& _input);

	/// @returns the stored output generated during previous compilation.
	Output const& output() const;

private:
	/// If a path is set, this instance will try to call the external compiler via IPC.
	std::optional<boost::filesystem::path> m_compilerPath;
    /// Last generated output. Will be none before initial compilation.
    std::optional<Output> m_output;
};

template<StandardJSONOutputType Output>
Output const& StandardJSONCompiler<Output>::compile(StandardJSONInput const& _input)
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

		auto parser = output::NlohmannParser{};
		auto deserialized = output::extract<StandardJSONOutput>(parser, output);
		m_output.emplace(Output{std::move(deserialized)});
	}
	else
	{
		auto json = StandardCompiler{}.compile(_input);
		auto deserialized = json.get<StandardJSONOutput>();
		m_output.emplace(Output{std::move(deserialized)});
	}
	return this->output();
}

template<StandardJSONOutputType Output>
Output const& StandardJSONCompiler<Output>::output() const
{
	solAssert(m_output.has_value(), "No output found. Please compile first.");
	return m_output.value();
}

}
