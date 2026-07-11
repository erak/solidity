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

#include <libsolidity/interface/StandardJSONInput.h>
#include <libsolidity/util/SoltestErrors.h>

#include <libsolutil/JSON.h>

#include <boost/asio.hpp>
#include <boost/process/v2.hpp>
#include <boost/process/v2/process.hpp>
#include <boost/process/v2/stdio.hpp>

#include <string>

namespace solidity::frontend::test
{

using namespace solidity::frontend::input;
using namespace output;

/**
 * Compiler implementation that delegates to an external solc binary via IPC.
 */
class IPCCompiler
{
public:
	explicit IPCCompiler(boost::filesystem::path _compilerPath):
		m_compilerPath(std::move(_compilerPath))
	{}

	/// Compiles the given input by spawning an external compiler process and
	/// communicating via stdin/stdout, then @returns the deserialized StandardJSONOutput.
	StandardJSONOutput compile(StandardJSONInput const& _input);

private:
	boost::filesystem::path m_compilerPath;
};

inline StandardJSONOutput IPCCompiler::compile(StandardJSONInput const& _input)
{
	namespace bp = boost::process::v2;
	namespace asio = boost::asio;

	asio::io_context ioCtx;
	asio::readable_pipe stdoutPipe{ioCtx};
	asio::writable_pipe stdinPipe{ioCtx};

	boost::system::error_code error;
	bp::process child{
		ioCtx,
		m_compilerPath,
		{"--standard-json"},
		bp::process_stdio{stdinPipe, stdoutPipe, {}}
	};
	soltestAssert(child.running(), "Failed to launch the external compiler '" + m_compilerPath.string() + "'.");

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
	return output::extract<StandardJSONOutput>(parser, output);
}

}