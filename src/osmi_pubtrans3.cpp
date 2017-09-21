/*
 * osmi_pubtrans3.cpp
 *
 *  Created on:  2017-08-21
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include <string>
#include <iostream>
#include <getopt.h>

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_collector.hpp>
// the indexes themselves have to be included first
#include <osmium/index/map/dense_mmap_array.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>
#include <osmium/index/map/dense_mem_array.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/relations/manager_util.hpp>
#include <osmium/visitor.hpp>

#include "railway_handler_pass1.hpp"
#include "turn_restriction_handler.hpp"
#include "railway_handler_pass2.hpp"
#include "route_manager.hpp"

using index_type = osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

void print_help(char* arg0) {
    std::cerr << "Usage: " << arg0 << " [OPTIONS] INFILE OUTFILE\n" \
              << "General Options:\n" \
              << "  -h, --help           This help message.\n" \
              << "  -f, --format         Output format (default: SQlite)\n" \
              << "  -i, --index          Set index type for location index (default: sparse_mem_array)\n";
#ifndef ONLYMERCATOROUTPUT
    std::cerr << "  -s EPSG, --srs=ESPG  Output projection (EPSG code) (default: 3857)\n";
#endif
    std::cerr << "  -v, --verbose        Verbose output\n" \
              << "\n" \
              << "Content Related Options:\n" \
              << "--no-crossings        Don't write the crossings layer.\n" \
              << "--no-platforms        Don't write the platforms layer.\n" \
              << "--no-points           Don't write a layer of points (railway=switch).\n" \
              << "--no-railway-details  Don't check if signals, buffer stops, milestones etc.\n" \
              << "--no-stations         Don't write the stations layer.\n" \
              << "--no-stops            Don't write the stops layer.\n" \
              << "                      are mapped on the way which represents the track.\n";
}

int main(int argc, char* argv[]) {

    const int NO_CROSSINGS = 1000;
    const int NO_PLATFORMS = 1001;
    const int NO_POINTS = 1002;
    const int NO_RAILWAY_DETAILS = 1003;
    const int NO_STOPS = 1004;
    const int NO_STATIONS = 1005;

    static struct option long_options[] = {
        {"no-crossings",   no_argument, 0, NO_CROSSINGS},
        {"help",   no_argument, 0, 'h'},
        {"format", required_argument, 0, 'f'},
        {"index", required_argument, 0, 'i'},
        {"no-platforms",   no_argument, 0, NO_PLATFORMS},
        {"no-points",   no_argument, 0, NO_POINTS},
        {"no-railway-details",   no_argument, 0, NO_RAILWAY_DETAILS},
        {"no-stations",   no_argument, 0, NO_STATIONS},
        {"no-stops",   no_argument, 0, NO_STOPS},
        {"srs", required_argument, 0, 's'},
        {"verbose",   no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    Options options;

    while (true) {
        int c = getopt_long(argc, argv, "hf:i:s:v", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                print_help(argv[0]);
                exit(1);
            case 'f':
                if (optarg) {
                    options.output_format = optarg;
                } else {
                    print_help(argv[0]);
                    exit(1);
                }
                break;
            case 'i':
                if (optarg) {
                    options.location_index_type = optarg;
                } else {
                    print_help(argv[0]);
                    exit(1);
                }
                break;
            case 's':
#ifdef ONLYMERCATOROUTPUT
                std::cerr << "ERROR: Usage of output projections other than " \
                        "EPSG:3857 is not compiled into this binary.\n";
                print_help(argv[0]);
                exit(1);
#else
                if (optarg) {
                    options.srs = atoi(optarg);
                } else {
                    print_help(argv[0]);
                    exit(1);
                }
#endif
                break;
            case NO_CROSSINGS:
                options.crossings = false;
                break;
            case NO_RAILWAY_DETAILS:
                options.railway_details = false;
                break;
            case NO_POINTS:
                options.points = false;
                break;
            case NO_STOPS:
                options.stops = false;
                break;
            case NO_PLATFORMS:
                options.platforms = false;
                break;
            case NO_STATIONS:
                options.stations = false;
                break;
            case 'v':
                options.verbose = true;
                break;
            default:
                print_help(argv[0]);
                exit(1);
        }
    }

    std::string input_filename;
    std::string output_filename;
    int remaining_args = argc - optind;
    if (remaining_args == 2) {
        input_filename =  argv[optind];
        output_filename = argv[optind+1];
    } else if (remaining_args == 1) {
        input_filename =  argv[optind];
    } else {
        input_filename = "-";
    }

    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();

    osmium::util::VerboseOutput verbose_output(options.verbose);
    gdalcpp::Dataset dataset(options.output_format, output_filename, gdalcpp::SRS(options.srs),
            OGROutputBase::get_gdal_default_options(options.output_format));

    RouteManager route_manager(dataset, options, verbose_output);

    {
        verbose_output << "Pass 1 (reading route relations) ...";
        osmium::io::File input_file(input_filename);
        osmium::relations::read_relations(input_file, route_manager);
        verbose_output << " done\n";
    }

    osmium::index::IdSetDense<osmium::unsigned_object_id_type> point_node_members;

    // This ItemStash collects the IDs of all nodes which are expected to be reference by a way because their tags require it.
    // Examples: points, signals, stop positions
    osmium::ItemStash must_on_track;
    std::unordered_map<osmium::object_id_type, osmium::ItemStash::handle_type> must_on_track_handles;
    {
        auto location_index = map_factory.create_map(options.location_index_type);
        location_handler_type location_handler(*location_index);
        RailwayHandlerPass1 railway_handler1(dataset, options, verbose_output, must_on_track, must_on_track_handles);

        verbose_output << "Pass 2 ...";
        osmium::io::Reader reader1(input_filename);
        if (options.points) {
            TurnRestrictionHandler tr_handler(point_node_members);
            osmium::apply(reader1, location_handler, railway_handler1, tr_handler, route_manager.handler());
        } else {
            osmium::apply(reader1, location_handler, railway_handler1, route_manager.handler());
        }
        route_manager.for_each_incomplete_relation([&](const osmium::relations::RelationHandle& handle){
            route_manager.process_route(*handle);
        });
        verbose_output << " done\n";

        reader1.close();
    }

    RailwayHandlerPass2 railway_handler2(point_node_members, must_on_track_handles, must_on_track, dataset, options, verbose_output);
    verbose_output << "Pass 3 ...";
    osmium::io::Reader reader2(input_filename, osmium::osm_entity_bits::node | osmium::osm_entity_bits::way);
    osmium::apply(reader2, railway_handler2);
    railway_handler2.after_ways();
    must_on_track.clear();
    must_on_track.garbage_collect();
    verbose_output << " done\n";
    verbose_output << "wrote output to " << output_filename << "\n";
}
