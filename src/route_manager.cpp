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
        m_writer(dataset, output_format, verbose_output, epsg) { }

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
    result |= check_roles_order_and_type(relation);
    result |= find_gaps(relation, member_objects, roles);
    return result;
}

RouteType RouteManager::get_route_type(const char* route) {
    assert(route);
    if (!strcmp(route, "train")) {
        return RouteType::TRAIN;
    } else if (!strcmp(route, "subway")) {
        return RouteType::SUBWAY;
    } else if (!strcmp(route, "tram")) {
        return RouteType::TRAM;
    } else if (!strcmp(route, "bus")) {
        return RouteType::BUS;
    } else if (!strcmp(route, "trolleybus")) {
        return RouteType::TROLLEYBUS;
    } else if (!strcmp(route, "ferry")) {
        return RouteType::FERRY;
    } else if (!strcmp(route, "aerialway")) {
        return RouteType::AERIALWAY;
    }
    return RouteType::NONE;
}

RouteError RouteManager::check_roles_order_and_type(const osmium::Relation& relation) {
    bool seen_road_member = false;
    bool seen_stop_platform = false;
    bool incomplete = false;
    RouteError error = RouteError::CLEAN;
    RouteType type = get_route_type(relation.get_value_by_key("route"));
    if (type == RouteType::NONE) {
        error |= RouteError::UNKNOWN_TYPE;
    }
    for (const osmium::RelationMember& member : relation.members()) {
        const osmium::OSMObject* object = nullptr;
        if (member.full_member()) {
            object = this->get_member_object(member);
        } else {
            incomplete = true;
            continue;
        }
        if (member.type() == osmium::item_type::way && !strcmp(member.role(), "")) {
            seen_road_member = true;
            if (!seen_stop_platform) {
                error |= RouteError::NO_STOPPLTF_AT_FRONT;
            }
            if (member.type() != osmium::item_type::way) {
                if (member.type() == osmium::item_type::node && object) {
                    const osmium::Node* node = static_cast<const osmium::Node*>(object);
                    m_writer.write_error_point(relation, node->id(), node->location(), "empty role for non-way object", 0);
                }
                error |= RouteError::EMPTY_ROLE_NON_WAY;
            }
            if (object) {
                error |= is_way_usable(relation, type, static_cast<const osmium::Way*>(object));
            }
        } else if (seen_road_member && (is_stop(member.role()) || is_platform(member.role()))) {
            error |= RouteError::STOPPLTF_AFTER_ROUTE;
            if (object) {
                switch (member.type()) {
                case osmium::item_type::node:
                    m_writer.write_error_point(relation, static_cast<const osmium::Node*>(object)->id(),
                            static_cast<const osmium::Node*>(object)->location(), "stop/platform after route", 0);
                    break;
                case osmium::item_type::way:
                    m_writer.write_error_way(relation, 0, "stop/platform after route",
                            static_cast<const osmium::Way*>(object));
                    break;
                default:
                    break;
                }
            }
        } else if (member.type() == osmium::item_type::node && is_stop(member.role())) {
            seen_stop_platform = true;
            // errors reported by check_stop_tags are not considered as severe
            if (object) {
                check_stop_tags(relation, static_cast<const osmium::Node*>(object), type);
            }
        } else if (member.type() == osmium::item_type::way && is_stop(member.role())) {
            error |= RouteError::STOP_IS_NOT_NODE;
            if (object) {
                m_writer.write_error_way(relation, 0, "stop is not a node", static_cast<const osmium::Way*>(object));
            }
        } else if (is_platform(member.role())) {
            seen_stop_platform = true;
            // errors reported by check_platform_tags are not considered as severe
            if (object) {
                check_platform_tags(relation, type, object);
            }
        } else if (strcmp(member.role(), "") && !is_stop(member.role()) && !is_platform(member.role())) {
            error |= RouteError::UNKNOWN_ROLE;
            if (object) {
                std::string error_msg = "unknown role '";
                error_msg += member.role();
                error_msg += "'";
                switch (member.type()) {
                case osmium::item_type::node:
                    m_writer.write_error_point(relation, member.ref(), static_cast<const osmium::Node*>(object)->location(), error_msg.c_str(), 0);
                    break;
                case osmium::item_type::way:
                    m_writer.write_error_way(relation, 0, error_msg.c_str(), static_cast<const osmium::Way*>(object));
                    break;
                default:
                    break;
                }
            }
        }
    }
    if (!seen_road_member && !incomplete) {
        // route contains no highway/railway members
        error |= RouteError::NO_ROUTE;
        // write all members as errors
        for (const osmium::RelationMember& member : relation.members()) {
            if (!member.full_member()) {
                continue;
            }
            switch (member.type()) {
            case osmium::item_type::node: {
                const osmium::Node* node = this->get_member_node(member.ref());
                m_writer.write_error_point(relation, node->id(), node->location(), "route has only stops/platforms", 0);
                break;
            }
            case osmium::item_type::way: {
                const osmium::Way* way = this->get_member_way(member.ref());
                m_writer.write_error_way(relation, 0, "route has only stops/platforms", way);
                break;
            }
            default:
                break;
            }
        }
    }
    return error;
}

