#include <cmath>
#include <iostream>
#include <math.h>
#include <random>
#include <cctype>

#include "TFile.h"
#include "TMath.h"
#include "TRandom.h"
#include "TF1.h"
#include "TF2.h"
#include "TH2D.h"
#include "TLorentzVector.h"

using namespace std;


void Sigma_compare() {

  gStyle->SetOptStat(0);

  TCanvas *c1 = new TCanvas("c1","c1",10,10,700,500);
   c1->SetGrid();
   c1->SetTopMargin(0.15);
 
  gPad->DrawFrame(0.,0.,35,0.55);

  const Int_t nx = 6;
  const char *configs[nx] = {"1 Tracker", "2 Trackers", "3 Trackers", "Air Chamber", "He Chamber", "Beam effects"};
  // Tracker Mean values 
  // 1 Tracker:    10.630444 +- 0.07297897
  // 2 Tracker:    10.455045 +- 0.12112166
  // 3 Tracker:    10.492915 +- 0.099390803
  // Air Chamber:  10.535435 +- 0.11108534
  // He Chamber:   10.594105 +- 0.082949932
  // Beam effects: 10.513104 +- 0.089902352
  //double y[nx] = {10.630444, 10.455045, 10.492915, 10.535435, 10.594105, 10.513104};
  //double y_e[nx] = {0.07297897, 0.12112166, 0.099390803, 0.11108534, 0.082949932, 0.08990235};
  
  //
  // Tracker RMS values 
  // 1 Tracker:    2.4005535 +- 0.051603928
  // 2 Tracker:    3.8129284 +- 0.085645949
  // 3 Tracker:    3.1130113 +- 0.070279911
  // Air Chamber:  3.2557670 +- 0.078549196
  // He Chamber:   3.3714837 +- 0.058654460
  // Beam effects: 3.3397241 +- 0.063570562
  //double y[nx] = {2.4005535, 3.8129284, 3.1130113, 3.2557670, 3.3714837, 3.3397241};
  //double y_e[nx] = {0.051603928, 0.085645949, 0.070279911, 0.078549196, 0.058654460, 0.063570562};
  //
  //
  // CAL sigma values 
  // 1 Tracker:    4.56350e-02 +- 2.55830e-03
  // 2 Tracker:    4.50073e-02 +- 2.91969e-03
  // 3 Tracker:    5.86422e-02 +- 6.00028e-03
  // Air Chamber:  4.52277e-02 +- 4.62463e-03
  // He Chamber:   3.93956e-02 +- 1.75667e-03
  // Beam effects: 4.03972e-02 +- 2.01825e-03
  double y[nx] = {4.56350e-02, 4.50073e-02, 5.86422e-02, 4.52277e-02, 3.93956e-02, 4.03972e-02};
  double y_e[nx] = {2.55830e-03, 2.91969e-03, 6.00028e-03, 4.62463e-03, 1.75667e-03, 2.01825e-03};

   TH1F *h = new TH1F("h","Tracker resolutions",3,0,3);
   h->SetStats(0);
   h->SetFillColor(38);
   h->SetCanExtend(TH1::kAllAxes);
   for (Int_t bin=1; bin < nx+1; bin++) {
      h->SetBinContent(bin, y[bin-1]); 
      h->SetBinError(bin, y_e[bin-1]); 
      h->GetXaxis()->SetBinLabel(bin, configs[bin-1]);
   }
   h->GetYaxis()->SetTitle("CAL #sigma");
   //h->GetYaxis()->SetTitle("E_{rec} rms");
   //h->GetYaxis()->SetTitle("<E_{rec}>");
   h->SetTitle("CAL");
   //h->SetTitle("Trackers");
   h->GetXaxis()->SetLabelSize(0.06);
   h->GetYaxis()->SetTitleSize(0.06);
   h->GetYaxis()->SetTitleOffset(0.8);
   //h->LabelsDeflate();
   h->Draw();
   //TPaveText *pt = new TPaveText(0.7,0.85,0.98,0.98,"brNDC");
   //pt->SetFillColor(18);
   //pt->SetTextAlign(12);
   //pt->AddText("Use the axis Context Menu LabelsOption");
   //pt->AddText(" \"a\"   to sort by alphabetic order");
   //pt->AddText(" \">\"   to sort by decreasing values");
   //pt->AddText(" \"<\"   to sort by increasing values");
   //pt->Draw();
   //return c1;
 

  gPad->SetGridx();
  gPad->SetGridy();

}
