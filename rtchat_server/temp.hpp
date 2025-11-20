#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include "json/json_impl.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <format>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;


net::awaitable<void>
continue_session_in_chat(websocket::stream<beast::tcp_stream> stream)
{

    co_return;
}

net::awaitable<void>
do_session(websocket::stream<beast::tcp_stream> stream)
{
    stream.set_option(
        websocket::stream_base::timeout::suggested(beast::role_type::server));
    // Set a decorator to change the Server of the handshake
    stream.set_option(websocket::stream_base::decorator(
        [](websocket::response_type& res)
        {
            res.set(
                http::field::server,
                "bair-chat-server");
        }));
    co_await stream.async_accept();

    for (;;) {
        beast::flat_buffer buffer;
        auto [ec, _] = co_await stream.async_read(buffer, net::as_tuple);
        if (ec == websocket::error::closed ||
            ec == net::error::connection_reset ||
            ec == net::error::eof) {
            co_return;
        }
        if (ec)
            throw boost::system::system_error{ ec };

        nlohmann::json server_response;
        try {
            auto client_msg = nlohmann::json::parse(
                beast::buffers_to_string(buffer.data())).get<ClientPrepareMessage>();
            server_response["in_room_id"] = 0;
            server_response["room_code"] = "ABCD";
        }
        catch (...) {
            server_response["error"] = "Unknown message";
        }
        co_await stream.async_write(net::buffer(server_response.dump()), net::as_tuple);
    }

}



net::awaitable<void>
do_listen(net::ip::tcp::endpoint endpoint)
{
    std::cout << std::format("Server is listening port {}\n", endpoint.port());
    auto executor = co_await net::this_coro::executor;
    auto acceptor = net::ip::tcp::acceptor{ executor, endpoint };
    for (;;) {
        net::co_spawn(
            executor,
            do_session(websocket::stream<beast::tcp_stream>{
            co_await acceptor.async_accept() }),
            [](std::exception_ptr e)
            {
                if (e)
                {
                    try {
                        std::rethrow_exception(e);
                    }
                    catch (std::exception& e) {
                        std::cerr << "Error in session: " << e.what() << "\n";
                    }
                }
            });
    }
}

int main() {
    constexpr auto num_threads = 4;
    net::io_context ioc(num_threads);

    net::co_spawn(
        ioc,
        do_listen(net::ip::tcp::endpoint{ net::ip::tcp::v4(), 8888 }),
        [](std::exception_ptr e)
        {
            if (e)
            {
                try {
                    std::rethrow_exception(e);
                }
                catch (std::exception& e) {
                    std::cerr << "Error in session: " << e.what() << "\n";
                }
            }
        });

    std::vector<std::thread> threads;
    threads.reserve(num_threads - 1);
    for (int i{}; i < threads.capacity(); ++i)
        threads.emplace_back([&ioc] {ioc.run(); });
    ioc.run();
    return EXIT_SUCCESS;
}