/*
 * route_writer.cpp
 *
 *  Created on:  2017-08-28
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include <ogr_core.h>
#include "route_writer.hpp"

/// indexes of fields – all layers
struct FieldIndexes {
    static constexpr int rel_id = 0;
    static constexpr int from = 1;
    static constexpr int to = 2;
    static constexpr int via = 3;
    static constexpr int ref = 4;
    static constexpr int name = 5;
    static constexpr int route = 6;
};

/// indexes of fields – valid and invalid routes layers
struct ValidInvalidFieldIndexes {
    static constexpr int _operator = 7;
};

/// indexes of fields – invalid routes layer
struct InvalidFieldIndexes {
    static constexpr int error_over_non_rail = 8;
    static constexpr int error_over_rail = 9;
    static constexpr int error_unordered_gap = 10;
    static constexpr int error_wrong_structure = 11;
    static constexpr int no_stops_pltf_at_begin = 12;
    static constexpr int stoppltf_after_route = 13;
    static constexpr int non_way_empty_role = 14;
    static constexpr int stop_not_on_way = 15;
    static constexpr int no_way_members = 16;
    static constexpr int unknown_role = 17;
    static constexpr int unknown_route_type = 18;
    static constexpr int stop_is_not_node = 19;
    static constexpr int error_over_non_ferry = 20;
};

/// indexes of fields – error layers
struct ErrorFieldIndexes {
    static constexpr int way_id = 7;
    static constexpr int node_id = 8;
    static constexpr int error = 9;
};

RouteWriter::RouteWriter(OGRWriter& writer, Options& options,
    osmium::util::VerboseOutput& verbose_output) :
        OGROutputBase(writer, verbose_output, options),
        m_ptv2_routes_valid(m_writer.create_layer("ptv2_routes_valid", wkbMultiLineString)),
        m_ptv2_routes_invalid(m_writer.create_layer("ptv2_routes_invalid", wkbMultiLineString)),
        m_ptv2_error_lines(m_writer.create_layer("ptv2_error_lines", wkbLineString)),
        m_ptv2_error_points(m_writer.create_layer("ptv2_error_points", wkbPoint)) {
    m_ptv2_routes_valid.add_field("rel_id", OFTString, 10);
    m_ptv2_routes_valid.add_field("from", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_valid.add_field("to", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_valid.add_field("via", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_valid.add_field("ref", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_valid.add_field("name", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_valid.add_field("route", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_valid.add_field("operator", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_invalid.add_field("rel_id", OFTString, 10);
    m_ptv2_routes_invalid.add_field("from", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_invalid.add_field("to", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_invalid.add_field("via", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_invalid.add_field("ref", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_invalid.add_field("name", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_invalid.add_field("route", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_invalid.add_field("operator", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_routes_invalid.add_field("error_over_non_rail", OFTString, 1);
    m_ptv2_routes_invalid.add_field("error_over_rail", OFTString, 1);
    m_ptv2_routes_invalid.add_field("error_unordered_gap", OFTString, 1);
    m_ptv2_routes_invalid.add_field("error_wrong_structure", OFTString, 1);
    m_ptv2_routes_invalid.add_field("no_stops_pltf_at_begin", OFTString, 1);
    m_ptv2_routes_invalid.add_field("stoppltf_after_route", OFTString, 1);
    m_ptv2_routes_invalid.add_field("non_way_empty_role", OFTString, 1);
    m_ptv2_routes_invalid.add_field("stop_not_on_way", OFTString, 1);
    m_ptv2_routes_invalid.add_field("no_way_members", OFTString, 1);
    m_ptv2_routes_invalid.add_field("unknown_role", OFTString, 1);
    m_ptv2_routes_invalid.add_field("unknown_route_type", OFTString, 1);
    m_ptv2_routes_invalid.add_field("stop_is_not_node", OFTString, 1);
    m_ptv2_routes_invalid.add_field("error_over_non_ferry", OFTString, 1);
    m_ptv2_error_lines.add_field("rel_id", OFTString, 10);
    m_ptv2_error_lines.add_field("from", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_lines.add_field("to", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_lines.add_field("via", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_lines.add_field("ref", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_lines.add_field("name", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_lines.add_field("route", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_lines.add_field("way_id", OFTString, 10);
    m_ptv2_error_lines.add_field("node_id", OFTString, 10);
    m_ptv2_error_lines.add_field("error", OFTString, 50);
    m_ptv2_error_points.add_field("rel_id", OFTString, 10);
    m_ptv2_error_points.add_field("from", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_points.add_field("to", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_points.add_field("via", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_points.add_field("ref", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_points.add_field("name", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_points.add_field("route", OFTString, MAX_FIELD_LENGTH);
    m_ptv2_error_points.add_field("way_id", OFTString, 10);
    m_ptv2_error_points.add_field("node_id", OFTString, 10);
    m_ptv2_error_points.add_field("error", OFTString, 50);
}



void RouteWriter::write_valid_route(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
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
            m_verbose_output << e.what() << '\n';
        }
    }
    gdalcpp::Feature feature(m_ptv2_routes_valid, std::unique_ptr<OGRGeometry> (ml));
    static char idbuffer[20];
    sprintf(idbuffer, "%ld", relation.id());
    feature.set_field(FieldIndexes::rel_id, idbuffer);
    feature.set_field(FieldIndexes::name, relation.get_value_by_key("name"));
    feature.set_field(FieldIndexes::ref, relation.get_value_by_key("ref"));
    feature.set_field(FieldIndexes::from, relation.get_value_by_key("from"));
    feature.set_field(FieldIndexes::to, relation.get_value_by_key("to"));
    feature.set_field(FieldIndexes::via, relation.get_value_by_key("via"));
    feature.set_field(FieldIndexes::route, relation.get_value_by_key("route"));
    feature.set_field(ValidInvalidFieldIndexes::_operator, relation.get_value_by_key("operator"));
    feature.add_to_layer();
}

void RouteWriter::write_invalid_route(const osmium::Relation& relation, std::vector<const osmium::OSMObject*>& member_objects,
        RouteError validation_result) {
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
    feature.set_field(FieldIndexes::rel_id, idbuffer);
    feature.set_field(FieldIndexes::name, relation.get_value_by_key("name"));
    feature.set_field(FieldIndexes::ref, relation.get_value_by_key("ref"));
    feature.set_field(FieldIndexes::from, relation.get_value_by_key("from"));
    feature.set_field(FieldIndexes::to, relation.get_value_by_key("to"));
    feature.set_field(FieldIndexes::via, relation.get_value_by_key("via"));
    feature.set_field(FieldIndexes::route, relation.get_value_by_key("route"));
    feature.set_field(ValidInvalidFieldIndexes::_operator, relation.get_value_by_key("operator"));
    if ((validation_result & RouteError::OVER_NON_RAIL) == RouteError::OVER_NON_RAIL) {
        feature.set_field(InvalidFieldIndexes::error_over_non_rail, "T");
    }
    if ((validation_result & RouteError::OVER_NON_ROAD) == RouteError::OVER_NON_ROAD) {
        feature.set_field(InvalidFieldIndexes::error_over_rail, "T");
    }
    if ((validation_result & RouteError::UNORDERED_GAP) == RouteError::UNORDERED_GAP) {
        feature.set_field(InvalidFieldIndexes::error_unordered_gap, "T");
    }
    if ((validation_result & RouteError::WRONG_STRUCTURE) == RouteError::WRONG_STRUCTURE) {
        feature.set_field(InvalidFieldIndexes::error_wrong_structure, "T");
    }
    if ((validation_result & RouteError::NO_STOPPLTF_AT_FRONT) == RouteError::NO_STOPPLTF_AT_FRONT) {
        feature.set_field(InvalidFieldIndexes::no_stops_pltf_at_begin, "T");
    }
    if ((validation_result & RouteError::EMPTY_ROLE_NON_WAY) == RouteError::EMPTY_ROLE_NON_WAY) {
        feature.set_field(InvalidFieldIndexes::non_way_empty_role, "T");
    }
    if ((validation_result & RouteError::STOPPLTF_AFTER_ROUTE) == RouteError::STOPPLTF_AFTER_ROUTE) {
        feature.set_field(InvalidFieldIndexes::stoppltf_after_route, "T");
    }
    if ((validation_result & RouteError::STOP_NOT_ON_WAY) == RouteError::STOP_NOT_ON_WAY) {
        feature.set_field(InvalidFieldIndexes::stop_not_on_way, "T");
    }
    if ((validation_result & RouteError::NO_ROUTE) == RouteError::NO_ROUTE) {
        feature.set_field(InvalidFieldIndexes::no_way_members, "T");
    }
    if ((validation_result & RouteError::UNKNOWN_ROLE) == RouteError::UNKNOWN_ROLE) {
        feature.set_field(InvalidFieldIndexes::unknown_role, "T");
    }
    if ((validation_result & RouteError::UNKNOWN_TYPE) == RouteError::UNKNOWN_TYPE) {
        feature.set_field(InvalidFieldIndexes::unknown_route_type, "T");
    }
    if ((validation_result & RouteError::STOP_IS_NOT_NODE) == RouteError::STOP_IS_NOT_NODE) {
        feature.set_field(InvalidFieldIndexes::stop_is_not_node, "T");
    }
    if ((validation_result & RouteError::NO_FERRY) == RouteError::NO_FERRY) {
        feature.set_field(InvalidFieldIndexes::error_over_non_ferry, "T");
    }
    feature.add_to_layer();
}

#ifdef TEST_NO_ERROR_WRITING
void RouteWriter::write_error_way(const osmium::Relation&, const osmium::object_id_type,
        const char*, const osmium::Way*) {}
#else
void RouteWriter::write_error_way(const osmium::Relation& relation, const osmium::object_id_type node_ref,
        const char* error_text, const osmium::Way* way) {
    try {
        gdalcpp::Feature feature(m_ptv2_error_lines, m_factory.create_linestring(*way));
        static char way_idbuffer[20];
        sprintf(way_idbuffer, "%ld", way->id());
        feature.set_field(ErrorFieldIndexes::way_id, way_idbuffer);
        static char node_idbuffer[20];
        sprintf(node_idbuffer, "%ld", node_ref);
        feature.set_field(ErrorFieldIndexes::node_id, node_idbuffer);
        static char rel_idbuffer[20];
        sprintf(rel_idbuffer, "%ld", relation.id());
        feature.set_field(FieldIndexes::rel_id, rel_idbuffer);
        feature.set_field(FieldIndexes::name, relation.get_value_by_key("name"));
        feature.set_field(FieldIndexes::ref, relation.get_value_by_key("ref"));
        feature.set_field(FieldIndexes::from, relation.get_value_by_key("from"));
        feature.set_field(FieldIndexes::to, relation.get_value_by_key("to"));
        feature.set_field(FieldIndexes::via, relation.get_value_by_key("via"));
        feature.set_field(FieldIndexes::route, relation.get_value_by_key("route"));
        feature.set_field(FieldIndexes::name, relation.get_value_by_key("name"));
        feature.set_field(ErrorFieldIndexes::error, error_text);
        feature.add_to_layer();
    } catch (osmium::geometry_error& err) {
        m_verbose_output << err.what() << '\n';
    }
}
#endif

void RouteWriter::write_error_point(const osmium::Relation& relation, const osmium::NodeRef* node_ref,
        const char* error_text, const osmium::object_id_type way_id) {
    write_error_point(relation, node_ref->ref(), node_ref->location(), error_text, way_id);
}

#ifdef TEST_NO_ERROR_WRITING
void RouteWriter::write_error_point(const osmium::Relation&, const osmium::object_id_type,
        const osmium::Location&, const char*, const osmium::object_id_type ) {}
#else
void RouteWriter::write_error_point(const osmium::Relation& relation, const osmium::object_id_type node_ref,
        const osmium::Location& location, const char* error_text, const osmium::object_id_type way_id) {
    gdalcpp::Feature feature(m_ptv2_error_points, m_factory.create_point(location));
    static char way_idbuffer[20];
    sprintf(way_idbuffer, "%ld", way_id);
    feature.set_field(ErrorFieldIndexes::way_id, way_idbuffer);
    static char node_idbuffer[20];
    sprintf(node_idbuffer, "%ld", node_ref);
    feature.set_field(ErrorFieldIndexes::node_id, node_idbuffer);
    static char rel_idbuffer[20];
    sprintf(rel_idbuffer, "%ld", relation.id());
    feature.set_field(FieldIndexes::rel_id, rel_idbuffer);
    feature.set_field(FieldIndexes::name, relation.get_value_by_key("name"));
    feature.set_field(FieldIndexes::ref, relation.get_value_by_key("ref"));
    feature.set_field(FieldIndexes::from, relation.get_value_by_key("from"));
    feature.set_field(FieldIndexes::to, relation.get_value_by_key("to"));
    feature.set_field(FieldIndexes::via, relation.get_value_by_key("via"));
    feature.set_field(FieldIndexes::route, relation.get_value_by_key("route"));
    feature.set_field(FieldIndexes::name, relation.get_value_by_key("name"));
    feature.set_field(ErrorFieldIndexes::error, error_text);
    feature.add_to_layer();
}
#endif

void RouteWriter::write_error_object(const osmium::Relation& relation, const osmium::OSMObject* object,
        const osmium::object_id_type node_id, const char* error_text) {
    if (!object) {
        return;
    }
    switch (object->type()) {
    case osmium::item_type::node: {
        const osmium::Node* node = static_cast<const osmium::Node*>(object);
        write_error_point(relation, node->id(), node->location(), error_text, 0);
        break;
    }
    case osmium::item_type::way: {
        const osmium::Way* way = static_cast<const osmium::Way*>(object);
        write_error_way(relation, node_id, error_text, way);
        break;
    }
    default:
        break;
    }
}
