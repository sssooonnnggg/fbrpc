#pragma once

#include "fbrpc/core/ssEmitter.h"

namespace fbrpc
{
	class sClient : public sEmitter<sClient>
	{
	public:
		virtual void connect() = 0;
	};
}