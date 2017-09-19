/*
 * route_manager.cpp
 *
 *  Created on:  2017-08-23
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include <osmium/osm/item_type.hpp>

#include "route_manager.hpp"


RouteManager::RouteManager(gdalcpp::Dataset& dataset, std::string& output_format,
    osmium::util::VerboseOutput& verbose_output, int epsg /*= 3857*/) :
        m_writer(dataset, output_format, verbose_output, epsg),
        m_checker(m_writer) { }

bool RouteManager::new_relation(const osmium::Relation& relation) const noexcept {
    const char* type = relation.get_value_by_key("type");
    if (!type || strcmp(type, "route")) {
        return false;
    }
    const char* route = relation.get_value_by_key("route");
    if (!route) {
        return false;
    }
    if (!strcmp(route, "train") || !strcmp(route, "light_rail") || !strcmp(route, "subway")
            || !strcmp(route, "tram") || !strcmp(route, "bus") || !strcmp(route, "ferry")
            || !strcmp(route, "trolleybus") || !strcmp(route, "share_taxi")) {
        return is_ptv2(relation);
    }
    return false;
}

void RouteManager::complete_relation(const osmium::Relation& relation) {
    process_route(relation);
}

void RouteManager::process_route(const osmium::Relation& relation) {
    std::vector<const osmium::OSMObject*> member_objects;
    std::vector<const char*> roles;
    member_objects.reserve(relation.members().size());
    roles.reserve(relation.members().size());
    for (const osmium::RelationMember& member : relation.members()) {
        member_objects.push_back(this->get_member_object(member));
        roles.push_back(member.role());
    }
    if (is_ptv2(relation)) {
        RouteError validation_result = is_valid(relation, member_objects, roles);
        if (validation_result == RouteError::CLEAN) {
            m_writer.write_valid_route(relation, member_objects, roles);
            return;
        }
        m_writer.write_invalid_route(relation, member_objects, roles, validation_result);
    }
}

bool RouteManager::is_ptv2(const osmium::Relation& relation) const noexcept {
    // check if it is a PTv2 route
    const char* ptv2 = relation.get_value_by_key("public_transport:version");
    if (!ptv2 || strcmp(ptv2, "2")) {
        return false;
    }
    return true;
}

RouteError RouteManager::is_valid(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
        std::vector<const char*>& roles) {
    RouteError result = RouteError::CLEAN;
    result |= m_checker.check_roles_order_and_type(relation, member_objects);
    result |= find_gaps(relation, member_objects, roles);
    return result;
}

