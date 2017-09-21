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
    int srs = 3857;
    bool verbose = false;
    bool crossings = true;
    bool points = true;
    bool railway_details = true;
};



#endif /* SRC_OPTIONS_HPP_ */
