#include "pch.hpp"

#include "onVariant/ConsoleMessage.hpp"
#include "unnuke.hpp"

void unnuke(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4You don't have permission to use this command.");
        
        send_gameMessage(event.peer, {
            .message = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0"
        });
        return;
    }

    if (pPeer->recent_worlds.empty())
        return;

    const std::string world_name = pPeer->recent_worlds.back();

    auto it = std::ranges::find(worlds, world_name, &::world::name);
    if (it == worlds.end()) return;

    if (!it->nuked)
    {
        on::ConsoleMessage(event.peer,
            "`4Hold on... who told you this world was nuked?");

        send_gameMessage(event.peer, {
            .message = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0"
        });
        
        return;
    }

    it->nuked = false;
    it->mysql_update("nuked", 0);
    send_gameMessage(event.peer, {.message = "action|play_sfx\nfile|audio/choir.wav\ndelayMS|0"});
    on::ConsoleMessage(event.peer, "`#This world has been restored.");
}
