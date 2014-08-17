
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010-2013 Francois Beaune, Jupiter Jazz Limited
// Copyright (c) 2014 Francois Beaune, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Interface header.
#include "sppmparameters.h"

// appleseed.renderer headers.
#include "renderer/global/globallogger.h"
#include "renderer/utility/paramarray.h"

// appleseed.foundation headers.
#include "foundation/utility/string.h"

// Standard headers.
#include <string>

using namespace foundation;
using namespace std;

namespace renderer
{

namespace
{
    size_t nz(const size_t x)
    {
        return x == 0 ? ~0 : x;
    }

    SPPMParameters::Mode get_mode(
        const ParamArray&           params,
        const char*                 name,
        const SPPMParameters::Mode  default_mode)
    {
        const string default_mode_str =
            default_mode == SPPMParameters::SPPM ? "sppm" :
            default_mode == SPPMParameters::RayTraced ? "rt" : "off";

        const string value = params.get_optional<string>(name, default_mode_str);

        if (value == "sppm")
            return SPPMParameters::SPPM;
        else if (value == "rt")
            return SPPMParameters::RayTraced;
        else if (value == "off")
            return SPPMParameters::Off;
        else
        {
            RENDERER_LOG_ERROR(
                "invalid value \"%s\" for parameter \"%s\", using default value \"%s\"",
                value.c_str(),
                name,
                default_mode_str.c_str());
            return default_mode;
        }
    }
}

SPPMParameters::SPPMParameters(const ParamArray& params)
  : m_dl_mode(get_mode(params, "dl_mode", RayTraced))
  , m_enable_ibl(params.get_optional<bool>("enable_ibl", true))
  , m_enable_caustics(params.get_optional<bool>("enable_caustics", true))
  , m_light_photon_count(params.get_optional<size_t>("light_photons_per_pass", 1000000))
  , m_env_photon_count(params.get_optional<size_t>("env_photons_per_pass", 1000000))
  , m_photon_packet_size(params.get_optional<size_t>("photon_packet_size", 100000))
  , m_photon_tracing_max_path_length(nz(params.get_optional<size_t>("photon_tracing_max_path_length", 0)))
  , m_photon_tracing_rr_min_path_length(nz(params.get_optional<size_t>("photon_tracing_rr_min_path_length", 3)))
  , m_path_tracing_max_path_length(nz(params.get_optional<size_t>("path_tracing_max_path_length", 0)))
  , m_path_tracing_rr_min_path_length(nz(params.get_optional<size_t>("path_tracing_rr_min_path_length", 3)))
  , m_transparency_threshold(params.get_optional<float>("transparency_threshold", 0.001f))
  , m_max_iterations(params.get_optional<size_t>("max_iterations", 1000))
  , m_initial_radius_percents(params.get_required<float>("initial_radius", 0.1f))
  , m_alpha(params.get_optional<float>("alpha", 0.7f))
  , m_max_photons_per_estimate(params.get_optional<size_t>("max_photons_per_estimate", 100))
  , m_dl_light_sample_count(params.get_optional<double>("dl_light_samples", 1.0))
  , m_view_photons(params.get_optional<bool>("view_photons", false))
  , m_view_photons_radius(params.get_optional<float>("view_photons_radius", 1.0e-3f))
{
    // Precompute the reciprocal of the number of light samples.
    m_rcp_dl_light_sample_count =
        m_dl_light_sample_count > 0.0 && m_dl_light_sample_count < 1.0
            ? static_cast<float>(1.0 / m_dl_light_sample_count)
            : 0.0f;
}

void SPPMParameters::print() const
{
    RENDERER_LOG_INFO(
        "sppm settings:\n"
        "  dl               %s\n"
        "  ibl              %s",
        m_dl_mode == RayTraced ? "ray traced" :
        m_dl_mode == SPPM ? "sppm" : "off",
        m_enable_ibl ? "on" : "off");

    RENDERER_LOG_INFO(
        "sppm photon tracing settings:\n"
        "  light photons    %s\n"
        "  env. photons     %s\n"
        "  max path length  %s\n"
        "  rr min path len. %s",
        pretty_uint(m_light_photon_count).c_str(),
        pretty_uint(m_env_photon_count).c_str(),
        m_photon_tracing_max_path_length == ~0 ? "infinite" : pretty_uint(m_photon_tracing_max_path_length).c_str(),
        m_photon_tracing_rr_min_path_length == ~0 ? "infinite" : pretty_uint(m_photon_tracing_rr_min_path_length).c_str());

    RENDERER_LOG_INFO(
        "sppm path tracing settings:\n"
        "  max path length  %s\n"
        "  rr min path len. %s\n"
        "  initial radius   %s%%\n"
        "  alpha            %s\n"
        "  max photons/est. %s\n"
        "  dl light samples %s",
        m_path_tracing_max_path_length == ~0 ? "infinite" : pretty_uint(m_path_tracing_max_path_length).c_str(),
        m_path_tracing_rr_min_path_length == ~0 ? "infinite" : pretty_uint(m_path_tracing_rr_min_path_length).c_str(),
        pretty_scalar(m_initial_radius_percents, 3).c_str(),
        pretty_scalar(m_alpha, 1).c_str(),
        pretty_uint(m_max_photons_per_estimate).c_str(),
        pretty_scalar(m_dl_light_sample_count).c_str());
}

}   // namespace renderer
