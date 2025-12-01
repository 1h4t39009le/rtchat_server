#pragma once

#include "common.hpp"
#include <memory>
#include <deque>
#include "room.hpp"


class RoomMember : public std::enable_shared_from_this<RoomMember> {
public:
    using Connection = beast::websocket::stream<beast::tcp_stream>;
    RoomMember(std::string name, Connection connection, std::shared_ptr<Room> room);

    RoomMember(const RoomMember&) = delete;
    RoomMember& operator=(const RoomMember&) = delete;

    void deliver(const std::string &message);
    void close_with_message(const std::string &message);
    void send_impl(const std::string &message, bool close_after = false);
    net::awaitable<void> run();

    std::string const &get_name() const;

private:
    net::awaitable<void> write_loop(std::shared_ptr<RoomMember> self);
    std::string m_name;
    std::size_t m_id;
    Connection m_connection;
    std::weak_ptr<Room> m_room; // room outlives session

    bool m_should_close = false;
    bool m_is_writing = false;
    std::deque<std::string> m_write_queue;
};
