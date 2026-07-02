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

#include <test/libsolidity/util/StandardJSONOutput.h>

#include <liblangutil/Exceptions.h>
#include <libsolutil/CommonData.h>
#include <libsolutil/JSON.h>
#include <libsolidity/ast/ASTEnums.h>

#include <range/v3/algorithm/transform.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

#include <cstddef>

using namespace solidity::frontend;
using namespace solidity::frontend::test;
using namespace solidity::frontend::test::output;

