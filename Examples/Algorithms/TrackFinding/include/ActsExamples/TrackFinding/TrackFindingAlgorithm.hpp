// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/Track.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/VectorTrackContainer.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/TrackFinding/CombinatorialKalmanFilter.hpp"
#include "Acts/TrackFinding/MeasurementSelector.hpp"
#include "Acts/TrackFinding/SourceLinkAccessorConcept.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/MagneticField/MagneticField.hpp"

#include <atomic>
#include <functional>
#include <vector>

#include <tbb/combinable.h>

namespace ActsExamples {

class TrackFindingAlgorithm final : public IAlgorithm {
 public:
  /// Track finder function that takes input measurements, initial trackstate
  /// and track finder options and returns some track-finder-specific result.
  using TrackFinderOptions =
      Acts::CombinatorialKalmanFilterOptions<IndexSourceLinkAccessor::Iterator,
                                             Acts::VectorMultiTrajectory>;
  using TrackFinderResult =
      Acts::Result<std::vector<TrackContainer::TrackProxy>>;

  /// Find function that takes the above parameters
  /// @note This is separated into a virtual interface to keep compilation units
  /// small
  class TrackFinderFunction {
   public:
    virtual ~TrackFinderFunction() = default;
    virtual TrackFinderResult operator()(const TrackParameters&,
                                         const TrackFinderOptions&,
                                         TrackContainer&) const = 0;
  };

  /// Create the track finder function implementation.
  ///
  /// The magnetic field is intentionally given by-value since the variant
  /// contains shared_ptr anyways.
  static std::shared_ptr<TrackFinderFunction> makeTrackFinderFunction(
      std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry,
      std::shared_ptr<const Acts::MagneticFieldProvider> magneticField);

  struct Config {
    /// Input measurements collection.
    std::string inputMeasurements;
    /// Input source links collection.
    std::string inputSourceLinks;
    /// Input initial track parameter estimates for for each proto track.
    std::string inputInitialTrackParameters;
    /// Output find trajectories collection.
    std::string outputTracks;
    /// Type erased track finder function.
    std::shared_ptr<TrackFinderFunction> findTracks;
    /// CKF measurement selector config
    Acts::MeasurementSelector::Config measurementSelectorCfg;
    /// Compute shared hit information
    bool computeSharedHits = false;
  };

  /// Constructor of the track finding algorithm
  ///
  /// @param config is the config struct to configure the algorithm
  /// @param level is the logging level
  TrackFindingAlgorithm(Config config, Acts::Logging::Level level);

  /// Framework execute method of the track finding algorithm
  ///
  /// @param ctx is the algorithm context that holds event-wise information
  /// @return a process code to steer the algorithm flow
  ActsExamples::ProcessCode execute(
      const ActsExamples::AlgorithmContext& ctx) const final;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  template <typename source_link_accessor_container_t>
  void computeSharedHits(const source_link_accessor_container_t& sourcelinks,
                         TrackContainer& tracks) const;

  ActsExamples::ProcessCode finalize() override;

 private:
  Config m_cfg;

  mutable std::atomic<size_t> m_nTotalSeeds{0};
  mutable std::atomic<size_t> m_nFailedSeeds{0};

  mutable tbb::combinable<Acts::VectorMultiTrajectory::Statistics>
      m_memoryStatistics{[]() {
        auto mtj = std::make_shared<Acts::VectorMultiTrajectory>();
        return mtj->statistics();
      }};
};

// TODO this is somewhat duplicated in AmbiguityResolutionAlgorithm.cpp
// TODO we should make a common implementation in the core at some point
template <typename source_link_accessor_container_t>
void TrackFindingAlgorithm::computeSharedHits(
    const source_link_accessor_container_t& sourceLinks,
    TrackContainer& tracks) const {
  // Compute shared hits from all the reconstructed tracks
  // Compute nSharedhits and Update ckf results
  // hit index -> list of multi traj indexes [traj, meas]

  std::vector<std::size_t> firstTrackOnTheHit(
      sourceLinks.size(), std::numeric_limits<std::size_t>::max());
  std::vector<std::size_t> firstStateOnTheHit(
      sourceLinks.size(), std::numeric_limits<std::size_t>::max());

  for (auto track : tracks) {
    for (auto state : track.trackStates()) {
      if (not state.typeFlags().test(Acts::TrackStateFlag::MeasurementFlag)) {
        continue;
      }

      std::size_t hitIndex = state.uncalibratedSourceLink()
                                 .template get<IndexSourceLink>()
                                 .index();

      // Check if hit not already used
      if (firstTrackOnTheHit.at(hitIndex) ==
          std::numeric_limits<std::size_t>::max()) {
        firstTrackOnTheHit.at(hitIndex) = track.index();
        firstStateOnTheHit.at(hitIndex) = state.index();
        continue;
      }

      // if already used, control if first track state has been marked
      // as shared
      int indexFirstTrack = firstTrackOnTheHit.at(hitIndex);
      int indexFirstState = firstStateOnTheHit.at(hitIndex);

      auto firstState = tracks.getTrack(indexFirstTrack)
                            .container()
                            .trackStateContainer()
                            .getTrackState(indexFirstState);
      if (not firstState.typeFlags().test(
              Acts::TrackStateFlag::SharedHitFlag)) {
        firstState.typeFlags().set(Acts::TrackStateFlag::SharedHitFlag);
      }

      // Decorate this track state
      state.typeFlags().set(Acts::TrackStateFlag::SharedHitFlag);
    }
  }
}

}  // namespace ActsExamples
