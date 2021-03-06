/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Common.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "ObjectGuid.h"
#include "WorldPacket.h"
#include "Database/DatabaseEnv.h"
#include "ItemEnchantmentMgr.h"

void AddItemsSetItem(Player*player,Item *item)
{
    ItemPrototype const *proto = item->GetProto();
    uint32 setid = proto->ItemSet;

    ItemSetEntry const *set = sItemSetStore.LookupEntry(setid);

    if (!set)
    {
        sLog.outErrorDb("Item set %u for item (id %u) not found, mods not applied.",setid,proto->ItemId);
        return;
    }

    if ( set->required_skill_id && player->GetSkillValue(set->required_skill_id) < set->required_skill_value )
        return;

    ItemSetEffect *eff = NULL;

    for(size_t x = 0; x < player->ItemSetEff.size(); ++x)
    {
        if (player->ItemSetEff[x] && player->ItemSetEff[x]->setid == setid)
        {
            eff = player->ItemSetEff[x];
            break;
        }
    }

    if (!eff)
    {
        eff = new ItemSetEffect;
        memset(eff,0,sizeof(ItemSetEffect));
        eff->setid = setid;

        size_t x = 0;
        for(; x < player->ItemSetEff.size(); x++)
            if (!player->ItemSetEff[x])
                break;

        if (x < player->ItemSetEff.size())
            player->ItemSetEff[x]=eff;
        else
            player->ItemSetEff.push_back(eff);
    }

    ++eff->item_count;

    for(uint32 x=0;x<8;x++)
    {
        if (!set->spells [x])
            continue;
        //not enough for  spell
        if (set->items_to_triggerspell[x] > eff->item_count)
            continue;

        uint32 z=0;
        for(;z<8;z++)
            if (eff->spells[z] && eff->spells[z]->Id==set->spells[x])
                break;

        if (z < 8)
            continue;

        //new spell
        for(uint32 y=0;y<8;y++)
        {
            if (!eff->spells[y])                             // free slot
            {
                SpellEntry const *spellInfo = sSpellStore.LookupEntry(set->spells[x]);
                if (!spellInfo)
                {
                    sLog.outError("WORLD: unknown spell id %u in items set %u effects", set->spells[x],setid);
                    break;
                }

                // spell casted only if fit form requirement, in other case will casted at form change
                player->ApplyEquipSpell(spellInfo,NULL,true);
                eff->spells[y] = spellInfo;
                break;
            }
        }
    }
}

void RemoveItemsSetItem(Player*player,ItemPrototype const *proto)
{
    uint32 setid = proto->ItemSet;

    ItemSetEntry const *set = sItemSetStore.LookupEntry(setid);

    if (!set)
    {
        sLog.outErrorDb("Item set #%u for item #%u not found, mods not removed.",setid,proto->ItemId);
        return;
    }

    ItemSetEffect *eff = NULL;
    size_t setindex = 0;
    for(;setindex < player->ItemSetEff.size(); setindex++)
    {
        if (player->ItemSetEff[setindex] && player->ItemSetEff[setindex]->setid == setid)
        {
            eff = player->ItemSetEff[setindex];
            break;
        }
    }

    // can be in case now enough skill requirement for set appling but set has been appliend when skill requirement not enough
    if (!eff)
        return;

    --eff->item_count;

    for(uint32 x=0;x<8;x++)
    {
        if (!set->spells[x])
            continue;

        // enough for spell
        if (set->items_to_triggerspell[x] <= eff->item_count)
            continue;

        for(uint32 z=0;z<8;z++)
        {
            if (eff->spells[z] && eff->spells[z]->Id==set->spells[x])
            {
                // spell can be not active if not fit form requirement
                player->ApplyEquipSpell(eff->spells[z],NULL,false);
                eff->spells[z]=NULL;
                break;
            }
        }
    }

    if (!eff->item_count)                                    //all items of a set were removed
    {
        ASSERT(eff == player->ItemSetEff[setindex]);
        delete eff;
        player->ItemSetEff[setindex] = NULL;
    }
}

bool ItemCanGoIntoBag(ItemPrototype const *pProto, ItemPrototype const *pBagProto)
{
    if (!pProto || !pBagProto)
        return false;

    switch(pBagProto->Class)
    {
        case ITEM_CLASS_CONTAINER:
            switch(pBagProto->SubClass)
            {
                case ITEM_SUBCLASS_CONTAINER:
                    return true;
                case ITEM_SUBCLASS_SOUL_CONTAINER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_SOUL_SHARDS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_HERB_CONTAINER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_HERBS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_ENCHANTING_CONTAINER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_ENCHANTING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_MINING_CONTAINER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_MINING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_ENGINEERING_CONTAINER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_ENGINEERING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_GEM_CONTAINER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_GEMS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_LEATHERWORKING_CONTAINER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_LEATHERWORKING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_INSCRIPTION_CONTAINER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_INSCRIPTION_SUPP))
                        return false;
                    return true;
                default:
                    return false;
            }
        case ITEM_CLASS_QUIVER:
            switch(pBagProto->SubClass)
            {
                case ITEM_SUBCLASS_QUIVER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_ARROWS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_AMMO_POUCH:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_BULLETS))
                        return false;
                    return true;
                default:
                    return false;
            }
    }
    return false;
}

