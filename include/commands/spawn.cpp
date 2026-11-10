
#include "pch.hpp"
#include "spawn_queue.hpp"

#include "spawn.hpp"
#include "onVariant/ConsoleMessage.hpp"

#include <charconv>
#include <limits>
#include <utility>

void spawn(ENetEvent& event, const std::string_view text)
{
    ::peer* pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role != DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4You don't have permission.");
        return;
    }

    std::string_view args = text.substr(sizeof("/spawn") - 1);

    const auto first = args.find_first_not_of(" \t");

    if (first == std::string_view::npos)
    {
        on::ConsoleMessage(event.peer, "`4Usage: /spawn <item-id>");
        return;
    }

    args.remove_prefix(first);

    int item_id{};

    const auto [ptr, ec] = std::from_chars(
        args.data(),
        args.data() + args.size(),
        item_id
    );

    if (ec != std::errc{} ||
        ptr != args.data() + args.size() ||
        item_id <= 0 ||
        item_id > std::numeric_limits<short>::max() ||
        item_id >= static_cast<int>(items.size()) ||
        items[item_id].id != item_id)
    {
        on::ConsoleMessage(event.peer, "`4Invalid item ID.");
        return;
    }

    if (pPeer->recent_worlds.empty())
    {
        on::ConsoleMessage(event.peer, "`4You are not in a world.");
        return;
    }

    const std::string& world_name = pPeer->recent_worlds.back();

    auto world_it = std::ranges::find(
        worlds,
        world_name,
        &::world::name
    );

    if (world_it == worlds.end())
    {
        on::ConsoleMessage(event.peer, "`4World not found.");
        return;
    }

    constexpr int RADIUS_X = 5;
    constexpr int RADIUS_Y = 5;
    constexpr int WIDTH = 100;

    const int height =
        static_cast<int>(world_it->blocks.size()) / WIDTH;

    const ::pos player_pos = pPeer->pos.by_32(true);

    const int center_x = player_pos.x_int();
    const int center_y = player_pos.y_int();

    std::vector<::pos> positions;

    for (int y = center_y - RADIUS_Y; y <= center_y + RADIUS_Y; ++y)
    {
        for (int x = center_x - RADIUS_X; x <= center_x + RADIUS_X; ++x)
        {
            if (x < 0 || x >= WIDTH || y < 0 || y >= height)
                continue;

            positions.emplace_back(::pos{x, y}.by_32(false));
        }
    }

    const int spawned = static_cast<int>(positions.size());

    if (!spawn_queue::enqueue(
        world_name,
        static_cast<short>(item_id),
        std::move(positions)
    ))
    {
        on::ConsoleMessage(event.peer, "`4Spawn queue is full.");
        return;
    }

    on::ConsoleMessage(
        event.peer,
        std::format(
            "`2Spawn request queued! `oItem ID: `w{} `o| Positions: `w{}",
            item_id,
            spawned
        )
    );
}
