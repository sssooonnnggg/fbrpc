#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <type_traits>

#include "core/ssCommon.h"
#include "core/ssError.h"
#include "core/ssEmitter.h"
#include "core/ssBuffer.h"

#include "interface/ssServer.h"
#include "interface/ssClient.h"

#include "interface/ssService.h"
#include "interface/ssStub.h"

namespace fbrpc
{
	struct sTCPOption
	{
		std::string address;
		unsigned int port;
	};

	class sFlatBufferRpcServer : public sEmitter<sFlatBufferRpcServer>
	{
	public:
		static std::unique_ptr<sFlatBufferRpcServer> create(sTCPOption option);

		void installService(std::unique_ptr<sService> service);
		template<class T, typename = std::enable_if_t<std::is_base_of_v<sService, T>>>
		void installService() { installService(std::make_unique<T>()); }
		void start();

		virtual void update() = 0;

	protected:
		sFlatBufferRpcServer(std::unique_ptr<sServer> server);
	private:
		void processServerError(const sError& error);
		void processClientError(int clientId, const sError& error);
		void processClientClose(int clientId, const sCloseEvent&);
		void processClientEnd(int clientId, const sEndEvent&);

		void processBuffer(sBufferView buffer);
		void processNextPackage();
		void sendResponse(sBuffer buffer);

	private:
		std::unique_ptr<sServer> m_server;
		std::weak_ptr<sConnection> m_client;
		std::unordered_map<std::size_t, std::unique_ptr<sService>> m_services;
		sBuffer m_buffer;
	};

	class sFlatBufferRpcClient : public sEmitter<sFlatBufferRpcClient>
	{
	public:
		virtual void update() = 0;

		static std::unique_ptr<sFlatBufferRpcClient> create(sTCPOption option);

		void connect();
		bool isConnected() const { return m_connected; }

		template <class T, typename = std::enable_if_t<std::is_base_of_v<sStub, T>>>
		std::weak_ptr<T> getStub()
		{
			auto it = std::find_if(m_stubs.begin(), m_stubs.end(), [](auto&& stub) { return T::typeHash() == stub->hash(); });
			if (it == m_stubs.end())
				return createStub<T>();
			else
				return std::weak_ptr<T>(std::static_pointer_cast<T>(*it));
		}

		using sOnResponse = std::function<void(sBufferView)>;
		void call(std::size_t service, std::size_t api, sBuffer buffer, sOnResponse onResponse);

	protected:
		sFlatBufferRpcClient(std::unique_ptr<sClient> client);

	private:

		template <class T, typename = std::enable_if_t<std::is_base_of_v<sStub, T>>>
		std::weak_ptr<T> createStub()
		{
			auto stub = std::make_shared<T>(this);
			m_stubs.push_back(stub);
			return stub;
		}

		void processError(const sError& error);
		void processConnection(const sConnectionEvent& e);
		void processClose(const sCloseEvent&);
		void processEnd(const sEndEvent&);

		void processBuffer(sBufferView buffer);
		void processNextPackage();

		std::size_t generateRequestId();

	private:
		std::unique_ptr<sClient> m_client;
		std::shared_ptr<sConnection> m_connection;
		sBuffer m_buffer;
		std::unordered_map<std::size_t, sOnResponse> m_responseMap;
		bool m_connected = false;
		std::vector<std::shared_ptr<sStub>> m_stubs;
	};
}