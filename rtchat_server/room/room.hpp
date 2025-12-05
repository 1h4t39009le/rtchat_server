#pragma once
#include "common/json_impl.hpp"
#include <mutex>
class RoomManager;
class RoomMember;
using RoomCode = std::string;

class Room {
public:
    Room(RoomCode code, std::function<void(const RoomCode &)> on_leave);
    RoomCode const & get_code();
    std::unordered_map<size_t, std::string> get_client_names();
    void sending(std::size_t id, const std::string &message);
    void leaving(std::size_t id);
private:
    void broadcast_message(const nlohmann::json &data, std::optional<std::size_t> exclude_id = std::nullopt);
    friend class RoomManager;
    friend class RoomMember;
    std::optional<std::size_t> add_client(std::shared_ptr<RoomMember> client);

    std::mutex m_mutex;
    std::function<void(RoomCode)> m_on_leave;
    bool m_is_dead = false;
    RoomCode m_code;
    std::size_t m_id_counter{};

    ClientNames m_client_names{};
    std::unordered_map<std::size_t, std::weak_ptr<RoomMember>> m_clients{};
};
