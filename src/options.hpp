/*
 * options.hpp
 *
 *  Created on:  2017-09-20
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_OPTIONS_HPP_
#define SRC_OPTIONS_HPP_

struct Options {
    std::string location_index_type = "sparse_mem_array";
    std::string output_format = "SQlite";
    std::string output_directory = "";
    bool verbose = false;
    bool crossings = true;
    bool platforms = true;
    bool points = true;
    bool railway_details = true;
    bool stations = true;
    bool stops = true;
};



#endif /* SRC_OPTIONS_HPP_ */
