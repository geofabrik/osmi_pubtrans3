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
    if (m_checker.find_gaps(relation, member_objects, roles) > 0) {
        result |= RouteError::UNORDERED_GAP;
    }
    return result;
}
