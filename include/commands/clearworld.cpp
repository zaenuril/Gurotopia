#include "pch.hpp"

#include "onVariant/ConsoleMessage.hpp"
#include "clearworld.hpp"

#include <algorithm>

void clearworld(ENetEvent &event, std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    // Moderator and developer only
    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4You don't have permission to use this command.");
        return;
    }

    // Find current world
    const std::string &world_name = pPeer->recent_worlds.back();

    auto it = std::ranges::find(worlds, world_name, &::world::name);

    if (it == worlds.end())
    {
        on::ConsoleMessage(event.peer, "`4World not found.");
        return;
    }

    ::world &world = *it;

    int cleared_blocks = 0;

    // Clear world blocks
    for (std::size_t i = 0; i < world.blocks.size(); ++i)
    {
        ::block &block = world.blocks[i];

        // Preserve Bedrock and Main Door / White Door
        if (block.fg == 8 || block.fg == 6)
            continue;

        // Skip empty tiles
        if (block.fg == 0 && block.bg == 0)
            continue;

        // Reset block
        block.reset();

        // Calculate tile position
        ::pos position{
            static_cast<int>(i % 100),
            static_cast<int>(i / 100)
        };

        // Create tile update packet
        ::gamePacket packet{
            .type = 0x05,
            .state = static_cast<int>(gamePacketState::EXTENDED),
            .punch = position
        };

        ::blob data = compress_state(packet);

        data.push_back(block.to_blob());
        data.u8(get_type(id_to_item(block.fg)));

        // Broadcast tile update to everyone in this world
        peers(world.name, PeerSearch::WORLD, [&data](ENetPeer *peer)
        {
            send_data(*peer, data);
        });

        ++cleared_blocks;
    }

    // Remove dropped objects from clients
    for (const ::object &object : world.objects)
    {
        ::gamePacket packet{
            .netid = -1,
            .uid = static_cast<int>(object.uid),
            .id = object.id
        };

        packet.type = 0x0e;

        ::blob data = compress_state(packet);

        peers(world.name, PeerSearch::WORLD, [&data](ENetPeer *peer)
        {
            send_data(*peer, data);
        });
    }

    // Clear dropped objects
    world.objects.clear();

    // Keep door metadata only for preserved door tiles
    std::erase_if(world.doors, [&](const ::door &door)
    {
        const int index = cord(
            static_cast<int>(door.pos.x),
            static_cast<int>(door.pos.y)
        );

        if (index < 0 || index >= static_cast<int>(world.blocks.size()))
            return true;

        const short fg = world.blocks[index].fg;

        return fg != 6;
    });

    // Clear other block metadata
    world.signs.clear();
    world.trees.clear();
    world.random_blocks.clear();

    // Notify everyone
    peers(world.name, PeerSearch::WORLD, [](ENetPeer *peer)
    {
        on::ConsoleMessage(peer, "`2World successfully cleared.");
        send_gameMessage(peer, {.message = "action|play_sfx\nfile|audio/wood_break.wav\ndelayMS|0"});
    });
}