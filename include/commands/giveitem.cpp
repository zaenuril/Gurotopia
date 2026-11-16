#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"

#include "giveitem.hpp"
#include "database/items.hpp"

#include <cstddef>
#include <string>
#include <sstream>
#include <format>
#include <vector>

constexpr int MIN_VALUE = 1;
constexpr int MAX_VALUE = 200;

void giveitem(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);


    // developer only
    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4You don't have permission to use this command.");
        return;
    }

    const std::string message{ text.substr(sizeof("/give ") -1)};


    std::istringstream args{message};

    int item_id = 0;
    int amount_spawn = 0;

    if (!(args >> item_id >> amount_spawn) ) {
    on::ConsoleMessage(event.peer,
        "`5Usage: /give <item_id> <amount>");
    send_gameMessage(event.peer, {.message = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0"});
    return;
    }

    if (item_id <= 0) {
        on::ConsoleMessage(event.peer, std::format("`5Please enter an ID starting from {}.", MIN_VALUE, item_id));
        send_gameMessage(event.peer, {.message = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0"});
        return;
    }

     if (static_cast<std::size_t>(item_id) >= items.size()){
         on::ConsoleMessage(event.peer, std::format("`5Invalid item ID. Please enter an ID between {} and {}", MIN_VALUE, items.size() - 1 ));
         send_gameMessage(event.peer, {.message = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0"});

         return;
     }

    if (amount_spawn <= 0 || amount_spawn > 200) {
        on::ConsoleMessage(event.peer, std::format("`5Invalid amount. Please enter a value between {} and {}.", MIN_VALUE, MAX_VALUE, amount_spawn));

        send_gameMessage(event.peer, {.message = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0"});
        return;
    }
    modify_item_inventory(event, ::slot(item_id, amount_spawn));

}
