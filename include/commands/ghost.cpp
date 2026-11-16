#include "pch.hpp"

#include "onVariant/SetClothing.hpp"
#include "onVariant/ConsoleMessage.hpp"

#include "ghost.hpp"

void ghost(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    pPeer->state ^= S_GHOST;

    on::SetClothing(event.peer);

    std::string msg = (pPeer->state & S_GHOST)
        ? "`9Your atoms suddenly become aware of quantum tunneling.`` (`2Ghost in the Shell mod added``)."
        : "`9Your body stops shimmering and return to normal.`` (`2Ghost in the shell mod removed``).";

    on::ConsoleMessage(event.peer, msg);

    send_gameMessage(event.peer, {
        .message = "action|play_sfx\nfile|audio/boo_ghost_be_gone.wav\ndelayMS|0"
    });
}
