/*
 * ptv2_checker.cpp
 *
 *  Created on:  2017-09-18
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "ptv2_checker.hpp"

#include <assert.h>


PTv2Checker::PTv2Checker(RouteWriter& writer) :
    m_writer(writer) {}

RouteType PTv2Checker::get_route_type(const char* route) {
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

bool PTv2Checker::is_stop(const char* role) {
    return !strcmp(role, "stop") || !strcmp(role, "stop_entry_only") || !strcmp(role, "stop_exit_only");
}

bool PTv2Checker::is_platform(const char* role) {
    return !strcmp(role, "platform") || !strcmp(role, "platform_entry_only") || !strcmp(role, "platform_exit_only");
}

bool PTv2Checker::vehicle_tags_matches_route_type(const osmium::TagList& tags, RouteType type) {
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

bool PTv2Checker::check_valid_railway_track(RouteType type, const osmium::TagList& member_tags) {
    const char* railway = member_tags.get_value_by_key("railway");
    if (!railway) {
        return is_ferry(member_tags);
    }
    if (type == RouteType::TRAIN || type == RouteType::TRAM) {
        if (!strcmp(railway, "rail") || !strcmp(railway, "light_rail") || !strcmp(railway, "tram") || !strcmp(railway, "subway")
                || !strcmp(railway, "funicular") || !strcmp(railway, "preserved") || !strcmp(railway, "miniature") || !strcmp(railway, "narrow_gauge")) {
            return true;
        }
    }
    if (type == RouteType::SUBWAY) {
            return true;
    }
    return is_ferry(member_tags);
}

bool PTv2Checker::check_valid_road_way(const osmium::TagList& member_tags) {
    const char* highway = member_tags.get_value_by_key("highway");
    if (!highway) {
        return is_ferry(member_tags);
    }
    return (!strcmp(highway, "motorway") || !strcmp(highway, "motorway_link") || !strcmp(highway, "trunk")
        || !strcmp(highway, "trunk_link") || !strcmp(highway, "primary") || !strcmp(highway, "primary_link")
        || !strcmp(highway, "secondary") || !strcmp(highway, "secondary_link") || !strcmp(highway, "tertiary")
        || !strcmp(highway, "tertiary_link") || !strcmp(highway, "unclassified") || !strcmp(highway, "residential")
        || !strcmp(highway, "service") || !strcmp(highway, "track") || !strcmp(highway, "pedestrian")
        || !strcmp(highway, "living_street"));
}

bool PTv2Checker::check_valid_trolleybus_way(const osmium::TagList& member_tags) {
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

bool PTv2Checker::is_ferry(const osmium::TagList& member_tags, bool permit_untagged_ways /* = false */) {
    const char* route = member_tags.get_value_by_key("route");
    return (route && !strcmp(route, "ferry")) || (permit_untagged_ways && !route);
}

bool PTv2Checker::roundabout_connected_to_previous_way(const BackOrFront previous_way_end, const osmium::Way* previous_way, const osmium::Way* way) {
    for (const osmium::NodeRef& nd_ref : way->nodes()) {
        if ((previous_way_end == BackOrFront::FRONT && previous_way->nodes().front().ref() == nd_ref.ref())
                || (previous_way_end == BackOrFront::BACK && previous_way->nodes().back().ref() == nd_ref.ref())) {
            return true;

        }
    }
    return false;
}

bool PTv2Checker::roundabout_as_second_after_gap(const osmium::Way* previous_way, const osmium::Way* way) {
    for (const osmium::NodeRef& nd_ref : way->nodes()) {
        if (previous_way->nodes().front().ref() == nd_ref.ref() || previous_way->nodes().back().ref() == nd_ref.ref()) {
            return true;
        }
    }
    return false;
}

BackOrFront PTv2Checker::roundabout_connected_to_next_way(const osmium::Way* previous_way, const osmium::Way* way) {
    for (const osmium::NodeRef& nd_ref : previous_way->nodes()) {
        if (way->nodes().front().ref() == nd_ref.ref()) {
            return BackOrFront::BACK;
        } else if (way->nodes().back().ref() == nd_ref.ref()) {
            return BackOrFront::FRONT;
        }
    }
    return BackOrFront::UNDEFINED;
}

