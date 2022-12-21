//////////////////////////////////////////////////////////////
// derived from emcal_barrel_particles_gen.cxx
//////////////////////////////////////////////////////////////
#include <cmath>
#include <iostream>
#include <math.h>
#include <random>
#include <cctype>
#include <fmt/core.h>

#include "TFile.h"
#include "TMath.h"
#include "TRandom.h"
#include "TF1.h"
#include "TF2.h"
#include "TH2D.h"
#include "TLorentzVector.h"

#include "HepMC3/GenEvent.h"
#include "HepMC3/Print.h"
#include "HepMC3/ReaderAscii.h"
#include "HepMC3/WriterAscii.h"

using namespace std;
using namespace HepMC3;

std::tuple <int, double> extract_particle_parameters(std::string particle_name);
std::tuple<double, double> DD_BH();
std::tuple<double, double> SD_BH();
std::tuple<double, double> SD_BH_trf();
std::vector<GenParticlePtr> GenBHphoton();
std::vector<GenParticlePtr> GenConvertedElectrons();
void InitializeFunctions();


TF1 *BH_E_trf;
TF1 *BH_E;
TF1 *BH_Theta;
TF1 *BH_Phi;
TF2 *QED_BH;
TF1 *PDGsplitting;

TLorentzVector electron;
TLorentzVector hadron;
TLorentzVector electron_trf; // target rest frame

// converter center: -55610, starting edge: -55609.5
double Vz = -55610; // Primary vertex location in mm

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


void lumi_particles(int n_events = 1e6, bool flat=true, bool convert = false, double Egamma_start = 10.0, double Egamma_end = 10.0, string out_fname="genParticles.hepmc") {
 
  TFile *fout = new TFile("genEventsDiagnostics.root","RECREATE");
  TH1D *BH_h1 = new TH1D("BH_h1","E",100,0,10);
  TH1D *BH_trf_h1 = new TH1D("BH_trf_h1","E",100,0,10);
  TH2D *BH_h2 = new TH2D("BH_h2","E vs theta",100,0,10, 100,0,0.0035);
  TH2D *QED_BH_h2 = new TH2D("QED_BH_h2","E vs theta",100,0,10, 100,0,0.0035);
  TH1D *QED_BH_h1 = new TH1D("QED_BH_h1","E",100,0,10);

  WriterAscii hepmc_output(out_fname);
  
  int events_parsed = 0;
  
  GenEvent evt(Units::GEV, Units::MM);

  // Random number generator
  TRandom* r1 = new TRandom();

  InitializeFunctions();

  
  // Create events
  for (events_parsed = 0; events_parsed < n_events; events_parsed++) {
    // FourVector( px, py, pz, e, pdgid, status )

    // container for particles of interest
    vector<GenParticlePtr> poi;

    // Create one vertex and add beam particles to it
    GenVertexPtr v1 = std::make_shared<GenVertex>();
    
    // Generate beam particles
    // status 1(final state particle), 4(beam particle)
    GenParticlePtr p1 = std::make_shared<GenParticle>(
        FourVector(0.0, 0.0, electronPz, sqrt( pow(electronPz, 2) + pow(electronMass, 2) ) ), 11, 4);
    GenParticlePtr p2 = std::make_shared<GenParticle>(
        FourVector(0.0, 0.0, hadronPz, sqrt( pow(hadronPz, 2) + pow(protonMass, 2) ) ), 2212, 4);
    
    v1->add_particle_in( p1 );
    v1->add_particle_in( p2 );


    // E and theta of photon
    Double_t E, theta;
    if( flat ) { // Flat
      E = r1->Uniform(Egamma_start, Egamma_end); 
      theta = TMath::Pi();
    }
    else { // Bethe-Heitler
      std::tie(E, theta) = SD_BH();
      //std::tie(E, theta) = DD_BH();
      theta = TMath::Pi() - theta; // photons go toward -Z
    }

    Double_t phi = BH_Phi->GetRandom();

    if( convert) { // converted photons (e+ e-), conversion according to PDG Eq 34.31, primoridal pT = 0
      
      Double_t E_electron = E * PDGsplitting->GetRandom();
      Double_t E_positron = E - E_electron;
      
      // Remove very low energy electrons
      if( E_electron < 0.001 || E_positron < 0.001 ) { continue; }
      
      Double_t p_electron = sqrt( pow(E_electron,2) - pow(electronMass,2) );
      Double_t p_positron = sqrt( pow(E_positron,2) - pow(electronMass,2) );
      
      // Add particles of interest
      auto [id, mass] = extract_particle_parameters( "electron" );
      GenParticlePtr p3 = std::make_shared<GenParticle>( 
          FourVector(p_electron*sin(theta)*cos(phi), p_electron*sin(theta)*sin(phi), p_electron*cos(theta), E_electron), id, 1);
      GenParticlePtr p4 = std::make_shared<GenParticle>( 
          FourVector(p_positron*sin(theta)*cos(phi), p_positron*sin(theta)*sin(phi), p_positron*cos(theta), E_positron), -id, 1);

      poi.push_back( p3 );
      poi.push_back( p4 );
    }
    else { // unconverted photons

      auto [id, mass] = extract_particle_parameters( "photon" );
      GenParticlePtr p3 = std::make_shared<GenParticle>( 
          FourVector(E*sin(theta)*cos(phi), E*sin(theta)*sin(phi), E*cos(theta), E), id, 1);

      // Add particle of interest
      poi.push_back( p3 );
    }

    for( auto el : poi ) { v1->add_particle_out( el ); }
    evt.add_vertex( v1 );

    // Shift the whole event to specific point
    evt.shift_position_to( FourVector(0,0, Vz, 0) );

    if (events_parsed == 0) {
      std::cout << "First event: " << std::endl;
      Print::listing(evt);
    }

    hepmc_output.write_event(evt);
    
    if (events_parsed % 10000 == 0) {
      std::cout << "Event: " << events_parsed << std::endl;
    }
    
    evt.clear();
  }
  hepmc_output.close();
  std::cout << "Events parsed and written: " << events_parsed << std::endl;

  BH_h1->Write();
  BH_trf_h1->Write();
  QED_BH_h1->Write();
  BH_h2->Write();
  QED_BH_h2->Write();
  fout->Close();

}

