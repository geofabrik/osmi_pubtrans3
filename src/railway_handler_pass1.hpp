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

#include <osmium/handler.hpp>
#include <osmium/storage/item_stash.hpp>

#include "ogr_output_base.hpp"

/**
 * This handler class creates the level crossings layer and populates the map of nodes which have
 * to be referenced by a way because their tags require it (signals, points, stop positions etc.).
 */
class RailwayHandlerPass1 : public osmium::handler::Handler {
    OGROutputBase m_output;

    /// item stash holds OSM objects to all nodes which have to be referenced by a way
    osmium::ItemStash& m_must_on_track;

    /// The map holds the handles which point to the objects in the ItemStash and makes them accessible by their OSM ID.
    std::unordered_map<osmium::object_id_type, osmium::ItemStash::handle_type>& m_must_on_track_handles;

    /// GDAL layer for level crossings
    std::unique_ptr<gdalcpp::Layer> m_crossings;

    /// GDAL layer for platforms
    std::unique_ptr<gdalcpp::Layer> m_platforms;
    std::unique_ptr<gdalcpp::Layer> m_platforms_l;

    /// GDAL layer for stations
    std::unique_ptr<gdalcpp::Layer> m_stations;
    std::unique_ptr<gdalcpp::Layer> m_stations_l;

    /// GDAL layer for stops
    std::unique_ptr<gdalcpp::Layer> m_stops;

    /// GDAL layer for stops/platforms which onyl have highway=bus_stop but no public_transport=*
    std::unique_ptr<gdalcpp::Layer> m_stops_only_highway;

    void handle_crossing(const osmium::Node& node);

    void add_crossing_node(const osmium::Node& node, const int third_field_index,
            const char* third_field_value, const int fourth_field_index, const char* fourth_field_value);

    void handle_stop(const osmium::OSMObject& object, const char* public_transport, const char* railway);

    void add_stop_pltf_node(gdalcpp::Layer& layer, const osmium::Node& node, bool refs, bool amenity);

    void add_stop_pltf_way(gdalcpp::Layer& layer, const osmium::Way& way, bool refs, bool amenity);

    static void set_fields(gdalcpp::Feature& feature, const osmium::OSMObject& object,
            bool refs, bool amenity);

    static void set_node_id(gdalcpp::Feature& feature, const osmium::Node& node);

    static void set_way_id(gdalcpp::Feature& feature, const osmium::Way& way);

public:

    RailwayHandlerPass1() = delete;

    RailwayHandlerPass1(OGRWriter& writer, Options& options, osmium::util::VerboseOutput& verbose_output,
            osmium::ItemStash& signals, std::unordered_map<osmium::object_id_type,
            osmium::ItemStash::handle_type>& must_on_track_handles);

    void node(const osmium::Node& node);

    void way(const osmium::Way&);

    void relation(const osmium::Relation&);
};



#endif /* SRC_RAILWAY_TRACK_HANDLER_HPP_ */