Item::Item( )
{
    m_objectType |= TYPEMASK_ITEM;
    m_objectTypeId = TYPEID_ITEM;

    m_updateFlag = UPDATEFLAG_HIGHGUID;

    m_valuesCount = ITEM_END;
    m_slot = 0;
    uState = ITEM_NEW;
    uQueuePos = -1;
    m_container = NULL;
    m_lootGenerated = false;
    mb_in_trade = false;
    m_ExtendedCostId = 0;
    m_price = 0;
    m_originalOwnerGUID = 0;
}

bool Item::Create( uint32 guidlow, uint32 itemid, Player const* owner)
{
    Object::_Create( guidlow, 0, HIGHGUID_ITEM );

    SetEntry(itemid);
    SetObjectScale(DEFAULT_OBJECT_SCALE);

    SetUInt64Value(ITEM_FIELD_OWNER, owner ? owner->GetGUID() : 0);
    SetUInt64Value(ITEM_FIELD_CONTAINED, owner ? owner->GetGUID() : 0);

    ItemPrototype const *itemProto = ObjectMgr::GetItemPrototype(itemid);
    if (!itemProto)
        return false;

    SetUInt32Value(ITEM_FIELD_STACK_COUNT, 1);
    SetUInt32Value(ITEM_FIELD_MAXDURABILITY, itemProto->MaxDurability);
    SetUInt32Value(ITEM_FIELD_DURABILITY, itemProto->MaxDurability);

    for(int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        SetSpellCharges(i,itemProto->Spells[i].SpellCharges);

    SetUInt32Value(ITEM_FIELD_FLAGS, itemProto->Flags);
    SetUInt32Value(ITEM_FIELD_DURATION, itemProto->Duration);

    return true;
}

void Item::UpdateDuration(Player* owner, uint32 diff)
{
    if (!GetUInt32Value(ITEM_FIELD_DURATION))
        return;

    //DEBUG_LOG("Item::UpdateDuration Item (Entry: %u Duration %u Diff %u)",GetEntry(),GetUInt32Value(ITEM_FIELD_DURATION),diff);

    if (GetUInt32Value(ITEM_FIELD_DURATION)<=diff)
    { 
        //Some items with duration create new item after expire
        if ((GetProto()->ExtraFlags & ITEM_EXTRA_CREATE_ITEM_ON_EXPIRE) && !loot.empty())
        {
            for(LootItemList::iterator itr = loot.items.begin(); itr != loot.items.end(); ++itr)
            {
                if (Item* Item = owner->StoreNewItemInInventorySlot((*itr).itemid, (*itr).count))
                    owner->SendNewItem(Item,(*itr).count, true, false);               
            }
        }
        owner->DestroyItem(GetBagSlot(), GetSlot(), true);
        return;
    }

    if (m_originalOwnerGUID && HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAGS_REFUNDABLE))
        if (!GetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME) || (owner->m_Played_time[0] > (GetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME) + 2*60*60)))
            m_originalOwnerGUID = 0;

    SetUInt32Value(ITEM_FIELD_DURATION, GetUInt32Value(ITEM_FIELD_DURATION) - diff);
    SetState(ITEM_CHANGED, owner);                          // save new time in database
}

