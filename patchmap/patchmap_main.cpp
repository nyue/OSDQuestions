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

void print_specific_level(const OpenSubdiv::Far::TopologyRefiner *refiner, int levelOfInterest) {

  OpenSubdiv::Far::TopologyLevel const &thisLevel = refiner->GetLevel(levelOfInterest);
  int thisLevelNumFaces = thisLevel.GetNumFaces();

  for (int faceIndex =0, ptexface=0; faceIndex < thisLevelNumFaces; ++faceIndex) {
    OpenSubdiv::Far::ConstIndexArray thisLevelFaceVertices =
        thisLevel.GetFaceVertices(faceIndex);

    std::cout << boost::format("thisLevelFaceVertices[lvl=%1%][%2%] : {") % levelOfInterest % faceIndex;
    for (auto faceVertex : thisLevelFaceVertices) {
      std::cout << boost::format(" %1%") % faceVertex;
    }
    std::cout << "}\n";
  }
}
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

#ifdef DONT

        OpenSubdiv::Far::TopologyLevel const & refBaseLevel = refiner->GetLevel(0);
        OpenSubdiv::Far::TopologyLevel const & refLevel1 = refiner->GetLevel(1);

        OpenSubdiv::Far::PtexIndices ptexIndices(*refiner);
        std::cout << boost::format("ptexIndices.GetNumFaces() = %1%\n") % ptexIndices.GetNumFaces();

        int baseLevelNFaces = refBaseLevel.GetNumFaces();
        std::cout << boost::format("baseLevelNFaces = %1%\n") % baseLevelNFaces;
        int adjfaces[4];
        int adjedges[4];

        int level1NFaces = refLevel1.GetNumFaces();
        std::cout << boost::format("level1NFaces = %1%\n") % level1NFaces;

        int _regFaceSize = OpenSubdiv::Sdc::SchemeTypeTraits::GetRegularFaceSize(refiner->GetSchemeType());
        std::cout << boost::format("_regFaceSize = %1%\n") % _regFaceSize;

        for (int face=0, ptexface=0; face< baseLevelNFaces; ++face) {

          OpenSubdiv::Far::ConstIndexArray baseLevelFVerts = refBaseLevel.GetFaceVertices(face);

          std::cout << boost::format("baseLevelFVerts[%1%] : {") % face;
          for (auto faceIndex : baseLevelFVerts) {
            std::cout << boost::format(" %1%") % faceIndex;
          }
          std::cout << "}\n";

          if (baseLevelFVerts.size()==_regFaceSize) {
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
            for (int vert=0; vert< baseLevelFVerts.size(); ++vert) {
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
            ptexface+= baseLevelFVerts.size();
          }
        }
        for (int face=0, ptexface=0; face< level1NFaces; ++face) {
          OpenSubdiv::Far::ConstIndexArray level1FVerts = refLevel1.GetFaceVertices(face);

          std::cout << boost::format("level1FVerts[%1%] : {") % face;
          for (auto faceIndex : level1FVerts) {
            std::cout << boost::format(" %1%") % faceIndex;
          }
          std::cout << "}\n";
        }
#endif
        print_specific_level(refiner, 0);
        print_specific_level(refiner, 1);

        OpenSubdiv::Far::PatchTable *patchTable =
            OpenSubdiv::Far::PatchTableFactory::Create(*refiner);
        if (patchTable) {
          OpenSubdiv::Far::PatchMap patchMap(*patchTable);

          double u = 0.4;
          double v = 0.4;
          int face_id = 2;

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
