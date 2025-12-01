#pragma once
class RoomManager;
class RoomMember;
using RoomCode = std::size_t;

class Room {
public:
    Room(RoomCode code, std::function<void(RoomCode)> on_leave);
    RoomCode get_code();
    void sending(std::size_t id, const std::string &message);
    void leaving(std::size_t id);
private:
    void broadcast_message(const nlohmann::json &data, std::optional<std::size_t> exclude_id = std::nullopt);
    friend class RoomManager;
    friend class RoomMember;
    std::optional<std::size_t> add_session(std::shared_ptr<RoomMember> session);

    std::mutex m_mutex;
    std::function<void(RoomCode)> m_on_leave;
    bool m_is_dead = false;
    RoomCode m_code;
    std::size_t m_id_counter{};
    std::unordered_map<std::size_t, std::weak_ptr<RoomMember>> m_sessions{};
};
