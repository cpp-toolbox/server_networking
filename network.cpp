#include "network.hpp"
#include "sbpt_generated_includes.hpp"

#include <algorithm>
#include <iostream>
#include <utility>

Network::Network(uint16_t port) : port(port), server(nullptr) { initialize_network(); }

Network::~Network() {
    if (server != nullptr) {
        enet_host_destroy(server);
    }
    enet_deinitialize();
}

void Network::initialize_network() {
    LogSection _(global_logger, "initialize_network");
    if (enet_initialize() != 0) {
        global_logger.error("an error occurred while initializing ENet.");
        throw std::runtime_error("ENet initialization failed.");
    }

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    server = enet_host_create(&address, 32, 2, 0, 0);
    if (server == nullptr) {
        global_logger.error("an error occurred while trying to create an ENet server host.");
        throw std::runtime_error("ENet server host creation failed.");
    }

    global_logger.info("network initialized on port {}.", port);
}

std::vector<PacketWithSize> Network::get_network_events_since_last_tick() {
    LogSection _(global_logger, "get_network_events_received_since_last_tick");

    ENetEvent event;
    std::vector<PacketWithSize> received_packets;

    PacketWithSize packet_with_size;

    while (enet_host_service(server, &event, 0) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT: {

            global_logger.info("a new client connected from {}:{}.", event.peer->address.host,
                               event.peer->address.port);

            unsigned int client_id = num_clients_that_connected; // The unique index for the new client
            client_id_to_enet_peer[client_id] = event.peer;

            global_logger.info("client added with unique index: {}", client_id);

            if (on_connect_callback) {
                on_connect_callback(client_id);
            } else {
                global_logger.warn("on_connect_callback is not set. Skipping callback.");
            }

            num_clients_that_connected++;
        } break;

        case ENET_EVENT_TYPE_RECEIVE:

            global_logger.info("packet received from peer {}: size {} bytes.", event.peer->address.host,
                               event.packet->dataLength);

            packet_with_size.data.resize(event.packet->dataLength);

            std::memcpy(packet_with_size.data.data(), event.packet->data, event.packet->dataLength);
            packet_with_size.size = event.packet->dataLength;

            received_packets.push_back(packet_with_size);

            enet_packet_destroy(event.packet);
            break;

        case ENET_EVENT_TYPE_DISCONNECT: {

            global_logger.info("client {} disconnected.", event.peer->address.host);

            auto it = std::find_if(client_id_to_enet_peer.begin(), client_id_to_enet_peer.end(),
                                   [&](const auto &pair) { return pair.second == event.peer; });

            auto client_id_of_disonnecting_peer = it->first;
            if (it != client_id_to_enet_peer.end()) {

                global_logger.info("client {} disconnected.", it->first);
                client_id_to_enet_peer.erase(it);

                if (on_disconnect_callback) {
                    event_emitter.emit(ClientDisconnectSignal{client_id_of_disonnecting_peer});
                    on_disconnect_callback(client_id_of_disonnecting_peer);
                } else {
                    global_logger.warn("on_disconnect_callback is not set. Skipping callback.");
                }
            }

            event.peer->data = nullptr;
        } break;

        default:
            break;
        }
    }
    return received_packets;
}

void Network::unreliable_send(unsigned int id_of_client_to_send_to, const void *data, size_t data_size) {
    if (client_id_to_enet_peer.find(id_of_client_to_send_to) == client_id_to_enet_peer.end())
        return;
    ENetPacket *packet = enet_packet_create(data, data_size, 0);
    ENetPeer *client_to_send_to = client_id_to_enet_peer.at(id_of_client_to_send_to);
    enet_peer_send(client_to_send_to, 0, packet);
    enet_host_flush(server);
}

void Network::unreliable_broadcast(const void *data, size_t data_size) {
    LogSection _(global_logger, "unreliable_broadcast");
    ENetPacket *packet = enet_packet_create(data, data_size, 0);
    for (auto &[id, client] : client_id_to_enet_peer) {
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
void Network::reliable_broadcast(const void *data, size_t data_size) {
    ENetPacket *packet = enet_packet_create(data, data_size, ENET_PACKET_FLAG_RELIABLE);
    for (auto &[id, client] : client_id_to_enet_peer) {
        enet_peer_send(client, 0, packet);
    }
    enet_host_flush(server);
}

void Network::reliable_broadcast_to_everyone_but(unsigned int id_of_client_to_exclude, const void *data,
                                                 size_t data_size) {
    ENetPacket *packet = enet_packet_create(data, data_size, ENET_PACKET_FLAG_RELIABLE);
    for (auto &[id, client] : client_id_to_enet_peer) {
        if (id != id_of_client_to_exclude) {
            enet_peer_send(client, 0, packet);
        }
    }
    enet_host_flush(server);
}

void Network::reliable_send(unsigned int id_of_client_to_send_to, const void *data, size_t data_size) {

    LogSection _(global_logger, "reliable_send");

    if (client_id_to_enet_peer.find(id_of_client_to_send_to) == client_id_to_enet_peer.end())
        return;
    ENetPacket *packet = enet_packet_create(data, data_size, ENET_PACKET_FLAG_RELIABLE);
    ENetPeer *client_to_send_to = client_id_to_enet_peer.at(id_of_client_to_send_to);

    global_logger.info("Sending packet of size {} bytes to client {}.", data_size, client_to_send_to->address.host);

    enet_peer_send(client_to_send_to, 0, packet);
    enet_host_flush(server);
}

std::vector<unsigned int> Network::get_connected_client_ids() {
    std::vector<unsigned int> client_ids;
    client_ids.reserve(client_id_to_enet_peer.size());

    for (const auto &pair : client_id_to_enet_peer) {
        client_ids.push_back(pair.first);
    }

    return client_ids;
}
