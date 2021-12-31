#pragma once

#include <memory>
#include <string>

#include "ssPrinter.h"

namespace fbrpc
{
	class sCppPrinter : public sPrinter
	{
	public:
		void addHeader() override;
		void addInclude(std::string_view includeFilePath);
		std::unique_ptr<sScope> addNamespace(std::string_view name);
		std::unique_ptr<sScope> addClass(std::string_view className, std::string_view baseClassName = "");
		void addClassAccessSpecifier(std::string_view specifier);

		std::unique_ptr<sScope> addScope() override;
	};
}