#ifndef TkStripMeasurementDet_H
#define TkStripMeasurementDet_H

#include "TrackingTools/MeasurementDet/interface/MeasurementDet.h"
#include "RecoLocalTracker/ClusterParameterEstimator/interface/StripClusterParameterEstimator.h"
#include "DataFormats/SiStripCluster/interface/SiStripClusterCollection.h"
#include "Geometry/TrackerGeometryBuilder/interface/StripGeomDetUnit.h"
#include "DataFormats/Common/interface/DetSetVector.h"
#include "DataFormats/SiStripCluster/interface/SiStripCluster.h"
#include "DataFormats/Common/interface/Handle.h"
#include "CondFormats/SiStripObjects/interface/SiStripNoises.h"
#include "CondFormats/SiStripObjects/interface/SiStripBadStrip.h"
#include "DataFormats/SiStripCommon/interface/SiStripRefGetter.h"


class TransientTrackingRecHit;

class TkStripMeasurementDet : public MeasurementDet {
public:

  //  typedef SiStripClusterCollection::Range                ClusterRange;
  //  typedef SiStripClusterCollection::ContainerIterator    ClusterIterator;
  typedef StripClusterParameterEstimator::LocalValues    LocalValues;

  typedef edm::Ref<edm::DetSetVector<SiStripCluster>, SiStripCluster, 
    edm::refhelper::FindForDetSetVector<SiStripCluster> > SiStripClusterRef;
  
  typedef edm::SiStripRefGetter<SiStripCluster>::value_ref  SiStripRegionalClusterRef;

  typedef edm::DetSetVector<SiStripCluster>::detset detset;
  typedef detset::const_iterator const_iterator;

  virtual ~TkStripMeasurementDet(){}

  TkStripMeasurementDet( const GeomDet* gdet,
			 const StripClusterParameterEstimator* cpe,
			 bool regional);

  void update( const detset & detSet, 
	       const edm::Handle<edm::DetSetVector<SiStripCluster> > h,
	       unsigned int id ) { 
    detSet_ = & detSet; 
    handle_ = h;
    id_ = id;
    empty = false;
    isRegional = false;
  }

  void update( std::vector<SiStripCluster>::const_iterator begin ,std::vector<SiStripCluster>::const_iterator end, 
	       const edm::Handle<edm::SiStripRefGetter<SiStripCluster> > h,
	       unsigned int id ) { 
    beginCluster = begin;
    endCluster   = end;
    regionalHandle_ = h;
    id_ = id;
    empty = false;
    isRegional = true;
  }
  
  void setEmpty(){empty = true;}
  
  virtual RecHitContainer recHits( const TrajectoryStateOnSurface&) const;

  virtual std::vector<TrajectoryMeasurement> 
  fastMeasurements( const TrajectoryStateOnSurface& stateOnThisDet, 
		    const TrajectoryStateOnSurface& startingState, 
		    const Propagator&, 
		    const MeasurementEstimator&) const;

  const StripGeomDetUnit& specificGeomDet() const {return *theStripGDU;}

  TransientTrackingRecHit::RecHitPointer
  buildRecHit( const SiStripClusterRef&, const LocalTrajectoryParameters& ltp) const;

  TransientTrackingRecHit::RecHitPointer
  buildRecHit( const SiStripRegionalClusterRef&, const LocalTrajectoryParameters& ltp) const;


  bool  isEmpty() {return empty;}
  const detset* theSet() {return detSet_;}
  int  size() {return endCluster - beginCluster ; }

  /** \brief Turn on/off the module for reconstruction (using info from DB, usually) */
  void setActive(bool active) { active_ = active; if (!active) empty = true; }
  /** \brief Is this module active in reconstruction? */
  bool isActive() const { return active_; }

  /** \brief does this module have at least one bad strip, APV or channel? */
  bool hasAllGoodChannels() const { return !hasAny128StripBad_ && badStripBlocks_.empty(); }

  /** \brief Sets the status of a block of 128 strips (or all blocks if idx=-1) */
  void set128StripStatus(bool good, int idx=-1);

  /** \brief return true if there are 'enough' good strips in the utraj +/- 3 uerr range.*/
  bool testStrips(float utraj, float uerr) const;

  struct BadStripBlock {
      short first;
      short last;
      BadStripBlock(const SiStripBadStrip::data &data) : first(data.firstStrip), last(data.firstStrip+data.range-1) { }
  };
  std::vector<BadStripBlock> &getBadStripBlocks() { return badStripBlocks_; }

private:

  const StripGeomDetUnit*               theStripGDU;
  const StripClusterParameterEstimator* theCPE;
  const detset * detSet_;
  edm::Handle<edm::DetSetVector<SiStripCluster> > handle_;
  unsigned int id_;
  bool empty;

  bool active_;
  bool bad128Strip_[6];
  bool hasAny128StripBad_;
  std::vector<BadStripBlock> badStripBlocks_;  
  int totalStrips_;

  // --- regional unpacking
  bool isRegional;
  edm::Handle<edm::SiStripRefGetter<SiStripCluster> > regionalHandle_;
  std::vector<SiStripCluster>::const_iterator beginCluster;
  std::vector<SiStripCluster>::const_iterator endCluster;
  // regional unpacking ---
};

#endif
