/*
 * railway_handler_pass1.cpp
 *
 *  Created on:  2017-08-21
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "railway_handler_pass1.hpp"
#include <iostream>

RailwayHandlerPass1::RailwayHandlerPass1(gdalcpp::Dataset& dataset, Options& options,
        osmium::util::VerboseOutput& verbose_output, osmium::ItemStash& signals,
        std::unordered_map<osmium::object_id_type, osmium::ItemStash::handle_type>& must_on_track_handles) :
        AbstractViewHandler(dataset, options, verbose_output),
        m_must_on_track(signals),
        m_must_on_track_handles(must_on_track_handles),
        m_options(options) {
    if (options.crossings) {
        m_crossings.reset(new gdalcpp::Layer(dataset, "crossings", wkbPoint, GDAL_DEFAULT_OPTIONS));
        // add fields to layers
        m_crossings->add_field("node_id", OFTString, 10);
        m_crossings->add_field("lastchange", OFTString, 21);
        m_crossings->add_field("barrier", OFTString, 50);
        m_crossings->add_field("lights", OFTString, 50);
    }
}

void RailwayHandlerPass1::node(const osmium::Node& node) {
    if (!node.location().valid()) {
        return;
    }
    const char* railway = node.get_value_by_key("railway");
    const char* public_transport = node.get_value_by_key("public_transport");
    if (m_options.railway_details && railway &&
            (!strcmp(railway, "signal") || !strcmp(railway, "stop")
             || !strcmp(railway, "buffer_stop") || !strcmp(railway, "level_crossing")
             || !strcmp(railway, "milestone") || !strcmp(railway, "derail")
             || !strcmp(railway, "isolated_track_section") || !strcmp(railway, "switch")
             || !strcmp(railway, "railway_crossing"))) {
        m_must_on_track_handles.emplace(node.id(), m_must_on_track.add_item(node));
    } else if (public_transport && !strcmp(public_transport, "stop_position")) {
        m_must_on_track_handles.emplace(node.id(), m_must_on_track.add_item(node));
    }
    if (railway && m_options.crossings && (!strcmp(railway, "level_crossing") || !strcmp(railway, "crossing"))) {
        handle_crossing(node);
    }
}

void RailwayHandlerPass1::handle_crossing(const osmium::Node& node) {
    assert(m_options.crossings);
    const char* barrier = node.get_value_by_key("crossing:barrier");
    const char* lights = node.get_value_by_key("crossing:light");
    std::string barrier_value;
    std::string lights_value;
    if (!barrier) {
        barrier_value = "NONE";
    } else if (barrier && strcmp(barrier, "no") && strcmp(barrier, "yes")
            && strcmp(barrier, "half") && strcmp(barrier, "double_half")
            && strcmp(barrier, "full") && strcmp(barrier, "gates")) {
        barrier_value = "UNKNOWN";
    } else {
        barrier_value = barrier;
    }
    if (!lights) {
        lights_value = "NONE";
    } else if (lights && strcmp(lights, "yes") && strcmp(lights, "no")) {
        lights_value = "UNKNOWN";
    } else {
        lights_value = lights;
    }
    add_error_node(node, "barrier", barrier_value.c_str(), "lights", lights_value.c_str());
}

void RailwayHandlerPass1::add_error_node(const osmium::Node& node, const char* third_field_name,
        const char* third_field_value, const char* fourth_field_name, const char* fourth_field_value) {
    gdalcpp::Feature feature(*m_crossings, m_factory.create_point(node));
    static char idbuffer[20];
    sprintf(idbuffer, "%ld", node.id());
    feature.set_field("node_id", idbuffer);
    std::string the_timestamp (node.timestamp().to_iso());
    feature.set_field("lastchange", the_timestamp.c_str());
    if (third_field_name && third_field_value) {
        feature.set_field(third_field_name, third_field_value);
    }
    if (fourth_field_name && fourth_field_value) {
        feature.set_field(fourth_field_name, fourth_field_value);
    }
    feature.add_to_layer();
}

void RailwayHandlerPass1::way(const osmium::Way&) {}

void RailwayHandlerPass1::relation(const osmium::Relation&) {}
