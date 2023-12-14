//
//   Copyright 2013 Pixar
//
//   Licensed under the Apache License, Version 2.0 (the "Apache License")
//   with the following modification; you may not use this file except in
//   compliance with the Apache License and the following modification to it:
//   Section 6. Trademarks. is deleted and replaced with:
//
//   6. Trademarks. This License does not grant permission to use the trade
//      names, trademarks, service marks, or product names of the Licensor
//      and its affiliates, except as required to comply with Section 4(c) of
//      the License and to reproduce the content of the NOTICE file.
//
//   You may obtain a copy of the Apache License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the Apache License with the above modification is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//   KIND, either express or implied. See the Apache License for the specific
//   language governing permissions and limitations under the Apache License.
//

//------------------------------------------------------------------------------
// Tutorial description:
//
// Building on tutorial 0, this example shows how to instantiate a simple mesh,
// refine it uniformly and then interpolate both 'vertex' and 'face-varying'
// primvar data.
// The resulting interpolated data is output as an 'obj' file, with the
// 'face-varying' data recorded in the uv texture layout.
//

#include <opensubdiv/far/primvarRefiner.h>
#include <opensubdiv/far/topologyDescriptor.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utils/shape_utils.h>

//------------------------------------------------------------------------------
// Face-varying implementation.
//
//
struct Vertex
{

  // Minimal required interface ----------------------
  Vertex() {}

  Vertex(Vertex const& src)
  {
    _position[0] = src._position[0];
    _position[1] = src._position[1];
    _position[2] = src._position[2];
  }

  void Clear(void* = 0) { _position[0] = _position[1] = _position[2] = 0.0f; }

  void AddWithWeight(Vertex const& src, float weight)
  {
    _position[0] += weight * src._position[0];
    _position[1] += weight * src._position[1];
    _position[2] += weight * src._position[2];
  }

  // Public interface ------------------------------------
  void SetPosition(float x, float y, float z)
  {
    _position[0] = x;
    _position[1] = y;
    _position[2] = z;
  }

  const float* GetPosition() const { return _position; }

private:
  float _position[3];
};

//------------------------------------------------------------------------------
// Face-varying container implementation.
//
// We are using a uv texture layout as a 'face-varying' primitive variable
// attribute. Because face-varying data is specified 'per-face-per-vertex',
// we cannot use the same container that we use for 'vertex' or 'varying'
// data. We specify a new container, which only carries (u,v) coordinates.
// Similarly to our 'Vertex' container, we add a minimalistic interpolation
// interface with a 'Clear()' and 'AddWithWeight()' methods.
//
struct FVarVertexUV
{

  // Minimal required interface ----------------------
  void Clear() { u = v = 0.0f; }

  void AddWithWeight(FVarVertexUV const& src, float weight)
  {
    u += weight * src.u;
    v += weight * src.v;
  }

  // Basic 'uv' layout channel
  float u, v;
};

