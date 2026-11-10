
#include "pch.hpp"

#include "onVariant/SetBux.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "commands/spawn_queue.hpp"

#include "item_activate_object.hpp"

void item_activate(ENetEvent& event, ::gamePacket gamePacket)
{
    ::peer* pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->recent_worlds.empty())
        return;

    auto world = std::ranges::find(
        worlds,
        pPeer->recent_worlds.back(),
        &::world::name
    );

    if (world == worlds.end())
        return;

    auto object = std::ranges::find(
        world->objects,
        gamePacket.id,
        &::object::uid
    );

    if (object == world->objects.end())
        return;

    const bool spawned = spawn_queue::is_spawned(
        world->name,
        object->uid
    );

    // Item hasil /spawn hanya boleh diambil developer.
    if (spawned && pPeer->role != DEVELOPER)
    {
        on::ConsoleMessage(
            event.peer,
            "`4This item can only be collected by developers."
        );
        return;
    }

    u_short left = 0;

    if (object->id != 112)
    {
        const ::item& item = id_to_item(object->id);

        const u_short remember_count = object->count;

        left = pPeer->emplace(
            ::slot(object->id, remember_count)
        );

        const u_short collected = remember_count - left;

        if (collected == 0)
            return;

        on::ConsoleMessage(
            event.peer,
            (item.rarity >= 999)
                ? std::format(
                    "Collected `w{} {}``.",
                    collected,
                    item.raw_name
                )
                : std::format(
                    "Collected `w{} {}``. Rarity: `w{}``",
                    collected,
                    item.raw_name,
                    item.rarity
                )
        );
    }
    else
    {
        pPeer->gems += object->count;
        pPeer->mysql_update("gems", pPeer->gems);
        on::SetBux(event);
    }

    remove_object(event, object->uid);

    const ::object remember_object = *object;

    world->objects.erase(object);

    // Hapus tracking UID lama.
    if (spawned)
    {
        spawn_queue::forget(
            world->name,
            remember_object.uid
        );
    }

    if (left > 0)
    {
        if (spawned)
        {
            // Buat object baru tanpa merge dengan item normal.
            ::object& remainder = world->objects.emplace_back(
                remember_object.id,
                left,
                remember_object.pos,
                ++world->last_object_uid
            );

            spawn_queue::mark(
                world->name,
                remainder.uid
            );

            item_change_object(
                event,
                ::gamePacket{
                    .netid = static_cast<int>(0xffffffffu),
                    .uid = static_cast<int>(remainder.uid),
                    .count = static_cast<float>(left),
                    .id = remainder.id,
                    .pos = remainder.pos
                }
            );
        }
        else
        {
            // Perilaku item normal tetap seperti sebelumnya.
            add_object(
                event,
                ::slot(remember_object.id, left),
                remember_object.pos,
                *world
            );
        }
    }
}
