#include "common.hpp"
#include <iostream>

bool session_ended(const boost::system::error_code& ec) {
    return
        ec == websocket::error::closed ||
        ec == net::error::connection_reset ||
        ec == net::error::eof
        ;
}
LogOnCatch::LogOnCatch(std::string source)
    :m_source(std::move(source)){
}
void LogOnCatch::operator()(std::exception_ptr e){
    if(e) try{
            std::rethrow_exception(e);
        }catch(std::exception &e){
            std::cerr << std::format("Error in {}:{}", m_source, e.what());
        }
}
