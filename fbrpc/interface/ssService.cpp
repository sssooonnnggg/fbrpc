#include <cassert>
#include "fbrpc/core/ssLogger.h"
#include "ssService.h"

namespace fbrpc
{
	void sService::processBuffer(sBufferView buffer, sResponder responder)
	{
		auto apiHash = buffer.read<std::size_t>();
		auto it = m_apiWrappers.find(apiHash);
		if (it != m_apiWrappers.end())
			it->second(buffer.view(sizeof(std::size_t)), std::move(responder));
		else
			logger().error("unknown api", apiHash);
	}

	void sService::addApiWrapper(std::size_t hash, sApiWrapper wrapper)
	{
		m_apiWrappers[hash] = std::move(wrapper);
	}

	void sService::trace(std::string_view content)
	{
		if (m_option.enableTracing)
			logger().info("[service] [trace]", content);
	}

}