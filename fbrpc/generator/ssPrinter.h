#pragma once

#include <memory>
#include <string>
#include <functional>
#include <unordered_map>

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
		std::size_t addContent(std::string_view lines, sVarsMap vars = {});

		std::string format(std::string_view content, sVarsMap vars);

		void setIndex(std::size_t index) { m_index = index; }

		virtual std::unique_ptr<sScope> addScope() = 0;
		std::string getOutput();

		std::size_t length() { return m_output.length(); }

		std::size_t indent() const;
		void setIndent(std::size_t indent);

		void moveTo(std::size_t index);
		void moveBack();

	protected:
		friend class sScope;

		void addSingleLine(std::string_view line, sVarsMap vars = {});
		void appendOrInsert(std::string_view str);


	private:
		std::string m_output;
		std::size_t m_indent = 0;
		std::size_t m_oldIndent = 0;
		sConfig m_config;
		std::size_t m_index = std::string::npos;
	};
}