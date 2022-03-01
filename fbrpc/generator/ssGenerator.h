#pragma once

#include <string_view>
#include <memory>
#include <vector>

#include "flatbuffers/idl.h"

namespace fbrpc
{
	enum class eLanguageType
	{
		kCpp,
		kNode,
		kTypeScript
	};

	class sLanguageGenerator;
	class sGenerator
	{
	public:
		sGenerator(eLanguageType language);
		bool start(std::string_view inputPath, std::string_view outputDirPath, std::string_view includePath);
	private:
		flatbuffers::Parser* addParser();
		bool generateForSingleProto(std::string_view protoFilePath, std::string_view outputDirPath, std::string_view includePath);
		bool writeFile(std::string content, std::string name);
		std::unique_ptr<sLanguageGenerator> createLanguageGenerator();
	private:
		eLanguageType m_language;
		std::string m_outputDirPath;
		std::vector<std::unique_ptr<flatbuffers::Parser>> m_parsers;
	};
}