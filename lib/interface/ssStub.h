#pragma once

#include "core/ssFlatBufferHelper.h"

namespace fbrpc
{
	class sFlatBufferRpcClient;
	class sStub : public sFlatBufferHelper
	{
	public:
		sStub(sFlatBufferRpcClient* client)
			: m_client(client)
		{}

		virtual std::size_t hash() const = 0;

	protected:
		sFlatBufferRpcClient* client() { return m_client; }

		template <class ... Args>
		void call(Args&& ... args) { client()->call(std::forward<Args>(args)...); }

	private:
		sFlatBufferRpcClient* m_client;
	};
}