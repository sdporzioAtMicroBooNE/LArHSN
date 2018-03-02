#include "FindDecayVertexAlg.h"

namespace FindDecayVertex
{
  // Constructor/destructor
  FindDecayVertexAlg::FindDecayVertexAlg(fhicl::ParameterSet const & pset)
  {
    reconfigure(pset);
    fGeometry = lar::providerFrom<geo::Geometry>();
    fDetectorProperties = lar::providerFrom<detinfo::DetectorPropertiesService>();
  }
  FindDecayVertexAlg::~FindDecayVertexAlg()
  {}

  void FindDecayVertexAlg::reconfigure(fhicl::ParameterSet const & pset)
  {
    fMinTpcBound = pset.get<std::vector<double>>("MinTpcBound");
    fMaxTpcBound = pset.get<std::vector<double>>("MaxTpcBound");
    fPfpLabel = pset.get<std::string>("PfpLabel");
    fDistanceCut = pset.get<double>("DistanceCut");
    fPrimaryOnly = pset.get<bool>("PrimaryOnly");
    fEndVerticesAlso = pset.get<bool>("EndVerticesAlso");
    fAnaType = pset.get<std::string>("AnalysisType");
    fVerbose = pset.get<bool>("VerboseMode");
  }



  // Take an event (evt), then fill the (empty!) track, shower and primaries pfp vectors (tracks,showers,pandora_primaryPFP) with all the PFParticles that are tracks, showers and primaries pfps respectively in the event. These vectors will then be used to determine all the possible vertices.
  void FindDecayVertexAlg::GetTrackShowerVectors(
            AuxEvent::EventDescriptor & evd,
            art::Event const & evt)
  {
    // Clear vectors that will be returned by function
    ana_manual_primaries.clear();
    ana_manual_tracks.clear();
    ana_manual_showers.clear();

    // Prepare the pfp handle
    art::InputTag pfpTag {fPfpLabel};
    const auto& pfpHandle = evt.getValidHandle< std::vector<recob::PFParticle> >(pfpTag);

    // Loop through each pfp
    for (auto const& pfp : (*pfpHandle)){
      if (pfp.IsPrimary()) {ana_manual_primaries.push_back(&pfp);}
      else
      {
        // Find current pfp parent and check if parent is primary (so that pfp are only secondaries, and not tertiaries, etc.), then fill the correspondending vector with the type of particle found
        if (fPrimaryOnly){
          auto const& parent = (*pfpHandle).at(pfp.Parent());
          if (parent.IsPrimary()){
            if (pfp.PdgCode()==13) {ana_manual_tracks.push_back(&pfp);}
            if (pfp.PdgCode()==11) {ana_manual_showers.push_back(&pfp);}
          }
        }
        // Fill the track and shower vectors with the products associated with the pfp (in any case, regardless of their "genealogy")
        else {
          if (pfp.PdgCode()==13) {ana_manual_tracks.push_back(&pfp);}
          if (pfp.PdgCode()==11) {ana_manual_showers.push_back(&pfp);}
        }
      }
    } // End of pfp loop

    // Fill the EventDescriptor and print diagnostic message
    evd.manual_NumPrimaries = (int) ana_manual_primaries.size();
    evd.manual_NumTracks = (int) ana_manual_tracks.size();
    evd.manual_NumShowers = (int) ana_manual_showers.size();
    if (fVerbose)
    {
      printf("GetTrackShowerVectors message:\n");
      printf("Loading %i primaries, %i secondary tracks and %i secondary showers.\n", evd.manual_NumPrimaries, evd.manual_NumTracks, evd.manual_NumShowers);
    }

    return;
  } // END function GetTrackShowerVectors


