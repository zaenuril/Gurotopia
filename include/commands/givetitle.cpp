
#include "pch.hpp"
#include "givetitle.hpp"
#include "onVariant/ConsoleMessage.hpp"

#include <sstream>
#include <unordered_map>
#include <cctype>

namespace
{
    const std::unordered_map<std::string, int> title_map
    {
        {"legendary", TITLE_LEGENDARY},
        {"supersupporter", TITLE_SUPER_SUPPORTER},
        {"g4g", TITLE_GROW4GOOD},
        {"rainbow", TITLE_RAINBOW}
    };

    void update_title_display(ENetPeer* target_peer, ::peer* target)
    {
        if (target->netid == 0)
            return;

        peers(target->recent_worlds.back(), PeerSearch::WORLD,
            [&](ENetPeer* receiver)
            {
                send_varlist(receiver, {
                    "OnNameChanged",
                    target->display_growid
                }, target->netid);

                std::string country = target->country;

                if (target->level[0] == 125)
                    country += "|maxLevel";

                if (target->title_flags & TITLE_GROW4GOOD)
                    country += "|donor";

                send_varlist(receiver, {
                    "OnCountryState",
                    country
                }, target->netid);
            });
    }
}

void givetitle(ENetEvent& event, std::string_view text)
{
    ::peer* admin = static_cast<::peer*>(event.peer->data);

    if (admin->role < MODERATOR)
        return;

    std::istringstream args{std::string(text)};

    std::string command, growid, title;
    args >> command >> growid >> title;

    if (growid.empty() || title.empty())
    {
        on::ConsoleMessage(event.peer,
            "Usage: /givetitle <name> <title>");
        return;
    }

    for (char& c : title)
        c = static_cast<char>(
            std::tolower(static_cast<unsigned char>(c)));

    auto it = title_map.find(title);

    if (it == title_map.end())
    {
        on::ConsoleMessage(event.peer,
            "Unknown title. Use legendary, supersupporter, g4g, rainbow.");
        return;
    }

    bool found = false;

    peers("", PeerSearch::EVERYONE,
        [&](ENetPeer* target_peer)
        {
            auto* target = static_cast<::peer*>(target_peer->data);

            if (target->growid != growid)
                return;

            found = true;

            if (target->title_flags & it->second)
            {
                on::ConsoleMessage(event.peer,
                    "Player already has this title.");
                return;
            }

            target->title_flags |= it->second;

            target->mysql_update("title_flags", target->title_flags);

            target->apply_title();
            update_title_display(target_peer, target);

            on::ConsoleMessage(event.peer,
                std::format(
                    "The {} title has been given to {}.",
                    title, growid
                ));

            on::ConsoleMessage(target_peer,
                std::format(
                    "You have received the {} title.",
                    title
                ));
        });

    if (!found)
    {
        on::ConsoleMessage(event.peer,
            "Player not found or is possibly offline.");
    }
}
