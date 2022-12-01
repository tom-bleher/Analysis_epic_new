//////////////////////////////////////////////////////////////
// derived from emcal_barrel_particles_gen.cxx
//////////////////////////////////////////////////////////////
#include "HepMC3/GenEvent.h"
#include "HepMC3/Print.h"
#include "HepMC3/ReaderAscii.h"
#include "HepMC3/WriterAscii.h"

#include "TMath.h"
#include "TRandom.h"

#include <cmath>
#include <iostream>
#include <math.h>
#include <random>
#include <cctype>
#include <fmt/core.h>

using namespace std;
using namespace HepMC3;

std::tuple <int, double> extract_particle_parameters(std::string particle_name);


void hepmc_particles(int n_events = 1e5, double e_start = 2.0, double e_end = 2.0, std::string particle_name = "photon") {
  
  std::string out_fname = "genParticles.hepmc";
  WriterAscii hepmc_output(out_fname);
  
  int events_parsed = 0;
  
  GenEvent evt(Units::GEV, Units::MM);

  // Random number generator
  TRandom* r1 = new TRandom();

  double protonMass = 0.938272;
  double electronMass = 0.51099895e-3;
  double electronPz = -18;
  double protonPz = 275;

  for (events_parsed = 0; events_parsed < n_events; events_parsed++) {
    // FourVector( px, py, pz, e, pdgid, status )
    // status 1(final state particle), 4(beam particle)

    // Add beam particles
    GenParticlePtr p1 = std::make_shared<GenParticle>(
        FourVector(0.0, 0.0, electronPz, sqrt( pow(electronPz, 2) + pow(electronMass, 2) ) ), 11, 4);
    GenParticlePtr p2 = std::make_shared<GenParticle>(
        FourVector(0.0, 0.0, protonPz, sqrt( pow(protonPz, 2) + pow(protonMass, 2) ) ), 2212, 4);

    // Define momentum for particle of interest
    Double_t p        = r1->Uniform(e_start, e_end);
    Double_t phi      = r1->Uniform(0.0, 2.0 * M_PI);
    Double_t theta    = TMath::Pi();
    Double_t px       = p * std::cos(phi) * std::sin(theta);
    Double_t py       = p * std::sin(phi) * std::sin(theta);
    Double_t pz       = p * std::cos(theta);
    
    // Add particle of interest
    auto [id, mass] = extract_particle_parameters(particle_name);
    GenParticlePtr p3 = std::make_shared<GenParticle>( 
        FourVector(px, py, pz, sqrt(p * p + (mass * mass))), id, 1);

    // Add all particles to one vertex
    GenVertexPtr v1 = std::make_shared<GenVertex>();
    v1->add_particle_in( p1 );
    v1->add_particle_in( p2 );
    v1->add_particle_out( p3 );
    evt.add_vertex( v1 );

    // Shift the whole event to specific point
    //evt.shift_position_to( FourVector(0,0,-19000, 0) );

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
}

//----------------------------------------------------------------------------
// Returns particle pdgID and mass in [GeV]
std::tuple <int, double> extract_particle_parameters(std::string particle_name) {
    if (particle_name == "electron") return std::make_tuple(11,    0.51099895e-3);
    if (particle_name == "photon")   return std::make_tuple(22,    0.0);
    if (particle_name == "positron") return std::make_tuple(-11,   0.51099895e-3);
    if (particle_name == "proton")   return std::make_tuple(2212,  0.938272);
    if (particle_name == "muon")     return std::make_tuple(13,    0.1056583745);
    if (particle_name == "pi0")      return std::make_tuple(111,   0.1349768);
    if (particle_name == "piplus")   return std::make_tuple(211,   0.13957039);
    if (particle_name == "piminus")  return std::make_tuple(-211,  0.13957039);

    std::cout << "wrong particle name" << std::endl;
    abort();
}

