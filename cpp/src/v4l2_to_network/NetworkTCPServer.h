#pragma once

#include <aRibeiroCore/aRibeiroCore.h>
#include <aRibeiroPlatform/aRibeiroPlatform.h>

using namespace aRibeiro;

class NetworkTCPServer
{

    void accept_thread_run()
    {
        PlatformSocketTCPAccept acceptSocket(
            true, //blocking
            true, //reuseAddress
            true  //noDelay
        );

        if (!acceptSocket.bindAndListen(
                "INADDR_ANY",                        // interface address
                NetworkConstants::PUBLIC_PORT_START, // port
                10                                   // input queue
                ))
        {
            return;
        }

        PlatformSocketTCP *clientSocket = new PlatformSocketTCP();

        while (!PlatformThread::isCurrentThreadInterrupted())
        {
            if (!acceptSocket.accept(clientSocket))
            {
                if (acceptSocket.isSignaled() || acceptSocket.isClosed() || !acceptSocket.isListening())
                    break;
                continue;
            }

            mutex.lock();
            connectedClients.push_back(clientSocket);
            mutex.unlock();

            clientSocket = new PlatformSocketTCP();
        }

        mutex.lock();
        for (size_t i = 0; i < connectedClients.size(); i++)
        {
            connectedClients[i]->close();
            delete connectedClients[i];
        }
        connectedClients.clear();
        mutex.unlock();

        aRibeiro::setNullAndDelete(clientSocket);

        acceptSocket.close();

        printf("accept_thread_run end\n");
    }

    void send_thread_run()
    {
        bool isSignaled;
        while (!PlatformThread::isCurrentThreadInterrupted())
        {

            ObjectBuffer *buffer = toSend.dequeue(&isSignaled);
            if (buffer == NULL || isSignaled)
                break;

            mutex.lock();
            for (int i = (int)connectedClients.size() - 1; i >= 0; i--)
            {
                if (!connectedClients[i]->write_buffer(buffer->data, buffer->size))
                {
                    printf("disconnecting client: %i\n", i);
                    connectedClients[i]->close();
                    delete connectedClients[i];
                    connectedClients.erase(connectedClients.begin() + i);
                }
            }
            mutex.unlock();

            pool.release(buffer);
        }

        printf("send_thread_run end\n");
    }

public:
    PlatformMutex mutex;
    std::vector<PlatformSocketTCP *> connectedClients;
    PlatformThread acceptThread;
    PlatformThread sendThread;

    ObjectQueue<ObjectBuffer *> toSend;
    ObjectPool<ObjectBuffer> pool;

    NetworkTCPServer() : acceptThread(this, &NetworkTCPServer::accept_thread_run),
                         sendThread(this, &NetworkTCPServer::send_thread_run)
    {
    }

    ~NetworkTCPServer()
    {
        stop();
    }

    void start()
    {
        acceptThread.start();
        sendThread.start();
    }
    void stop()
    {
        acceptThread.interrupt();
        acceptThread.wait();

        sendThread.interrupt();
        sendThread.wait();

        mutex.lock();
        for (size_t i = 0; i < connectedClients.size(); i++)
        {
            connectedClients[i]->close();
            delete connectedClients[i];
        }
        connectedClients.clear();
        mutex.unlock();

        while (toSend.size() > 0)
            pool.release(toSend.dequeue(NULL,true));
    }

    void write(const uint8_t *data, uint32_t size, uint32_t width, uint32_t height, uint32_t format)
    {

        if (toSend.size() > 32)
        {
            printf("queue full...\n");
            return;
        }

        //printf("Send size bytes: %u\n", size);

        ObjectBuffer *buffer = pool.create(true);
        buffer->setSize(size + sizeof(uint32_t) * 4);

        memcpy(buffer->data + sizeof(uint32_t) * 0, &size, sizeof(uint32_t));
        memcpy(buffer->data + sizeof(uint32_t) * 1, &width, sizeof(uint32_t));
        memcpy(buffer->data + sizeof(uint32_t) * 2, &height, sizeof(uint32_t));
        memcpy(buffer->data + sizeof(uint32_t) * 3, &format, sizeof(uint32_t));

        memcpy(buffer->data + sizeof(uint32_t) * 4, data, size);

        toSend.enqueue(buffer);
    }
};