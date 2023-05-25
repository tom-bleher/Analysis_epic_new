#ifndef VARIABLES_H
#define VARIABLES_H

#include <vector>

typedef std::tuple<double,double,double> TrackHit;

struct TrackClass {
  double charge;
  double X0;
  double Y0;
  double slopeX;
  double slopeY;
  double theta;
  double phi;
  double Chi2;
};

struct TreeTrackClass {
  std::vector<double> X0_e;
  std::vector<double> Y0_e;
  std::vector<double> slopeX_e;
  std::vector<double> slopeY_e;

  std::vector<double> X0_p;
  std::vector<double> Y0_p;
  std::vector<double> slopeX_p;
  std::vector<double> slopeY_p;
};

namespace variables {
  extern const int maxModules;
  extern const int maxSectors;

  extern int Nhits_min; // cluster hits min

  extern double Einput;

  extern double LumiSpecMag_Z;
  extern double LumiSpecMag_DZ;
  extern double LumiSpecCAL_Z;
  extern double LumiSpecCALTower_DZ;
  extern double LumiSpecCAL_DXY;
  extern double LumiSpecCAL_FiveSigma;
  extern double LumiConverter_Z;
  extern double LumiSpecMagEnd_Z;
  extern double LumiConverterCut_DXY;

  extern double pT;   
  extern double RmagPreFactor; 

  extern double SpecMag_to_SpecCAL_DZ;

  extern double LumiSpecTracker_Z1;
  extern double LumiSpecTracker_Z2;
  extern double LumiSpecTracker_Z3;
  extern std::vector<double> Tracker_Zs;

  extern double Tracker_meanZ;

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

#endif
