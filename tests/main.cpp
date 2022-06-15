#include <string>

#include "generated/Example_generated.h"
#include "generated/ExampleService_generated.h"
#include "generated/ExampleStub_generated.h"

#include "fbrpc/ssFlatBufferRpc.h"
#include "fbrpc/core/ssLogger.h"
#include "fbrpc/utils/ssTimer.h"

using namespace fbrpc;
using namespace ExampleNamespace;

class ExampleServiceImpl : public ExampleService
{
public:
    void helloWorld(const HelloWorldRequest* request, std::unique_ptr<sPromise<HelloWorldResponse>> response) override
    {
        logger().info("[server] [<==] receive request helloworld");
        std::string message = std::string("hello world, ") + request->name()->c_str() + "!";

        auto value = CreateHelloWorldResponse(response->builder(), response->createString(message));
        response->resolve(value);
    };

    void delayAdd(const DelayAddRequest* request, std::unique_ptr<sPromise<DelayAddResponse>> response) override
    {
        m_delayAddTimer = sTimer::create();
        m_delayAdd = std::move(response);
        auto a = request->a();
        auto b = request->b();

        logger().info("[server] [<==] receive request delayAdd, a :", a, ", b :", b);

        // send response after 5 seconds
        m_delayAddTimer->once(5000, [this, a, b]()
            {
                auto result = a + b;
                logger().info("[server] [==>] send delayAdd response :", result);
                m_delayAdd->resolve(CreateDelayAddResponse(m_delayAdd->builder(), result));
            }
        );
    }

    // simulate send event to client
    void subscribeObjectCreateEvent(const EventDataFilter* filter, std::unique_ptr<sEventEmitter<ObjectCreateEvent>> emitter) override
    {
        m_eventTimer = sTimer::create();
        m_eventEmitter = std::move(emitter);

        auto min = filter->min();
        logger().info("[server] [<==] receive subscribeObjectCreateEvent, filter :", min);

        // trigger event per second
        constexpr auto interval = 1000;
        m_eventTimer->repeat(interval, [this, min]()
            {
                auto data = std::rand();
                if (data > min)
                {
                    logger().info("[server] [==>] send ObjectCreateEvent to client :", data);
                    m_eventEmitter->emit(CreateObjectCreateEvent(m_eventEmitter->builder(), data));
                }
            }
        );
    }

    void testUnion(const GetUnionRequest* request, std::unique_ptr<sPromise<GetUnionResponse>> response) override
    {
        auto& fbb = response->builder();
        std::vector<std::string> stringVector = { "a", "b", "c" };
        auto temp = fbb.CreateVectorOfStrings(stringVector);
        auto result = CreateStringVector(fbb, temp).Union();
        GetUnionResponseBuilder builder(fbb);
        builder.add_data_type(UnionData::UnionData_StringVector);
        builder.add_data(result);
        response->resolve(builder.Finish());
    }

private:
    std::unique_ptr<sTimer> m_delayAddTimer;
    std::unique_ptr<sPromise<DelayAddResponse>> m_delayAdd;

    std::unique_ptr<sTimer> m_eventTimer;
    std::unique_ptr<sPromise<ObjectCreateEvent>> m_eventEmitter;
};

int main(int argc, const char** argv)
{
    auto isServer = [argc, argv]() { return argc == 1 || (argc == 2 && std::string(argv[1]) == "-s"); };
    auto isClient = [argc, argv]() { return argc == 1 || (argc == 2 && std::string(argv[1]) == "-c"); };

    sTCPOption option = { "127.0.0.1", 8080 };

    auto server = isServer() ? sFlatBufferRpcServer::create(option) : nullptr;

    if (isServer())
    {
        server->installService<ExampleServiceImpl>();
        server->start();
    }

    auto client = isClient() ? sFlatBufferRpcClient::create(option) : nullptr;

    if (isClient())
        client->connect();

    bool rpcCallFinished = false;
    while (true)
    {
        if (isServer())
            server->update();

        if (isClient())
            client->update();

        if (isClient() && client->isConnected() && !rpcCallFinished)
        {
            auto stub = client->getStub<ExampleStub>();
            if (auto sharedStub = stub.lock())
            {
                auto req = CreateHelloWorldRequest(sharedStub->builder(), sharedStub->createString("Song"));
                logger().info("[client] [==>] helloWorld request");
                sharedStub->helloWorld(req, 
                    [](const HelloWorldResponse* res)
                    {
                        logger().info("[client] [<==] helloWorld response", res->message()->c_str());
                    }
                );

                double a = 1.0, b = 2.0;
                logger().info("[client] [==>] delay add request, a :", a, ", b :", b);
                sharedStub->delayAdd(CreateDelayAddRequest(sharedStub->builder(), a, b), 
                    [](const DelayAddResponse* res)
                    {
                        logger().info("[client] [<==] delay add response, result:", res->result());
                    }
                );

                logger().info("[client] [==>] subscribe ObjectCreateEvent");
                auto filter = CreateEventDataFilter(sharedStub->builder(), 42);
                sharedStub->subscribeObjectCreateEvent(filter, 
                    [](const ObjectCreateEvent* e)
                    {
                        logger().info("[client] [<==] receive ObjectCreateEvent, data:", e->data());
                    }
                );

                logger().info("[client] [==>] test get union");
                sharedStub->testUnion(CreateGetUnionRequest(sharedStub->builder()),
                    [](const GetUnionResponse* res)
                    {
                        auto vec = res->data_as_StringVector();
                        auto strings = vec->strings();
                        for (int i = 0; i < strings->size(); ++i)
                            logger().info("[client] [<==] test get union", strings->Get(i)->str());
                    }
                );

                rpcCallFinished = true;
            }
        }
    }
    
    return 0;
}