void Item::SaveToDB()
{
    uint32 guid = GetGUIDLow();
    switch (uState)
    {
        case ITEM_NEW:
        {
            std::string text = m_text;
            CharacterDatabase.escape_string(text);
            CharacterDatabase.PExecute( "DELETE FROM item_instance WHERE guid = '%u'", guid );
            CharacterDatabase.PExecute( "DELETE FROM item_refund_info WHERE guid = '%u'", guid );
            std::ostringstream ss;
            ss << "INSERT INTO item_instance (guid,itemEntry,owner_guid,creatorGuid,giftCreatorGuid,count,"
               << "duration,charges,flags,enchantments,randomPropertyId,durability,playedTime,text) VALUES ("
               << guid << "," << GetEntry() << "," << GUID_LOPART(GetOwnerGUID()) << "," << GUID_LOPART(GetUInt64Value(ITEM_FIELD_CREATOR))
               << "," << GUID_LOPART(GetUInt64Value(ITEM_FIELD_GIFTCREATOR)) << "," << GetCount() << "," << GetUInt32Value(ITEM_FIELD_DURATION)
               << ",'";
            for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
                ss << GetSpellCharges(i) << ' ';

            uint32 flags = GetUInt32Value(ITEM_FIELD_FLAGS);
            while(flags > 0xFFFFFF) { flags -= 0xFFFFFF; }
            ss << "'," << flags << ",'";

            for (uint8 i = 0; i < MAX_ENCHANTMENT_SLOT; ++i)
            {
                ss << GetEnchantmentId(EnchantmentSlot(i)) << ' '
                   << GetEnchantmentDuration(EnchantmentSlot(i)) << ' '
                   << GetEnchantmentCharges(EnchantmentSlot(i)) << ' ';
            }

            ss << "'," << GetItemRandomPropertyId() << "," << GetUInt32Value(ITEM_FIELD_DURABILITY) << ","
               << GetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME) << ",'" << text << "')";
            CharacterDatabase.Execute( ss.str().c_str() );

            if (HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAGS_REFUNDABLE) && GetOriginalOwner())
            {
                CharacterDatabase.PExecute(
                    "INSERT INTO item_refund_info (guid,ExtendedCost,price,original_owner_guid) VALUES (%u, %u, %u, %u)",
                    guid, m_ExtendedCostId, m_price, m_originalOwnerGUID);
            }
        } break;
        case ITEM_CHANGED:
        {
            std::string text = m_text;
            CharacterDatabase.escape_string(text);
            std::ostringstream ss;

            ss << "UPDATE item_instance SET owner_guid = '" << GUID_LOPART(GetOwnerGUID())
               << "', creatorGuid = '" << GUID_LOPART(GetUInt64Value(ITEM_FIELD_CREATOR))
               << "', giftCreatorGuid = '" << GUID_LOPART(GetUInt64Value(ITEM_FIELD_GIFTCREATOR))
               << "', count = '" << GetCount() << "', duration = '" << GetUInt32Value(ITEM_FIELD_DURATION)
               << "', charges = '";

            for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
                ss << GetSpellCharges(i) << ' ';

            uint32 flags = GetUInt32Value(ITEM_FIELD_FLAGS);
            while(flags > 0xFFFFFF) { flags -= 0xFFFFFF; }
            ss << "', flags = '" << flags << "', enchantments = '";

            for (uint8 i = 0; i < MAX_ENCHANTMENT_SLOT; ++i)
            {
                ss << GetEnchantmentId(EnchantmentSlot(i)) << ' '
                   << GetEnchantmentDuration(EnchantmentSlot(i)) << ' '
                   << GetEnchantmentCharges(EnchantmentSlot(i)) << ' ';
            }

            ss << "', randomPropertyId = '" << GetItemRandomPropertyId() << "', durability = '"
               << GetUInt32Value(ITEM_FIELD_DURABILITY) << "', playedTime = '" << GetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME)
               << "', text = '" << text << "'" << "WHERE guid = '" << guid << "'";

            CharacterDatabase.Execute( ss.str().c_str() );

            if (HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAGS_WRAPPED))
                CharacterDatabase.PExecute("UPDATE character_gifts SET guid = '%u' WHERE item_guid = '%u'", GUID_LOPART(GetOwnerGUID()),GetGUIDLow());

            if (!HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAGS_REFUNDABLE) || GetOriginalOwner())
                CharacterDatabase.PExecute("DELETE FROM item_refund_info WHERE guid= %u",guid);
        } break;
        case ITEM_REMOVED:
        {
            CharacterDatabase.PExecute("DELETE FROM item_instance WHERE guid = '%u'", guid);
            if (HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAGS_WRAPPED))
                CharacterDatabase.PExecute("DELETE FROM character_gifts WHERE item_guid = '%u'", GetGUIDLow());
            if (HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAGS_REFUNDABLE) && GetOriginalOwner())
                CharacterDatabase.PExecute("DELETE FROM item_refund_info WHERE guid= %u",guid);
            delete this;
            return;
        }
        case ITEM_UNCHANGED:
            break;
    }
    SetState(ITEM_UNCHANGED);
}

