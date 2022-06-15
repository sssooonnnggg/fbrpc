#include <filesystem>
#include <fstream>
#include <optional>

#include "flatbuffers/idl.h"

#include "fbrpc/core/ssLogger.h"

#include "ssPrinter.h"
#include "ssGenerator.h"
#include "ssCppGenerator.h"
#include "ssNodeGenerator.h"
#include "fbrpc/generator/ts/ssTSGenerator.h"

namespace fbrpc
{
	namespace
	{
		constexpr std::array<unsigned char, 3> Utf8Bom = { 0xEF, 0xBB, 0xBF };
	}

	sGenerator::sGenerator(eLanguageType language)
		: m_language(language)
	{

	}

	bool sGenerator::start(std::string_view inputPath, std::string_view outputDirPath, std::string_view includePath)
	{
		m_outputDirPath = outputDirPath;

		if (std::filesystem::is_directory(inputPath))
		{
			for (auto const& dirEntry : std::filesystem::directory_iterator{ std::filesystem::path(inputPath) })
			{
				std::filesystem::path protoFilePath = dirEntry.path();
				if (!std::filesystem::is_directory(protoFilePath.string()))
				{
					if (!generateForSingleProto(protoFilePath.string(), outputDirPath, includePath))
						return false;
				}
			}

			std::vector<flatbuffers::ServiceDef*> services;
			std::vector<flatbuffers::Parser*> parsers;
			for (auto&& parser : m_parsers)
			{
				parsers.push_back(parser.get());
				if (parser->services_.vec.size() > 0)
					services.push_back(parser->services_.vec[0]);
			}

			auto generator = createLanguageGenerator();
			return generator->finish({ std::move(parsers), std::move(services) });
		}
		else
		{
			return generateForSingleProto(inputPath, outputDirPath, includePath);
		}
	}

	flatbuffers::Parser* sGenerator::addParser()
	{
		auto parser = new flatbuffers::Parser();
		m_parsers.push_back(std::unique_ptr<flatbuffers::Parser>(parser));
		return parser;
	}

	bool sGenerator::generateForSingleProto(std::string_view protoFilePath, std::string_view outputDirPath, std::string_view includeP)
	{
		std::filesystem::path protoPath(protoFilePath);
		if (!std::filesystem::exists(protoPath))
		{
			logger().error("proto path doesn't exist", protoFilePath);
			return false;
		}

		std::string sourceName = protoPath.filename().string();
		std::string parentPath = protoPath.parent_path().string();
		std::vector<const char*> includePath = { parentPath.c_str(), includeP.data(), nullptr };

		std::ifstream file(protoFilePath.data(), std::ios_base::binary | std::ios_base::in);
		if (!file.is_open())
		{
			logger().error("read proto file failed", protoFilePath);
			return false;
		}

		std::vector<char> buffer(std::istreambuf_iterator<char>(file), {});
		buffer.push_back('\0');

		std::size_t offset = 0;
		if (buffer.size() > 3 
			&& static_cast<unsigned char>(buffer[0]) == Utf8Bom[0]
			&& static_cast<unsigned char>(buffer[1]) == Utf8Bom[1]
			&& static_cast<unsigned char>(buffer[2]) == Utf8Bom[2])
			offset = 3;

		flatbuffers::Parser& parser = *addParser();
		if (!parser.Parse(&buffer[0] + offset, &includePath[0], sourceName.c_str()))
		{
			logger().error("parse proto file failed", sourceName, parser.error_);
			return false;
		}

		auto generator = createLanguageGenerator();
		if (!generator)
		{
			logger().error("no available generator");
			return false;
		}

		const auto& services = parser.services_.vec;
		if (services.size() == 0)
		{
			logger().info("no service found, generate dummy file for", sourceName);
			return generator->generateDummyFile(std::filesystem::path(sourceName).stem().string());
		}

		if (services.size() > 1)
		{
			logger().error("more than one service found");
			return false;
		}

		auto service = services[0];

		if (service->name + ".fbs" != sourceName)
		{
			logger().error("mismatch between service name and file name, service name:", service->name, ",service file name:", sourceName);
			return false;
		}

		auto namespaces = service->defined_namespace->components;
		if (namespaces.size() > 1)
		{
			logger().error("unsupport nested namespace");
			return false;
		}
		
		return generator->start(service);
	}

	std::unique_ptr<sLanguageGenerator> sGenerator::createLanguageGenerator()
	{
		auto writter = [this](std::string content, std::string name) { return writeFile(std::move(content), std::move(name)); };

		if (m_language == eLanguageType::kCpp)
			return std::make_unique<sCppGenerator>(std::move(writter));
		else if (m_language == eLanguageType::kNode)
			return std::make_unique<sNodeGenerator>(std::move(writter));
		else if (m_language == eLanguageType::kTypeScript)
			return std::make_unique<sTSGenerator>(std::move(writter));

		return nullptr;
	}

	bool sGenerator::writeFile(std::string content, std::string name)
	{
		std::filesystem::path outputPath(m_outputDirPath);
		if (!std::filesystem::exists(outputPath))
			std::filesystem::create_directory(outputPath);

		outputPath /= name;

		std::ofstream file(outputPath.string(), std::ios_base::binary);
		if (!file.is_open())
		{
			logger().error("genenrate file failed", outputPath.string());
			return false;
		}

		file << Utf8Bom[0] << Utf8Bom[1] << Utf8Bom[2] << content;
		file.close();
		return true;
	}
}