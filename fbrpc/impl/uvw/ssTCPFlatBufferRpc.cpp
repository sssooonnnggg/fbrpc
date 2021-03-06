#include "ssUvLoop.h"
#include "ssTCPServer.h"
#include "ssTCPClient.h"
#include "ssTCPFlatBufferRpc.h"

namespace fbrpc::uvwDetail
{
	std::unique_ptr<sFlatBufferRpcServer> sTCPFlatBufferRpcServer::create(sTCPOption option)
	{
		auto server = std::make_unique<sTCPServer>();
		server->bind(option.address, option.port);
		return std::unique_ptr<sFlatBufferRpcServer>(new uvwDetail::sTCPFlatBufferRpcServer(std::move(server)));
	}

	sTCPFlatBufferRpcServer::sTCPFlatBufferRpcServer(std::unique_ptr<sServer> server)
		: sFlatBufferRpcServer(std::move(server))
	{

	}

	void sTCPFlatBufferRpcServer::update()
	{
		bool pendingReq = false;
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
		do {
			sUvLoop::getUvLoop()->run<uvw::Loop::Mode::NOWAIT>();
			pendingReq = sUvLoop::getUvLoop()->raw()->pending_reqs_tail != 0;
		} while (pendingReq); 
#else
		sUvLoop::getUvLoop()->run<uvw::Loop::Mode::NOWAIT>();
#endif
		sFlatBufferRpcServer::update();
	}

	std::unique_ptr<sFlatBufferRpcClient> sTCPFlatBufferRpcClient::create(sTCPOption option)
	{
		auto client = std::make_unique<sTCPClient>();
		client->setServerAddress(option.address, option.port);
		return std::unique_ptr<sFlatBufferRpcClient>(new uvwDetail::sTCPFlatBufferRpcClient(std::move(client)));
	}

	sTCPFlatBufferRpcClient::sTCPFlatBufferRpcClient(std::unique_ptr<sClient> client)
		: sFlatBufferRpcClient(std::move(client))
	{

	}

	void sTCPFlatBufferRpcClient::update()
	{
		sUvLoop::getUvLoop()->run<uvw::Loop::Mode::NOWAIT>();
	}
}