#include "json/json_impl.hpp"
#include <iostream>
#include "common.hpp"
#include "room_member.hpp"

RoomMember::RoomMember(std::string name, Connection connection, std::shared_ptr<Room> room)
    : m_name(std::move(name)), m_connection(std::move(connection)), m_room(room){
}

void RoomMember::deliver(const std::string &message){
    send_impl(message);
}
void RoomMember::close_with_message(const std::string &message) {
    send_impl(message, true);
}

void RoomMember::send_impl(const std::string &message, bool close_after){
    net::post(
        m_connection.get_executor(),
        [self = shared_from_this(), message = std::move(message), close_after]{
            self->m_write_queue.push_back(message);
            if(close_after){
                self->m_should_close = true;
            }
            if(!self->m_is_writing){
                self->m_is_writing = true;
                net::co_spawn(
                    self->m_connection.get_executor(),
                    self->write_loop(std::move(self)),
                    LogOnCatch("session write_loop")
                    );
            }
        });
}

std::string const & RoomMember::get_name() const {
    return m_name;
}

net::awaitable<void> RoomMember::run(http::request<http::string_body> req){
    auto room = m_room.lock();
    if (!room) co_return;
    co_await m_connection.async_accept(req, net::use_awaitable);
    auto self = shared_from_this();
    auto room_code = room->get_code();
    auto id_opt = room->add_client(self);
    if(!id_opt) {
        close_with_message(nlohmann::json(ServerPrepareResponse{.error = ServerPrepareError::InvalidRoomCode}).dump());
        co_return;
    }
    m_id = *id_opt;
    // session starts here
    try {
        beast::flat_buffer buffer;
        nlohmann::json start_response = ServerPrepareResponse{.client_id = m_id, .client_names = room->get_client_names(), .room_code = room_code};


        deliver(start_response.dump());

        for (;;) {
            co_await m_connection.async_read(buffer, net::use_awaitable);
            room->sending(m_id, beast::buffers_to_string(buffer.data()));
            buffer.consume(buffer.size());
        }
    }
    catch (const boost::system::system_error& e) {
        if (SessionEnded(e.code())) {
            std::cout << std::format("Session ended\n");

        } else throw;
    }
    room->leaving(m_id);
}
net::awaitable<void> RoomMember::write_loop(std::shared_ptr<RoomMember> self){
    try{
        while(!m_write_queue.empty()){
            const auto &msg = m_write_queue.front();
            co_await m_connection.async_write(net::buffer(msg), net::use_awaitable);
            m_write_queue.pop_front();
        }
        if (self->m_should_close) {
            co_await self->m_connection.async_close(websocket::close_code::policy_error, net::use_awaitable);
        }
    } catch(...) {}

    m_is_writing = false;
}
