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

double Z = 1;
double electronPz = -18;
double hadronPz = 275;

double prefactor = 2.3179; // 4 alpha r_e^2 (mb)
double protonMass = 0.938272;
double electronMass = 0.51099895e-3;
double photonMass = 0;
double muonMass = 0.1056583745;
double pionZeroMass = 0.1349768;
double pionMass = 0.13957039;

void BH_plotter() {

  TF1 *BH_E = new TF1("BH_E", "[0] * ([1] - x)/(x*[1])*([1]/([1] - x) + ([1] - x)/[1] - 2/3.)*(log(4*[2]*[1]*([1] - x)/([3]*[4]*x)) - 0.5)", 0.1,20);
  BH_E->SetParameter( 0, Z*Z*prefactor );
  BH_E->SetParameter( 1, fabs( electronPz ) );
  BH_E->SetParameter( 2, fabs( hadronPz ) );
  BH_E->SetParameter( 3, protonMass );
  BH_E->SetParameter( 4, electronMass );
  BH_E->SetNpx( 10000 );
  BH_E->GetXaxis()->SetTitle("E (GeV)");
  BH_E->GetYaxis()->SetTitle("d#sigma/dE (mb/GeV)");
  BH_E->SetTitle("Bethe-Heitler");
  BH_E->GetXaxis()->SetRangeUser(0.1,18);

  BH_E->Draw();

  gPad->SetLogy();

}
