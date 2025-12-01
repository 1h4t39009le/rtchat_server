#pragma once
#include <boost/beast.hpp>
#include <boost/asio.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;

bool session_ended(const boost::system::error_code& ec);
class LogOnCatch{
public:
    LogOnCatch(std::string source);
    void operator()(std::exception_ptr e);
private:
    std::string m_source;
};
