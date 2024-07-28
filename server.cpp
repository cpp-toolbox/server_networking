#include "server.hpp"
#include "sbpt_generated_includes.hpp"

#include <algorithm>

void default_packet_callback(ENetPacket *packet) {
    spdlog::info("Default packet callback: Packet received with length {}.", packet->dataLength);
}

Server::Server(uint16_t port, PacketCallback packet_callback)
    : port(port), packet_callback(packet_callback), server(nullptr) {}

Server::~Server() {
    if (server != nullptr) {
        enet_host_destroy(server);
    }
    enet_deinitialize();
}

void Server::initialize_network() {
    if (enet_initialize() != 0) {
        spdlog::get(Systems::networking)->error("An error occurred while initializing ENet.");
        throw std::runtime_error("ENet initialization failed.");
    }

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    server = enet_host_create(&address, 32, 2, 0, 0);
    if (server == nullptr) {
        spdlog::get(Systems::networking)->error("An error occurred while trying to create an ENet server host.");
        throw std::runtime_error("ENet server host creation failed.");
    }

    spdlog::get(Systems::networking)->info("Server initialized on port {}.", port);
}

void Server::process_network_events() {
    ENetEvent event;
    while (enet_host_service(server, &event, 0) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            spdlog::get(Systems::networking)->info("A new client connected from {}:{}.",
                         event.peer->address.host, event.peer->address.port);
            clients.push_back(event.peer);
            break;

        case ENET_EVENT_TYPE_RECEIVE:
            spdlog::get(Systems::networking)->info("Packet received from client {}.", event.peer->address.host);
            packet_callback(event.packet);
            enet_packet_destroy(event.packet);
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            spdlog::get(Systems::networking)->info("Client {} disconnected.", event.peer->address.host);
            clients.erase(std::remove(clients.begin(), clients.end(), event.peer), clients.end());
            event.peer->data = nullptr;
            break;

        default:
            break;
        }
    }
}

void Server::unreliable_broadcast(const void *data, size_t data_size) {
    ENetPacket *packet = enet_packet_create(data, data_size, 0);
    for (auto &client : clients) {
        enet_peer_send(client, 0, packet);
    }
    enet_host_flush(server);
}

void Server::reliable_broadcast(const void *data, size_t data_size) {
    ENetPacket *packet = enet_packet_create(data, data_size, ENET_PACKET_FLAG_RELIABLE);
    for (auto &client : clients) {
        enet_peer_send(client, 0, packet);
    }
    enet_host_flush(server);
}