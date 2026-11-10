#include "pch.hpp"

#include "clearspawn.hpp"
#include "spawn_queue.hpp"

#include "onVariant/ConsoleMessage.hpp"

void clearspawn(
    ENetEvent& event,
    const std::string_view text
)
{
    ::peer* pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role != DEVELOPER)
    {
        on::ConsoleMessage(
            event.peer,
            "`4You don't have permission."
        );
        return;
    }

    if (pPeer->recent_worlds.empty())
    {
        on::ConsoleMessage(
            event.peer,
            "`4You are not in a world."
        );
        return;
    }

    // Lanjutkan logic clearspawn di sini
    const std::string world_name = pPeer->recent_worlds.back();
    
    auto world = std::ranges::find(
        worlds,
        world_name,
        &::world::name
    );
    
    if (world == worlds.end())
    {
        on::ConsoleMessage(event.peer, "`4World not found.");
        return;
    }
    
    // Batalkan spawn yang masih mengantre.
    spawn_queue::cancel_world(world_name);
    
    int removed = 0;
    
    for (auto it = world->objects.begin(); it != world->objects.end();)
    {
        if (spawn_queue::is_spawned(world_name, it->uid))
        {
            remove_object(event, static_cast<signed>(it->uid));
    
            it = world->objects.erase(it);
            ++removed;
        }
        else
        {
            ++it;
        }
    }
    
    spawn_queue::clear_tracking(world_name);
    
    on::ConsoleMessage(
        event.peer,
        std::format("`2Cleared {} spawned objects.", removed)
    );
}