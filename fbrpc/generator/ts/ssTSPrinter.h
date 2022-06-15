#pragma once

#include "fbrpc/generator/ssPrinter.h"

namespace fbrpc
{
	class sTSPrinter : public sPrinter
	{
	public:
		std::size_t addImport(std::string_view target, std::string_view from);
		std::unique_ptr<sScope> addExport(std::string_view target);
		std::unique_ptr<sScope> addScope() override;
	};
}