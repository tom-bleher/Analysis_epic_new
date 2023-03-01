
R__LOAD_LIBRARY(libDDRec)
R__LOAD_LIBRARY(libfmt)

#include <DD4hep/Detector.h>
#include <DDRec/CellIDPositionConverter.h>
#include <DDSegmentation/BitFieldCoder.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <bitset>

#include <ROOT/RDataFrame.hxx>
#include <TH2D.h>
#include <TProfile.h>
#include <TFile.h>
#include <TTree.h>
#include <THashList.h>
#include <TCollection.h>

#include <edm4hep/SimTrackerHit.h>
#include <edm4hep/SimCalorimeterHit.h>
#include <edm4hep/RawCalorimeterHit.h>

#include <fmt/format.h>

int LumiHitsAnalysis() {

  TString simFile = "../simulations/Output.edm4hep.root";
  TString geomFile = "/home/dhevan/eic/epic/epic_ip6.xml";
  TFile *fout = new TFile("anaOutput.root","RECREATE");

  THashList *gHistList = new THashList();
  gHistList->AddLast( new TH1D("hErawTotal", "summed hit energy (raw)", 2500, 0, 50) );
  

  // Get a handle to the global detector
  dd4hep::Detector* det = &(dd4hep::Detector::getInstance());

  // specify detector geometry file
  det->fromCompact(geomFile.Data());
  
  det->volumeManager();
  det->apply("DD4hepVolumeManager", 0, 0);
  
  // Create our cellID converter. This is useful to get cell coordinates. Not used below
  auto cellIDConverter = std::make_shared<const dd4hep::rec::CellIDPositionConverter>(*det);

  // Get our readout ID spec
  auto idSpec = det->readout("LumiSpecCALHits").idSpec();
  // Get our cell ID decoder
  auto id_dec = idSpec.decoder();

  const int sector_idx = id_dec->index("sector");
  const int module_idx = id_dec->index("module");

  // C++ Lambda function to contain the analysis algorithm.
  auto LumiSpecCALroutine =
      [&]( const std::vector<edm4hep::SimCalorimeterHitData>& h ) {
        
        double E_total = 0;
        
        for (size_t i = 0; i < h.size(); ++i) {
          const int sector = id_dec->get(h[i].cellID, sector_idx);
          const int module = id_dec->get(h[i].cellID, module_idx);

          auto gpos = cellIDConverter->position( h[i].cellID );
          cout<<"hit X: "<<gpos.x()<<"   hit Y: "<<gpos.y()<<endl;

          double E = h[i].energy;
          E_total += E;
        }
        
        if( E_total > 0 ) { ((TH1D *)gHistList->FindObject("hErawTotal"))->Fill( E_total ); }
      };

  // Create a Root DataFrame from a root file with a tree of name "events". 
  ROOT::RDataFrame df{"events", simFile.Data()};

  // Run the Lambda function over the contents of the DataFrame
  df.Foreach( LumiSpecCALroutine, {"LumiSpecCALHits"} );

  // Save each element of gHistList to file
  TIter iter(gHistList);
  TObject *obj = iter.Next();
  
  while ( obj ) {
    obj->Write();
    obj = iter.Next();
  }

  fout->Close();

  return 0;
}
