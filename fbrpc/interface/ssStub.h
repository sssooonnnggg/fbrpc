#pragma once

#include "fbrpc/core/ssCommon.h"
#include "fbrpc/core/ssFlatBufferHelper.h"

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

	private:
		sFlatBufferRpcClient* m_client;
	};
}