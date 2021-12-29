#pragma once

#include "fbrpc/ssFlatBufferRpc.h"

namespace fbrpc::uvwDetail
{
	class sTCPFlatBufferRpcServer : public sFlatBufferRpcServer
	{
	public:
		static std::unique_ptr<sFlatBufferRpcServer> create(sTCPOption option);
		void update() override;
	protected:
		sTCPFlatBufferRpcServer(std::unique_ptr<sServer> server);
	};

	class sTCPFlatBufferRpcClient : public sFlatBufferRpcClient
	{
	public:
		static std::unique_ptr<sFlatBufferRpcClient> create(sTCPOption option);
		void update() override;
	protected:
		sTCPFlatBufferRpcClient(std::unique_ptr<sClient> client);
	};
}