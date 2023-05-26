#include "HepMC3/GenEvent.h"
#include "HepMC3/ReaderAscii.h"
#include "HepMC3/WriterRoot.h"
#include "HepMC3/Print.h"
#include <iostream>

using namespace HepMC3;

int hepmcToRoot(string infile, string outfile) {

  ReaderAscii text_input ( infile );
  WriterRoot  root_output( outfile );

  int events_parsed = 0;

  while( !text_input.failed() ) {
    GenEvent evt(Units::GEV,Units::MM);
    text_input.read_event(evt);
    if( text_input.failed() ) break;
    if( events_parsed == 0 ) {
      std::cout << "First event: " << std::endl;
      Print::listing(evt);
    }
    root_output.write_event(evt);
    ++events_parsed;
    if( events_parsed%100 == 0 ) {
      std::cout << "Event: " << events_parsed << std::endl;
    }
  }
  
  text_input.close();
  root_output.close();

  std::cout << "Events parsed and written: " << events_parsed << std::endl;
  return 0;
}
