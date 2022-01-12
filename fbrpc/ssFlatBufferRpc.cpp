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
					logger().info("new client connection", client->getId());
					client->on<sError>([this, client](const sError& error) { processClientError(client->getId(), error); });
					client->on<sCloseEvent>([this, client](const sCloseEvent& e) { processClientClose(client->getId(), e); });
					client->on<sEndEvent>([this, client](const sEndEvent& e) { processClientEnd(client->getId(), e); });
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
		m_buffer.append(buffer.data, buffer.length);
		processNextPackage();
	}

	void sFlatBufferRpcServer::processNextPackage()
	{
		if (m_buffer.empty())
			return;

		sPackageReader reader(m_buffer.view());

		if (reader.ready())
		{
			auto packageSize = reader.packageSize();
			auto serviceHash = reader.serviceHash();

			auto it = m_services.find(serviceHash);
			if (it != m_services.end())
			{
				auto& service = it->second;

				sBuffer header = sBuffer::clone(m_buffer, ePackageOffset::kRequestId, ePackageOffset::kFlatBuffer - ePackageOffset::kRequestId);

				auto responder = [this, packageHeader = std::move(header)](sBuffer response)
				{
					sBuffer package = sPackageWritter().write(std::move(packageHeader)).write(std::move(response)).getBuffer();
					sendResponse(std::move(package));
				};

				service->processBuffer(m_buffer.view(ePackageOffset::kApiHash), std::move(responder));
			}
			else
			{
				logger().error("unknown service", serviceHash);
			}

			m_buffer.consume(packageSize);
			processNextPackage();
		}
	}

	void sFlatBufferRpcServer::sendResponse(sBuffer response)
	{
		if (auto client = m_client.lock())
			client->send(std::move(response.data), response.length);
	}

	////////////////////////////////////////////////////////////////////////////////

	std::unique_ptr<sFlatBufferRpcClient> sFlatBufferRpcClient::create(sTCPOption option)
	{
		return uvwDetail::sTCPFlatBufferRpcClient::create(option);
	}

	sFlatBufferRpcClient::sFlatBufferRpcClient(std::unique_ptr<sClient> client)
		: m_client(std::move(client))
	{

	}

	void sFlatBufferRpcClient::connect()
	{
		m_client->on<sError>([this](const sError& e) { processError(e); });
		m_client->on<sConnectionEvent>([this](const sConnectionEvent& connection) { processConnection(connection); });
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

		m_connection->on<sError>([this](const sError& e) { processError(e); });
		m_connection->on<sEndEvent>([this](const sEndEvent& e) { processEnd(e); });
		m_connection->on<sCloseEvent>([this](const sCloseEvent& e) { processClose(e); });

		m_connection->on<sDataEvent>([this](const sDataEvent& e) { processBuffer(sBufferView{ e.data, e.length }); });

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
				auto& responseHandler = it->second;
				responseHandler(m_buffer.view(ePackageOffset::kFlatBuffer));
			}
			else
			{
				logger().error("unknown request id", requestId);
			}

			m_buffer.consume(packageSize);
			processNextPackage();
		}
	}

	void sFlatBufferRpcClient::call(std::size_t service, std::size_t api, sBuffer buffer, sOnResponse onResponse)
	{
		auto requestId = generateRequestId();
		m_responseMap[requestId] = std::move(onResponse);

		sBuffer package = sPackageWritter().write(requestId).write(service).write(api).write(std::move(buffer)).getBuffer();
		m_connection->send(std::move(package.data), package.length);
	}

	std::size_t sFlatBufferRpcClient::generateRequestId()
	{
		static std::size_t requestId = 0;
		return ++requestId;
	}
}