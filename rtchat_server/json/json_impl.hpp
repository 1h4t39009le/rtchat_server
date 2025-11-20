#pragma once
#include <optional>
#include <string>
#include <nlohmann/json.hpp>

NLOHMANN_JSON_NAMESPACE_BEGIN
template <typename T>
struct adl_serializer<std::optional<T>> {
    static void to_json(json& j, const std::optional<T>& opt) {
        if (opt) j = *opt; else j = nullptr;
    }
    static void from_json(const json& j, std::optional<T>& opt) {
        if (j.is_null()) opt = std::nullopt; else opt = j.get<T>();
    }
};
NLOHMANN_JSON_NAMESPACE_END

enum class ClientPrepareMessageAction { Join, Create };

struct ClientPrepareMessage {
    ClientPrepareMessageAction action;
    std::string name;
    std::optional<std::string> room_code;
};

struct ServerPrepareResponse {
    std::optional<size_t> in_room_id;
    std::optional<std::string> room_code;
    std::optional<std::string> error;
};

NLOHMANN_JSON_SERIALIZE_ENUM(ClientPrepareMessageAction, {
    {ClientPrepareMessageAction::Join, 0},
    {ClientPrepareMessageAction::Create, 1},
})

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ClientPrepareMessage, action, name, room_code)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ServerPrepareResponse, in_room_id, room_code, error)