#include <fstream>
#include <iostream>
#include <sstream>

#include <boost/format.hpp>

#include <opensubdiv/bfr/surface.h> // Dummy include to ensure that we are using OpenSubdiv v3 with bfr
#include <opensubdiv/far/patchMap.h>
#include <opensubdiv/far/patchTable.h>
#include <opensubdiv/far/patchTableFactory.h>
#include <opensubdiv/far/stencilTableFactory.h>
#include <opensubdiv/far/topologyDescriptor.h>
#include <opensubdiv/far/topologyRefiner.h>
#include <opensubdiv/far/topologyRefinerFactory.h>
#include <opensubdiv/osd/cpuVertexBuffer.h>
#include <opensubdiv/vtr/types.h> // Dummy include to ensure that we are using OpenSubdiv v3 or later

#include "shape_utils.h"

int main(int argc, char **argv) {

  if (argc == 2) {
    std::cout << argc << "\n";

    std::ifstream objstream(argv[1]);
    std::stringstream objbuffer;
    objbuffer << objstream.rdbuf();

    Shape *shape = Shape::parseObj(objbuffer.str().c_str(), Scheme::kCatmark);
    if (shape) {
      typedef OpenSubdiv::Far::TopologyDescriptor Descriptor;
      Descriptor desc;
      desc.numVertices = shape->GetNumVertices();
      desc.numFaces = shape->GetNumFaces();
      desc.numVertsPerFace = shape->nvertsPerFace.data();
      desc.vertIndicesPerFace = shape->faceverts.data();

      OpenSubdiv::Sdc::SchemeType type = OpenSubdiv::Sdc::SCHEME_CATMARK;

      OpenSubdiv::Sdc::Options options;
      // options.SetVtxBoundaryInterpolation(OpenSubdiv::Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);
      // options.SetFVarLinearInterpolation(OpenSubdiv::Sdc::Options::FVAR_LINEAR_BOUNDARIES);

      OpenSubdiv::Far::TopologyRefiner *refiner;

      refiner = OpenSubdiv::Far::TopologyRefinerFactory<Descriptor>::Create(
          desc, OpenSubdiv::Far::TopologyRefinerFactory<Descriptor>::Options(
                    type, options));

      if (refiner) {

        OpenSubdiv::Far::TopologyRefiner::UniformOptions refineOptions(1);
        refineOptions.fullTopologyInLastLevel = true;

        refiner->RefineUniform(refineOptions);

        std::cout << boost::format("MaxLevel %1%\n") % refiner->GetMaxLevel();
        std::cout << boost::format("NumLevels %1%\n") % refiner->GetNumLevels();
        std::cout << boost::format("NumEdges %1%\n") %
                         refiner->GetNumEdgesTotal();
        std::cout << boost::format("NumFaces %1%\n") %
                         refiner->GetNumFacesTotal();
        std::cout << boost::format("NumVertices %1%\n") %
                         refiner->GetNumVerticesTotal();
        OpenSubdiv::Far::PatchTable *pt =
            OpenSubdiv::Far::PatchTableFactory::Create(*refiner);
        if (pt) {
          OpenSubdiv::Far::PatchMap pm(*pt);

          double u = 0.5;
          double v = 0.5;
          int face_id = 1;
          const OpenSubdiv::Far::PatchTable::PatchHandle *handle =
              pm.FindPatch(face_id, u, v);
          if (handle) {
            std::cout << "Handle FOUND\n";
            std::cout << boost::format("arrayIndex %1%\n") % handle->arrayIndex;
            std::cout << boost::format("patchIndex %1%\n") % handle->patchIndex;
            std::cout << boost::format("vertIndex %1%\n") % handle->vertIndex;
          } else {
            std::cout << "Handle NOT FOUND\n";
          }
        }
      }
    }
  }
  return 0;
}
