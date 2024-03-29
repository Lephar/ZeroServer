#include "server.hpp"

namespace zero::network {
    void optimizeSocket(int32_t socket) {
        int32_t nodelayValue = 1;
        uint8_t tosValue = IPTOS_LOWDELAY;

        setsockopt(socket, IPPROTO_IP, IP_TOS, &tosValue, sizeof(uint8_t));
        setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &nodelayValue, sizeof(int32_t));
    }

    Server::Server(uint32_t port) : statusLock(), listLock(), listener(), receiver(), sender(), clients() {
        active = false;

        pthread_rwlock_init(&statusLock, nullptr);
        pthread_rwlock_init(&listLock, nullptr);

        server = socket(AF_INET, SOCK_STREAM, 0);

        int32_t reuseaddrValue = 1;
        setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuseaddrValue, sizeof(int32_t));

        optimizeSocket(server);

        sockaddr_in serverAddress = {
                .sin_family = AF_INET,
                .sin_port = htons(port),
                .sin_addr = {
                        .s_addr = INADDR_ANY
                }
        };

        static_cast<void>(bind(server, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(sockaddr_in)));
    }

    bool Server::isActive() {
        pthread_rwlock_rdlock(&statusLock);
        bool value = active;
        pthread_rwlock_unlock(&statusLock);

        return value;
    }

    void Server::setActive(bool value) {
        pthread_rwlock_wrlock(&statusLock);
        active = value;
        pthread_rwlock_unlock(&statusLock);
    }

    void Server::listenLoop() {
        while (isActive()) {
            sockaddr_in clientAddress{};
            socklen_t addressLength = sizeof(sockaddr_in);

            int32_t client = accept(server, reinterpret_cast<sockaddr *>(&clientAddress), &addressLength);
            optimizeSocket(client);

            pthread_rwlock_wrlock(&listLock);
            clients.emplace_back(client, clientAddress.sin_addr.s_addr, clientAddress.sin_port);
            pthread_rwlock_unlock(&listLock);
        }
    }

    void Server::receiveLoop() {
        while (isActive()) {

        }
    }

    void Server::sendLoop() {
        while (isActive()) {

        }
    }

    void *listenerProxy(void *context) {
        auto server = reinterpret_cast<Server *>(context);
        server->listenLoop();
        return nullptr;
    }

    void *receiverProxy(void *context) {
        auto server = reinterpret_cast<Server *>(context);
        server->receiveLoop();
        return nullptr;
    }

    void *senderProxy(void *context) {
        auto server = reinterpret_cast<Server *>(context);
        server->sendLoop();
        return nullptr;
    }

    void Server::start() {
        listen(server, SOMAXCONN);

        setActive(true);

        pthread_create(&listener, nullptr, listenerProxy, this);
        pthread_create(&receiver, nullptr, receiverProxy, this);
        pthread_create(&sender, nullptr, senderProxy, this);
    }

    void Server::stop() {
        if (!isActive())
            return;

        setActive(false);

        pthread_cancel(listener);

        pthread_join(listener, nullptr);
        pthread_join(receiver, nullptr);
        pthread_join(sender, nullptr);

        close(server);
    }

    Server::~Server() {
        stop();
    }
}