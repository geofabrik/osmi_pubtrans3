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
                || !strcmp(railway, "funicular") || !strcmp(railway, "miniature")) {
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
        || !strcmp(highway, "service") || !strcmp(highway, "track"));
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

bool PTv2Checker::is_ferry(const osmium::TagList& member_tags) {
    const char* route = member_tags.get_value_by_key("route");
    return (route && !strcmp(route, "ferry"));
}

bool PTv2Checker::roundabout_connected_to_previous_way(const osmium::NodeRef* common_node, const osmium::Way* way) {
    for (const osmium::NodeRef& nd_ref : way->nodes()) {
        if (common_node->ref() == nd_ref.ref()) {
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

const osmium::NodeRef* PTv2Checker::roundabout_connected_to_next_way(const osmium::Way* previous_way, const osmium::Way* way) {
    for (const osmium::NodeRef& nd_ref : previous_way->nodes()) {
        if (way->nodes().front().ref() == nd_ref.ref()) {
            return &(way->nodes().back());
        } else if (way->nodes().back().ref() == nd_ref.ref()) {
            return &(way->nodes().front());
        }
    }
    return nullptr;
}

