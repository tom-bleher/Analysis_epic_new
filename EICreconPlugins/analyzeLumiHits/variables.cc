#ifndef VARIABLES_CC
#define VARIABLES_CC

#include "variables.h"

namespace variables {
  
  const int maxModules = 3;
  const int maxSectors = 2;

  int Nhits_min = 1; // cluster hits min

  double Einput = 0;

  // spectrometer dimensions/placements in mm
  // DXY, and DZ stand for FULL widths
  // Old design
  //double LumiAnalyzerMag_Z = -56000;
  //double LumiAnalyzerMag_DZ = 780;
  //double LumiSpecCALTower_DZ = 200;
  //double LumiSpecCAL_DXY = 200;
  //double LumiSpecCAL_Z = -65000;
  //double LumiSpecCAL_FiveSigma = 69;
  //// cyclotron radius = speed / cyclotron frequency -> p/(q*B) = E/(c*q*B) in ultrarelativistic limit
  //double pT = 0.117; // GeV. 0.3*B(T)*dZ(m)
  //double RmagPreFactor = 6670.79; // (J/GeV)/(c * q * B), multiply this by E in GeV to get R in mm
  
  // New design
  double LumiAnalyzerMag_Z = -44000;
  double LumiAnalyzerMag_DZ = 1000;
  double LumiSpecCALTower_DZ = 200;
  double LumiSpecCAL_DXY = 200;
  double LumiSpecCAL_Z = -48000 - LumiSpecCALTower_DZ/2.;
  double LumiSpecCAL_FiveSigma = 50;
  // cyclotron radius = speed / cyclotron frequency -> p/(q*B) = E/(c*q*B) in ultrarelativistic limit
  double pT = 0.3; // GeV. 0.3*B(T)*dZ(m)
  double RmagPreFactor = 3335.3950; // (J/GeV)/(c * q * B), multiply this by E in GeV to get R in mm

  double LumiConverter_Z = LumiAnalyzerMag_Z + LumiAnalyzerMag_DZ/2.0;
  double LumiAnalyzerMagEnd_Z = LumiAnalyzerMag_Z - LumiAnalyzerMag_DZ/2.0;
  double LumiConverterCut_DXY = 60;

  // spectrometer dimensions/placements in mm
  double LumiSpecTracker_Z1 = LumiSpecCAL_Z + LumiSpecCALTower_DZ/2.0 + 210;
  double LumiSpecTracker_Z2 = LumiSpecCAL_Z + LumiSpecCALTower_DZ/2.0 + 110;
  double LumiSpecTracker_Z3 = LumiSpecCAL_Z + LumiSpecCALTower_DZ/2.0 + 10;
  std::vector<double> Tracker_Zs = {LumiSpecTracker_Z1, LumiSpecTracker_Z2, LumiSpecTracker_Z3};

  double Tracker_meanZ = (LumiSpecTracker_Z1 + LumiSpecTracker_Z2 + LumiSpecTracker_Z3)/3.;

  double Tracker_pixelSize = 0.05; // mm
                            //maximal reduced chi^2 for tracks
  double max_chi2ndf = 0.01;
  double Tracker_sigma = 3.9; // mm, from distribution of reconstructed photon decaying to 2 electrons.

  double E_hit = 0;
  double x_hit = 0;
  double y_hit = 0;
  double z_hit = 0;
  double r_hit = 0;
  double t_hit = 0;
  int sec_id = 0;
  int mod_id = 0;
  int fiber_x_id = 0;
  int fiber_y_id = 0;

  int Nhits_cluster = 0;
  double E_cluster = 0;
  double x_cluster = 0;
  double y_cluster = 0;
  double r_cluster = 0;
  double t_cluster = 0;
  double Radius_cluster = 0;
  double Dispersion_cluster = 0;
  double SigmaThetaPhi1_cluster = 0;
  double SigmaThetaPhi2_cluster = 0;

}

#endif
