
#pragma once

#include "database/peer.hpp"

#include <string>
#include <vector>

namespace spawn_queue
{
    bool enqueue(
        std::string world_name,
        short item_id,
        std::vector<::pos> positions
    );

    void update();

    void mark(const std::string& world_name, unsigned int uid);

    bool is_spawned(
        const std::string& world_name,
        unsigned int uid
    );

    void forget(const std::string& world_name, unsigned int uid);

    void clear_tracking(const std::string& world_name);
    void cancel_world(const std::string& world_name);
}
