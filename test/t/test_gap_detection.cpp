/*
 * test_node_handler.cpp
 *
 *  Created on: 13.07.2016
 *      Author: michael
 */

#include "catch.hpp"
#include "object_builder_utilities.hpp"

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <gdalcpp.hpp>
#include <ptv2_checker.hpp>

bool file_exists(std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

static osmium::item_type NODE = osmium::item_type::node;
static osmium::item_type WAY = osmium::item_type::way;
static osmium::item_type RELATION = osmium::item_type::relation;


TEST_CASE("check if gap detection works") {
    Options options;
    options.output_directory = ".tmp-";
    options.output_format = "GeoJSON";
    srand (time(NULL));
    options.output_directory += std::to_string(rand());
    options.output_directory += "-testoutput.sqlite";
    if (file_exists(options.output_directory)) {
        std::cerr << options.output_directory << " already exists!\n";
        exit(1);
    }
    if (mkdir(options.output_directory.c_str(), 0744) != 0) {
        std::cerr << "Failed to create directory " << options.output_directory << '\n';
        exit(1);
    }

    osmium::util::VerboseOutput vout {false};
    OGRWriter ogr_writer{options, vout};
    RouteWriter writer (ogr_writer, options, vout);
    PTv2Checker checker(writer);

    SECTION("simple tests") {
        std::vector<osmium::item_type> types = {NODE, NODE, WAY, WAY, WAY};
        std::vector<osmium::object_id_type> ids = {1, 2, 1, 2, 3};
        std::vector<std::string> roles = {"platform", "platform", "", "", ""};

        std::map<std::string, std::string> stop_pos;
        stop_pos.emplace("highway", "bus_stop");

        std::map<std::string, std::string> tags1;
        tags1.emplace("highway", "secondary");

        static constexpr int buffer_size = 10 * 1000 * 1000;
        osmium::memory::Buffer buffer(buffer_size);

        std::map<std::string, std::string> tags_rel = test_utils::get_bus_route_tags();

        SECTION("three ways, no gaps") {
            std::vector<const osmium::NodeRef*> node_refs1 {new osmium::NodeRef(1), new osmium::NodeRef(2), new osmium::NodeRef(3), new osmium::NodeRef(4)};
            std::vector<const osmium::NodeRef*> node_refs2 {new osmium::NodeRef(4), new osmium::NodeRef(5), new osmium::NodeRef(6), new osmium::NodeRef(7)};
            std::vector<const osmium::NodeRef*> node_refs3 {new osmium::NodeRef(7), new osmium::NodeRef(8), new osmium::NodeRef(9)};

            osmium::Way& way1 = test_utils::create_way(buffer, 1, node_refs1, tags1);
            buffer.commit();
            osmium::Way& way2 = test_utils::create_way(buffer, 2, node_refs2, tags1);
            buffer.commit();
            osmium::Way& way3 = test_utils::create_way(buffer, 3, node_refs3, tags1);
            buffer.commit();
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, &way1, &way2, &way3};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            CHECK(checker.find_gaps(relation1, objects) == 0);
        }

        SECTION("three ways, gaps between first and second") {
            std::vector<const osmium::NodeRef*> node_refs1 {new osmium::NodeRef(1), new osmium::NodeRef(2), new osmium::NodeRef(3), new osmium::NodeRef(4)};
            std::vector<const osmium::NodeRef*> node_refs2 {new osmium::NodeRef(5), new osmium::NodeRef(6), new osmium::NodeRef(7)};
            std::vector<const osmium::NodeRef*> node_refs3 {new osmium::NodeRef(7), new osmium::NodeRef(8), new osmium::NodeRef(9)};

            osmium::Way& way1 = test_utils::create_way(buffer, 1, node_refs1, tags1);
            buffer.commit();
            osmium::Way& way2 = test_utils::create_way(buffer, 2, node_refs2, tags1);
            buffer.commit();
            osmium::Way& way3 = test_utils::create_way(buffer, 3, node_refs3, tags1);
            buffer.commit();
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, &way1, &way2, &way3};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            CHECK(checker.find_gaps(relation1, objects) == 1);
        }

        SECTION("three ways, second way is a roundabout, no gap") {
            std::vector<const osmium::NodeRef*> node_refs1 {new osmium::NodeRef(1), new osmium::NodeRef(2), new osmium::NodeRef(3), new osmium::NodeRef(4)};
            std::vector<const osmium::NodeRef*> node_refs2 {new osmium::NodeRef(4), new osmium::NodeRef(5), new osmium::NodeRef(6), new osmium::NodeRef(7), new osmium::NodeRef(4)};
            std::vector<const osmium::NodeRef*> node_refs3 {new osmium::NodeRef(7), new osmium::NodeRef(8), new osmium::NodeRef(9)};

            std::map<std::string, std::string> tags_roundabout;
            tags_roundabout.emplace("highway", "secondary");
            tags_roundabout.emplace("junction", "roundabout");

            osmium::Way& way1 = test_utils::create_way(buffer, 1, node_refs1, tags1);
            buffer.commit();
            osmium::Way& way2 = test_utils::create_way(buffer, 2, node_refs2, tags_roundabout);
            buffer.commit();
            osmium::Way& way3 = test_utils::create_way(buffer, 3, node_refs3, tags1);
            buffer.commit();
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, &way1, &way2, &way3};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            CHECK(checker.find_gaps(relation1, objects) == 0);
        }

        SECTION("three ways, second way is a roundabout but not connected to anything") {
            std::vector<const osmium::NodeRef*> node_refs1 {new osmium::NodeRef(1), new osmium::NodeRef(2), new osmium::NodeRef(3), new osmium::NodeRef(4)};
            std::vector<const osmium::NodeRef*> node_refs2 {new osmium::NodeRef(12), new osmium::NodeRef(5), new osmium::NodeRef(6), new osmium::NodeRef(13), new osmium::NodeRef(12)};
            std::vector<const osmium::NodeRef*> node_refs3 {new osmium::NodeRef(7), new osmium::NodeRef(8), new osmium::NodeRef(9)};

            std::map<std::string, std::string> tags_roundabout;
            tags_roundabout.emplace("highway", "secondary");
            tags_roundabout.emplace("junction", "roundabout");

            osmium::Way& way1 = test_utils::create_way(buffer, 1, node_refs1, tags1);
            buffer.commit();
            osmium::Way& way2 = test_utils::create_way(buffer, 2, node_refs2, tags_roundabout);
            buffer.commit();
            osmium::Way& way3 = test_utils::create_way(buffer, 3, node_refs3, tags1);
            buffer.commit();
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, &way1, &way2, &way3};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            CHECK(checker.find_gaps(relation1, objects) == 2);
        }

        SECTION("three ways, first way is a roundabout but not connected to anything") {
            std::vector<const osmium::NodeRef*> node_refs1 {new osmium::NodeRef(1), new osmium::NodeRef(2), new osmium::NodeRef(3), new osmium::NodeRef(1)};
            std::vector<const osmium::NodeRef*> node_refs2 {new osmium::NodeRef(4), new osmium::NodeRef(5), new osmium::NodeRef(6), new osmium::NodeRef(13), new osmium::NodeRef(7)};
            std::vector<const osmium::NodeRef*> node_refs3 {new osmium::NodeRef(7), new osmium::NodeRef(8), new osmium::NodeRef(9)};

            std::map<std::string, std::string> tags_roundabout;
            tags_roundabout.emplace("highway", "secondary");
            tags_roundabout.emplace("junction", "roundabout");

            osmium::Way& way1 = test_utils::create_way(buffer, 1, node_refs1, tags1);
            buffer.commit();
            osmium::Way& way2 = test_utils::create_way(buffer, 2, node_refs2, tags_roundabout);
            buffer.commit();
            osmium::Way& way3 = test_utils::create_way(buffer, 3, node_refs3, tags1);
            buffer.commit();
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, &way1, &way2, &way3};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            CHECK(checker.find_gaps(relation1, objects) == 1);
        }
    }

    SECTION("advanced tests with 6 ways") {
        std::vector<osmium::item_type> types = {NODE, NODE, WAY, WAY, WAY, WAY, WAY, WAY};
        std::vector<std::string> roles = {"platform", "platform", "", "", "", "", "", ""};

        std::map<std::string, std::string> stop_pos;
        stop_pos.emplace("highway", "bus_stop");

        std::map<std::string, std::string> tags1;
        tags1.emplace("highway", "secondary");

        static constexpr int buffer_size = 10 * 1000 * 1000;
        osmium::memory::Buffer buffer(buffer_size);

        std::map<std::string, std::string> tags_rel = test_utils::get_bus_route_tags();

        std::vector<const osmium::NodeRef*> node_refs1 {new osmium::NodeRef(1), new osmium::NodeRef(2), new osmium::NodeRef(3), new osmium::NodeRef(4)};
        std::vector<const osmium::NodeRef*> node_refs2 {new osmium::NodeRef(4), new osmium::NodeRef(5), new osmium::NodeRef(6), new osmium::NodeRef(13), new osmium::NodeRef(7)};
        std::vector<const osmium::NodeRef*> node_refs3 {new osmium::NodeRef(7), new osmium::NodeRef(8), new osmium::NodeRef(9)};
        std::vector<const osmium::NodeRef*> node_refs4 {new osmium::NodeRef(9), new osmium::NodeRef(10), new osmium::NodeRef(11)};
        std::vector<const osmium::NodeRef*> node_refs5 {new osmium::NodeRef(13), new osmium::NodeRef(12), new osmium::NodeRef(11)};

        osmium::Way& way1 = test_utils::create_way(buffer, 1, node_refs1, tags1);
        buffer.commit();
        osmium::Way& way2 = test_utils::create_way(buffer, 2, node_refs2, tags1);
        buffer.commit();
        osmium::Way& way3 = test_utils::create_way(buffer, 3, node_refs3, tags1);
        buffer.commit();
        osmium::Way& way4 = test_utils::create_way(buffer, 4, node_refs4, tags1);
        buffer.commit();
        osmium::Way& way5 = test_utils::create_way(buffer, 5, node_refs5, tags1);
        buffer.commit();

        SECTION("first and second way are the same – this is valid") {
            std::vector<osmium::object_id_type> ids = {1, 2, 1, 1, 2, 3, 4, 5};
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, &way1, &way1, &way2, &way3, &way4, &way5};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            CHECK(checker.find_gaps(relation1, objects) == 0);
        }

        SECTION("first and second way are the same and only two nodes long, first/second way pointing away from the route – this is valid") {
            std::vector<osmium::object_id_type> ids = {1, 2, 6, 6, 2, 3, 4, 5};
            std::vector<const osmium::NodeRef*> node_refs6 {new osmium::NodeRef(4), new osmium::NodeRef(1)};
            osmium::Way& way6 = test_utils::create_way(buffer, 6, node_refs6, tags1);
            buffer.commit();
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, &way6, &way6, &way2, &way3, &way4, &way5};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            CHECK(checker.find_gaps(relation1, objects) == 0);
        }

        SECTION("first and second way are the same and only two nodes long, first/second way pointing to the route – this is valid") {
            std::vector<osmium::object_id_type> ids = {1, 2, 6, 6, 2, 3, 4, 5};
            std::vector<const osmium::NodeRef*> node_refs6 {new osmium::NodeRef(1), new osmium::NodeRef(4)};
            osmium::Way& way6 = test_utils::create_way(buffer, 6, node_refs6, tags1);
            buffer.commit();
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, &way6, &way6, &way2, &way3, &way4, &way5};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            CHECK(checker.find_gaps(relation1, objects) == 0);
        }

        SECTION("fifth and sixth way not ordered properly in the member list") {
            std::vector<osmium::object_id_type> ids = {1, 2, 1, 2, 3, 4, 7, 5};
            std::vector<const osmium::NodeRef*> node_refs7 {new osmium::NodeRef(13), new osmium::NodeRef(14)};
            osmium::Way& way7 = test_utils::create_way(buffer, 7, node_refs7, tags1);
            buffer.commit();
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, &way1, &way2, &way3, &way4, &way5, &way7};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            CHECK(checker.find_gaps(relation1, objects) == 0);
        }

        SECTION("fifth and sixth are diverging branches of the route") {
            std::vector<osmium::object_id_type> ids = {1, 2, 1, 2, 3, 4, 5, 8};
            std::vector<const osmium::NodeRef*> node_refs8 {new osmium::NodeRef(11), new osmium::NodeRef(14)};
            osmium::Way& way8 = test_utils::create_way(buffer, 8, node_refs8, tags1);
            buffer.commit();
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, &way1, &way2, &way3, &way4, &way5, &way8};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            CHECK(checker.find_gaps(relation1, objects) == 1);
        }
    }

    SECTION("roundabout test") {
        std::vector<osmium::item_type> types = {NODE, NODE, NODE, WAY, WAY, WAY, WAY};
        std::vector<std::string> roles = {"platform", "platform", "platform", "", "", "", ""};

        std::map<std::string, std::string> stop_pos;
        stop_pos.emplace("highway", "bus_stop");

        std::map<std::string, std::string> tags1;
        tags1.emplace("highway", "secondary");

        std::map<std::string, std::string> tags_roundabout;
        tags_roundabout.emplace("highway", "secondary");
        tags_roundabout.emplace("junction", "roundabout");

        static constexpr int buffer_size = 10 * 1000 * 1000;
        osmium::memory::Buffer buffer(buffer_size);

        std::map<std::string, std::string> tags_rel = test_utils::get_bus_route_tags();

        std::vector<const osmium::NodeRef*> node_refs1 {new osmium::NodeRef(1), new osmium::NodeRef(2), new osmium::NodeRef(3), new osmium::NodeRef(4)};
        std::vector<const osmium::NodeRef*> node_refs2 {new osmium::NodeRef(4), new osmium::NodeRef(5), new osmium::NodeRef(6), new osmium::NodeRef(13), new osmium::NodeRef(7)};
        std::vector<const osmium::NodeRef*> node_refs3 {new osmium::NodeRef(7), new osmium::NodeRef(8), new osmium::NodeRef(9), new osmium::NodeRef(10), new osmium::NodeRef(11), new osmium::NodeRef(12), new osmium::NodeRef(7)};
        std::vector<const osmium::NodeRef*> node_refs3a {new osmium::NodeRef(8), new osmium::NodeRef(9), new osmium::NodeRef(10), new osmium::NodeRef(11), new osmium::NodeRef(12), new osmium::NodeRef(7), new osmium::NodeRef(8)};

        osmium::Way& way1 = test_utils::create_way(buffer, 1, node_refs1, tags1);
        buffer.commit();
        osmium::Way& way2 = test_utils::create_way(buffer, 2, node_refs2, tags1);
        buffer.commit();
        osmium::Way& way3 = test_utils::create_way(buffer, 3, node_refs3, tags_roundabout);
        buffer.commit();
        osmium::Way& way3a = test_utils::create_way(buffer, 4, node_refs3a, tags_roundabout);
        buffer.commit();

        SECTION("route makes u-turn at roundabout") {
            std::vector<osmium::object_id_type> ids = {1, 2, 3, 1, 2, 3, 2};
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, nullptr, &way1, &way2, &way3, &way2};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            CHECK(checker.find_gaps(relation1, objects) == 0);
        }

        SECTION("route makes u-turn at roundabout but roundabout does not begin/end at entry point") {
            std::vector<osmium::object_id_type> ids = {1, 2, 3, 1, 2, 4, 2};
            std::vector<const osmium::OSMObject*> objects {nullptr, nullptr, nullptr, &way1, &way2, &way3a, &way2};

            osmium::Relation& relation1 = test_utils::create_relation(buffer, 1, tags_rel, ids, types, roles, objects);
            CHECK(checker.find_gaps(relation1, objects) == 0);
        }
    }

    if (test_utils::delete_directory(options.output_directory.c_str()) != 0) {
        std::cerr << " deleting " << options.output_directory << " after running the unit test failed!\n";
        exit(1);
    }
}