bool RouteManager::is_stop(const char* role) {
    return !strcmp(role, "stop") || !strcmp(role, "stop_entry_only") || !strcmp(role, "stop_exit_only");
}

bool RouteManager::is_platform(const char* role) {
    return !strcmp(role, "platform") || !strcmp(role, "platform_entry_only") || !strcmp(role, "platform_exit_only");
}

RouteError RouteManager::check_stop_tags(const osmium::Relation& relation, const osmium::Node* node, RouteType type) {
    if (node->tags().has_tag("public_transport", "stop_position") && vehicle_tags_matches_route_type(node->tags(), type)) {
        return RouteError::CLEAN;
    }
    if ((type == RouteType::BUS || type == RouteType::TROLLEYBUS) && !node->tags().has_tag("highway", "bus_stop")) {
        m_writer.write_error_point(relation, node->id(), node->location(), "stop without proper tags", 0);
        return RouteError::STOP_TAG_MISSING;
    }
    if (type == RouteType::TRAIN && !node->tags().has_tag("railway", "station")
            && !node->tags().has_tag("railway", "halt") && !node->tags().has_tag("railway", "tram_stop")) {
        m_writer.write_error_point(relation, node->id(), node->location(), "stop without proper tags", 0);
        return RouteError::STOP_TAG_MISSING;
    }
    if (type == RouteType::SUBWAY && !node->tags().has_tag("railway", "station")) {
        m_writer.write_error_point(relation, node->id(), node->location(), "stop without proper tags", 0);
        return RouteError::STOP_TAG_MISSING;
    }
    if (type == RouteType::FERRY && !node->tags().has_tag("amenity", "ferry_terminal")) {
        m_writer.write_error_point(relation, node->id(), node->location(), "stop without proper tags", 0);
        return RouteError::STOP_TAG_MISSING;
    }
    if (type == RouteType::AERIALWAY && !node->tags().has_tag("aerialway", "station")) {
        m_writer.write_error_point(relation, node->id(), node->location(), "stop without proper tags", 0);
        return RouteError::STOP_TAG_MISSING;
    }
    return RouteError::CLEAN;
}

bool RouteManager::vehicle_tags_matches_route_type(const osmium::TagList& tags, RouteType type) {
    switch (type) {
    case RouteType::BUS:
        return tags.has_tag("bus", "yes");
    case RouteType::TROLLEYBUS:
        return tags.has_tag("trolleybus", "yes");
    case RouteType::TRAIN:
        return tags.has_tag("train", "yes");
    case RouteType::TRAM:
        return tags.has_tag("tram", "yes");
    case RouteType::SUBWAY:
        return tags.has_tag("subway", "yes");
    case RouteType::FERRY:
        return tags.has_tag("ferry", "yes");
    case RouteType::AERIALWAY:
        return tags.has_tag("aerialway", "yes");
    default:
        return false;
    }
}

RouteError RouteManager::check_platform_tags(const osmium::Relation& relation, const RouteType type, const osmium::OSMObject* object) {
    if (object->tags().has_tag("public_transport", "platform")) {
        return RouteError::CLEAN;
    }
    if (((type == RouteType::BUS || type == RouteType::TROLLEYBUS)
            && !object->tags().has_tag("highway", "bus_stop") && !object->tags().has_tag("highway", "platform"))
            || ((type == RouteType::TRAIN || type == RouteType::TRAM || type == RouteType::SUBWAY)
            && !object->tags().has_tag("railway", "platform"))) {
        if (object->type() == osmium::item_type::node) {
            const osmium::Node* node = static_cast<const osmium::Node*>(object);
            m_writer.write_error_point(relation, node->id(), node->location(), "platform without proper tags", 0);
        } else if (object->type() == osmium::item_type::way) {
            m_writer.write_error_way(relation, 0, "platform without proper tags", static_cast<const osmium::Way*>(object));
        }
        return RouteError::PLTF_TAG_MISSING;
    }
    return RouteError::CLEAN;
}

RouteError RouteManager::is_way_usable(const osmium::Relation& relation, RouteType type, const osmium::Way* way) {
    switch (type) {
    case RouteType::TRAIN:
    case RouteType::TRAM:
    case RouteType::SUBWAY:
        if (!check_valid_railway_track(type, way->tags())) {
            m_writer.write_error_way(relation, 0, "rail-guided route over non-rail", way);
            return RouteError::OVER_NON_RAIL;
        }
        break;

    case RouteType::BUS:
        if (!check_valid_road_way(way->tags())) {
            m_writer.write_error_way(relation, 0, "road vehicle route over non-road", way);
            return RouteError::OVER_NON_ROAD;
        }
        break;
    case RouteType::TROLLEYBUS:
        if (!check_valid_trolleybus_way(way->tags())) {
            m_writer.write_error_way(relation, 0, "trolley bus without trolley wire", way);
            return RouteError::NO_TROLLEY_WIRE;
        }
        break;
    default:
        return RouteError::CLEAN;
    }
    return RouteError::CLEAN;
}

