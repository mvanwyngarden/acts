// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/SimSeed.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Acts {
class TrackingGeometry;
}

namespace ActsExamples {

/// Construct track seeds from particles.
class TruthSeedingAlgorithm final : public IAlgorithm {
 public:
  struct Config {
    /// The input truth particles that should be used for truth seeding.
    std::string inputParticles;
    /// The input hit-particles map collection.
    std::string inputMeasurementParticlesMap;
    /// Input source links collection.
    std::string inputSourceLinks;
    /// Input space point collections.
    ///
    /// We allow multiple space point collections to allow different parts of
    /// the detector to use different algorithms for space point construction,
    /// e.g. single-hit space points for pixel-like detectors or double-hit
    /// space points for strip-like detectors.
    std::vector<std::string> inputSpacePoints;
    /// Output successfully seeded truth particles.
    std::string outputParticles;
    /// Output full proto track collection.
    std::string outputFullProtoTracks;
    /// Output seed collection.
    std::string outputSeeds;
    /// Output proto track collection.
    std::string outputProtoTracks;
    /// Minimum deltaR between space points in a seed
    float deltaRMin = 1. * Acts::UnitConstants::mm;
    /// Maximum deltaR between space points in a seed
    float deltaRMax = 100. * Acts::UnitConstants::mm;
  };

  /// Construct the truth seeding algorithm.
  ///
  /// @param cfg is the algorithm configuration
  /// @param lvl is the logging level
  TruthSeedingAlgorithm(Config cfg, Acts::Logging::Level lvl);

  /// Run the truth seeding algorithm.
  ///
  /// @param ctx is the algorithm context with event information
  /// @return a process code indication success or failure
  ProcessCode execute(const AlgorithmContext& ctx) const override;

  /// Const access to the config
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;
};

}  // namespace ActsExamples
