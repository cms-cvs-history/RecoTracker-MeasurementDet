#include "RecoTracker/MeasurementDet/interface/TkGluedMeasurementDet.h"
#include "TrackingTools/MeasurementDet/interface/MeasurementDetException.h"
#include "RecoTracker/TransientTrackingRecHit/interface/TSiStripMatchedRecHit.h"
#include "Geometry/TrackerGeometryBuilder/interface/GluedGeomDet.h"



#include "DataFormats/Math/interface/SSEVec.h"

void TkGluedMeasurementDet::doubleMatch(const TrajectoryStateOnSurface& ts, Collector & collector) const {

  using  mathSSE::Vec3F;
  using  mathSSE::Vec3D;
  using  mathSSE::Rot3F;
  typedef  GloballyPositioned<float> ToGlobal;
  typedef  Typename GloballyPositioned<float>::ToLocal ToLocal;

  GlobalVector glbDir = (ts.isValid() ? ts.globalParameters().momentum() : position()-GlobalPoint(0,0,0));
 
 //  static SiStripRecHitMatcher::SimpleHitCollection vsStereoHits;
  // vsStereoHits.resize(simpleSteroHitsByValue.size());
  //std::transform(simpleSteroHitsByValue.begin(), simpleSteroHitsByValue.end(), vsStereoHits.begin(), take_address());

  RecHitContainer monoHits = theMonoDet->recHits( ts);
  if (monoHits.empty()) {
      // make stereo TTRHs and project them
      projectOnGluedDet( collector, theStereoDet->recHits(ts), glbDir);
      return;
  }
 
  // collect simple stereo hits
  static std::vector<SiStripRecHit2D> simpleSteroHitsByValue;
  simpleSteroHitsByValue.clear();
  theStereoDet->simpleRecHits(ts, simpleSteroHitsByValue);
  if (simpleSteroHitsByValue.empty()) {
    projectOnGluedDet( collector, monoHits, glbDir);
    return;
  }

  // hits in both mono and stero
  // match
    
  LocalVector trackdirection = (ts.isValid() ? ts.localDirection() : surface().toLocal( position()-GlobalPoint(0,0,0)));
  bool notk = trackdirection.mag2()<FLT_MIN;

 // toGlobal is fast,  toLocal is slow
  ToGlobal const & stripDetTrans =  specificGeomDet().monoDet().surface();
  ToGlobal const & partnerStripDetTrans = specificGeomDet().stereoDet().surface();
  ToLocal          gluedDetInvTrans(specificGeomDet().surface());

  const GluedGeomDet* gluedDet = &specificGeomDet();
  const GeomDetUnit* stripdet = specificGeomDet().monoDet();
  const GeomDetUnit* partnerstripdet = specificGeomDet().stereoDet();
  const StripTopology& topol=(const StripTopology&)stripdet->topology();


  for (RecHitContainer::const_iterator monoHit = monoHits.begin();
       monoHit != monoHits.end(); ++monoHit) {
    const TrackingRecHit* tkhit = (**monoHit).hit();
    const SiStripRecHit2D* monoRH = reinterpret_cast<const SiStripRecHit2D*>(tkhit);

    // position of the initial and final point of the strip (RPHI cluster) in local strip coordinates
    double RPHIpointX = topol.measurementPosition(monoRH->localPositionFast()).x();
    MeasurementPoint RPHIpointini(RPHIpointX,-0.5);
    MeasurementPoint RPHIpointend(RPHIpointX,0.5);

    // position of the initial and final point of the strip in local coordinates (mono det)
    //StripPosition stripmono=StripPosition(topol.localPosition(RPHIpointini),topol.localPosition(RPHIpointend));
    LocalPoint locp1o = topol.localPosition(RPHIpointini);
    LocalPoint locp2o = topol.localPosition(RPHIpointend);

    // in case of no track hypothesis assume a track from the origin through the center of the strip
    if(notk){
      LocalPoint lcenterofstrip=monoRH->localPositionFast();
      GlobalPoint gcenterofstrip= stripDetTrans.toGlobal(lcenterofstrip);
      GlobalVector gtrackdirection=gcenterofstrip-GlobalPoint(0,0,0);
      trackdirection=gluedDetInvTrans.toLocal(gtrackdirection);
    }

	      
    //project mono hit on glued det
    //StripPosition projectedstripmono=project(stripdet,gluedDet,stripmono,trackdirection);
    
       
    GlobalPoint globalpointini=stripDetTrans.toGlobal(locp1o);
    GlobalPoint globalpointend=stripDetTrans.toGlobal(locp2o);
    
    // position of the initial and final point of the strip in glued local coordinates
    LocalPoint positiononGluedini=gluedDetInvTrans.toLocal(globalpointini);
    LocalPoint positiononGluedend=gluedDetInvTrans.toLocal(globalpointend); 
    
    Vec3F offset = trdir.basicVector().v * positiononGluedini.basicVector().v.get1(2)/trdir.basicVector().v.get1(2);
    

    Vec3F projini= positiononGluedini.basicVector().v - offset;
    Vec3F projend = positiononGluedend.basicVector().v -offset;

    // ret1o = ret1o + (trdir * (ret1o.getSimd(2) / trdirz));
    // ret2o = ret2o + (trdir * (ret2o.getSimd(2) / trdirz));
    
    const StripTopology& partnertopol=(const StripTopology&)partnerstripdet->topology();
    
    double m00 = -(projend.arr[1] - projini.arr[1]);//-(projectedstripmono.second.y()-projectedstripmono.first.y()); 
    double m01 =  (projend.arr[0] - projini.arr[0]); // (projectedstripmono.second.x()-projectedstripmono.first.x());
    double c0  =  m01*projini.arr[1] + m00*projini.arr[0];//m01*projectedstripmono.first.y()   + m00*projectedstripmono.first.x();

    Vec2D c0vec(c0,c0);
    Vec2D minv00(-m01, m00);
    
    //error calculation (the part that depends on mono RH only)
    double c1 = -m00;
    double s1 = -m01;
    double l1 = 1./(c1*c1+s1*s1);
    
    // FIXME: here for test...
    double sigmap12 = monoRH->sigmaPitch();
    if (sigmap12<0) {
      
      LocalError tmpError(monoRH->localPositionErrorFast());
      HelpertRecHit2DLocalPos::updateWithAPE(tmpError,*stripdet);
      MeasurementError errormonoRH=topol.measurementError(monoRH->localPositionFast(),tmpError);

      double pitch=topol.localPitch(monoRH->localPositionFast());
      monoRH->setSigmaPitch(sigmap12=errormonoRH.uu()*pitch*pitch);
    }
    //float code
    Vec3F scc1(s1, c1, c1, 0);
    Vec3F ssc1(s1, s1, c1, 0);
    Vec3F l1vec; l1vec.set1(l1);
    const Vec3F cslsimd = scc1 * ssc1 * l1vec;
    Vec3F sigmap12simd; sigmap12simd.set1(sigmap12);
    
    for ( std::vector<SiStripRecHit2D>::const_iterator seconditer=simpleSteroHitsByValue.begin();  
	  seconditer!=simpleSteroHitsByValue.end();
	  ++seconditer
	  ){//iterate on stereo rechits

      const SiStripRecHit2D & secondHit = (*seconditer);

      double STEREOpointX=partnertopol.measurementPosition( secondHit.localPositionFast()).x();
      MeasurementPoint STEREOpointini(STEREOpointX,-0.5);
      MeasurementPoint STEREOpointend(STEREOpointX,0.5);
      
      LocalPoint locp1 = partnertopol.localPosition(STEREOpointini);
      LocalPoint locp2 = partnertopol.localPosition(STEREOpointend);
      
      GlobalPoint globalpointini=partnerStripDetTrans.toGlobal(locp1);
      GlobalPoint globalpointend=partnerStripDetTrans.toGlobal(locp2);
    
      // position of the initial and final point of the strip in glued local coordinates
      LocalPoint positiononGluedini=gluedDetInvTrans.toLocal(globalpointini);
      LocalPoint positiononGluedend=gluedDetInvTrans.toLocal(globalpointend); 
    
      Vec3F offset = trdir.basicVector().v * positiononGluedini.basicVector().v.get1(2)/trdir.basicVector().v.get1(2);
    

      Vec3F ret1 = positiononGluedini.basicVector().v - offset;
      Vec3F ret2 = positiononGluedend.basicVector().v - offset;
  
      double m10=-(ret2.arr[1] - ret1.arr[1]); 
      double m11=  ret2.arr[0] - ret1.arr[0];
      
      Vec2D minv10(m11, -m10);
      Vec2D mult; mult.set1(1./(m00*m11 - m01*m10));
      Vec2D c1vec; c1vec.set1(m11*ret1.arr[1] + m10 * ret1.arr[0]);
      Vec2D resultmatmul = mult * (minv10 * c0vec + minv00 * c1vec);
	       
      LocalPoint position(resultmatmul.arr[0], resultmatmul.arr[1]);

      LocalError tempError (100,0,100);
      if (!((gluedDet->surface()).bounds().inside(position,tempError,theMatcher->scale_))) continue;                                                       
      
      double c2 = -m10;
      double s2 = -m11;
      double l2 = 1./(c2*c2+s2*s2);
      double sigmap22 =secondHit.sigmaPitch();
      if (sigmap22<0) {
	LocalError tmpError((*seconditer)->localPositionErrorFast());
	HelpertRecHit2DLocalPos::updateWithAPE(tmpError, *partnerstripdet);
	MeasurementError errorstereoRH=partnertopol.measurementError(secondHit.localPositionFast(),tmpError);

	double pitch=partnertopol.localPitch((*seconditer)->localPositionFast());
	secondHit.setSigmaPitch(sigmap22=errorstereoRH.uu()*pitch*pitch);
      }
      
      double diff=(c1*s2-c2*s1);
      double invdet2 = 1/(diff*diff*l1*l2);
      
      Vec3F invdet2simd(invdet2, -invdet2, invdet2, 0);
      Vec3F ccssimd(s2, c2, c2, 0);
      Vec3F csssimd(s2, s2, c2, 0);
      Vec3F l2simd; l2simd.set1(l2);
      Vec3F sigmap22simd(sigmap22);
      Vec3F result = invdet2simd * (sigmap22simd * cslsimd + sigmap12simd * ccssimd * csssimd * l2simd);
      
      
      LocalError error(result.arr[0], result.arr[1], result.arr[2]);
      
      
      if((gluedDet->surface()).bounds().inside(position,error,theMatcher->scale_)){ //if it is inside the gluedet bonds
	
	//Change NSigmaInside in the configuration file to accept more hits
	//...and add it to the Rechit collection 
	
	collector.collector()(SiStripMatchedRecHit2D(position, error,gluedDet->geographicalId() ,
						     monoRH,&secondHit));
      }

    }
    if (collector.hasNewMatchedHits()) {
      collector.clearNewMatchedHitsFlag();
    } else {
      collector.addProjected( **monoHit, glbDir );
    }
  } // loop on mono hit

}