bool Item::LoadFromDB(uint32 guid, uint64 owner_guid, uint32 itemid)
{
    // create item before any checks for store correct guid
    // and allow use "FSetState(ITEM_REMOVED); SaveToDB();" for deleting item from DB
    Object::_Create(guid, 0, HIGHGUID_ITEM);
    SetUInt32Value(OBJECT_FIELD_ENTRY, itemid);

    ItemPrototype const* proto = GetProto();
    if (!proto)
        return false;

    QueryResult *result = CharacterDatabase.PQuery(
    //          0            1                2      3         4        5      6
        "SELECT creatorGuid, giftCreatorGuid, count, duration, charges, flags, enchantments, "
    //   7                 8           9           10    11
        "randomPropertyId, durability, playedTime, text, owner_guid FROM item_instance WHERE guid = '%u'", guid);

    if (!result)
    {
        sLog.outError("Item (GUID: %u owner: %u) not found in table `item_instance`, can't load. ",guid,GUID_LOPART(owner_guid));
        return false;
    }

    

    bool need_save = false;              // need explicit save data at load fixes
    Field *fields = result->Fetch();

    SetUInt64Value(ITEM_FIELD_CREATOR, MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER));
    SetUInt64Value(ITEM_FIELD_GIFTCREATOR, MAKE_NEW_GUID(fields[1].GetUInt32(), 0, HIGHGUID_PLAYER));
    SetCount(fields[2].GetUInt32());

    uint32 duration = fields[3].GetUInt32();
    SetUInt32Value(ITEM_FIELD_DURATION, duration);
    
    // update duration if need, and remove if not need
    if ((proto->Duration == 0) != (duration == 0))
    {
        SetUInt32Value(ITEM_FIELD_DURATION, proto->Duration);
        need_save = true;
    }

    Tokens tokens = StrSplit(fields[4].GetString(), " ");
    if (tokens.size() == MAX_ITEM_PROTO_SPELLS)
    {
        Tokens::iterator iter;
        int index;
        for (iter = tokens.begin(), index = 0; index < MAX_ITEM_PROTO_SPELLS; ++iter, ++index)
        {
            SetSpellCharges(index, atol((*iter).c_str()));
        }
    }

    uint32 flags = fields[5].GetUInt32();
    if(flags == 0xFFFFFF)
    {
        flags = GetProto()->Flags;
        need_save = true;
    }
    else
        flags |= GetProto()->Flags;
    SetUInt32Value(ITEM_FIELD_FLAGS, flags);

    // Remove bind flag for items vs NO_BIND set
    if (IsSoulBound() && proto->Bonding == NO_BIND)
    {
        ApplyModFlag(ITEM_FIELD_FLAGS,ITEM_FLAGS_BINDED, false);
        need_save = true;
    }

    tokens = StrSplit(fields[6].GetString(), " ");
    if (tokens.size() == MAX_ENCHANTMENT_SLOT*3)
    {
        Tokens::iterator iter;
        int index;
        for (iter = tokens.begin(), index = 0; index < MAX_ENCHANTMENT_SLOT; ++iter, ++index)
        {
            SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + index*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_ID_OFFSET, atoi((*iter).c_str()));
            ++iter;
            SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + index*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET, atoi((*iter).c_str()));
            ++iter;
            SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + index*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET, atoi((*iter).c_str()));
        }
    }

    SetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID, fields[7].GetInt16());
    // recalculate suffix factor
    if (GetItemRandomPropertyId() < 0)
    {
        if (UpdateItemSuffixFactor())
            need_save = true;
    }

    uint32 durability = fields[8].GetUInt16();
    SetUInt32Value(ITEM_FIELD_DURABILITY, durability);
    // update max durability (and durability) if need
    SetUInt32Value(ITEM_FIELD_MAXDURABILITY,proto->MaxDurability);
    if (durability > proto->MaxDurability)
    {
        SetUInt32Value(ITEM_FIELD_DURABILITY,proto->MaxDurability);
        need_save = true;
    }

    SetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME, fields[9].GetUInt32());

    SetText(fields[10].GetCppString());

     // set correct owner
    uint64 set_owner_guid = MAKE_NEW_GUID(fields[11].GetUInt32(), 0, HIGHGUID_PLAYER);
    if (owner_guid != 0 && set_owner_guid != owner_guid)
    {
        set_owner_guid = owner_guid;
        need_save = true;
    }
    SetOwnerGUID(set_owner_guid);

    if (need_save)                                          // normal item changed state set not work at loading
    {
         std::ostringstream ss;

          ss << "UPDATE item_instance SET duration = '" << GetUInt32Value(ITEM_FIELD_DURATION)
             << "', flags = '" << GetUInt32Value(ITEM_FIELD_FLAGS) << "', randomPropertyId = '"
             << GetItemRandomPropertyId() << "', durability = '"
             << GetUInt32Value(ITEM_FIELD_DURABILITY) << "', owner_guid = '"
             << GUID_LOPART(set_owner_guid) << "'  WHERE guid = '" << guid << "'";

         CharacterDatabase.Execute( ss.str().c_str() );
    }

    delete result;

    //Set extended cost for refundable item
    if (HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAGS_REFUNDABLE))
    {
        //                                                         0             1      2
        QueryResult *result_ext = CharacterDatabase.PQuery("SELECT ExtendedCost, price, original_owner_guid FROM item_refund_info WHERE guid = '%u'", guid);
        if (result_ext)
        {
            fields = result_ext->Fetch();
            m_ExtendedCostId = fields[0].GetUInt32();
            m_price = fields[1].GetUInt32();
            m_originalOwnerGUID = fields[2].GetUInt32();
            delete result_ext;
        }
    }
    return true;
}

