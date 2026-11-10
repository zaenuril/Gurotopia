#include "pch.hpp"

#include "onVariant/RequestWorldSelectMenu.hpp"
#include "onVariant/ConsoleMessage.hpp"

#include "quit_to_exit.hpp"

 void action::quit_to_exit( ENetEvent& event, const std::string& header, bool skip_selection, bool keep_world)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->netid == 0 || pPeer->recent_worlds.empty())
        return;

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return; // @note peer was not in a world, therefore nothing to exit from.

    std::string message = std::format("`5<{} left, `w{}`` others here>``", pPeer->display_growid, world->visitors-1);
    std::string netid = std::format("netID|{}\n", pPeer->netid);
    std::string pId = std::format("pId|{}\n", pPeer->user_id); // @note this is found during OnSpawn 'eid', the value is the same for user_id.
    peers(pPeer->recent_worlds.back(), PeerSearch::WORLD, [&event, pPeer, message, netid, pId](ENetPeer *peer)
    {
        ::peer *pOthers = static_cast<::peer*>(peer->data);
        if (pOthers->user_id == pPeer->user_id) return;

        on::ConsoleMessage(event.peer, message);
        send_varlist(peer, { "OnRemove", netid, pId }); // @todo
    });

    if (--world->visitors <= 0 && !keep_world)
        worlds.erase(world);
    pPeer->display_growid = std::format("`w{}``", pPeer->growid);
    pPeer->apply_title();
    if (!skip_selection) on::RequestWorldSelectMenu(event);
}
