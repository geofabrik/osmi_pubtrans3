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


#include "ogr_output_base.hpp"

OGROutputBase::OGROutputBase(OGRWriter& writer, osmium::util::VerboseOutput& verbose_output, Options& options) :
        m_writer(writer),
#ifndef ONLYMERCATOROUTPUT
        m_factory(osmium::geom::Projection(options.srs)),
#endif
        m_verbose_output(verbose_output),
        m_options(options) { }

OGRWriter& OGROutputBase::writer() {
    return m_writer;
}

const Options& OGROutputBase::options() {
    return m_options;
}

ogr_factory_type& OGROutputBase::factory() {
    return m_factory;
}

osmium::util::VerboseOutput& OGROutputBase::verbose_output() {
    return m_verbose_output;
}
