/*
 * turn_restriction_handler.cpp
 *
 *  Created on:  2017-08-21
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "turn_restriction_handler.hpp"

TurnRestrictionHandler::TurnRestrictionHandler(osmium::index::IdSetDense<osmium::unsigned_object_id_type>& point_node_members) :
        m_point_node_members(point_node_members) {}

void TurnRestrictionHandler::relation(const osmium::Relation& relation) {
    for (const osmium::RelationMember& member : relation.members()) {
        if (member.type() == osmium::item_type::node) {
            m_point_node_members.set(static_cast<osmium::unsigned_object_id_type>(member.ref()));
        }
    }
}
