/*
 * route_manager.cpp
 *
 *  Created on:  2017-08-23
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include <ogr_core.h>
#include <osmium/osm/item_type.hpp>

#include "route_manager.hpp"


RouteManager::RouteManager(gdalcpp::Dataset& dataset, std::string& output_format,
    osmium::util::VerboseOutput& verbose_output, int epsg /*= 3857*/) :
        OGROutputBase(verbose_output, output_format, epsg),
        m_dataset(dataset),
        m_ptv2_routes_valid(dataset, "ptv2_routes_valid", wkbMultiLineString, GDAL_DEFAULT_OPTIONS),
        m_ptv2_routes_invalid(dataset, "ptv2_routes_invalid", wkbMultiLineString, GDAL_DEFAULT_OPTIONS),
        m_ptv2_error_lines(dataset, "ptv2_error_lines", wkbLineString, GDAL_DEFAULT_OPTIONS),
        m_ptv2_error_points(dataset, "ptv2_error_points", wkbPoint, GDAL_DEFAULT_OPTIONS) {
    m_ptv2_routes_valid.add_field("from", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_valid.add_field("to", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_valid.add_field("via", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_valid.add_field("ref", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_valid.add_field("name", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_valid.add_field("operator", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_valid.add_field("route", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_valid.add_field("rel_id", OFTString, 10);
    m_ptv2_routes_invalid.add_field("from", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_invalid.add_field("to", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_invalid.add_field("via", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_invalid.add_field("ref", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_invalid.add_field("name", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_invalid.add_field("operator", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_invalid.add_field("route", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_invalid.add_field("rel_id", OFTString, 10);
    m_ptv2_routes_invalid.add_field("error_over_non_rail", OFTString, 1);
    m_ptv2_routes_invalid.add_field("error_over_rail", OFTString, 1);
    m_ptv2_routes_invalid.add_field("error_unordered_gap", OFTString, 1);
    m_ptv2_routes_invalid.add_field("error_wrong_structure", OFTString, 1);
    m_ptv2_routes_invalid.add_field("no_stops_pltf_at_begin", OFTString, 1);
    m_ptv2_routes_invalid.add_field("non_way_empty_role", OFTString, 1);
    m_ptv2_error_lines.add_field("rel_id", OFTString, 10);
    m_ptv2_error_lines.add_field("way_id", OFTString, 10);
    m_ptv2_error_lines.add_field("node_id", OFTString, 10);
    m_ptv2_error_lines.add_field("error", OFTString, 50);
    m_ptv2_error_lines.add_field("from", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_lines.add_field("to", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_lines.add_field("via", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_lines.add_field("ref", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_lines.add_field("name", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_lines.add_field("route", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_points.add_field("rel_id", OFTString, 10);
    m_ptv2_error_points.add_field("way_id", OFTString, 10);
    m_ptv2_error_points.add_field("node_id", OFTString, 10);
    m_ptv2_error_points.add_field("error", OFTString, 50);
    m_ptv2_error_points.add_field("from", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_points.add_field("to", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_points.add_field("via", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_points.add_field("ref", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_points.add_field("name", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_points.add_field("route", OFTString, MAX_FIELD_LENGTH);
}

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
        return true;
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
        size_t validation_result = is_valid(relation, member_objects, roles);
        if (validation_result == 0) {
            write_valid_route(relation, member_objects, roles);
            return;
        }
        write_invalid_route(relation, member_objects, roles, validation_result);
    }
}

bool RouteManager::is_ptv2(const osmium::Relation& relation) {
    // check if it is a PTv2 route
    const char* ptv2 = relation.get_value_by_key("public_transport:version");
    if (!ptv2 || strcmp(ptv2, "2")) {
        return false;
    }
    return true;
}

size_t RouteManager::is_valid(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
        std::vector<const char*>& roles) {
    size_t result = 0;
    RouteType type = RouteType::NONE;
    const char* route = relation.get_value_by_key("route");
    assert(route);
    if (!strcmp(route, "train") || !strcmp(route, "light_rail") || !strcmp(route, "subway")
                || !strcmp(route, "tram")) {
        type = RouteType::RAIL;
    } else if (!strcmp(route, "bus") || !strcmp(route, "trolleybus") || !strcmp(route, "share_taxi")) {
        type = RouteType::BUS;
    }
    result += check_roles_order_and_type(type, member_objects, roles);
    result += find_gaps(relation, member_objects, roles);
    return result;
}

size_t RouteManager::check_roles_order_and_type(const RouteType type, std::vector<const osmium::OSMObject*>& member_objects,
        std::vector<const char*>& roles) {
    bool seen_road_member = false;
    bool seen_stop_platform = false;
    for (size_t i = 0; i < member_objects.size(); ++i) {
        if (!member_objects.at(i)) {
            //TODO work with RelationMemberList class instead of std::vector<osmium::OSMObject*>
            continue;
        }
        const char* role = roles.at(i);
        if (!strcmp(role, "")) {
            seen_road_member = true;
            if (!seen_stop_platform) {
                //TODO write error "stop/platform missing at beginning"
                return 16;
            }
            if (member_objects.at(i)->type() != osmium::item_type::way) {
                //TODO write error "empty role for non-way object"
                return 32;
            }
//            if (type == RouteType::RAIL) {
//                const char* railway = member_objects.at(i)->get_value_by_key("railway");
//                const char* route = member_objects.at(i)->get_value_by_key("route");
//                if (!check_valid_railway_track(railway, route)) {
//                    //TODO write error "rail-guided route over non-rail"
//                    // no fatal error
//                    return 1;
//                }
//            }
//            if (type == RouteType::ROAD) {
//                const char* highway = member_objects.at(i)->get_value_by_key("highway");
//                const char* route = member_objects.at(i)->get_value_by_key("route");
//                if (!check_valid_road_way(highway, route)) {
//                    //TODO write error "road vehicle route over non-road"
//                    // no fatal error
//                    return 2;
//                }
//            }
        } else if (seen_road_member && (!strcmp(role, "stop") || !strcmp(role, "platform"))) {
            //TODO write error "stop/platform after route"
            return 64;
        } else if (!strcmp(role, "stop") || !strcmp(role, "platform")) {
            seen_stop_platform = true;
        }
        //TODO check for routes only consisting of stops and platforms
    }
    return 0;
}

size_t RouteManager::find_gaps(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
        std::vector<const char*>& roles) {
    MemberStatus status = MemberStatus::BEFORE_FIRST;
    const osmium::NodeRef* next_node;
    bool error_found = false;
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
            write_error_way(relation, 0, "gap", way);
        }
        if (way->tags().has_tag("junction", "roundabout") && way->nodes().ends_have_same_id()) {
            if (status == MemberStatus::AFTER_ROUNDABOUT) {
                // roundabout after another roundabout, this is an impossible geometry and shoud be fixed
                write_error_way(relation, 0, "roundabout after roundabout", way);
                error_found = true;
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
                write_error_way(relation, 0, "gap", way);
                status = MemberStatus::AFTER_GAP;
                error_found = true;
            } else {
                status = MemberStatus::NORMAL;
            }
        } else if (status == MemberStatus::ROUNDABOUT) {
            // check if any of the nodes of the roundabout matches the beginning or end node
            if (!roundabout_connected_to_previous_way(next_node, way)) {
                write_error_way(relation, 0, "gap", way);
                write_error_point(relation, next_node, "gap at this location", way->id());
                error_found = true;
            }
            status = MemberStatus::AFTER_ROUNDABOUT;
        } else if (status == MemberStatus::SECOND_ROUNDABOUT) {
            if (!roundabout_as_second_after_gap(previous_way, way)) {
                write_error_way(relation, 0, "gap", way);
                error_found = true;
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
                write_error_way(relation, 0, "gap", previous_way);
                status = MemberStatus::AFTER_GAP;
                error_found = true;
            }
        } else if (status == MemberStatus::NORMAL) {
            if (way->nodes().front().ref() == next_node->ref()) {
                next_node = &(way->nodes().back());
            } else if (way->nodes().back().ref() == next_node->ref()) {
                next_node = &(way->nodes().front());
            } else {
                write_error_way(relation, next_node->ref(), "gap", previous_way);
                write_error_point(relation, next_node, "gap at this location", way->id());
                error_found = true;
                status = MemberStatus::SECOND;
            }
        }
    }
    if (error_found) {
        return 4;
    }
    return 0;
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

void RouteManager::write_valid_route(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
        std::vector<const char*>& roles) {
    OGRMultiLineString* ml = new OGRMultiLineString();
    for (size_t i = 0; i < member_objects.size(); ++i) {
        const osmium::OSMObject* member = member_objects.at(i);
        if (!member || member->type() != osmium::item_type::way) {
            continue;
        }
        const char* role = roles.at(i);
        if (!role || (strcmp(role, "") && strcmp(role, "forward") && strcmp(role, "backward"))) {
            continue;
        }
        const osmium::Way* way = static_cast<const osmium::Way*>(member);
        try {
            std::unique_ptr<OGRLineString> geom = m_factory.create_linestring(*way);
            ml->addGeometry(geom.get());
        }
        catch (osmium::geometry_error& e) {
            std::cerr << e.what() << std::endl;
        }
    }
    gdalcpp::Feature feature(m_ptv2_routes_valid, std::unique_ptr<OGRGeometry> (ml));
    static char idbuffer[20];
    sprintf(idbuffer, "%ld", relation.id());
    feature.set_field("rel_id", idbuffer);
    feature.set_field("name", relation.get_value_by_key("name"));
    feature.set_field("ref", relation.get_value_by_key("ref"));
    feature.set_field("from", relation.get_value_by_key("from"));
    feature.set_field("to", relation.get_value_by_key("to"));
    feature.set_field("via", relation.get_value_by_key("via"));
    feature.set_field("route", relation.get_value_by_key("route"));
    feature.set_field("operator", relation.get_value_by_key("operator"));
    feature.add_to_layer();
}

void RouteManager::write_invalid_route(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
        std::vector<const char*>& roles, size_t validation_result) {
    OGRMultiLineString* ml = new OGRMultiLineString();
    for (const osmium::OSMObject* member : member_objects) {
        if (!member) {
            continue;
        }
        if (member->type() != osmium::item_type::way) {
            continue;
        }
        const osmium::Way* way = static_cast<const osmium::Way*>(member);
        try {
            std::unique_ptr<OGRLineString> geom = m_factory.create_linestring(*way);
            ml->addGeometry(geom.get());
        }
        catch (osmium::geometry_error& e) {
            std::cerr << e.what() << std::endl;
        }
    }
    gdalcpp::Feature feature(m_ptv2_routes_invalid, std::unique_ptr<OGRGeometry>(ml));
    static char idbuffer[20];
    sprintf(idbuffer, "%ld", relation.id());
    feature.set_field("rel_id", idbuffer);
    feature.set_field("name", relation.get_value_by_key("name"));
    feature.set_field("ref", relation.get_value_by_key("ref"));
    feature.set_field("from", relation.get_value_by_key("from"));
    feature.set_field("to", relation.get_value_by_key("to"));
    feature.set_field("via", relation.get_value_by_key("via"));
    feature.set_field("route", relation.get_value_by_key("route"));
    feature.set_field("operator", relation.get_value_by_key("operator"));
    if ((validation_result & 1) == 1) {
        feature.set_field("error_over_non_rail", "T");
    }
    if ((validation_result & 2) == 2) {
        feature.set_field("error_over_rail", "T");
    }
    if ((validation_result & 4) == 4) {
        feature.set_field("error_unordered_gap", "T");
    }
    if ((validation_result & 8) == 8) {
        feature.set_field("error_wrong_structure", "T");
    }
    if ((validation_result & 16) == 16) {
        feature.set_field("no_stops_pltf_at_begin", "T");
    }
    if ((validation_result & 32) == 32) {
        feature.set_field("non_way_empty_role", "T");
    }
    feature.add_to_layer();
}

void RouteManager::write_error_way(const osmium::Relation& relation, const osmium::object_id_type node_ref,
        const char* error_text, const osmium::Way* way) {
    try {
        gdalcpp::Feature feature(m_ptv2_error_lines, m_factory.create_linestring(*way));
        static char way_idbuffer[20];
        sprintf(way_idbuffer, "%ld", way->id());
        feature.set_field("way_id", way_idbuffer);
        static char node_idbuffer[20];
        sprintf(node_idbuffer, "%ld", node_ref);
        feature.set_field("node_id", node_idbuffer);
        static char rel_idbuffer[20];
        sprintf(rel_idbuffer, "%ld", relation.id());
        feature.set_field("rel_id", rel_idbuffer);
        feature.set_field("name", relation.get_value_by_key("name"));
        feature.set_field("ref", relation.get_value_by_key("ref"));
        feature.set_field("from", relation.get_value_by_key("from"));
        feature.set_field("to", relation.get_value_by_key("to"));
        feature.set_field("via", relation.get_value_by_key("via"));
        feature.set_field("route", relation.get_value_by_key("route"));
        feature.set_field("name", relation.get_value_by_key("name"));
        feature.set_field("error", error_text);
        feature.add_to_layer();
    } catch (osmium::geometry_error& err) {
        m_verbose_output << err.what() << '\n';
    }
}

void RouteManager::write_error_point(const osmium::Relation& relation, const osmium::NodeRef* node_ref,
        const char* error_text, const osmium::object_id_type way_id) {
    gdalcpp::Feature feature(m_ptv2_error_points, m_factory.create_point(*node_ref));
    static char way_idbuffer[20];
    sprintf(way_idbuffer, "%ld", way_id);
    feature.set_field("way_id", way_idbuffer);
    static char node_idbuffer[20];
    sprintf(node_idbuffer, "%ld", node_ref->ref());
    feature.set_field("node_id", node_idbuffer);
    static char rel_idbuffer[20];
    sprintf(rel_idbuffer, "%ld", relation.id());
    feature.set_field("rel_id", rel_idbuffer);
    feature.set_field("name", relation.get_value_by_key("name"));
    feature.set_field("ref", relation.get_value_by_key("ref"));
    feature.set_field("from", relation.get_value_by_key("from"));
    feature.set_field("to", relation.get_value_by_key("to"));
    feature.set_field("via", relation.get_value_by_key("via"));
    feature.set_field("route", relation.get_value_by_key("route"));
    feature.set_field("name", relation.get_value_by_key("name"));
    feature.set_field("error", error_text);
    feature.add_to_layer();
}
