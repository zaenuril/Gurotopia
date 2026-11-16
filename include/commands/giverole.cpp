#include "pch.hpp"

#include "giverole.hpp"
#include "onVariant/ConsoleMessage.hpp"

#include <sstream>
#include <algorithm>
#include <cctype>

void giverole(ENetEvent& event, const std::string_view text)
{
    ::peer* pPeer = static_cast<::peer*>(event.peer->data);

    // Developer only
    if (pPeer->role != DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4You don't have permission to use this command.");
        return;
    }

    std::istringstream args{ std::string(text.substr(10)) };

    std::string growid;
    std::string role_name;
    std::string extra;

    if (!(args >> growid >> role_name) || (args >> extra))
    {
        on::ConsoleMessage(event.peer, "`4Usage: /giverole <growid> <role>");
        send_gameMessage(event.peer, {.message = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0"});

        return;
    }

    std::ranges::transform(role_name, role_name.begin(),
        [](unsigned char c) { return std::toupper(c); });

    ::role new_role;

    if (role_name == "PLAYER")
        new_role = PLAYER;
    else if (role_name == "MODERATOR")
        new_role = MODERATOR;
    else if (role_name == "DEVELOPER")
        new_role = DEVELOPER;
    else
    {
        on::ConsoleMessage(event.peer, "`4Invalid role. Use `wPLAYER, `wMODERATOR, or `wDEVELOPER.");
        send_gameMessage(event.peer, {.message = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0"});

        return;
    }

    // Find account
    ::peer target{};

    if (!target.exists(growid))
    {
        on::ConsoleMessage(event.peer, "`4Player not found.");
        send_gameMessage(event.peer, {.message = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0"});

        return;
    }

    ::hStmt hStmt{
        "UPDATE peer SET role = ?, role_notice = 1 WHERE growid = ?"
    };

    signed role_value = static_cast<signed>(new_role);

    MYSQL_BIND params[2] = {
        make_bind_in(role_value),
        make_bind_in(growid)
    };

    hStmt.bind_param(params);
    hStmt.execute();

    // Update online player
    bool online = false;

    peers("", PeerSearch::EVERYONE, [&](ENetPeer* peer)
    {
        ::peer* other = static_cast<::peer*>(peer->data);

        if (other->growid == growid)
        {
            other->role = new_role;
            online = true;
        }
    });

    const std::string message = std::format(
        "`2Successfully changed `w{}`` role to `w{}``.",
        growid,
        role_name
    );

    send_gameMessage(event.peer, {.message = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0"});

    send_varlist(event.peer, {
        "OnAddNotification",
        "interface/atomic_button.rttex",
        message,
        "audio/hub_open.wav",
        0
    });

    on::ConsoleMessage(event.peer, message);

    if (online)
    {
        peers("", PeerSearch::EVERYONE, [&](ENetPeer* peer)
        {
            ::peer* other = static_cast<::peer*>(peer->data);

            if (other->growid == growid)
                on::ConsoleMessage(peer, "`2Your role has been updated.");
            send_gameMessage(event.peer, {.message = "action|play_sfx\nfile|audio/beep.wav\ndelayMS|0"});

            send_varlist(event.peer, {
                "OnAddNotification",
                "interface/atomic_button.rttex",
                message,
                "audio/hub_open.wav",
                0
            });

        });

        on::ConsoleMessage(event.peer, message);

    }
}