//------------------------------------------------------------------------------
int main(int argc, char** argv)
{

  if (argc != 3)
  {
    std::cerr << "Usage: app <level> <obj>\n";
    return EXIT_FAILURE;
  }
  int maxlevel = atoi(argv[1]);
  int channelUV = 0;
  std::ifstream objstream(argv[2]);
  std::stringstream objbuffer;
  objbuffer << objstream.rdbuf();

  Shape* shape = Shape::parseObj(objbuffer.str().c_str(), Scheme::kCatmark);
  if (shape && shape->HasUV())
  {

    typedef OpenSubdiv::Far::TopologyDescriptor Descriptor;

    OpenSubdiv::Sdc::SchemeType type = OpenSubdiv::Sdc::SCHEME_CATMARK;

    OpenSubdiv::Sdc::Options options;
    options.SetVtxBoundaryInterpolation(OpenSubdiv::Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);
    options.SetFVarLinearInterpolation(OpenSubdiv::Sdc::Options::FVAR_LINEAR_NONE);

    // Populate a topology descriptor with our raw data
    Descriptor desc;
    desc.numVertices = shape->GetNumVertices();
    desc.numFaces = shape->GetNumFaces();
    desc.numVertsPerFace = shape->nvertsPerFace.data();
    desc.vertIndicesPerFace = shape->faceverts.data();

    int channelUV = 0;

    // Create a face-varying channel descriptor
    Descriptor::FVarChannel channels[1];
    channels[channelUV].numValues = shape->faceuvs.size();
    channels[channelUV].valueIndices = shape->faceuvs.data();

    // Add the channel topology to the main descriptor
    desc.numFVarChannels = 1;
    desc.fvarChannels = channels;

    // Instantiate a Far::TopologyRefiner from the descriptor
    OpenSubdiv::Far::TopologyRefiner* refiner = OpenSubdiv::Far::TopologyRefinerFactory<Descriptor>::Create(
      desc, OpenSubdiv::Far::TopologyRefinerFactory<Descriptor>::Options(type, options));

    // Uniformly refine the topology up to 'maxlevel'
    // note: fullTopologyInLastLevel must be true to work with face-varying data
    {
      OpenSubdiv::Far::TopologyRefiner::UniformOptions refineOptions(maxlevel);
      refineOptions.fullTopologyInLastLevel = true;
      refiner->RefineUniform(refineOptions);
    }

    // Allocate and initialize the 'vertex' primvar data (see tutorial 2 for
    // more details).
    std::vector<Vertex> vbuffer(refiner->GetNumVerticesTotal());
    Vertex* verts = &vbuffer[0];

    for (int i = 0; i < desc.numVertices; ++i)
    {
      verts[i].SetPosition(shape->verts[i*3], shape->verts[i*3+1], shape->verts[i*3+2]);
    }

    // Allocate and initialize the first channel of 'face-varying' primvar data (UVs)
    std::vector<FVarVertexUV> fvBufferUV(refiner->GetNumFVarValuesTotal(channelUV));
    FVarVertexUV* fvVertsUV = &fvBufferUV[0];
    for (int i = 0; i < shape->faceuvs.size(); ++i)
    {

      fvVertsUV[i].u = shape->uvs[i*2];
      fvVertsUV[i].v = shape->uvs[i*2+1];
    }

    // Interpolate both vertex and face-varying primvar data
    OpenSubdiv::Far::PrimvarRefiner primvarRefiner(*refiner);

    Vertex* srcVert = verts;
    FVarVertexUV* srcFVarUV = fvVertsUV;

    for (int level = 1; level <= maxlevel; ++level)
    {
      Vertex* dstVert = srcVert + refiner->GetLevel(level - 1).GetNumVertices();
      FVarVertexUV* dstFVarUV =
        srcFVarUV + refiner->GetLevel(level - 1).GetNumFVarValues(channelUV);

      primvarRefiner.Interpolate(level, srcVert, dstVert);
      primvarRefiner.InterpolateFaceVarying(level, srcFVarUV, dstFVarUV, channelUV);

      srcVert = dstVert;
      srcFVarUV = dstFVarUV;
    }

    { // Output OBJ of the highest level refined -----------

      OpenSubdiv::Far::TopologyLevel const& refLastLevel = refiner->GetLevel(maxlevel);

      int nverts = refLastLevel.GetNumVertices();
      int nuvs = refLastLevel.GetNumFVarValues(channelUV);
      int nfaces = refLastLevel.GetNumFaces();

      // Print vertex positions
      int firstOfLastVerts = refiner->GetNumVerticesTotal() - nverts;

      for (int vert = 0; vert < nverts; ++vert)
      {
        float const* pos = verts[firstOfLastVerts + vert].GetPosition();
        printf("v %f %f %f\n", pos[0], pos[1], pos[2]);
      }

      // Print uvs
      int firstOfLastUvs = refiner->GetNumFVarValuesTotal(channelUV) - nuvs;

      for (int fvvert = 0; fvvert < nuvs; ++fvvert)
      {
        FVarVertexUV const& uv = fvVertsUV[firstOfLastUvs + fvvert];
        printf("vt %f %f\n", uv.u, uv.v);
      }

      // Print faces
      for (int face = 0; face < nfaces; ++face)
      {

        OpenSubdiv::Far::ConstIndexArray fverts = refLastLevel.GetFaceVertices(face);
        OpenSubdiv::Far::ConstIndexArray fuvs = refLastLevel.GetFaceFVarValues(face, channelUV);

        // all refined Catmark faces should be quads
        assert(fverts.size() == 4 && fuvs.size() == 4);

        printf("f ");
        for (int vert = 0; vert < fverts.size(); ++vert)
        {
          // OBJ uses 1-based arrays...
          printf("%d/%d ", fverts[vert] + 1, fuvs[vert] + 1);
        }
        printf("\n");
      }
    }

    delete refiner;
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}
//------------------------------------------------------------------------------
