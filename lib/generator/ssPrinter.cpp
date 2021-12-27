#include <cassert>

#include "ssPrinter.h"

namespace fbrpc
{
	sPrinter::sScope::sScope(sPrinter* printer, eType type)
		: m_printer(printer), m_type(type)
	{
		m_printer->addSingleLine("{");
		m_printer->setIndent(m_printer->indent() + m_printer->config().indentSpaceCount);
	}

	sPrinter::sScope::~sScope()
	{
		m_printer->setIndent(m_printer->indent() - m_printer->config().indentSpaceCount);
		assert(m_printer->indent() >= 0);

		m_printer->addSingleLine(m_type == eType::kClass ? "};" : "}");
	}

	void sPrinter::nextLine(std::size_t lineCount)
	{
		for (auto i = 0; i < lineCount; ++i)
			m_output = m_output + config().lineEndings;
	}

	void sPrinter::addInclude(std::string_view includeFilePath)
	{
		addSingleLine(std::string(R"#(#include ")#") + std::string(includeFilePath) + "\"");
	}

	void sPrinter::addContent(std::string_view line)
	{
		auto lineEndings = config().lineEndings;
		std::size_t offset = 0;
		do 
		{
			auto prev = offset;
			offset = line.find_first_of(lineEndings, offset);
			if (offset == prev)
			{
				addSingleLine("");
			}
			else
			{
				auto singleLine = line.substr(prev, offset == std::string::npos ? std::string::npos : (offset - prev));
				addSingleLine(singleLine);
			}
			
			if (offset != std::string::npos)
				offset += lineEndings.size();

		} while (offset != std::string::npos);
	}

	std::unique_ptr<sPrinter::sScope> sPrinter::addNamespace(std::string_view name)
	{
		addSingleLine(std::string("namespace ") + std::string(name) + " ");
		return std::make_unique<sScope>(this);
	}

	std::unique_ptr<sPrinter::sScope> sPrinter::addClass(std::string_view className, std::string_view baseClassName)
	{
		addSingleLine(std::string("class ") + std::string(className) + " : public " + std::string(baseClassName));
		return std::make_unique<sScope>(this, sScope::eType::kClass);
	}

	void sPrinter::addClassAccessSpecifier(std::string_view specifier)
	{
		auto oldIndent = indent();
		setIndent(indent() - config().indentSpaceCount);
		addSingleLine(specifier);
		setIndent(oldIndent);
	}

	void sPrinter::addSingleLine(std::string_view singleLine)
	{
		m_output = m_output + std::string(m_indent, ' ') + std::string(singleLine) + config().lineEndings;
	}

	std::unique_ptr<sPrinter::sScope> sPrinter::addScope()
	{
		return std::make_unique<sScope>(this);
	}

	std::size_t sPrinter::indent() const
	{
		return m_indent;
	}

	void sPrinter::setIndent(std::size_t indent)
	{
		m_indent = indent;
	}

	void sPrinter::setConfig(sPrinter::sConfig config)
	{
		m_config = std::move(config);
	}

	const sPrinter::sConfig& sPrinter::config() const
	{
		return m_config;
	}

	std::string sPrinter::getOutput()
	{
		return m_output;
	}
}