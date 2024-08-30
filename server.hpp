#ifndef SERVER_HPP
#define SERVER_HPP

#include <enet/enet.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>
#include <functional>

#include "sbpt_generated_includes.hpp"

using OnConnectCallback = std::function<void(unsigned int)>;

/**
 * \details A server that keeps track of the connected clients and provides methods for sending and receving data
 * every connected client has a unique id, which is the index at which the client is stored in the clients vector
 */
class Server {
  public:
    explicit Server(uint16_t port);
    ~Server();

    void set_on_connect_callback(OnConnectCallback &connect_cb) {this->on_connect_callback = connect_cb;};
    void initialize_network();
    std::vector<PacketData> get_network_events_since_last_tick();
    void unreliable_broadcast(const void *data, size_t data_size);
    void reliable_broadcast(const void *data, size_t data_size);

  private:
    unsigned int num_clients_that_connected = 0;
    uint16_t port;
    OnConnectCallback on_connect_callback;
    ENetHost *server;
    std::vector<ENetPeer *> clients;
};

#endif // SERVER_HPP
