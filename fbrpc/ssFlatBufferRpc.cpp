#include "core/ssLogger.h"
#include "core/ssPackage.h"
#include "impl/uvw/ssTCPServer.h"
#include "impl/uvw/ssTCPFlatBufferRpc.h"
#include "ssFlatBufferRpc.h"

namespace fbrpc
{
	std::unique_ptr<sFlatBufferRpcServer> sFlatBufferRpcServer::create(sTCPOption option)
	{
		return uvwDetail::sTCPFlatBufferRpcServer::create(std::move(option));
	}

	sFlatBufferRpcServer::sFlatBufferRpcServer(std::unique_ptr<sServer> server)
		: m_server(std::move(server))
	{

	}

	void sFlatBufferRpcServer::installService(std::unique_ptr<sService> service)
	{
		if (service)
		{
			service->init();
			m_services[service->hash()] = std::move(service);
		}
	}

	void sFlatBufferRpcServer::start()
	{
		m_server->on<sError>([this](const sError& error) { processServerError(error); });

		m_server->on<sConnectionEvent>([this](const sConnectionEvent& conn)
			{
				m_client = conn.connection;
				if (auto client = m_client.lock())
				{
					auto id = client->getId();
					logger().info("new client connection", id);
					client->on<sError>([this, id](const sError& error) { processClientError(id, error); });
					client->on<sCloseEvent>([this, id](const sCloseEvent& e) { processClientClose(id, e); });
					client->on<sEndEvent>([this, id](const sEndEvent& e) { processClientEnd(id, e); });
					client->on<sDataEvent>([this](const sDataEvent& e) { processBuffer(sBufferView{ e.data, e.length }); });
				}
			}
		);

		m_server->listen();
	}

	void sFlatBufferRpcServer::processServerError(const sError& error)
	{
		logger().error("server error", error.errorCode, error.msg);
		emit(error);
	}

	void sFlatBufferRpcServer::processClientError(int clientId, const sError& error)
	{
		logger().error("client error", clientId, error.errorCode, error.msg);
		emit(error);
	}

	void sFlatBufferRpcServer::processClientClose(int clientId, const sCloseEvent& close)
	{
		logger().error("client close", clientId);
		emit(close);
	}

	void sFlatBufferRpcServer::processClientEnd(int clientId, const sEndEvent& end)
	{
		logger().error("client end", clientId);
		emit(end);
	}

	void sFlatBufferRpcServer::processBuffer(sBufferView buffer)
	{
		m_received.append(buffer.data, buffer.length);
		processNextPackage();
	}

	void sFlatBufferRpcServer::processNextPackage()
	{
		while (true)
		{
			if (m_received.empty())
				break;

			sPackageReader reader(m_received.view());
			if (!reader.ready())
				break;

			auto packageSize = reader.packageSize();
			auto serviceHash = reader.serviceHash();

			auto it = m_services.find(serviceHash);
			if (it != m_services.end())
			{
				auto& service = it->second;

				sBuffer header = sBuffer::clone(m_received, ePackageOffset::kRequestId, ePackageOffset::kFlatBuffer - ePackageOffset::kRequestId);

				auto responder = [this, packageHeader = std::move(header)](sBuffer response)
				{
					sBuffer package = sPackageWritter().write(std::move(packageHeader)).write(std::move(response)).getBuffer();
					sendResponse(std::move(package));
				};

				service->processBuffer(m_received.view(ePackageOffset::kApiHash), std::move(responder));
			}
			else
			{
				logger().error("unknown service", serviceHash);
			}

			m_received.consume(packageSize);
		}
	}

	void sFlatBufferRpcServer::sendResponse(sBuffer response)
	{
		m_pending.append(std::move(response));
	}
	
	void sFlatBufferRpcServer::update()
	{
		for (const auto& [_, service] : getServices())
			service->update();

		if (auto client = m_client.lock())
			if (!m_pending.empty())
				client->send(std::move(m_pending));
	}

	////////////////////////////////////////////////////////////////////////////////

	std::unique_ptr<sFlatBufferRpcClient> sFlatBufferRpcClient::create(sTCPOption option)
	{
		return uvwDetail::sTCPFlatBufferRpcClient::create(option);
	}

	sFlatBufferRpcClient::sFlatBufferRpcClient(std::unique_ptr<sClient> client)
		: m_client(std::move(client))
	{
		m_client->on<sError>([this](const sError& e) { processError(e); });
		m_client->on<sConnectionEvent>([this](const sConnectionEvent& connection) { processConnection(connection); });
		m_client->on<sEndEvent>([this](const sEndEvent& e) { processEnd(e); });
		m_client->on<sCloseEvent>([this](const sCloseEvent& e) { processClose(e); });
		m_client->on<sDataEvent>([this](const sDataEvent& e) { processBuffer(sBufferView{ e.data, e.length }); });
	}

	void sFlatBufferRpcClient::connect()
	{
		m_client->connect();
	}

	void sFlatBufferRpcClient::processError(const sError& error)
	{
		logger().error("client error", error.errorCode, error.msg);
		emit(error);
	}

	void sFlatBufferRpcClient::processClose(const sCloseEvent& close)
	{
		logger().error("client close");
		m_connected = false;
		emit(close);
	}

	void sFlatBufferRpcClient::processEnd(const sEndEvent& end)
	{
		logger().error("client end");
		m_connected = false;
		emit(end);
	}

	void sFlatBufferRpcClient::processConnection(const sConnectionEvent& e)
	{
		logger().info("connected");
		m_connection = e.connection.lock();
		m_connected = true;
		emit(e);
	}

	void sFlatBufferRpcClient::processBuffer(sBufferView buffer)
	{
		m_buffer.append(buffer.data, buffer.length);
		processNextPackage();
	}

	void sFlatBufferRpcClient::processNextPackage()
	{
		if (m_buffer.empty())
			return;

		sPackageReader reader(m_buffer.view());

		if (reader.ready())
		{
			auto packageSize = reader.packageSize();
			auto requestId = reader.requestId();

			auto it = m_responseMap.find(requestId);
			if (it != m_responseMap.end())
			{
				auto& responseHandler = it->second.onResponse;
				responseHandler(m_buffer.view(ePackageOffset::kFlatBuffer));

				if (!it->second.repeat)
					m_responseMap.erase(it);
			}
			else
			{
				logger().error("unknown request id", requestId);
			}

			m_buffer.consume(packageSize);
			processNextPackage();
		}
	}

	void sFlatBufferRpcClient::call(std::size_t service, std::size_t api, sBuffer buffer, sOnResponse onResponse, bool repeat)
	{
		auto requestId = generateRequestId();
		m_responseMap[requestId] = { std::move(onResponse), repeat };

		sBuffer package = sPackageWritter().write(requestId).write(service).write(api).write(std::move(buffer)).getBuffer();
		m_connection->send(std::move(package));
	}

	std::size_t sFlatBufferRpcClient::generateRequestId()
	{
		static std::size_t requestId = 0;
		return ++requestId;
	}
}