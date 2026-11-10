#include "pch.hpp"

#include "CountryState.hpp"

void on::CountryState(ENetEvent& event)
{
    ::peer* pPeer = static_cast<::peer*>(event.peer->data);

    std::string country = pPeer->country;

    if (pPeer->level[0] == 125)
        country += "|maxLevel";

    if (pPeer->title_flags & TITLE_GROW4GOOD)
        country += "|donor";

    send_varlist(event.peer, {
        "OnCountryState",
        country
    }, pPeer->netid);
}
