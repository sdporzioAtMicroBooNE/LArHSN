#ifndef PRESELECTHSN_H
#define PRESELECTHSN_H

// c++ includes
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <exception>

// root includes
#include "TInterpreter.h"
#include "TROOT.h"
#include "TH1.h"
#include "TH2D.h"
#include "TH2I.h"
#include "TFile.h"
#include "TNtuple.h"
#include "TClonesArray.h"
#include "TCanvas.h"
#include "TGraph.h"

// framework includes
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "fhiclcpp/ParameterSet.h"

// art includes
#include "canvas/Utilities/InputTag.h"
#include "canvas/Persistency/Common/FindMany.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "canvas/Persistency/Common/FindOne.h"
#include "canvas/Persistency/Common/FindOneP.h"

// larsoft object includes
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/Shower.h"
#include "lardataobj/RecoBase/Vertex.h"
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Wire.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/TrackingTypes.h"
#include "lardataobj/RawData/RawDigit.h"
#include "larcore/Geometry/geo.h"
#include "larcore/Geometry/Geometry.h"
#include "larcore/CoreUtils/ServiceUtil.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"

// Auxiliary objects includes
#include "Algorithms/FindDecayVertexAlg.h"
#include "Algorithms/FindPandoraVertexAlg.h"
#include "Algorithms/CalorimetryRadiusAlg.h"
#include "DataObjects/DecayVertex.h"
#include "DataObjects/EventDescriptor.h"


// Analyzer class
class PreSelectHSN : public art::EDAnalyzer
{
public:
  explicit PreSelectHSN(fhicl::ParameterSet const & pset);
  virtual ~PreSelectHSN();
  void analyze(art::Event const & evt);
  void beginJob();
  void endJob();
private:
  // Algorithms
  FindDecayVertex::FindDecayVertexAlg fFindDecayVertexAlg;
  FindPandoraVertex::FindPandoraVertexAlg fFindPandoraVertexAlg;
  CalorimetryRadius::CalorimetryRadiusAlg fCalorimetryRadiusAlg;

  // Fhiclcpp variables
  std::string fInstanceName;
  int fIteration;
  std::vector<double> fMinTpcBound;
  std::vector<double> fMaxTpcBound;
  std::string fDataType;
  std::string fPfpLabel;
  std::string fHitLabel;
  int fDecayChannel;
  double fSterileMass;
  double fDistanceCut;
  std::vector<double> fRadiusProfileLimits;
  int fRadiusProfileBins;
  double fChannelNorm;
  double fTickNorm;
  bool fPrimaryOnly;
  bool fEndVerticesAlso;
  std::string fAnaType;
  bool fManualSearch;
  bool fPandoraSearch;
  bool fVerbose;
  bool fSaveDrawTree;

  // Declare services
  geo::GeometryCore const* fGeometry; // Pointer to the Geometry service
  detinfo::DetectorProperties const* fDetectorProperties; // Pointer to the Detector Properties

  // Declare trees
  TTree *metaTree;
  TTree *manualTree;
  TTree *manualDrawTree;
  TTree *pandoraTree;
  TTree *pandoraDrawTree;

  // Declare tree helpers
  AuxEvent::EventDescriptor evd;

  // Declare analysis variables
  std::vector<float> profileTicks, pairDistance, potPairDistance;
  std::vector<int> nTrackHits, nShowerHits;
  std::vector<std::vector<float>> totChargeInRadius, par1ChargeInRadius, par2ChargeInRadius, caloRatio; // For each dv in event (usually one) and for each radius
  std::vector<bool> tree_diagnostic_pathologicalVertex;

  // Declare pandora analysis variables
  std::vector<AuxVertex::DecayVertex> ana_pandora_decayVertices;
  std::vector<recob::PFParticle const*> ana_pandora_neutrinos, ana_pandora_tracks, ana_pandora_showers;
  int tree_pandora_nNeutrinos, tree_pandora_nTwoProngedNeutrinos, tree_pandora_nInsideTwoProngedNeutrinos;
  std::vector<int> tree_pandora_neutrinoPdgCode, tree_pandora_neutrinoNumDaughters, tree_pandora_neutrinoNumTracks, tree_pandora_neutrinoNumShowers;
  std::vector<bool> tree_pandora_neutrinoInTPC;
  std::vector<std::vector<float>> tree_pandoraCalo_totChargeInRadius, tree_pandoraCalo_par1ChargeInRadius, tree_pandoraCalo_par2ChargeInRadius, tree_pandoraCalo_caloRatio; // For each dv in event (usually one) and for each radius
  int tree_pandoraCalo_nTotHits;
  std::vector<int> tree_pandoraCalo_nTrackHits, tree_pandoraCalo_nShowerHits;
  std::vector<std::vector<int>> tree_pandoraDiagnostic_nVerticesInPfp;
  std::vector<bool> tree_pandoraDiagnostic_pathologicalVertex;

  // Declare drawTree variables
  std::vector<std::vector<int>> dv_wireCoordinates,
    par1_wireCoordinates, par2_wireCoordinates,
    par1_hits_p0_wireCoordinates, par1_hits_p1_wireCoordinates, par1_hits_p2_wireCoordinates,
    par2_hits_p0_wireCoordinates, par2_hits_p1_wireCoordinates, par2_hits_p2_wireCoordinates,
    tot_hits_p0_wireCoordinates, tot_hits_p1_wireCoordinates, tot_hits_p2_wireCoordinates;

  std::vector<std::vector<float>> dv_xyzCoordinates, dv_tickCoordinates,
    par1_xyzCoordinates, par1_tickCoordinates,
    par2_xyzCoordinates, par2_tickCoordinates,
    par1_hits_p0_tickCoordinates, par1_hits_p1_tickCoordinates, par1_hits_p2_tickCoordinates,
    par2_hits_p0_tickCoordinates, par2_hits_p1_tickCoordinates, par2_hits_p2_tickCoordinates,
    tot_hits_p0_tickCoordinates, tot_hits_p1_tickCoordinates, tot_hits_p2_tickCoordinates;

  // Declare analysis functions
  void ClearData();
  void FillTree();
  void FillDrawTree(
          const std::vector<AuxVertex::DecayVertex>& cleanVertices,
          const std::vector<std::vector<recob::Hit const*>>& totHitsInMaxRadius,
          const std::vector<std::vector<recob::Hit const*>>& trackHits,
          const std::vector<std::vector<recob::Hit const*>>& showerHits);
}; // End class PreSelectHSN

#endif