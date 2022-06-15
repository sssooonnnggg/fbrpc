#include "ssTSPrinter.h"

namespace fbrpc
{
	std::size_t sTSPrinter::addImport(std::string_view target, std::string_view from)
	{
		return addContent(std::string("import ") + target.data() + " from '" + from.data() + "'");
	}

	std::unique_ptr<sPrinter::sScope> sTSPrinter::addExport(std::string_view target)
	{
		addContent(std::string("export ") + target.data() + " {");
		return addScope();
	}

	std::unique_ptr<sPrinter::sScope> sTSPrinter::addScope()
	{
		return std::make_unique<sScope>(this, nullptr, [this]() {addSingleLine("}"); });
	}
}