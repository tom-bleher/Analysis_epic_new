#ifndef VARIABLES_H
#define VARIABLES_H

#include <vector>
#include <math.h>
//                  hitX , hitY , hitZ , Edep , time , primary?
typedef std::tuple<double,double,double,double,double,double> TrackHit;


namespace variables {
  extern const int maxModules;
  extern const int maxSectors;

  extern int Nhits_min; // cluster hits min

  extern double Ephoton;
  extern double Eelectron;
  extern double Epositron;
  extern double Xphoton;
  extern double Xelectron;
  extern double Xpositron;
  extern double Yphoton;
  extern double Yelectron;
  extern double Ypositron;
  extern double ThetaPhoton;
  extern double PhiPhoton;

  extern double LumiAnalyzerMag_Z;
  extern double LumiAnalyzerMag_DZ;
  extern double LumiSpecCAL_Z;
  extern double LumiSpecCALTower_DZ;
  extern double LumiSpecCAL_DXY;
  extern double LumiBeamDiv_pref;
  extern double LumiSpecCAL_FiveSigma;
  extern double LumiConverter_Z;
  extern double LumiAnalyzerMagStart_Z;
  extern double LumiAnalyzerMagEnd_Z;
  extern double LumiConverterCut_DXY;
  extern double LumiPhotonDCAcut;

  extern double RmagPreFactor; 

  extern double LumiSpecTracker_Z1;
  extern double LumiSpecTracker_Z2;

  extern double Tracker_pixelSize;

  extern double max_chi2ndf;
  extern double Tracker_sigma;

  extern double E_hit;
  extern double x_hit;
  extern double y_hit;
  extern double z_hit;
  extern double r_hit;
  extern double t_hit;
  extern int sec_id ;
  extern int mod_id ;
  extern int lay_id ;
  extern int fiber_x_id;
  extern int fiber_y_id;

  extern int Nhits_cluster;
  extern double E_cluster;
  extern double x_cluster;
  extern double y_cluster;
  extern double r_cluster;
  extern double t_cluster;
  extern double Radius_cluster;
  extern double Dispersion_cluster;
  extern double SigmaThetaPhi1_cluster;
  extern double SigmaThetaPhi2_cluster;
}

struct TrackClass {
  double e;
  double charge;
  double x0; // x intercept of least-chi2 fit
  double y0; // y intercept of least-chi2 fit
  double xGamma; // x that ideally corresponds to xGamma
  double yGamma; // y that ideally corresponds to yGamma
  double slopeX;
  double slopeY;
  double theta;
  double phi;
  double chi2;
  double nHits;
  std::vector<double> eDeps;
  std::vector<double> time;
  bool primary;
};

struct PhotonGenClass {
  double e;
  double eElec;
  double ePos;
  double theta;
  double phi;
  double x;
  double y;
};

struct PhotonRecClass {
  double e;
  double eTop;
  double eBot;
  double mass;
  double x;
  double y;
  double dca;
  double eGen;
  double xGen;
  double yGen;
  double thetaGen;
  double phiGen;
  double chi2Top;
  double nHitsTop;
  bool primaryTop;
  double chi2Bot;
  double nHitsBot;
  bool primaryBot;
};

#endif
