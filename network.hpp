#ifndef SERVER_NETWORK_HPP
#define SERVER_NETWORK_HPP

#include <enet/enet.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>
#include <functional>

#include "sbpt_generated_includes.hpp"

using OnConnectCallback = std::function<void(unsigned int)>;
using OnDisconnectCallback = std::function<void(unsigned int)>;

/**
 * \details A server that keeps track of the connected clients and provides methods for sending and receving data
 * every connected client has a unique id, which is the index at which the client is stored in the clients vector
 * you must call initialize_network() before working with the system
 */

class Network {
  public:
    explicit Network(uint16_t port, const std::vector<spdlog::sink_ptr> &sinks = {});
    ~Network();

    LoggerComponent logger_component;

    void set_on_connect_callback(OnConnectCallback &connect_cb) { this->on_connect_callback = connect_cb; };
    void set_on_disconnect_callback(OnDisconnectCallback &disconnect_cb) {
        this->on_disconnect_callback = disconnect_cb;
    };
    void initialize_network();

    std::vector<PacketWithSize> get_network_events_since_last_tick();
    void unreliable_broadcast(const void *data, size_t data_size);
    void unreliable_send(unsigned int id_of_client_to_send_to, const void *data, size_t data_size);
    void reliable_broadcast(const void *data, size_t data_size);
    void reliable_broadcast_to_everyone_but(unsigned int id_of_client_to_exclude, const void *data, size_t data_size);
    void reliable_send(unsigned int id_of_client_to_send_to, const void *data, size_t data_size);
    std::vector<unsigned int> get_connected_client_ids();

  private:
    unsigned int num_clients_that_connected = 0;
    uint16_t port;
    OnConnectCallback on_connect_callback;
    OnDisconnectCallback on_disconnect_callback;
    ENetHost *server;
    std::unordered_map<unsigned int, ENetPeer *> client_id_to_enet_peer;
};

#endif // SERVER_NETWORK_HPP
