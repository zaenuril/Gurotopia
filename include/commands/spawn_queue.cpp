#include "pch.hpp"
#include "spawn_queue.hpp"

#include <algorithm>
#include <chrono>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace
{
    struct spawn_request
    {
        std::string world_name;
        short item_id;
        std::vector<::pos> positions;
        std::size_t index = 0;
    };

    std::deque<spawn_request> requests;

    std::unordered_map<
        std::string,
        std::unordered_set<unsigned int>
    > spawned_objects;

    constexpr std::size_t ITEMS_PER_BATCH = 5;
    constexpr auto BATCH_DELAY = std::chrono::milliseconds(100);

    auto next_batch = std::chrono::steady_clock::now();
}

bool spawn_queue::enqueue(
    std::string world_name,
    short item_id,
    std::vector<::pos> positions
)
{
    if (requests.size() >= 8 || positions.empty())
        return false;

    requests.push_back({
        std::move(world_name),
        item_id,
        std::move(positions),
        0
    });

    return true;
}

void spawn_queue::mark(
    const std::string& world_name,
    unsigned int uid
)
{
    spawned_objects[world_name].insert(uid);
}

bool spawn_queue::is_spawned(
    const std::string& world_name,
    unsigned int uid
)
{
    auto world = spawned_objects.find(world_name);

    if (world == spawned_objects.end())
        return false;

    return world->second.contains(uid);
}

void spawn_queue::forget(
    const std::string& world_name,
    unsigned int uid
)
{
    auto world = spawned_objects.find(world_name);

    if (world == spawned_objects.end())
        return;

    world->second.erase(uid);

    if (world->second.empty())
        spawned_objects.erase(world);
}

void spawn_queue::clear_tracking(const std::string& world_name)
{
    spawned_objects.erase(world_name);
}

void spawn_queue::update()
{
    if (requests.empty())
        return;

    const auto now = std::chrono::steady_clock::now();

    if (now < next_batch)
        return;

    auto& request = requests.front();

    auto world_it = std::ranges::find(
        worlds,
        request.world_name,
        &::world::name
    );

    if (world_it == worlds.end())
    {
        requests.pop_front();
        return;
    }

    ::world& world = *world_it;

    std::size_t processed = 0;

    while (
        request.index < request.positions.size() &&
        processed < ITEMS_PER_BATCH
    )
    {
        const ::pos position = request.positions[request.index];

        // Jangan merge dengan dropped item normal.
        ::object& dropped = world.objects.emplace_back(
            request.item_id,
            1,
            position,
            ++world.last_object_uid
        );

        // Catat UID item hasil spawn.
        mark(request.world_name, dropped.uid);

        ::gamePacket packet{
            .type = 0x0e,
            .netid = static_cast<int>(0xffffffffu),
            .uid = static_cast<int>(dropped.uid),
            .count = 1.0f,
            .id = request.item_id,
            .pos = position
        };

        const ::blob data = compress_state(packet);

        peers(
            request.world_name,
            PeerSearch::WORLD,
            [&data](ENetPeer* peer)
            {
                send_data(*peer, data);
            }
        );

        ++request.index;
        ++processed;
    }

    if (request.index >= request.positions.size())
        requests.pop_front();

    next_batch = now + BATCH_DELAY;
}

void spawn_queue::cancel_world(const std::string& world_name)
{
    std::erase_if(requests, [&](const spawn_request& request)
    {
        return request.world_name == world_name;
    });
}
