#pragma once

#include <uvw.hpp>
#include "core/ssEmitter.h"

namespace fbrpc::uvwDetail
{
	class sUvLoop
	{
	public:
		static std::shared_ptr<uvw::Loop> getUvLoop();

		using OnError = std::function<void(const uvw::ErrorEvent&, uvw::TCPHandle&)>;
		static std::shared_ptr<uvw::TCPHandle> allocHandle(OnError errorHandler);
	};
	
}