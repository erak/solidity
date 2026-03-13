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


#include <liblangutil/EVMVersion.h>
#include <liblangutil/Exceptions.h>
#include <liblangutil/SourceLocation.h>

#include <libsolutil/FixedHash.h>

#include <libsolidity/ast/ASTEnums.h>
#include <libsolidity/interface/DebugSettings.h>
#include <libsolidity/interface/MetadataSettings.h>
#include <libsolidity/interface/OptimiserSettings.h>

#include <memory>
#include <range/v3/algorithm.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/split.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/range/conversion.hpp>

#include <optional>
#include <vector>

using namespace solidity;
using namespace solidity::util;

namespace solidity::frontend::test::output
{

struct SourceLocation
{
	/// The name of the source file.
	std::string file;
	/// The start of the source position.
	int start;
	/// The end of the source position.
	int end;
	/// If this is a secondary source location, a message should exist.
	std::optional<std::string> message;

	/// @returns this source location converted to the compiler's internal type.
	langutil::SourceLocation toInternalSourceLocation() const
	{
		return langutil::SourceLocation{start, end, std::make_shared<std::string>(file)};
	}
};

struct Error
{
	/// Location within the source file.
	std::optional<SourceLocation> sourceLocation;
	/// Further locations (e.g. places of conflicting declarations).
	std::optional<std::vector<SourceLocation>> secondarySourceLocations;
	/// Unique code for the cause of the error.
	std::optional<langutil::ErrorId> errorCode;
	/// The error type.
	langutil::Error::Type type;
	/// The error message.
	std::optional<std::string> message;

	/// @returns this error converted to the compiler's internal type.
	langutil::Error toInternalError() const
	{
		auto locations = secondarySourceLocations.value_or(std::vector<SourceLocation>{});
		return {
			errorCode.value_or({}),
			type,
			message.value_or({}),
			sourceLocation.value_or({}).toInternalSourceLocation(),
			langutil::SecondarySourceLocation{
				locations | ranges::views::filter([](auto const& s) {
					return s.message.has_value();
				}) | ranges::views::transform([](auto const& s) {
					return std::pair{s.message.value(), s.toInternalSourceLocation()};
				}) | ranges::to<std::vector>()
			}
		};
	}
};

struct Source
{
	/// Identifier of the source (used in source maps)
	size_t id;
};

struct ABIParameter
{
	/// The ABI-level type name, e.g. "address", "uint256"
	std::string name;
	/// The ABI-level type, e.g. "address", "uint256", "tuple"
	std::string type;
	/// The Solidity-level type, may differ for e.g. enums or user-defined value types.
	std::optional<std::string> internalType;
	/// Whether this parameter is indexed. Only present for event inputs.
	std::optional<bool> indexed;
	/// Component parameters, only present when type == "tuple"
	std::optional<std::vector<ABIParameter>> components;
};

struct ABIConstructor
{
	/// The constrcutor's input parameters.
	std::vector<ABIParameter> inputs;
	/// The state mutability of the constructor.
	StateMutability stateMutability;
};

struct ABIFunction
{
	/// The name of the function.
	std::string name;
	/// The function's input parameters.
	std::vector<ABIParameter> inputs;
	/// The function's output parameters.
	std::vector<ABIParameter> outputs;
	/// The state mutability of the function.
	StateMutability stateMutability;
};

struct ABIFallback
{
	/// State mutability of the fallback function.
	StateMutability stateMutability;
};

struct ABIReceive
{
	/// State mutability of the fallback function.
	StateMutability stateMutability;
};


struct ABIEvent
{
	/// The name of the event.
	std::string name;
	/// The event's parameters.
	std::vector<ABIParameter> inputs;
	/// Whether the event is anonymous. Anonymous events do not have their
	/// signature included in the topic list.
	bool isAnonymous;
};

struct ABIError
{
	/// The name of the error.
	std::string name;
	/// The error's parameters.
	std::vector<ABIParameter> inputs;
};

struct ByteOffset
{
	/// The start of the bytes to replace.
	size_t start;
	/// The length of the bytes to replace.
	size_t length;
};

using LinkReferences = std::map<std::string, std::vector<ByteOffset>>;

struct Bytecode
{
	/// The bytecode as a hex.
	bytes object;
	/// The byte offsets per source and library. If not empty, this is an unlinked object.
	std::map<std::string, LinkReferences> linkReferences;
};

struct EVM
{
	/// The bytecode as a hex.
	Bytecode bytecode;
	/// The list of function hashes.
	std::map<std::string, std::string> methodIdentifiers;
};

/// A single entry in the contract ABI, either an event or a function.
using ABIEntry = std::variant<ABIConstructor, ABIFunction, ABIFallback, ABIReceive, ABIEvent, ABIError>;
using ABI = std::vector<ABIEntry>;

/**
 * Represents a compiled contract. Carries the contract-specific compiler output.
 */
struct Contract
{
	std::string name;
	ABI abi;
	EVM evm;
	std::string metadata;
};

using Errors = std::vector<output::Error>;
using Sources = std::map<std::string, output::Source>;
using Contracts = std::map<std::string, std::map<std::string, output::Contract>>;

/**
 * Output generated by the compiler during the last compilation run. It stores
 * the raw JSON output and provides accessors that deserialize lazily.
 */
struct StandardJSONOutput
{
	/// Creates a standard output object
	StandardJSONOutput() = default;

	/// Noncopyable, but has move semantics.
	StandardJSONOutput(const StandardJSONOutput&) = delete;
	StandardJSONOutput& operator=(const StandardJSONOutput&) = delete;
	StandardJSONOutput(StandardJSONOutput&&) noexcept = default;
	StandardJSONOutput& operator=(StandardJSONOutput&&) noexcept = default;

	~StandardJSONOutput() = default;

	output::Errors errors;
	output::Sources sources;
	output::Contracts contracts;
};

/// Enables JSON deserialization for standard output types.
/// Supports parsing via `nlohmann::json`'s ADL pattern.
template<typename SubDocument>
void from_json(SubDocument const&, SourceLocation&);

template<typename SubDocument>
void from_json(SubDocument const&, Error&);

template<typename SubDocument>
void from_json(SubDocument const&, Source&);

template<typename SubDocument>
void from_json(SubDocument const&, ABIParameter&);

template<typename SubDocument>
void from_json(SubDocument const&, ABIConstructor&);

template<typename SubDocument>
void from_json(SubDocument const&, ABIFunction&);

template<typename SubDocument>
void from_json(SubDocument const&, ABIFallback&);

template<typename SubDocument>
void from_json(SubDocument const&, ABIReceive&);

template<typename SubDocument>
void from_json(SubDocument const&, ABIEvent&);

template<typename SubDocument>
void from_json(SubDocument const&, ABIError&);

template<typename SubDocument>
void from_json(SubDocument const&, ABIEntry&);

template<typename SubDocument>
void from_json(SubDocument const&, ByteOffset&);

template<typename SubDocument>
void from_json(SubDocument const&, Bytecode&);

template<typename SubDocument>
void from_json(SubDocument const&, EVM&);

template<typename SubDocument>
void from_json(SubDocument const&, Contract&);

template<typename SubDocument>
void from_json(SubDocument const&, Contracts&);

template<typename SubDocument>
void from_json(SubDocument const&, StandardJSONOutput&);

}