void Item::DeleteFromDB()
{
    CharacterDatabase.PExecute("DELETE FROM item_instance WHERE guid = '%u'",GetGUIDLow());
    CharacterDatabase.PExecute("DELETE FROM item_refund_info WHERE guid = '%u'",GetGUIDLow());
}

void Item::DeleteFromInventoryDB()
{
    CharacterDatabase.PExecute("DELETE FROM character_inventory WHERE item = '%u'",GetGUIDLow());
}

ItemPrototype const *Item::GetProto() const
{
    return ObjectMgr::GetItemPrototype(GetEntry());
}

Player* Item::GetOwner()const
{
    return sObjectMgr.GetPlayer(GetOwnerGUID());
}

int32 Item::GenerateItemRandomPropertyId(uint32 item_id)
{
    ItemPrototype const *itemProto = sItemStorage.LookupEntry<ItemPrototype>(item_id);

    if (!itemProto)
        return 0;

    // item must have one from this field values not null if it can have random enchantments
    if ((!itemProto->RandomProperty) && (!itemProto->RandomSuffix))
        return 0;

    // item can have not null only one from field values
    if ((itemProto->RandomProperty) && (itemProto->RandomSuffix))
    {
        sLog.outErrorDb("Item template %u have RandomProperty==%u and RandomSuffix==%u, but must have one from field =0",itemProto->ItemId,itemProto->RandomProperty,itemProto->RandomSuffix);
        return 0;
    }

    // RandomProperty case
    if (itemProto->RandomProperty)
    {
        uint32 randomPropId = GetItemEnchantMod(itemProto->RandomProperty);
        ItemRandomPropertiesEntry const *random_id = sItemRandomPropertiesStore.LookupEntry(randomPropId);
        if (!random_id)
        {
            sLog.outErrorDb("Enchantment id #%u used but it doesn't have records in 'ItemRandomProperties.dbc'",randomPropId);
            return 0;
        }

        return random_id->ID;
    }
    // RandomSuffix case
    else
    {
        uint32 randomPropId = GetItemEnchantMod(itemProto->RandomSuffix);
        ItemRandomSuffixEntry const *random_id = sItemRandomSuffixStore.LookupEntry(randomPropId);
        if (!random_id)
        {
            sLog.outErrorDb("Enchantment id #%u used but it doesn't have records in sItemRandomSuffixStore.",randomPropId);
            return 0;
        }

        return -int32(random_id->ID);
    }
}

void Item::SetItemRandomProperties(int32 randomPropId)
{
    if (!randomPropId)
        return;

    if (randomPropId > 0)
    {
        ItemRandomPropertiesEntry const *item_rand = sItemRandomPropertiesStore.LookupEntry(randomPropId);
        if (item_rand)
        {
            if (GetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID) != int32(item_rand->ID))
            {
                SetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID,item_rand->ID);
                SetState(ITEM_CHANGED);
            }
            for(uint32 i = PROP_ENCHANTMENT_SLOT_2; i < PROP_ENCHANTMENT_SLOT_2 + 3; ++i)
                SetEnchantment(EnchantmentSlot(i),item_rand->enchant_id[i - PROP_ENCHANTMENT_SLOT_2],0,0);
        }
    }
    else
    {
        ItemRandomSuffixEntry const *item_rand = sItemRandomSuffixStore.LookupEntry(-randomPropId);
        if (item_rand)
        {
            if ( GetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID) != -int32(item_rand->ID) ||
                !GetItemSuffixFactor())
            {
                SetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID,-int32(item_rand->ID));
                UpdateItemSuffixFactor();
                SetState(ITEM_CHANGED);
            }

            for(uint32 i = PROP_ENCHANTMENT_SLOT_0; i < PROP_ENCHANTMENT_SLOT_0 + 3; ++i)
                SetEnchantment(EnchantmentSlot(i),item_rand->enchant_id[i - PROP_ENCHANTMENT_SLOT_0],0,0);
        }
    }
}

bool Item::UpdateItemSuffixFactor()
{
    uint32 suffixFactor = GenerateEnchSuffixFactor(GetEntry());
    if (GetItemSuffixFactor()==suffixFactor)
        return false;
    SetUInt32Value(ITEM_FIELD_PROPERTY_SEED,suffixFactor);
    return true;
}

void Item::SetState(ItemUpdateState state, Player *forplayer)
{
    if (uState == ITEM_NEW && state == ITEM_REMOVED)
    {
        // pretend the item never existed
        RemoveFromUpdateQueueOf(forplayer);
        delete this;
        return;
    }

    if (state != ITEM_UNCHANGED)
    {
        // new items must stay in new state until saved
        if (uState != ITEM_NEW) uState = state;
        AddToUpdateQueueOf(forplayer);
    }
    else
    {
        // unset in queue
        // the item must be removed from the queue manually
        uQueuePos = -1;
        uState = ITEM_UNCHANGED;
    }
}

