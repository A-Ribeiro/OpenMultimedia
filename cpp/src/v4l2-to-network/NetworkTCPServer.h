#pragma once

#include <InteractiveToolkit/InteractiveToolkit.h>

class NetworkTCPServer : public EventCore::HandleCallback
{

    void accept_thread_run()
    {
        Platform::SocketTCPAccept acceptSocket(
            true, //blocking
            true, //reuseAddress
            true  //noDelay
        );

        if (!acceptSocket.bindAndListen(
                "INADDR_ANY",                        // interface address
                Platform::NetworkConstants::PUBLIC_PORT_START, // port
                10                                   // input queue
                ))
        {
            return;
        }

        Platform::SocketTCP *clientSocket = new Platform::SocketTCP();

        while (!Platform::Thread::isCurrentThreadInterrupted())
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

            clientSocket = new Platform::SocketTCP();
        }

        mutex.lock();
        for (size_t i = 0; i < connectedClients.size(); i++)
        {
            connectedClients[i]->close();
            delete connectedClients[i];
        }
        connectedClients.clear();
        mutex.unlock();

        if (clientSocket != NULL){
            delete clientSocket;
            clientSocket = NULL;
        }

        acceptSocket.close();

        printf("accept_thread_run end\n");
    }

    void send_thread_run()
    {
        bool isSignaled;
        while (!Platform::Thread::isCurrentThreadInterrupted())
        {

            Platform::ObjectBuffer *buffer = toSend.dequeue(&isSignaled);
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
    Platform::Mutex mutex;
    std::vector<Platform::SocketTCP *> connectedClients;
    Platform::Thread acceptThread;
    Platform::Thread sendThread;

    Platform::ObjectQueue<Platform::ObjectBuffer *> toSend;
    Platform::ObjectPool<Platform::ObjectBuffer> pool;

    NetworkTCPServer() : acceptThread( EventCore::CallbackWrapper(&NetworkTCPServer::accept_thread_run, this) ),
                         sendThread( EventCore::CallbackWrapper(&NetworkTCPServer::send_thread_run, this) )
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

        Platform::ObjectBuffer *buffer = pool.create(true);
        buffer->setSize(size + sizeof(uint32_t) * 4);

        memcpy(buffer->data + sizeof(uint32_t) * 0, &size, sizeof(uint32_t));
        memcpy(buffer->data + sizeof(uint32_t) * 1, &width, sizeof(uint32_t));
        memcpy(buffer->data + sizeof(uint32_t) * 2, &height, sizeof(uint32_t));
        memcpy(buffer->data + sizeof(uint32_t) * 3, &format, sizeof(uint32_t));

        memcpy(buffer->data + sizeof(uint32_t) * 4, data, size);

        toSend.enqueue(buffer);
    }
};