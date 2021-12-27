#pragma once

#include <memory>
#include <string>

namespace fbrpc
{
	class sPrinter
	{
	public:

		struct sConfig
		{
			std::size_t indentSpaceCount = 4;
			std::string lineEndings = "\n";
		};

		void nextLine(std::size_t lineCount = 1);
		void addInclude(std::string_view includeFilePath);
		void addContent(std::string_view line);

		struct sScope
		{
			enum class eType
			{
				kNamespace,
				kClass
			};
			sScope(sPrinter* printer, eType type = eType::kNamespace);
			~sScope();
		private:
			sPrinter* m_printer;
			eType m_type;
		};
		std::unique_ptr<sScope> addNamespace(std::string_view name);
		std::unique_ptr<sScope> addClass(std::string_view className, std::string_view baseClassName);
		void addClassAccessSpecifier(std::string_view specifier);

		std::unique_ptr<sScope> addScope();

		void setConfig(sConfig config);
		const sConfig& config() const;
		std::string getOutput();

	private:
		friend struct sScope;

		void addSingleLine(std::string_view line);

		std::size_t indent() const;
		void setIndent(std::size_t indent);

	private:
		std::string m_output;
		std::size_t m_indent = 0;
		sConfig m_config;
	};
}