void Item::AddToUpdateQueueOf(Player *player)
{
    if (IsInUpdateQueue()) return;

    if (!player)
    {
        player = GetOwner();
        if (!player)
        {
            sLog.outError("Item::AddToUpdateQueueOf - GetPlayer didn't find a player matching owner's guid (%u)!", GUID_LOPART(GetOwnerGUID()));
            return;
        }
    }

    if (player->GetGUID() != GetOwnerGUID())
    {
        sLog.outError("Item::AddToUpdateQueueOf - Owner's guid (%u) and player's guid (%u) don't match!", GUID_LOPART(GetOwnerGUID()), player->GetGUIDLow());
        return;
    }

    if (player->m_itemUpdateQueueBlocked) return;

    player->m_itemUpdateQueue.push_back(this);
    uQueuePos = player->m_itemUpdateQueue.size()-1;
}

void Item::RemoveFromUpdateQueueOf(Player *player)
{
    if (!IsInUpdateQueue()) return;

    if (!player)
    {
        player = GetOwner();
        if (!player)
        {
            sLog.outError("Item::RemoveFromUpdateQueueOf - GetPlayer didn't find a player matching owner's guid (%u)!", GUID_LOPART(GetOwnerGUID()));
            return;
        }
    }

    if (player->GetGUID() != GetOwnerGUID())
    {
        sLog.outError("Item::RemoveFromUpdateQueueOf - Owner's guid (%u) and player's guid (%u) don't match!", GUID_LOPART(GetOwnerGUID()), player->GetGUIDLow());
        return;
    }

    if (player->m_itemUpdateQueueBlocked) return;

    player->m_itemUpdateQueue[uQueuePos] = NULL;
    uQueuePos = -1;
}

uint8 Item::GetBagSlot() const
{
    return m_container ? m_container->GetSlot() : uint8(INVENTORY_SLOT_BAG_0);
}

bool Item::IsEquipped() const
{
    return !IsInBag() && m_slot < EQUIPMENT_SLOT_END;
}

bool Item::CanBeTraded(bool mail) const
{
    if (m_lootGenerated)
        return false;

    if ((!mail || !IsBoundAccountWide()) && IsSoulBound())
        return false;

    if (IsBag() && (Player::IsBagPos(GetPos()) || !((Bag const*)this)->IsEmpty()) )
        return false;

    if (Player* owner = GetOwner())
    {
        if (owner->CanUnequipItem(GetPos(),false) !=  EQUIP_ERR_OK )
            return false;
        if (owner->GetLootGUID()==GetGUID())
            return false;
    }

    if (IsBoundByEnchant())
        return false;

    return true;
}

bool Item::IsBoundByEnchant() const
{
    // Check all enchants for soulbound
    for(uint32 enchant_slot = PERM_ENCHANTMENT_SLOT; enchant_slot < MAX_ENCHANTMENT_SLOT; ++enchant_slot)
    {
        uint32 enchant_id = GetEnchantmentId(EnchantmentSlot(enchant_slot));
        if (!enchant_id)
            continue;

        SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if (!enchantEntry)
            continue;

        if (enchantEntry->slot & ENCHANTMENT_CAN_SOULBOUND)
            return true;
    }
    return false;
}

bool Item::IsFitToSpellRequirements(SpellEntry const* spellInfo) const
{
    ItemPrototype const* proto = GetProto();

    // Enchant spells only use Effect[0] (patch 3.3.2)
    if (proto->IsVellum() && spellInfo->Effect[EFFECT_INDEX_0] == SPELL_EFFECT_ENCHANT_ITEM)
    {
        // EffectItemType[0] is the associated scroll itemID, if a scroll can be made
        if (spellInfo->EffectItemType[EFFECT_INDEX_0] == 0)
            return false;
        // Other checks do not apply to vellum enchants, so return final result
        return ((proto->SubClass == ITEM_SUBCLASS_WEAPON_ENCHANTMENT && spellInfo->EquippedItemClass == ITEM_CLASS_WEAPON) ||
                (proto->SubClass == ITEM_SUBCLASS_ARMOR_ENCHANTMENT && spellInfo->EquippedItemClass == ITEM_CLASS_ARMOR));
    }

    if (spellInfo->EquippedItemClass != -1)                 // -1 == any item class
    {
        if (spellInfo->EquippedItemClass != int32(proto->Class))
            return false;                                   //  wrong item class

        if (spellInfo->EquippedItemSubClassMask != 0)        // 0 == any subclass
        {
            if ((spellInfo->EquippedItemSubClassMask & (1 << proto->SubClass)) == 0)
                return false;                               // subclass not present in mask
        }
    }

    // Only check for item enchantments (TARGET_FLAG_ITEM), all other spells are either NPC spells
    // or spells where slot requirements are already handled with AttributesEx3 fields
    // and special code (Titan's Grip, Windfury Attack). Check clearly not applicable for Lava Lash.
    if (spellInfo->EquippedItemInventoryTypeMask != 0 && (spellInfo->Targets & TARGET_FLAG_ITEM))    // 0 == any inventory type
    {
        if ((spellInfo->EquippedItemInventoryTypeMask  & (1 << proto->InventoryType)) == 0)
            return false;                                   // inventory type not present in mask
    }

    return true;
}

