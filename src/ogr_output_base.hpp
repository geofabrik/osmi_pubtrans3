/*
 *  © 2017 Geofabrik GmbH
 *
 *  This file is part of osmi_simple_views.
 *
 *  osmi_simple_views is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  osmi_simple_views is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with osmi_simple_views. If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef SRC_OGR_OUTPUT_BASE_HPP_
#define SRC_OGR_OUTPUT_BASE_HPP_

#include <gdalcpp.hpp>

#include <osmium/geom/ogr.hpp>

#ifdef ONLYMERCATOROUTPUT
    #include <osmium/geom/mercator_projection.hpp>
#else
    #include <osmium/geom/projection.hpp>
#endif

#include <osmium/util/verbose_output.hpp>

#include "options.hpp"
#include "ogr_writer.hpp"

#ifdef ONLYMERCATOROUTPUT
    /// factory to build OGR geometries in Web Mercator projection
    using ogr_factory_type = osmium::geom::OGRFactory<osmium::geom::MercatorProjection>;
#else
    /// factory to build OGR geometries with a coordinate transformation if necessary
    using ogr_factory_type = osmium::geom::OGRFactory<osmium::geom::Projection>;
#endif

/**
 * Provide commont things for working with GDAL. This class does not care for the dataset
 * because the dataset is shared.
 */
class OGROutputBase {

protected:
    OGRWriter& m_writer;

    /**
     * If ONLYMERCATOROUTPUT is defined, output coordinates are always Web
     * Mercator coordinates. If it is not defined, we will transform them if
     * the output SRS is different from the input SRS (4326).
     */

    ogr_factory_type m_factory;

    /// reference to output manager for STDERR
    osmium::util::VerboseOutput& m_verbose_output;

    Options& m_options;

    /// maximum length of a string field
    static constexpr size_t MAX_FIELD_LENGTH = 254;

    static constexpr double UPPER_LIMIT_LATITUDE = 90.0;

public:
    OGROutputBase() = delete;

    OGROutputBase(OGRWriter& writer, osmium::util::VerboseOutput& verbose_output, Options& options);

    OGRWriter& writer();

    const Options& options();

    ogr_factory_type& factory();

    osmium::util::VerboseOutput& verbose_output();

    inline bool coordinates_valid(const osmium::Location& location) {
#ifdef ONLYMERCATOROUTPUT
        return location.valid() && location.lat() < UPPER_LIMIT_LATITUDE && location.lat() > -UPPER_LIMIT_LATITUDE;
#else
        return location.valid() && (
                   m_options.srs != 3857 ||
                   (location.lat() < UPPER_LIMIT_LATITUDE && location.lat() > -UPPER_LIMIT_LATITUDE)
               );
#endif
    }

    inline bool coordinates_valid(const osmium::Node& node) {
        return coordinates_valid(node.location());
    }

    inline bool coordinates_valid(const osmium::NodeRefList& nl) {
        for (auto& nr : nl) {
            if (!coordinates_valid(nr.location())) {
                return false;
            }
        }
        return true;
    }

    inline bool coordinates_valid(const osmium::Way& way) {
        return coordinates_valid(way.nodes());
    }

    inline bool coordinates_valid(const osmium::OSMObject& object) {
        switch (object.type()) {
        case osmium::item_type::node :
            return coordinates_valid(static_cast<const osmium::Node&>(object));
        case osmium::item_type::way :
            return coordinates_valid(static_cast<const osmium::Way&>(object));
        default :
            return false;
        }
    }
};

#endif /* SRC_OGR_OUTPUT_BASE_HPP_ */