//----------------------------------------------------------------------------
// Returns particle pdgID and mass in [GeV]
std::tuple <int, double> extract_particle_parameters(std::string particle_name) {
  if (particle_name == "electron") return std::make_tuple(11,    electronMass);
  if (particle_name == "photon")   return std::make_tuple(22,    photonMass);
  if (particle_name == "positron") return std::make_tuple(-11,   electronMass);
  if (particle_name == "proton")   return std::make_tuple(2212,  protonMass);
  if (particle_name == "muon")     return std::make_tuple(13,    muonMass);
  if (particle_name == "pi0")      return std::make_tuple(111,   pionZeroMass);
  if (particle_name == "piplus")   return std::make_tuple(211,   pionMass);
  if (particle_name == "piminus")  return std::make_tuple(-211,  pionMass);

  std::cout << "wrong particle name" << std::endl;
  abort();
}

//----------------------------------------------------------------------------
void InitializeFunctions() {
        
  electron.SetXYZM(0, 0, electronPz, electronMass);
  hadron.SetXYZM(0, 0, hadronPz, protonMass);
  electron_trf = electron;
  electron_trf.Boost(0, 0, -hadron.Beta());

  // Bethe-Heitler photon energy in the Lab Frame
  BH_E = new TF1("BH_E", "[0] * ([1] - x)/(x*[1])*([1]/([1] - x) + ([1] - x)/[1] - 2/3.)*(log(4*[2]*[1]*([1] - x)/([3]*[4]*x)) - 0.5)", 0.1,5);
  BH_E->SetParameter( 0, Z*Z*prefactor );
  BH_E->SetParameter( 1, fabs( electron.E() ) );
  BH_E->SetParameter( 2, fabs( hadron.E() ) );
  BH_E->SetParameter( 3, protonMass );
  BH_E->SetParameter( 4, electronMass );
  BH_E->SetNpx( 10000 );

  BH_E_trf = new TF1("BH_E_trf", "[0] * ([1] - x)/(x*[1])*([1]/([1] - x) + ([1] - x)/[1] - 2/3.)*(log(2*[1]*([1] - x)/([2]*x)) - 0.5)", 0.1,5);
  BH_E_trf->SetParameter( 0, Z*Z*prefactor );
  //BH_E_trf->SetParameter( 1, fabs( electron.E() ) );
  //cout<<electron.E()<<"  "<<electron_trf.E()<<endl;
  BH_E_trf->SetParameter( 1, fabs( electron_trf.E() ) );
  BH_E_trf->SetParameter( 2, electronMass );
  BH_E_trf->SetNpx( 10000 );

  // Lifshitz doubly-differential Bethe-Heitler in hadron rest frame
  QED_BH = new TF2("QED_BH", "[0] * 1/x * ([1] - x)/[1] * y/pow(1 + y*y, 2) * ( ( [1]/([1] - x) + ([1] - x)/[1] - 4*y*y/pow(1 + y*y, 2) ) * log(2*[1]*([1] - x)/([2]*x)) - 0.5*( [1]/([1] - x) + ([1] - x)/[1] + 2 - 16*y*y/pow(1 + y*y, 2)) )", 0.1,5, 0,10);
  QED_BH->SetParameter( 0, 2*Z*Z*prefactor );
  QED_BH->SetParameter( 1, fabs(electron_trf.E()) );
  QED_BH->SetParameter( 2, electronMass );
  QED_BH->SetNpx( 10000 );
  QED_BH->SetNpy( 100 );

  // Bethe-Heitler photon angle wrt beam electron in the Lab Frame
  BH_Theta = new TF1("BH_Theta", "x / pow( [0]*[0] + x*x, 2)", 0,TMath::Pi());
  BH_Theta->SetParameter(0, electronMass / sqrt( pow(electronMass,2) + pow(electronPz,2) ) );
  BH_Theta->SetNpx( 100000 );

  // Flat azimuthal angle
  BH_Phi = new TF1("BH_Phi","1", 0, 2*TMath::Pi() );

  // photon splitting fuction from PDG Eq 34.31
  PDGsplitting = new TF1("PDGsplitting","1 - 4/3.*x*(1-x)", 0,1);

}