  // Given track and shower vectors (tracks,showers), fill vertices vectors (trackVertices,showerVertices) taking all the start and end points for tracks and start points for showers. The last two numbers (j,j) indicate only their idx in the xxxVertices vector (which is reduntant at this stage). However, later on, it will be used to identify the vertices uniquely, and the idxs from two vertices will be used to define the parent idx of the mean vertex created between them.
  void FindDecayVertexAlg::GetOriginVertices(
            AuxEvent::EventDescriptor & evd,
            art::Event const & evt,
            const std::vector<recob::PFParticle const*>& tracks,
            const std::vector<recob::PFParticle const*>& showers)
  {
    // Clear vectors that will be returned by function
    ana_manual_trackVertices.clear();
    ana_manual_showerVertices.clear();

    // Initialize associations
    art::InputTag pfpTag {fPfpLabel};
    art::FindMany<recob::Track> pta(tracks,evt,pfpTag);
    art::FindMany<recob::Shower> psa(showers,evt,pfpTag);

    // Perform for tracks
    for(std::vector<int>::size_type i=0; i!=tracks.size(); i++)
    {
      // Annoying stupid way of getting associated object
      std::vector<recob::Track const*> vTrack;
      pta.get(i,vTrack);
      if (vTrack.size()==1)
      {
        recob::Track const* track = vTrack[0];
        // Create vertex
        AuxVertex::DecayVertex tempV1(track->Vertex().X(),track->Vertex().Y(),track->Vertex().Z(),i,i,"t","t","front","front");
        tempV1.SetDetectorCoordinates(fMinTpcBound,fMaxTpcBound,fDistanceCut,fGeometry,fDetectorProperties);
        if (tempV1.IsInsideTPC())
        {
          ana_manual_trackVertices.push_back(tempV1);
        }
        if (fEndVerticesAlso)
        {
          AuxVertex::DecayVertex tempV2(track->End().X(),track->End().Y(),track->End().Z(),i,i,"t","t","end","end");
          tempV2.SetDetectorCoordinates(fMinTpcBound,fMaxTpcBound,fDistanceCut,fGeometry,fDetectorProperties);
          if (tempV2.IsInsideTPC())
          {
            ana_manual_trackVertices.push_back(tempV2);
          }
        }
      } 
      else
      {
        evd.manual_NumUnassociatedTracks += 1;
      } // END if association is present
    }

    // And for showers
    for(std::vector<int>::size_type i=0; i!=showers.size(); i++)
    {
      // Annoying stupid way of getting associated object
      std::vector<recob::Shower const*> vShower;
      psa.get(i,vShower);
      if (vShower.size()==1)
      {
        recob::Shower const* shower = vShower[0];
        // Create vertex
        AuxVertex::DecayVertex tempV(shower->ShowerStart().X(),shower->ShowerStart().Y(),shower->ShowerStart().Z(),i,i,"s","s","front","front");
        tempV.SetDetectorCoordinates(fMinTpcBound,fMaxTpcBound,fDistanceCut,fGeometry,fDetectorProperties);
        if (tempV.IsInsideTPC())
        {
          ana_manual_showerVertices.push_back(tempV);
        }
      }
      else
      {
        evd.manual_NumUnassociatedShowers += 1;
      } // END if association is present
    }

    // Fill the EventDescriptor and print diagnostic message
    evd.manual_NumTrackVertices = (int) ana_manual_trackVertices.size();
    evd.manual_NumShowerVertices = (int) ana_manual_showerVertices.size();
    if (fVerbose)
    {
      printf("GetOriginVertices message:\n");
      printf("Processing %i origin vertices (%i from tracks, %i from showers).\n", evd.manual_NumTrackVertices+ evd.manual_NumShowerVertices,  evd.manual_NumTrackVertices,  evd.manual_NumShowerVertices);
    }
    return;
  } // END function GetOriginVertices




