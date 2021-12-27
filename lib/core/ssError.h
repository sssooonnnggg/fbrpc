#pragma once

#include <string>

namespace fbrpc
{
	struct sError
	{
		int errorCode;
		std::string msg;
	};
}