#pragma once

#include <memory>
#include <string>
#include <functional>

namespace fbrpc
{
	class sPrinter
	{
	public:
		virtual ~sPrinter() = default;

		class sScope
		{
		public:
			using sCallback = std::function<void()>;
			sScope(sPrinter* printer, sCallback onEnter, sCallback onLeave);
			~sScope();
		private:
			sPrinter* m_printer;
			sCallback m_onEnter;
			sCallback m_onLeave;
		};

		struct sConfig
		{
			std::size_t indentSpaceCount = 4;
			std::string lineEndings = "\n";
		};

		void setConfig(sConfig config);
		const sConfig& config() const;

		virtual void addHeader();

		void nextLine(std::size_t lineCount = 1);

		using sVarsMap = std::unordered_map<std::string_view, std::string_view>;
		void addContent(std::string_view lines, sVarsMap vars = {});

		virtual std::unique_ptr<sScope> addScope() = 0;
		std::string getOutput();

	protected:
		friend class sScope;

		void addSingleLine(std::string_view line, sVarsMap vars = {});

		std::size_t indent() const;
		void setIndent(std::size_t indent);

	private:
		std::string m_output;
		std::size_t m_indent = 0;
		sConfig m_config;
	};
}