  // Given the reco vertices establishing the origin of tracks and showers (trackVertices,showerVertices), take all the possible mean vertices by paring them two by two (potVertices), then impose that in radius (fDistanceCut) only two original vertices are present (cleanVertices). Now the previously used (j,j) index are used to define the parent vertices (which track or shower originating vertex was used to determine the mean vertex between them).
  void FindDecayVertexAlg::GetDecayVertices(
            AuxEvent::EventDescriptor & evd,
            const std::vector<AuxVertex::DecayVertex>& trackVertices,
            const std::vector<AuxVertex::DecayVertex>& showerVertices)
  {
    // Clear vectors that will be returned by function
    ana_manual_potVertices.clear();
    ana_manual_cleanVertices.clear();

    // For track-track
    if (fAnaType == "tt")
    {
      for(std::vector<int>::size_type i=0; i!=trackVertices.size(); i++)
      {
        for(std::vector<int>::size_type j=i+1; j!=trackVertices.size(); j++)
        {
          AuxVertex::DecayVertex v1 = trackVertices[i];
          AuxVertex::DecayVertex v2 = trackVertices[j];
          float distance = Distance(v1,v2);
          evd.manual_pairDistances.push_back(distance);
          bool isInRadius = (distance<fDistanceCut);
          if (isInRadius)
          {
            AuxVertex::DecayVertex v3 = MeanVertex(v1, v2);
            v3.SetDetectorCoordinates(fMinTpcBound,fMaxTpcBound,fDistanceCut,fGeometry,fDetectorProperties);
            ana_manual_potVertices.push_back(v3);
            evd.manual_potPairDistances.push_back(distance);
          }
        }
      }
      // Make sure the potential mean vertices are clean (two decay vertices only in radius)
      // (start with assumption that isGoodVertex == true, then if you find extra particles in radius, turn it in bad vertex. Only good vertices are saved)
      for(std::vector<int>::size_type i=0; i!=ana_manual_potVertices.size(); i++)
      {
        AuxVertex::DecayVertex mv = ana_manual_potVertices[i];
        bool isGoodVertex = true;
        for(std::vector<int>::size_type j=0; j!=trackVertices.size(); j++)
        {
          AuxVertex::DecayVertex v1 = trackVertices[j];
          bool isParent1 = (mv.GetParIdx1() == v1.GetParIdx1());
          bool isParent2 = (mv.GetParIdx2() == v1.GetParIdx2());
          bool notParent = !(isParent1 || isParent2);
          bool isInRadius = (Distance(mv,v1)<fDistanceCut);
          if (isInRadius && notParent) isGoodVertex = false;
        }
        if (isGoodVertex) ana_manual_cleanVertices.push_back(mv);
      }
    }

    // And for track-shower
    else if (fAnaType == "ts")
    {
      for(std::vector<int>::size_type i=0; i!=trackVertices.size(); i++)
      {
        for(std::vector<int>::size_type j=0; j!=showerVertices.size(); j++)
        {
          AuxVertex::DecayVertex v1 = trackVertices[i];
          AuxVertex::DecayVertex v2 = showerVertices[j];
          float distance = Distance(v1,v2);
          evd.manual_pairDistances.push_back(distance);
          bool isInRadius = (distance<fDistanceCut);
          if (isInRadius)
          {
            AuxVertex::DecayVertex v3 = MeanVertex(v1, v2);
            v3.SetDetectorCoordinates(fMinTpcBound,fMaxTpcBound,fDistanceCut,fGeometry,fDetectorProperties);
            ana_manual_potVertices.push_back(v3);
            evd.manual_potPairDistances.push_back(distance);
          }
        }
      }
      // Make sure the potential mean vertices are clean (two decay vertices only in radius)
      // (start with assumption that isGoodVertex == true, then if you find extra particles in radius, turn it in bad vertex. Only good vertices are saved)
      for(std::vector<int>::size_type i=0; i!=ana_manual_potVertices.size(); i++)
      {
        AuxVertex::DecayVertex mv = ana_manual_potVertices[i];
        bool isGoodVertex = true;
        for(std::vector<int>::size_type j=0; j!=trackVertices.size(); j++)
        {
          AuxVertex::DecayVertex v1 = trackVertices[j];
          bool notParent = (mv.GetParIdx1() != v1.GetParIdx1());
          bool isInRadius = (Distance(mv,v1)<fDistanceCut);
          if (isInRadius && notParent) isGoodVertex = false;
        }
        for(std::vector<int>::size_type j=0; j!=showerVertices.size(); j++)
        {
          AuxVertex::DecayVertex v2 = showerVertices[j];
          bool notParent = (mv.GetParIdx2() != v2.GetParIdx2());
          bool isInRadius = (Distance(mv,v2)<fDistanceCut);
          if (isInRadius && notParent) isGoodVertex = false;
        }
        if (isGoodVertex) {ana_manual_cleanVertices.push_back(mv);}
      }
    }
    // Throw error if fAnaType wasn't right.
    else {
      throw std::invalid_argument("Invalid fAnaType. Must be 'tt' or 'ts'!");
    }

    // Fill the EventDescriptor and print diagnostic message
    evd.manual_NumPotVertices = (int) ana_manual_potVertices.size();
    evd.manual_NumCleanVertices = (int) ana_manual_cleanVertices.size();
    if (fVerbose)
    {
      printf("GetDecayVertices message:\n");
      printf("Selecting %i potential vertices, %i of which are clean.\n", evd.manual_NumPotVertices, evd.manual_NumPotVertices);
    }
    return;
  } // END function GetDecayVertices

} // END namespace FindDecayVertex