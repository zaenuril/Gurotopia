#include "pch.hpp"

#include "action/quit_to_exit.hpp"
#include "onVariant/ConsoleMessage.hpp"

#include "nuke.hpp"

void nuke(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "You don't have permission to use this command.");
        return;
    }

    if (pPeer->netid == 0) return;
    if (pPeer->recent_worlds.empty())
        return;

    const std::string world_name = pPeer->recent_worlds.back();

    auto it = std::ranges::find(worlds, world_name, &::world::name);
    if (it == worlds.end()) return;

    if (it->nuked)
    {
        on::ConsoleMessage(event.peer, "`4This world has already been nuked.");
        
        send_gameMessage(event.peer, {
            .message = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0"
        });
        return;
    }

    it->nuked = true;
    const std::string message = std::format(
        "`4{} was nuked from orbit! It's the only way to make sure. Play nice everybody!",
        world_name
    );

    peers("", PeerSearch::EVERYONE, [&message](ENetPeer* peer)
    {
        on::ConsoleMessage(peer, message);

        send_gameMessage(peer, {
            .message = "action|play_sfx\nfile|audio/bigboom.wav\ndelayMS|0"
        });
    });
    it->mysql_update("nuked", 1);

    std::vector<ENetPeer*> targets;

    peers(world_name, PeerSearch::WORLD, [&](ENetPeer *peer)
    {
        ::peer *target = static_cast<::peer*>(peer->data);

        if (target->role == PLAYER)
            targets.push_back(peer);
    });

    for (ENetPeer *target : targets)
    {
        ENetEvent target_event{};
        target_event.peer = target;

        on::ConsoleMessage(target, "This world has been nuked.");
        action::quit_to_exit(target_event, "", false, true);
    }
}
