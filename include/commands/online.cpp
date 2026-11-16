#include "pch.hpp"

#include "onVariant/ConsoleMessage.hpp"

#include "online.hpp"

#include <string>
#include <format>

void online(ENetEvent& event, std::string_view)
{
    int count = 0;
    peers("", PeerSearch::EVERYONE, [&count](ENetPeer* peer){
        count++;
    });
    std::string_view label = (count == 1) ? "player" : "players"; 
    on::ConsoleMessage(event.peer,
        std::format(
            "This server has {} {} online.", count, label
        ));
    send_gameMessage(event.peer, {.message = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0"});
    
}