RouteError RouteManager::find_gaps(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
        std::vector<const char*>& roles) {
    MemberStatus status = MemberStatus::BEFORE_FIRST;
    const osmium::NodeRef* next_node;
    RouteError result = RouteError::CLEAN;
    for (size_t i = 0; i < relation.members().size(); ++i) {
        const osmium::OSMObject* member = member_objects.at(i);
        const char* role = roles.at(i);
        if (member == nullptr && status == MemberStatus::BEFORE_FIRST) {
            // We can ignore missing members if we are still in the stop/platform section
            continue;
        } else if (member == nullptr) {
            status = MemberStatus::AFTER_MISSING;
            continue;
        }
        // check role and type
        if (status == MemberStatus::BEFORE_FIRST && member->type() == osmium::item_type::way && !strcmp(role, "")) {
            status = MemberStatus::FIRST;
        }
        if (status == MemberStatus::BEFORE_FIRST) {
            // All further checks are only done for the members after the list of stops/platforms.
            continue;
        }

        // check if it is a way
        if (member->type() != osmium::item_type::way || strcmp(role, "")) {
            status = MemberStatus::AFTER_GAP;
            continue;
        }

        // check if it is a roundabout
        const osmium::Way* way = static_cast<const osmium::Way*>(member);
        if (status == MemberStatus::AFTER_GAP) {
            // write this way member as error
            m_writer.write_error_way(relation, 0, "gap", way);
        }
        if (way->tags().has_tag("junction", "roundabout") && way->nodes().ends_have_same_id()) {
            if (status == MemberStatus::AFTER_ROUNDABOUT) {
                // roundabout after another roundabout, this is an impossible geometry and shoud be fixed
                m_writer.write_error_way(relation, 0, "roundabout after roundabout", way);
                result |= RouteError::UNORDERED_GAP;
                continue;
            }
            if (status == MemberStatus::FIRST || status == MemberStatus::AFTER_GAP) {
                status = MemberStatus::AFTER_ROUNDABOUT;
                continue;
            }
            if (status == MemberStatus::SECOND) {
                status = MemberStatus::SECOND_ROUNDABOUT;
            } else {
                status = MemberStatus::ROUNDABOUT;
            }
        }
        if (status == MemberStatus::FIRST || status == MemberStatus::AFTER_MISSING) {
            status = MemberStatus::SECOND;
            continue;
        }
        if (status == MemberStatus::AFTER_GAP) {
            status = MemberStatus::SECOND;
            continue;
        }

        // Now the real comparisons begin.
        assert(member_objects.at(i - 1));
        const osmium::Way* previous_way = static_cast<const osmium::Way*>(member_objects.at(i - 1));
        if (status == MemberStatus::AFTER_ROUNDABOUT) {
            next_node = m_checker.roundabout_connected_to_next_way(previous_way, way);
            if (next_node == nullptr) {
                m_writer.write_error_way(relation, 0, "gap", way);
                status = MemberStatus::AFTER_GAP;
                result |= RouteError::UNORDERED_GAP;
            } else {
                status = MemberStatus::NORMAL;
            }
        } else if (status == MemberStatus::ROUNDABOUT) {
            // check if any of the nodes of the roundabout matches the beginning or end node
            if (!m_checker.roundabout_connected_to_previous_way(next_node, way)) {
                m_writer.write_error_way(relation, 0, "gap", way);
                m_writer.write_error_point(relation, next_node, "gap at this location", way->id());
                result |= RouteError::UNORDERED_GAP;
            }
            status = MemberStatus::AFTER_ROUNDABOUT;
        } else if (status == MemberStatus::SECOND_ROUNDABOUT) {
            if (!m_checker.roundabout_as_second_after_gap(previous_way, way)) {
                m_writer.write_error_way(relation, 0, "gap", way);
                result |= RouteError::UNORDERED_GAP;
            }
            status = MemberStatus::AFTER_ROUNDABOUT;
        }
        else if (status == MemberStatus::SECOND) {
            //TODO secure for incomplete relations
            if (way->nodes().front().ref() == previous_way->nodes().front().ref()
                    || way->nodes().front().ref() == previous_way->nodes().back().ref()) {
                next_node = &(way->nodes().back());
                status = MemberStatus::NORMAL;
            } else if (way->nodes().back().ref() == previous_way->nodes().front().ref()
                    || way->nodes().back().ref() == previous_way->nodes().back().ref()) {
                next_node = &(way->nodes().front());
                status = MemberStatus::NORMAL;
            } else {
                m_writer.write_error_way(relation, 0, "gap", previous_way);
                status = MemberStatus::AFTER_GAP;
                result |= RouteError::UNORDERED_GAP;
            }
        } else if (status == MemberStatus::NORMAL) {
            if (way->nodes().front().ref() == next_node->ref()) {
                next_node = &(way->nodes().back());
            } else if (way->nodes().back().ref() == next_node->ref()) {
                next_node = &(way->nodes().front());
            } else {
                m_writer.write_error_way(relation, next_node->ref(), "gap", previous_way);
                m_writer.write_error_point(relation, next_node, "gap at this location", way->id());
                result |= RouteError::UNORDERED_GAP;
                status = MemberStatus::SECOND;
            }
        }
    }
    return result;
}