RouteError PTv2Checker::is_way_usable(const osmium::Relation& relation, RouteType type, const osmium::Way* way) {
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
    case RouteType::FERRY:
        if (!is_ferry(way->tags(), true)) {
            m_writer.write_error_way(relation, 0, "trolley bus without trolley wire", way);
            return RouteError::NO_FERRY;
        }
        break;
    default:
        return RouteError::CLEAN;
    }
    return RouteError::CLEAN;
}

RouteError PTv2Checker::check_stop_tags(const osmium::Relation& relation, const osmium::Node* node, RouteType type) {
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

RouteError PTv2Checker::check_platform_tags(const osmium::Relation& relation, const RouteType type, const osmium::OSMObject* object) {
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

RouteError PTv2Checker::check_roles_order_and_type(const osmium::Relation& relation,
        std::vector<const osmium::OSMObject*>& member_objects) {
    // Have we already passed a member which is neither a stop nor a platform?
    bool seen_road_member = false;
    // Have we already passed a member which is a stop or a platform?
    bool seen_stop_platform = false;
    // Is the route incomplete (some members not available in the input file)?
    bool incomplete = false;
    RouteError error = RouteError::CLEAN;
    RouteType type = get_route_type(relation.get_value_by_key("route"));
    if (type == RouteType::NONE) {
        error |= RouteError::UNKNOWN_TYPE;
    }
    std::vector<const osmium::OSMObject*>::const_iterator obj_it = member_objects.cbegin();
    osmium::RelationMemberList::const_iterator member_it = relation.members().cbegin();
    for (; obj_it != member_objects.cend(), member_it != relation.members().cend();
            ++obj_it, ++member_it) {
        if (*obj_it == nullptr) {
            incomplete = true;
        }
        const osmium::OSMObject* object = *obj_it;
        if (member_it->type() == osmium::item_type::way && !strcmp(member_it->role(), "")) {
            seen_road_member = true;
            error |= role_check_handle_road_member(relation, type, object, seen_stop_platform);
        } else if (member_it->type() != osmium::item_type::way && !strcmp(member_it->role(), "")) {
            if (member_it->type() == osmium::item_type::node && object) {
                const osmium::Node* node = static_cast<const osmium::Node*>(object);
                m_writer.write_error_point(relation, node->id(), node->location(), "empty role for non-way object", 0);
            }
            error |= RouteError::EMPTY_ROLE_NON_WAY;
        } else if (seen_road_member && (is_stop(member_it->role()) || is_platform(member_it->role()))) {
            error |= handle_errorneous_stop_platform(relation, object);
        } else if (member_it->type() == osmium::item_type::node && is_stop(member_it->role())) {
            seen_stop_platform = true;
            // errors reported by check_stop_tags are not considered as severe
            if (object) {
                check_stop_tags(relation, static_cast<const osmium::Node*>(object), type);
            }
        } else if (member_it->type() == osmium::item_type::way && is_stop(member_it->role())) {
            error |= RouteError::STOP_IS_NOT_NODE;
            if (object) {
                m_writer.write_error_way(relation, 0, "stop is not a node", static_cast<const osmium::Way*>(object));
            }
        } else if (is_platform(member_it->role())) {
            seen_stop_platform = true;
            // errors reported by check_platform_tags are not considered as severe
            if (object) {
                check_platform_tags(relation, type, object);
            }
        } else if (strcmp(member_it->role(), "") && !is_stop(member_it->role()) && !is_platform(member_it->role())) {
            error |= handle_unknown_role(relation, object, member_it->role());
        }
    }
    if (!seen_road_member && !incomplete) {
        // route contains no highway/railway members
        error |= RouteError::NO_ROUTE;
        // write all members as errors
        obj_it = member_objects.cbegin();
        for (; obj_it != member_objects.cend(); ++obj_it) {
            if (*obj_it == nullptr) {
                continue;
            }
            switch ((*obj_it)->type()) {
            case osmium::item_type::node: {
                const osmium::Node* node = static_cast<const osmium::Node*>(*obj_it);
                m_writer.write_error_point(relation, node->id(), node->location(), "route has only stops/platforms", 0);
                break;
            }
            case osmium::item_type::way: {
                const osmium::Way* way = static_cast<const osmium::Way*>(*obj_it);
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

RouteError PTv2Checker::role_check_handle_road_member(const osmium::Relation& relation, const RouteType type,
        const osmium::OSMObject* object, const bool seen_stop_platform) {
    RouteError error = RouteError::CLEAN;
    if (!seen_stop_platform) {
        error |= RouteError::NO_STOPPLTF_AT_FRONT;
    }
    if (object) {
        error |= is_way_usable(relation, type, static_cast<const osmium::Way*>(object));
    }
    return error;
}

RouteError PTv2Checker::handle_errorneous_stop_platform(const osmium::Relation& relation, const osmium::OSMObject* object) {
    if (object) {
        switch (object->type()) {
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
    return RouteError::STOPPLTF_AFTER_ROUTE;
}

RouteError PTv2Checker::handle_unknown_role(const osmium::Relation& relation, const osmium::OSMObject* object, const char* role) {
    RouteError error = RouteError::CLEAN;
    error |= RouteError::UNKNOWN_ROLE;
    if (object) {
        std::string error_msg = "unknown role '";
        error_msg += role;
        error_msg += "'";
        switch (object->type()) {
        case osmium::item_type::node:
            m_writer.write_error_point(relation, object->id(), static_cast<const osmium::Node*>(object)->location(), error_msg.c_str(), 0);
            break;
        case osmium::item_type::way:
            m_writer.write_error_way(relation, 0, error_msg.c_str(), static_cast<const osmium::Way*>(object));
            break;
        default:
            break;
        }
    }
    return error;
}

int PTv2Checker::find_gaps(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects) {
    MemberStatus status = MemberStatus::BEFORE_FIRST;
    BackOrFront previous_way_end = BackOrFront::UNDEFINED;
    int gaps_count = 0;
    const osmium::Way* previous_way = nullptr;
    std::vector<const osmium::OSMObject*>::const_iterator obj_it = member_objects.cbegin();
    osmium::RelationMemberList::const_iterator member_it = relation.members().cbegin();
    for (size_t i = 0; obj_it != member_objects.cend(), member_it != relation.members().cend(); ++obj_it, ++member_it, ++i) {
        const osmium::OSMObject* object = *obj_it;
        const osmium::Way* way = nullptr;
        if (member_it->type() == osmium::item_type::way && object != nullptr) {
            way = static_cast<const osmium::Way*>(object);
        }
        gaps_count += gap_detector_member_handling(relation, way, previous_way, member_it, status, previous_way_end);
        if (member_it->type() == osmium::item_type::way && object) {
            previous_way = static_cast<const osmium::Way*>(object);
        }
    }
    return gaps_count;
}

int PTv2Checker::gap_detector_member_handling(const osmium::Relation& relation, const osmium::Way* way,
        const osmium::Way* previous_way, osmium::RelationMemberList::const_iterator member_it, MemberStatus& status,
        BackOrFront& previous_way_end) {
    const char* role = member_it->role();
    if (way == nullptr && status == MemberStatus::BEFORE_FIRST) {
        // We can ignore missing members if we are still in the stop/platform section
        return 0;
    } else if (way == nullptr && member_it->type() == osmium::item_type::way) {
        status = MemberStatus::AFTER_MISSING;
        return 0;
    }
    // check role and type
    if (status == MemberStatus::BEFORE_FIRST && member_it->type() == osmium::item_type::way && !strcmp(role, "")) {
        status = MemberStatus::FIRST;
    }
    if (status == MemberStatus::BEFORE_FIRST) {
        // All further checks are only done for the members after the list of stops/platforms.
        return 0;
    }

    // check if it is a way
    if (member_it->type() != osmium::item_type::way || strcmp(role, "")) {
        status = MemberStatus::AFTER_GAP;
        return 0;
    }

    // check if it is a roundabout
    if (status == MemberStatus::AFTER_GAP) {
        // write this way member as error
        m_writer.write_error_way(relation, 0, "gap", way);
    }
    // special treatment for roundabouts
    // Mappers don't have to split roundabouts if they are used by routes.
    if (way->tags().has_tag("junction", "roundabout") && way->nodes().ends_have_same_id()) {
        if (status == MemberStatus::AFTER_ROUNDABOUT) {
            // roundabout after another roundabout, this is an impossible geometry and shoud be fixed
            m_writer.write_error_way(relation, 0, "roundabout after roundabout", way);
            // The status AFTER_ROUNDABOUT is kept because the next way after this double-roundabout still has this status.
            return 1;
        }
        if (status == MemberStatus::FIRST || status == MemberStatus::AFTER_GAP) {
            // If this way is the first way in the route or the first after a gap, no other checks are necessary.
            status = MemberStatus::AFTER_ROUNDABOUT;
            return 0;
        }
        if (status == MemberStatus::SECOND) {
            status = MemberStatus::SECOND_ROUNDABOUT;
        } else {
            status = MemberStatus::ROUNDABOUT;
        }
    }
    if (status == MemberStatus::FIRST || status == MemberStatus::AFTER_MISSING) {
        status = MemberStatus::SECOND;
        return 0;
    }
    if (status == MemberStatus::AFTER_GAP) {
        status = MemberStatus::SECOND;
        return 0;
    }

    // Now the real comparisons begin.
    assert(previous_way && member_it->type() == osmium::item_type::way);
    if (status == MemberStatus::AFTER_ROUNDABOUT) {
        // check which end of the way is connected to the roundabout
        previous_way_end = roundabout_connected_to_next_way(previous_way, way);
        if (previous_way_end == BackOrFront::UNDEFINED) {
            m_writer.write_error_way(relation, 0, "gap or unordered before this way", way);
            status = MemberStatus::AFTER_GAP;
            return 1;
        } else {
            status = MemberStatus::NORMAL;
        }
    } else if (status == MemberStatus::ROUNDABOUT) {
        // check if any of the nodes of the roundabout matches the beginning or end node
        status = MemberStatus::AFTER_ROUNDABOUT;
        if (!roundabout_connected_to_previous_way(previous_way_end, previous_way, way)) {
            m_writer.write_error_way(relation, 0, "gap", way);
            const osmium::NodeRef* next_node = back_or_front_to_node_ref(previous_way_end, previous_way);
            m_writer.write_error_point(relation, next_node, "open end at this location", way->id());
            return 1;
        }
    } else if (status == MemberStatus::SECOND_ROUNDABOUT) {
        status = MemberStatus::AFTER_ROUNDABOUT;
        if (!roundabout_as_second_after_gap(previous_way, way)) {
            m_writer.write_error_way(relation, 0, "gap", way);
            return 1;
        }
    }
    else if (status == MemberStatus::SECOND) {
        if (previous_way->id() == way->id()) {
            // The first and the second way are equal. This is valid in some cases. So we just return.
            return 0;
        }
        //TODO secure for incomplete relations
        if (way->nodes().front().ref() == previous_way->nodes().front().ref()
                || way->nodes().front().ref() == previous_way->nodes().back().ref()) {
            previous_way_end = BackOrFront::BACK;
            status = MemberStatus::NORMAL;
        } else if (way->nodes().back().ref() == previous_way->nodes().front().ref()
                || way->nodes().back().ref() == previous_way->nodes().back().ref()) {
            previous_way_end = BackOrFront::FRONT;
            status = MemberStatus::NORMAL;
        } else {
            m_writer.write_error_way(relation, 0, "gap or unordered after this way", previous_way);
            status = MemberStatus::AFTER_GAP;
            return 1;
        }
    } else if (status == MemberStatus::NORMAL) {
        const osmium::NodeRef* next_node = back_or_front_to_node_ref(previous_way_end, previous_way);
        if (way->nodes().front().ref() == next_node->ref()) {
            previous_way_end = BackOrFront::BACK;
        } else if (way->nodes().back().ref() == next_node->ref()) {
            previous_way_end = BackOrFront::FRONT;
        } else {
            m_writer.write_error_way(relation, next_node->ref(), "gap", previous_way);
            m_writer.write_error_point(relation, next_node, "gap or unordered before this way", way->id());
            status = MemberStatus::SECOND;
            return 1;
        }
    }
    return 0;
}

/*static*/ const osmium::NodeRef* PTv2Checker::back_or_front_to_node_ref(BackOrFront back_or_front, const osmium::Way* way) {
    assert(back_or_front != BackOrFront::UNDEFINED);
    const osmium::NodeRef* next_node = nullptr;
    if (back_or_front == BackOrFront::BACK) {
        next_node = &(way->nodes().back());
    } else if (back_or_front == BackOrFront::FRONT) {
        next_node = &(way->nodes().front());
    }
    return next_node;
}
