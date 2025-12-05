add_library(CommonLib
	common.hpp
	common.cpp
	json_impl.hpp
)
target_precompile_headers(CommonLib PUBLIC
    <boost/asio.hpp>
    <boost/beast.hpp>
    <nlohmann/json.hpp>
)
target_include_directories(CommonLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(CommonLib PUBLIC Boost::headers nlohmann_json::nlohmann_json)
