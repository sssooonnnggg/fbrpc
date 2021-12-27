
#include <cstdio>
#include "generator/ssGenerator.h"

void printUsage()
{
	printf(R"#(Usage: fbrpc_generator [inputFile] [outputFolder])#");
}

int main(int argc, const char** argv)
{
	if (argc != 3)
	{
		printUsage();
		return -1;
	}

	fbrpc::sGenerator gen;
	return gen.start(argv[1], argv[2]) ? 0 : -1;
}