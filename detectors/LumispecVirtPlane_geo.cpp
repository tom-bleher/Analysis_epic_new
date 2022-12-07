#include "DD4hep/DetFactoryHelper.h"
#include "DD4hep/Printout.h"
#include "DD4hep/Shapes.h"
#include "DDRec/DetectorData.h"
#include "DDRec/Surface.h"
#include "TMath.h"
#include "XML/Layering.h"
#include <XML/Helper.h>

using namespace std;
using namespace dd4hep;

static Ref_t create_detector(Detector& description, xml_h e, SensitiveDetector sens)
{

  sens.setType("tracker");

  xml_det_t     x_det           = e;

  xml_comp_t    x_dim           = x_det.dimensions();
  xml_comp_t    x_pos           = x_det.position();
  xml_comp_t    x_rot           = x_det.rotation();
 
  string        det_name        = x_det.nameStr();
  int		det_ID		= x_det.id();
  DetElement 	det(det_name, det_ID);

  Volume motherVol = description.pickMotherVolume( det );

  string        mat_name        = dd4hep::getAttrOrDefault<string>( x_det, _U(material), "Silicon" );
  
  Box box( x_dim.x(), x_dim.y(), x_dim.z());
  Volume vol( det_name + "_vol", box, description.material( mat_name ) );
  vol.setVisAttributes( description.visAttributes( x_det.visStr() ) );
  vol.setSensitiveDetector( sens );

  Transform3D  pos( RotationZYX(x_rot.x(), x_rot.y(), x_rot.z()), Position( x_pos.x(), x_pos.y(), x_pos.z()) );

  PlacedVolume detPV = motherVol.placeVolume( vol, pos );
  detPV.addPhysVolID("system", det_ID);
  det.setPlacement(detPV);

  return det;
}

DECLARE_DETELEMENT(LumiSpecVirtPlane, create_detector)