// Double-Differential Bethe-Heitler (target rest frame)
//----------------------------------------------------------------------------
std::tuple<double, double> DD_BH() {

  double E=0, delta=0, theta=0, phi=0;

  // Sample 2D E and delta in Target Rest Frame
  QED_BH->GetRandom2( E, delta );
  phi = BH_Phi->GetRandom();

  // From Lifshitz QED, delta = E_e/m_e * theta
  theta = TMath::Pi() - electronMass / fabs(electronPz) * delta;

  // create photon 4-vector in Target Rest Frame
  TLorentzVector gamma_TRF;
  gamma_TRF.SetXYZM( E*sin(theta)*cos(phi), E*sin(theta)*sin(phi), E*cos(theta), 0.0 );

  // Boost to Lab Frame and return those quantities
  // TODO: Doesn't work at the moment
  TLorentzVector gamma_Lab = gamma_TRF;
  gamma_Lab.Boost(0, 0, hadron.Beta());
  //E = gamma_Lab.E();
  //theta = TMath::Pi() - gamma_Lab.Theta();

  //cout<<"TRF E,Theta: "<<E<<", "<<theta<<"    Lab E,Theta: "<<gamma_Lab.E()<<", "<<gamma_Lab.Theta()<<endl;
  //cout<<"Beta: "<<hadron.Beta()<<endl;

  return std::make_tuple( E, theta );
}

// Single-Differential Bethe-Heitler (Lab frame)
//----------------------------------------------------------------------------
std::tuple<double, double> SD_BH() {

  double E = BH_E->GetRandom();
  double theta = BH_Theta->GetRandom();

  return std::make_tuple( E, theta );
}

// Single-Differential Bethe-Heitler (target rest frame)
//----------------------------------------------------------------------------
std::tuple<double, double> SD_BH_trf() {

  double E = BH_E_trf->GetRandom();
  double theta = BH_Theta->GetRandom();

  TLorentzVector gamma_TRF;
  gamma_TRF.SetXYZM( E*sin(theta), 0, -E*cos(theta), 0.0 );

  TLorentzVector gamma_Lab = gamma_TRF;
  gamma_Lab.Boost(0, 0, hadron.Beta());
  //E = gamma_Lab.E();

  return std::make_tuple( E, theta );
}
