#include <fstream>
#include <iostream>
#include <sstream>

#include <boost/format.hpp>

#include <opensubdiv/bfr/surface.h> // Dummy include to ensure that we are using OpenSubdiv v3 with bfr
#include <opensubdiv/far/patchMap.h>
#include <opensubdiv/far/patchTable.h>
#include <opensubdiv/far/patchTableFactory.h>
#include <opensubdiv/far/ptexIndices.h>
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
      std::cout << boost::format("desc.numVertices %1%\n") % desc.numVertices;
      std::cout << boost::format("desc.numFaces %1%\n") % desc.numFaces;

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

        OpenSubdiv::Far::TopologyLevel const & refBaseLevel = refiner->GetLevel(0);

        OpenSubdiv::Far::PtexIndices ptexIndices(*refiner);
        std::cout << boost::format("ptexIndices.GetNumFaces() = %1%\n") % ptexIndices.GetNumFaces();

        int nfaces = refBaseLevel.GetNumFaces();
        int adjfaces[4];
        int adjedges[4];

        int _regFaceSize = OpenSubdiv::Sdc::SchemeTypeTraits::GetRegularFaceSize(refiner->GetSchemeType());
        std::cout << boost::format("_regFaceSize = %1%\n") % _regFaceSize;

        for (int face=0, ptexface=0; face<nfaces; ++face) {

          OpenSubdiv::Far::ConstIndexArray fverts = refBaseLevel.GetFaceVertices(face);

          if (fverts.size()==_regFaceSize) {
            ptexIndices.GetAdjacency(*refiner, face, 0, adjfaces, adjedges);
            // _adjacency[ptexface] = FaceInfo(adjfaces, adjedges, false);
            std::cout << boost::format("IS _regFaceSize adjfaces[%1% %2% %3% %4%] adjedges[%5% %6% %7% %8%]\n")
                % adjfaces[0]
                             % adjfaces[1]
                             % adjfaces[2]
                             % adjfaces[3]

                             % adjedges[0]
                             % adjedges[1]
                             % adjedges[2]
                             % adjedges[3]
                ;
            ++ptexface;
          } else {
            for (int vert=0; vert<fverts.size(); ++vert) {
              ptexIndices.GetAdjacency(*refiner, face, vert, adjfaces, adjedges);
              // _adjacency[ptexface+vert] = FaceInfo(adjfaces, adjedges, true);
            }
            std::cout << boost::format("IS NOT _regFaceSize adjfaces[%1% %2% %3% %4%] adjedges[%5% %6% %7% %8%]\n")
                             % adjfaces[0]
                             % adjfaces[1]
                             % adjfaces[2]
                             % adjfaces[3]

                             % adjedges[0]
                             % adjedges[1]
                             % adjedges[2]
                             % adjedges[3]
                ;
            ptexface+=fverts.size();
          }
        }



        OpenSubdiv::Far::PatchTable *patchTable =
            OpenSubdiv::Far::PatchTableFactory::Create(*refiner);
        if (patchTable) {
          OpenSubdiv::Far::PatchMap patchMap(*patchTable);

          double u = 0.5;
          double v = 0.5;
          int face_id = 1;


          const OpenSubdiv::Far::PatchTable::PatchHandle *handle =
              patchMap.FindPatch(face_id, u, v);
          if (handle) {
            std::cout << "Handle FOUND\n";
            std::cout << boost::format("arrayIndex %1%\n") % handle->arrayIndex;
            std::cout << boost::format("patchIndex %1%\n") % handle->patchIndex;
            std::cout << boost::format("vertIndex %1%\n") % handle->vertIndex;

            auto vertices = patchTable->GetPatchVertices(*handle);
            for (auto iter = vertices.begin();iter!=vertices.end();++iter) {
              std::cout << boost::format("iter %1%\n") % *iter;
              // I have vertices index via *iter but where can I use this index to look for the vertices ?
            }
          } else {
            std::cout << "Handle NOT FOUND\n";
          }
        }
      }
    }
  }
  return 0;
}
