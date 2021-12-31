#include <cassert>

#include "ssCppPrinter.h"

namespace fbrpc
{
	void sCppPrinter::addHeader()
	{
		sPrinter::addHeader();
		addContent("#pragma once");
	}

	void sCppPrinter::addInclude(std::string_view includeFilePath)
	{
		addSingleLine(std::string(R"#(#include ")#") + std::string(includeFilePath) + "\"");
	}

	std::unique_ptr<sPrinter::sScope> sCppPrinter::addNamespace(std::string_view name)
	{
		addSingleLine(std::string("namespace ") + std::string(name) + " ");
		return addScope();
	}

	std::unique_ptr<sPrinter::sScope> sCppPrinter::addClass(std::string_view className, std::string_view baseClassName)
	{
		addSingleLine(std::string("class ") + std::string(className) + (baseClassName.empty() ? "" : " : public " + std::string(baseClassName)));
		return std::make_unique<sScope>(this, [this]() {addSingleLine("{"); }, [this]() {addSingleLine("};"); });
	}

	void sCppPrinter::addClassAccessSpecifier(std::string_view specifier)
	{
		auto oldIndent = indent();
		setIndent(indent() - config().indentSpaceCount);
		addSingleLine(specifier);
		setIndent(oldIndent);
	}

	std::unique_ptr<sPrinter::sScope> sCppPrinter::addScope()
	{
		return std::make_unique<sScope>(this, [this]() {addSingleLine("{"); }, [this]() {addSingleLine("}"); });
	}
}