bool Item::IsTargetValidForItemUse(Unit* pUnitTarget)
{
    ItemRequiredTargetMapBounds bounds = sObjectMgr.GetItemRequiredTargetMapBounds(GetProto()->ItemId);

    if (bounds.first == bounds.second)
        return true;

    if (!pUnitTarget)
        return false;

    for(ItemRequiredTargetMap::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
        if (itr->second.IsFitToRequirements(pUnitTarget))
            return true;

    return false;
}

void Item::SetEnchantment(EnchantmentSlot slot, uint32 id, uint32 duration, uint32 charges)
{
    // Better lost small time at check in comparison lost time at item save to DB.
    if ((GetEnchantmentId(slot) == id) && (GetEnchantmentDuration(slot) == duration) && (GetEnchantmentCharges(slot) == charges))
        return;

    SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_ID_OFFSET,id);
    SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET,duration);
    SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET,charges);
    SetState(ITEM_CHANGED);
}

void Item::SetEnchantmentDuration(EnchantmentSlot slot, uint32 duration)
{
    if (GetEnchantmentDuration(slot) == duration)
        return;

    SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET,duration);
    SetState(ITEM_CHANGED);
}

void Item::SetEnchantmentCharges(EnchantmentSlot slot, uint32 charges)
{
    if (GetEnchantmentCharges(slot) == charges)
        return;

    SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET,charges);
    SetState(ITEM_CHANGED);
}

void Item::ClearEnchantment(EnchantmentSlot slot)
{
    if (!GetEnchantmentId(slot))
        return;

    for(uint8 x = 0; x < 3; ++x)
        SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + x, 0);
    SetState(ITEM_CHANGED);
}

bool Item::GemsFitSockets() const
{
    bool fits = true;
    for(uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+MAX_GEM_SOCKETS; ++enchant_slot)
    {
        uint8 SocketColor = GetProto()->Socket[enchant_slot-SOCK_ENCHANTMENT_SLOT].Color;

        uint32 enchant_id = GetEnchantmentId(EnchantmentSlot(enchant_slot));
        if (!enchant_id)
        {
            if (SocketColor) fits &= false;
            continue;
        }

        SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if (!enchantEntry)
        {
            if (SocketColor) fits &= false;
            continue;
        }

        uint8 GemColor = 0;

        uint32 gemid = enchantEntry->GemID;
        if (gemid)
        {
            ItemPrototype const* gemProto = sItemStorage.LookupEntry<ItemPrototype>(gemid);
            if (gemProto)
            {
                GemPropertiesEntry const* gemProperty = sGemPropertiesStore.LookupEntry(gemProto->GemProperties);
                if (gemProperty)
                    GemColor = gemProperty->color;
            }
        }

        SocketColor = SocketColor ? SocketColor : PRISMATIC_SOCKET; 
        fits &= (GemColor & SocketColor) ? true : false;
    }
    return fits;
}

uint8 Item::GetGemCountWithID(uint32 GemID) const
{
    uint8 count = 0;
    for(uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+MAX_GEM_SOCKETS; ++enchant_slot)
    {
        uint32 enchant_id = GetEnchantmentId(EnchantmentSlot(enchant_slot));
        if (!enchant_id)
            continue;

        SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if (!enchantEntry)
            continue;

        if (GemID == enchantEntry->GemID)
            ++count;
    }
    return count;
}

uint8 Item::GetGemCountWithLimitCategory(uint32 limitCategory) const
{
    uint8 count = 0;
    for(uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+MAX_GEM_SOCKETS; ++enchant_slot)
    {
        uint32 enchant_id = GetEnchantmentId(EnchantmentSlot(enchant_slot));
        if (!enchant_id)
            continue;

        SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if (!enchantEntry)
            continue;

        ItemPrototype const* gemProto = ObjectMgr::GetItemPrototype(enchantEntry->GemID);
        if (!gemProto)
            continue;

        if (gemProto->ItemLimitCategory==limitCategory)
            ++count;
    }
    return count;
}

bool Item::IsLimitedToAnotherMapOrZone( uint32 cur_mapId, uint32 cur_zoneId) const
{
    ItemPrototype const* proto = GetProto();
    return proto && (proto->Map && proto->Map != cur_mapId || proto->Area && proto->Area != cur_zoneId );
}

