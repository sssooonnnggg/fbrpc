#pragma once

#include <string_view>

namespace flatbuffers
{
	struct ServiceDef;
	struct RPCCall;
}

namespace fbrpc
{
	class sGenerator
	{
	public:
		bool start(std::string_view protoFilePath, std::string_view outputDirPath);
	private:
		bool generateServiceFile(flatbuffers::ServiceDef* service);
		bool generateStubFile(flatbuffers::ServiceDef* service);
		bool writeFile(std::string content, std::string name);
	private:
		std::string m_outputDirPath;
	};
}