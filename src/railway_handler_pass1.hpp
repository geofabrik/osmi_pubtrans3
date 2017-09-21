/*
 * railway_track_handler.hpp
 *
 *  Created on:  2017-08-21
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_RAILWAY_TRACK_HANDLER_HPP_
#define SRC_RAILWAY_TRACK_HANDLER_HPP_

#include <unordered_map>
#include <memory>

#include <osmium/storage/item_stash.hpp>

#include "abstract_view_handler.hpp"

/**
 * This handler class creates the level crossings layer and populates the map of nodes which have
 * to be referenced by a way because their tags require it (signals, points, stop positions etc.).
 */
class RailwayHandlerPass1 : public AbstractViewHandler {
    /// item stash holds OSM objects to all nodes which have to be referenced by a way
    osmium::ItemStash& m_must_on_track;

    /// The map holds the handles which point to the objects in the ItemStash and makes them accessible by their OSM ID.
    std::unordered_map<osmium::object_id_type, osmium::ItemStash::handle_type>& m_must_on_track_handles;

    /// GDAL layer for level crossings
    std::unique_ptr<gdalcpp::Layer> m_crossings;

    Options& m_options;

    void handle_crossing(const osmium::Node& node);

    void add_error_node(const osmium::Node& node, const char* third_field_name,
            const char* third_field_value, const char* fourth_field_name, const char* fourth_field_value);

public:
    RailwayHandlerPass1() = delete;

    RailwayHandlerPass1(gdalcpp::Dataset& dataset, Options& options,
            osmium::util::VerboseOutput& verbose_output, osmium::ItemStash& signals,
            std::unordered_map<osmium::object_id_type, osmium::ItemStash::handle_type>& must_on_track_handles);

    void node(const osmium::Node& node);

    static std::string tags_string(const osmium::TagList& tags);

    void way(const osmium::Way&);

    void relation(const osmium::Relation&);
};



#endif /* SRC_RAILWAY_TRACK_HANDLER_HPP_ */
