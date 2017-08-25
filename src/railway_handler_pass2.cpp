/*
 * railway_handler_pass2.cpp
 *
 *  Created on:  2017-08-22
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "railway_handler_pass2.hpp"

RailwayHandlerPass2::RailwayHandlerPass2(osmium::index::IdSetDense<osmium::unsigned_object_id_type>& via_nodes,
        std::unordered_map<osmium::object_id_type, osmium::ItemStash::handle_type>& must_on_track_handles,
        osmium::ItemStash& must_on_track, gdalcpp::Dataset& dataset, std::string& output_format,
        osmium::util::VerboseOutput& verbose_output, int epsg /*= 3857*/) :
        AbstractViewHandler(dataset, output_format, verbose_output, epsg),
        m_must_on_track_handles(must_on_track_handles),
        m_must_on_track(must_on_track),
        m_via_nodes(via_nodes),
        m_points(dataset, "points", wkbPoint, GDAL_DEFAULT_OPTIONS),
        m_on_track(dataset, "on_track", wkbPoint, GDAL_DEFAULT_OPTIONS) {
    // add fields to layers
    m_points.add_field("node_id", OFTString, 10);
    m_points.add_field("lastchange", OFTString, 21);
    m_points.add_field("type", OFTString, 50);
    m_points.add_field("ref", OFTString, 50);
    m_on_track.add_field("node_id", OFTString, 10);
    m_on_track.add_field("lastchange", OFTString, 21);
    m_on_track.add_field("type", OFTString, 21);
    m_on_track.add_field("error", OFTString, 21);
}

void RailwayHandlerPass2::node(const osmium::Node& node) {
    const char* railway = node.get_value_by_key("railway");
    if (railway && !strcmp(railway, "switch")) {
        handle_point(node);
    }
}

void RailwayHandlerPass2::handle_point(const osmium::Node& node) {
    gdalcpp::Feature feature(m_points, m_factory.create_point(node));
    static char idbuffer[20];
    sprintf(idbuffer, "%ld", node.id());
    feature.set_field("node_id", idbuffer);
    std::string the_timestamp (node.timestamp().to_iso());
    feature.set_field("lastchange", the_timestamp.c_str());

    const char* switch_type = node.get_value_by_key("railway:switch");
    if (switch_type && (!strcmp(switch_type, "default") || !strcmp(switch_type, "double_slip"))) {
        feature.set_field("type", switch_type);
    } else if (switch_type && !strcmp(switch_type, "single_slip")) {
        if (m_via_nodes.get(static_cast<osmium::unsigned_object_id_type>(node.id()))) {
            feature.set_field("type", "single_slip");
        } else {
            feature.set_field("type", "single_slip_incomplete");
        }
    } else if (switch_type) {
        feature.set_field("type", "UNKNOWN_VALUE");
    } else {
        feature.set_field("type", "");
    }

    const char* ref = node.get_value_by_key("ref", "");
    feature.set_field("ref", ref);
    feature.add_to_layer();
}

void RailwayHandlerPass2::way(const osmium::Way& way) {
    for (const osmium::NodeRef& nd_ref : way.nodes()) {
        // look for handle
        auto it = m_must_on_track_handles.find(nd_ref.ref());
        if (it == m_must_on_track_handles.end()) {
            continue;
        }
        // remove from item stash
        m_must_on_track.remove_item(it->second);
        m_must_on_track_handles.erase(it);
    }
}

void RailwayHandlerPass2::after_ways() {
    for (auto it = m_must_on_track_handles.begin(); it != m_must_on_track_handles.end(); ++it) {
        // get node by handle
        const osmium::Node& node = static_cast<const osmium::Node&>(m_must_on_track.get_item(it->second));
        const char* railway = node.get_value_by_key("railway");
        if (!railway) {
            continue;
        }
        if (!node.location().valid()) {
            continue;
        }
        gdalcpp::Feature feature(m_on_track, m_factory.create_point(node));
        static char idbuffer[20];
        sprintf(idbuffer, "%ld", node.id());
        feature.set_field("node_id", idbuffer);
        std::string the_timestamp (node.timestamp().to_iso());
        feature.set_field("lastchange", the_timestamp.c_str());
        feature.set_field("error", "not on track");
        feature.set_field("type", railway);
        feature.add_to_layer();
    }
}

void RailwayHandlerPass2::relation(const osmium::Relation&) {}