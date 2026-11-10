#include "pch.hpp"

#include "onVariant/ConsoleMessage.hpp"

#include "setlevel.hpp"

#include <sstream>
#include <algorithm>
#include <cctype>
#include <limits>

void setlevel(ENetEvent& event, std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    // Developer only
    if (pPeer->role != DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4You don't have permission.");
        return;
    }

    // Parse arguments
    std::istringstream input{std::string(text)};

    std::string command;
    std::string target_name;
    std::string extra;
    unsigned int requested_level{};

    if (!(input >> command >> target_name >> requested_level) ||
        (input >> extra))
    {
        on::ConsoleMessage(
            event.peer,
            "Usage: /setlevel <name> <level>"
        );
        return;
    }

    if (requested_level < 1 || requested_level > 125)
    {
        on::ConsoleMessage(
            event.peer,
            "`4Level must be between 1 and 125."
        );
        return;
    }

    // Case-insensitive GrowID comparison
    auto equal_growid = [](std::string_view a, std::string_view b)
    {
        if (a.size() != b.size())
            return false;

        return std::equal(
            a.begin(), a.end(), b.begin(),
            [](unsigned char x, unsigned char y)
            {
                return std::toupper(x) == std::toupper(y);
            }
        );
    };

    // Find online player
    ENetPeer *target_enet{};

    peers("", PeerSearch::EVERYONE, [&](ENetPeer *peer)
    {
        ::peer *target = static_cast<::peer*>(peer->data);

        if (target && equal_growid(target->growid, target_name))
            target_enet = peer;
    });

    if (!target_enet)
    {
        on::ConsoleMessage(
            event.peer,
            "`4Player not found or offline."
        );
        return;
    }

    ::peer *target = static_cast<::peer*>(target_enet->data);

    if (requested_level <= target->level[0])
    {
        on::ConsoleMessage(
            event.peer,
            "`4Target must have a lower level than the requested level."
        );
        return;
    }

    // Calculate XP required for each level
    std::uint64_t xp_to_add{};

    for (std::uint32_t lvl = target->level[0];
         lvl < requested_level;
         ++lvl)
    {
        xp_to_add += 50ull * (lvl * lvl + 2ull);
    }

    if (xp_to_add >
        std::numeric_limits<std::uint32_t>::max() - target->level[1])
    {
        on::ConsoleMessage(
            event.peer,
            "`4XP calculation overflow."
        );
        return;
    }

    // Use target's ENetEvent so rewards and notifications
    // are sent to the correct player.
    ENetEvent target_event = event;
    target_event.peer = target_enet;

    target->add_xp(
        target_event,
        static_cast<std::uint32_t>(xp_to_add)
    );

    // Save target data
    target->mysql_update_all();

    on::ConsoleMessage(
        event.peer,
        std::format(
            "`2Successfully set {} to level {}. Added {} XP.",
            target->growid,
            target->level[0],
            xp_to_add
        )
    );
}