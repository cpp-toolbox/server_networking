#include "server.hpp"
#include "sbpt_generated_includes.hpp"

#include <algorithm>
#include <utility>


Server::Server(uint16_t port) : port(port), server(nullptr) {}

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

std::vector<PacketData> Server::get_network_events_since_last_tick() {
    ENetEvent event;

    std::vector<PacketData> received_packets;


    while (enet_host_service(server, &event, 0) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT: {
            spdlog::get(Systems::networking)->info("A new client connected from {}:{}.",
                                                   event.peer->address.host, event.peer->address.port);
            clients.push_back(event.peer);

            unsigned int client_id = num_clients_that_connected; // The unique index for the new client

            spdlog::get(Systems::networking)->info("Client added with unique index: {}", client_id);
            on_connect_callback(client_id);

            num_clients_that_connected += 1;
        } break;

        case ENET_EVENT_TYPE_RECEIVE:
            spdlog::get(Systems::networking)->info("Packet received from client {}.", event.peer->address.host);
            received_packets.push_back(parse_packet(event.packet));
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
    return received_packets;
}

void Server::unreliable_send(unsigned int id_of_client_to_send_to, const void *data, size_t data_size) {
    ENetPacket *packet = enet_packet_create(data, data_size, 0);
    ENetPeer * client_to_send_to = clients.at(id_of_client_to_send_to);
    enet_peer_send(client_to_send_to, 0, packet);
    enet_host_flush(server);
}

void Server::unreliable_broadcast(const void *data, size_t data_size) {
    ENetPacket *packet = enet_packet_create(data, data_size, 0);
    for (auto &client : clients) {
        enet_peer_send(client, 0, packet);
    }
    enet_host_flush(server);
}

/**
 * @brief Sends out a reliable packet with the specified data immediately
 *
 * @param data
 * @param data_size
 */
void Server::reliable_broadcast(const void *data, size_t data_size) {
    ENetPacket *packet = enet_packet_create(data, data_size, ENET_PACKET_FLAG_RELIABLE);
    for (auto &client : clients) {
        enet_peer_send(client, 0, packet);
    }
    enet_host_flush(server);
}

void Server::reliable_send(unsigned int id_of_client_to_send_to, const void *data, size_t data_size) {
    ENetPacket *packet = enet_packet_create(data, data_size, ENET_PACKET_FLAG_RELIABLE);
    ENetPeer *client_to_send_to = clients.at(id_of_client_to_send_to);
    enet_peer_send(client_to_send_to, 0, packet);
    enet_host_flush(server);
}
