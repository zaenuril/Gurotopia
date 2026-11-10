
#include "pch.hpp"

#include "onVariant/ConsoleMessage.hpp"
#include "givexp.hpp"

#include <sstream>
#include <algorithm>
#include <cctype>
#include <limits>
#include <cstdint>

void givexp(ENetEvent& event, std::string_view text)
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

    std::string command, target_name, extra;
    std::uint64_t amount{};

    if (!(input >> command >> target_name >> amount) || (input >> extra))
    {
        on::ConsoleMessage(event.peer, "`4Usage: /givexp <GrowID> <amount>");
        return;
    }

    if (amount == 0)
    {
        on::ConsoleMessage(event.peer, "`4XP amount must be greater than 0.");
        return;
    }

    // Case-insensitive GrowID comparison
    auto equal_growid = [](std::string_view a, std::string_view b)
    {
        if (a.size() != b.size()) return false;

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
        on::ConsoleMessage(event.peer, "`4Player not found or offline.");
        return;
    }

    ::peer *target = static_cast<::peer*>(target_enet->data);

    // Prevent XP overflow in database INT
    const std::uint64_t max_xp =
        static_cast<std::uint64_t>(std::numeric_limits<int>::max());

    if (target->level[1] > max_xp ||
        amount > max_xp - target->level[1])
    {
        on::ConsoleMessage(event.peer, "`4XP amount exceeds the supported limit.");
        return;
    }

    // Give XP to target
    ENetEvent target_event = event;
    target_event.peer = target_enet;

    target->add_xp(target_event, static_cast<std::uint32_t>(amount));
    target->mysql_update_all();

    // Confirmation
    on::ConsoleMessage(
        event.peer,
        std::format(
            "`2Gave {} XP to {}. Current level: {}",
            amount,
            target->growid,
            target->level[0]
        )
    );
}
