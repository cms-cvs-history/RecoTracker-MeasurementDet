#ifndef TkPixelMeasurementDet_H
#define TkPixelMeasurementDet_H

#include "TrackingTools/MeasurementDet/interface/MeasurementDet.h"
#include "RecoLocalTracker/ClusterParameterEstimator/interface/PixelClusterParameterEstimator.h"
//#include "DataFormats/SiPixelCluster/interface/SiPixelClusterFwd.h"
#include "Geometry/TrackerGeometryBuilder/interface/PixelGeomDetUnit.h"
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/Common/interface/DetSetVector.h"
#include "DataFormats/SiPixelCluster/interface/SiPixelCluster.h"

class TransientTrackingRecHit;
class LocalTrajectoryParameters;

class TkPixelMeasurementDet : public MeasurementDet {
public:

  typedef edm::Ref<edmNew::DetSetVector<SiPixelCluster>, SiPixelCluster> SiPixelClusterRef;
  
  typedef edmNew::DetSet<SiPixelCluster> detset;
  typedef detset::const_iterator const_iterator;
  typedef PixelClusterParameterEstimator::LocalValues    LocalValues;

  TkPixelMeasurementDet( const GeomDet* gdet,
			 const PixelClusterParameterEstimator* cpe);

  void update( const detset & detSet, 
	       const edm::Handle<edmNew::DetSetVector<SiPixelCluster> > h,
	       unsigned int id ) { 
    detSet_ = detSet; 
    handle_ = h;
    id_ = id;
    empty = false;
  }
  void setEmpty(){empty = true;}

  virtual ~TkPixelMeasurementDet() { }

  virtual RecHitContainer recHits( const TrajectoryStateOnSurface& ) const;

  virtual std::vector<TrajectoryMeasurement> 
  fastMeasurements( const TrajectoryStateOnSurface& stateOnThisDet, 
		    const TrajectoryStateOnSurface& startingState, 
		    const Propagator&, 
		    const MeasurementEstimator&) const;

  const PixelGeomDetUnit& specificGeomDet() const {return *thePixelGDU;}

  TransientTrackingRecHit::RecHitPointer 
  buildRecHit( const SiPixelClusterRef & cluster,
	       const LocalTrajectoryParameters & ltp) const;

  /** \brief Turn on/off the module for reconstruction (using info from DB, usually) */
  void setActive(bool active) { active_ = active; if (!active) empty = true; }
  /** \brief Is this module active in reconstruction? */
  bool isActive() const { return active_; }

private:

  const PixelGeomDetUnit*               thePixelGDU;
  const PixelClusterParameterEstimator* theCPE;
  detset detSet_;
  edm::Handle<edmNew::DetSetVector<SiPixelCluster> > handle_;
  unsigned int id_;
  bool empty;
  bool active_;
};

#endif
