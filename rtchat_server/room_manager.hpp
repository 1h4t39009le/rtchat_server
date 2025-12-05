#pragma once
#include "room/room.hpp"
class RoomManager {
public:
    RoomManager();
    std::shared_ptr<Room> create_empty_room();
    std::optional<std::shared_ptr<Room>> get_room(const RoomCode &code);
private:
    RoomCode generate_code();
    std::mutex m_mutex;
    std::default_random_engine m_rand_eng;
    std::unordered_map<RoomCode, std::shared_ptr<Room>> m_rooms;
};