bool RouteManager::check_valid_railway_track(RouteType type, const osmium::TagList& member_tags) {
    const char* railway = member_tags.get_value_by_key("railway");
    if (!railway) {
        return is_ferry(member_tags);
    }
    if (type == RouteType::TRAIN || type == RouteType::TRAM) {
        if (!strcmp(railway, "rail") || !strcmp(railway, "light_rail") || !strcmp(railway, "tram") || !strcmp(railway, "subway")
                || !strcmp(railway, "funicular") || !strcmp(railway, "miniature")) {
            return true;
        }
    }
    if (type == RouteType::SUBWAY) {
            return true;
    }
    return is_ferry(member_tags);
}

bool RouteManager::check_valid_road_way(const osmium::TagList& member_tags) {
    const char* highway = member_tags.get_value_by_key("highway");
    if (!highway) {
        return is_ferry(member_tags);
    }
    return (!strcmp(highway, "motorway") || !strcmp(highway, "motorway_link") || !strcmp(highway, "trunk")
        || !strcmp(highway, "trunk_link") || !strcmp(highway, "primary") || !strcmp(highway, "primary_link")
        || !strcmp(highway, "secondary") || !strcmp(highway, "secondary_link") || !strcmp(highway, "tertiary")
        || !strcmp(highway, "tertiary_link") || !strcmp(highway, "unclassified") || !strcmp(highway, "residential")
        || !strcmp(highway, "service") || !strcmp(highway, "track"));
}

bool RouteManager::check_valid_trolleybus_way(const osmium::TagList& member_tags) {
    if (member_tags.has_tag("trolley_wire", "yes")) {
        return check_valid_road_way(member_tags);
    }
    //TODO check direction (forward wire but using the way in opposite direction might cause problems ;-)
    if (member_tags.has_tag("trolley_wire:forward", "yes") || member_tags.has_tag("trolley_wire", "forward")) {
        return check_valid_road_way(member_tags);
    }
    return (member_tags.has_tag("trolley_wire:backward", "yes") || member_tags.has_tag("trolley_wire", "backward"))
            && check_valid_road_way(member_tags);
}

bool RouteManager::is_ferry(const osmium::TagList& member_tags) {
    const char* route = member_tags.get_value_by_key("route");
    return (route && !strcmp(route, "ferry"));
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
            next_node = roundabout_connected_to_next_way(previous_way, way);
            if (next_node == nullptr) {
                m_writer.write_error_way(relation, 0, "gap", way);
                status = MemberStatus::AFTER_GAP;
                result |= RouteError::UNORDERED_GAP;
            } else {
                status = MemberStatus::NORMAL;
            }
        } else if (status == MemberStatus::ROUNDABOUT) {
            // check if any of the nodes of the roundabout matches the beginning or end node
            if (!roundabout_connected_to_previous_way(next_node, way)) {
                m_writer.write_error_way(relation, 0, "gap", way);
                m_writer.write_error_point(relation, next_node, "gap at this location", way->id());
                result |= RouteError::UNORDERED_GAP;
            }
            status = MemberStatus::AFTER_ROUNDABOUT;
        } else if (status == MemberStatus::SECOND_ROUNDABOUT) {
            if (!roundabout_as_second_after_gap(previous_way, way)) {
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

bool RouteManager::roundabout_connected_to_previous_way(const osmium::NodeRef* common_node, const osmium::Way* way) {
    for (const osmium::NodeRef& nd_ref : way->nodes()) {
        if (common_node->ref() == nd_ref.ref()) {
            return true;
        }
    }
    return false;
}

bool RouteManager::roundabout_as_second_after_gap(const osmium::Way* previous_way, const osmium::Way* way) {
    for (const osmium::NodeRef& nd_ref : way->nodes()) {
        if (previous_way->nodes().front().ref() == nd_ref.ref() || previous_way->nodes().back().ref() == nd_ref.ref()) {
            return true;
        }
    }
    return false;
}

const osmium::NodeRef* RouteManager::roundabout_connected_to_next_way(const osmium::Way* previous_way, const osmium::Way* way) {
    for (const osmium::NodeRef& nd_ref : previous_way->nodes()) {
        if (way->nodes().front().ref() == nd_ref.ref()) {
            return &(way->nodes().back());
        } else if (way->nodes().back().ref() == nd_ref.ref()) {
            return &(way->nodes().front());
        }
    }
    return nullptr;
}
