#include <cassert>
#include "core/ssLogger.h"
#include "ssService.h"

namespace fbrpc
{
	std::size_t sService::hash()
	{
		static auto nameHash = std::hash<std::string>{}(name());
		return nameHash;
	}

	void sService::processBuffer(sBufferView buffer, sResponder responder)
	{
		auto apiHash = buffer.read<std::size_t>();
		auto it = m_apiWrappers.find(apiHash);
		if (it != m_apiWrappers.end())
		{
			it->second(buffer.view(sizeof(std::size_t)), std::move(responder));
		}
		else
		{
			logger().error("unknown api", apiHash);
		}
	}

	void sService::addApiWrapper(std::size_t hash, sApiWrapper wrapper)
	{
		m_apiWrappers[hash] = std::move(wrapper);
	}
}