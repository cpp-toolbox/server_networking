#ifndef SERVER_HPP
#define SERVER_HPP

#include <enet/enet.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <functional>

// Default packet callback that logs receipt of packets
void default_packet_callback(ENetPacket *packet);

using PacketCallback = std::function<void(ENetPacket *)>;

class Server {
  public:
    explicit Server(uint16_t port, PacketCallback packet_callback = default_packet_callback);
    ~Server();

    void initialize_network();
    void process_network_events();
    void unreliable_broadcast(const void *data, size_t data_size);
    void reliable_broadcast(const void *data, size_t data_size);

  private:
    uint16_t port;
    PacketCallback packet_callback;
    ENetHost *server;
    std::vector<ENetPeer *> clients;
};

#endif // SERVER_HPP