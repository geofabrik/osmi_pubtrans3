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
    std::string dataset_name = ".tmp-";
    srand (time(NULL));
    dataset_name += std::to_string(rand());
    dataset_name += "-testoutput.sqlite";
    if (file_exists(dataset_name)) {
        std::cerr << dataset_name << " already exists!\n";
        exit(1);
    }
    Options options;
    gdalcpp::Dataset dataset(options.output_format, dataset_name, gdalcpp::SRS(4326),
            OGROutputBase::get_gdal_default_options(options.output_format));
    osmium::util::VerboseOutput vout {false};
    RouteWriter writer (dataset, options, vout);
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
    }

    if (std::remove(dataset_name.c_str()) != 0) {
        std::cerr << " deleting " << dataset_name << " after running the unit test failed!\n";
        exit(1);
    }
}
