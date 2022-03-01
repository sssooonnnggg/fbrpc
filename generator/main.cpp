
#include <cstdio>
#include "fbrpc/generator/ssGenerator.h"

void printUsage()
{
	printf(R"#(Usage: fbrpc_generator [language type] [proto file path or folder] [output folder]
language type:
	-c, --cpp			generate cpp files
	-n, --node			generate node binding files
	-t, --ts			generate typescript binding files
	-I PATH : 			when encountering include statements, attempt to load the files from this path. Paths will be tried in the order given, and if all fail (or none are specified) it will try to load relative to the path of the schema file being parsed.
if pass a proto file path, generate files for this proto file.
if pass a folder path which containing proto files, generate for all proto files in folder.
)#");
}

enum eArgType
{
	kLanguageType = 1,
	kProtoFilePath,
	kOutputFolder,
	kIncludeFlag,
	kIncludePath,
	kArgMax
};

class sArgParser
{
public:
	sArgParser(int argc, const char** argv)
		: m_argc(argc), m_argv(argv)
	{}

	bool parse()
	{
		if (m_argc > kArgMax)
			return false;

		std::string languageType = m_argv[1];

		if (languageType == "-c" || languageType == "--cpp")
			m_languageType = fbrpc::eLanguageType::kCpp;
		else if (languageType == "-n" || languageType == "--node")
			m_languageType = fbrpc::eLanguageType::kNode;
		else if (languageType == "-t" || languageType == "--ts")
			m_languageType = fbrpc::eLanguageType::kTypeScript;
		else
			return false;
		if(std::string("-I") == m_argv[2])
		{
			m_includePath = m_argv[3];
			m_inputPath = m_argv[4];
			m_outputFolder = m_argv[5];
		}		else
		{
		m_inputPath = m_argv[2];
		m_outputFolder = m_argv[3];
		}
		return true;
	}

	fbrpc::eLanguageType languageType() const { return m_languageType; }
	std::string inputPath() const { return m_inputPath; }
	std::string outputFolder() const { return m_outputFolder; }
	std::string includePath() const { return m_includePath; }private:
	int m_argc = 0;
	const char** m_argv;
	fbrpc::eLanguageType m_languageType;
	std::string m_inputPath;
	std::string m_outputFolder;
	std::string m_includePath = std::string();
};

int main(int argc, const char** argv)
{
	sArgParser parser(argc, argv);
	if (!parser.parse())
	{
		printUsage();
		return -1;
	}

	fbrpc::sGenerator gen(parser.languageType());
	return gen.start(parser.inputPath(), parser.outputFolder(), parser.includePath()) ? 0 : -1;
}