// Though the client has the information in the item's data field,
// we have to send SMSG_ITEM_TIME_UPDATE to display the remaining
// time.
void Item::SendTimeUpdate(Player* owner)
{
    if (!GetUInt32Value(ITEM_FIELD_DURATION))
        return;

    WorldPacket data(SMSG_ITEM_TIME_UPDATE, (8+4));
    data << (uint64)GetGUID();
    data << (uint32)GetUInt32Value(ITEM_FIELD_DURATION);
    owner->GetSession()->SendPacket(&data);
}

Item* Item::CreateItem( uint32 item, uint32 count, Player const* player )
{
    if ( count < 1 )
        return NULL;                                        //don't create item at zero count

    ItemPrototype const *pProto = ObjectMgr::GetItemPrototype( item );
    if ( pProto )
    {
        if ( count > pProto->GetMaxStackSize())
            count = pProto->GetMaxStackSize();

        ASSERT(count !=0 && "pProto->Stackable==0 but checked at loading already");

        Item *pItem = NewItemOrBag( pProto );
        if ( pItem->Create(sObjectMgr.GenerateLowGuid(HIGHGUID_ITEM), item, player) )
        {
            pItem->SetCount( count );
            return pItem;
        }
        else
            delete pItem;
    }
    return NULL;
}

Item* Item::CloneItem( uint32 count, Player const* player ) const
{
    Item* newItem = CreateItem( GetEntry(), count, player );
    if (!newItem)
        return NULL;

    newItem->SetGuidValue(ITEM_FIELD_CREATOR,     GetGuidValue(ITEM_FIELD_CREATOR));
    newItem->SetGuidValue(ITEM_FIELD_GIFTCREATOR, GetGuidValue(ITEM_FIELD_GIFTCREATOR));
    newItem->SetUInt32Value(ITEM_FIELD_FLAGS,     GetUInt32Value(ITEM_FIELD_FLAGS));
    newItem->SetUInt32Value(ITEM_FIELD_DURATION,  GetUInt32Value(ITEM_FIELD_DURATION));
    newItem->SetItemRandomProperties(GetItemRandomPropertyId());
    return newItem;
}

bool Item::IsBindedNotWith( Player const* player ) const
{
    // not binded item
    if (!IsSoulBound())
        return false;

    // own item
    if (GetOwnerGUID()== player->GetGUID())
        return false;

    // not BOA item case
    if (!IsBoundAccountWide())
        return true;

    // online
    if (Player* owner = sObjectMgr.GetPlayer(GetOwnerGUID()))
    {
        return owner->GetSession()->GetAccountId() != player->GetSession()->GetAccountId();
    }
    // offline slow case
    else
    {
        return sObjectMgr.GetPlayerAccountIdByGUID(GetOwnerGUID()) != player->GetSession()->GetAccountId();
    }
}

void Item::AddToClientUpdateList()
{
    if (Player* pl = GetOwner())
        pl->GetMap()->AddUpdateObject(this);
}

void Item::RemoveFromClientUpdateList()
{
    if (Player* pl = GetOwner())
        pl->GetMap()->RemoveUpdateObject(this);
}

void Item::BuildUpdateData(UpdateDataMapType& update_players)
{
    if (Player* pl = GetOwner())
        BuildUpdateDataForPlayer(pl, update_players);

    ClearUpdateMask(false);
}

uint8 Item::CanBeMergedPartlyWith( ItemPrototype const* proto ) const
{
    // check item type
    if (GetEntry() != proto->ItemId)
        return EQUIP_ERR_ITEM_CANT_STACK;

    // check free space (full stacks can't be target of merge
    if (GetCount() >= proto->GetMaxStackSize())
        return EQUIP_ERR_ITEM_CANT_STACK;

    // not allow merge looting currently items
    if (m_lootGenerated)
        return EQUIP_ERR_ALREADY_LOOTED;

    return EQUIP_ERR_OK;
}

bool ItemRequiredTarget::IsFitToRequirements( Unit* pUnitTarget ) const
{
    if (pUnitTarget->GetTypeId() != TYPEID_UNIT)
        return false;

    if (pUnitTarget->GetEntry() != m_uiTargetEntry)
        return false;

    switch(m_uiType)
    {
        case ITEM_TARGET_TYPE_CREATURE:
            return pUnitTarget->isAlive();
        case ITEM_TARGET_TYPE_DEAD:
            return !pUnitTarget->isAlive();
        default:
            return false;
    }
}

bool Item::HasMaxCharges() const
{
    ItemPrototype const* itemProto = GetProto();

    for(int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        if (GetSpellCharges(i) != itemProto->Spells[i].SpellCharges)
            return false;

    return true;
}

void Item::RestoreCharges()
{
    ItemPrototype const* itemProto = GetProto();

    for(int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
    {
        if (GetSpellCharges(i) != itemProto->Spells[i].SpellCharges)
        {
            SetSpellCharges(i, itemProto->Spells[i].SpellCharges);
            SetState(ITEM_CHANGED);
        }
    }
}
