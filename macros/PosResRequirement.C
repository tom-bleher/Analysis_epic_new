#include <cmath>
#include <iostream>
#include <math.h>
#include <random>
#include <cctype>

#include "TFile.h"
#include "TMath.h"
#include "TRandom3.h"
#include "TF1.h"
#include "TF2.h"
#include "TH2D.h"
#include "TLorentzVector.h"

using namespace std;

// Criteria
// e beam might move by 3*sigma at the most (loose conservative estimate from discussions with Christoph Montag
// Use a long-tail function (exp) to estimate extrapolation bias of a 2*sigma "clipped" Gaussian
const double ClippedNsigma = 1; // Nsigma from beam center
const double posRes = 10; // position resolution in mm
const double OneSigma = 12; // (1 sigma at 58 m) = 12 mm
const double Kmax = 3125e-7;
const double posMax = 5*OneSigma;
const double Nfills = 100000;

void PosResRequirement() {

  gStyle->SetOptStat(0);

  TRandom3 *rndm = new TRandom3();
  rndm->SetSeed(0);
  
  TCanvas *can1 = new TCanvas("can1","",10,10,700,500);
  gPad->DrawFrame(0.,0.,20,0.55);

  double expectedIntegral = Nfills / (OneSigma * sqrt(2*TMath::Pi()));

  //TF1 *source = new TF1("source", "[0]*(gaus(1) + ((fabs(x)<10)?0.0:1.0)*[4]*pow(fabs(x/[3]),-3))", -posMax,posMax);
  //source->FixParameter(0,expectedIntegral);
  //source->FixParameter(1, 0);
  //source->FixParameter(2, OneSigma);
  //source->FixParameter(4, Kmax );
  TF1 *source = new TF1("source", "[0]*gaus(1)", -posMax,posMax);
  source->FixParameter(0, expectedIntegral );
  source->FixParameter(1, 1);
  source->FixParameter(2, 0);
  source->FixParameter(3, OneSigma);
  source->SetLineColor(4);
  source->SetNpx(1000);

  //
  TF1 *mygauss = new TF1("mygauss", "[0]*exp( -0.5*pow((x-[1])/[2],2) )", -posMax,posMax);
  mygauss->SetParameter(0, expectedIntegral);
  mygauss->SetParameter(1, 0);
  mygauss->SetParameter(2, OneSigma);
  mygauss->SetLineStyle(2);

  TH2D *acceptError = new TH2D("acceptError","Acceptance error;truncation (N#sigma);position resolution (mm)",6,-0.5,5.5, 21,-0.5,20.5);

  bool done = false;
  for( int clip = 0; clip < 6; clip++ ) {
    for( int res = 0; res < 21; res++ ) {
      if( done ) continue;
      double Nbins = 2*posMax / posRes;
      TH1D *dist = new TH1D(Form("dist_%i%i",clip,res),";x_{#gamma};N_{#gamma}", round(2*posMax/1.0),-posMax,posMax);
      dist->SetMarkerStyle(25);
      dist->Sumw2();
      TLegend *leg = new TLegend(0.6,0.65, 0.9,0.9);

      for( int n = 0; n < Nfills; n++ ) {
        double x = source->GetRandom( -posMax, posMax );
        if( x > clip * OneSigma ) { continue; }
        double x_smeared = x + rndm->Gaus(0, res);
        dist->Fill( x_smeared );
      }

      dist->Fit( mygauss, "im", "n", -posMax, 0.75*clip*OneSigma );

      if( clip == round(ClippedNsigma) && res == round(posRes) ) {
        
        dist->DrawCopy();
        source->Draw("same");
        mygauss->DrawCopy("same");
        leg->AddEntry(source, "source", "l");
        leg->AddEntry(dist, "simulated", "p");
        leg->AddEntry(mygauss, "fit to simulated", "l");
        leg->Draw("same");

        double trueYield = source->Integral(-posMax, posMax);
        double FitYield = mygauss->Integral(-5*mygauss->GetParameter(2), 5*mygauss->GetParameter(2));
        double accErr = fabs(FitYield - trueYield)/trueYield;
        cout<<"Xi2/NDF: "<<mygauss->GetChisquare() / mygauss->GetNDF() <<endl;
        cout<<"clip: "<<clip<<"  res: "<<res<<"   Acceptance error: "<<accErr<<endl;
        acceptError->Fill( clip, res, accErr );
        done = true;
      }
      dist->Delete();
    }
  }

  TCanvas *can2 = new TCanvas("can2","",10,600,700,500);
  gPad->DrawFrame(0.,0.,20,0.55);

  acceptError->Draw("colz");

 
}

