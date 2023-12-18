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
// This tutorial shows how to interpolate surface limits at arbitrary
// parametric locations using feature adaptive Far::PatchTables.
//
// The evaluation of the limit surface at arbitrary locations requires the
// adaptive isolation of topological features. This process converts the
// input polygonal control cage into a collection of bi-cubic patches.
//
// We can then evaluate the patches at random parametric locations and
// obtain analytical positions and tangents on the limit surface.
//
// The results are dumped into a MEL script that draws 'streak' particle
// systems that show the tangent and bi-tangent at the random samples locations.
//

#include <opensubdiv/far/patchMap.h>
#include <opensubdiv/far/patchTableFactory.h>
#include <opensubdiv/far/primvarRefiner.h>
#include <opensubdiv/far/ptexIndices.h>
#include <opensubdiv/far/topologyDescriptor.h>

#include <vtkNew.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkStructuredPoints.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridWriter.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include <boost/format.hpp>

#include <Imath/ImathVec.h>

#include <utils/shape_utils.h>

using namespace OpenSubdiv;

typedef float Real; // Change from default double to match intended Shapes data

// Creates a Far::TopologyRefiner from the pyramid shape above
static Far::TopologyRefiner* createTopologyRefiner(const Shape* shape);

//------------------------------------------------------------------------------
// Vertex container implementation.
//
struct Vertex
{

  // Minimal required interface ----------------------
  Vertex() {}

  void Clear(void* = 0) { point[0] = point[1] = point[2] = 0.0f; }

  void AddWithWeight(Vertex const& src, Real weight)
  {
    point[0] += weight * src.point[0];
    point[1] += weight * src.point[1];
    point[2] += weight * src.point[2];
  }

  Real point[3];
};

//------------------------------------------------------------------------------
// Limit frame container implementation -- this interface is not strictly
// required but follows a similar pattern to Vertex.
//
struct LimitFrame
{

  void Clear(void* = 0)
  {
    point[0] = point[1] = point[2] = 0.0f;
    deriv1[0] = deriv1[1] = deriv1[2] = 0.0f;
    deriv2[0] = deriv2[1] = deriv2[2] = 0.0f;
  }

  void AddWithWeight(Vertex const& src, Real weight, Real d1Weight, Real d2Weight)
  {

    point[0] += weight * src.point[0];
    point[1] += weight * src.point[1];
    point[2] += weight * src.point[2];

    deriv1[0] += d1Weight * src.point[0];
    deriv1[1] += d1Weight * src.point[1];
    deriv1[2] += d1Weight * src.point[2];

    deriv2[0] += d2Weight * src.point[0];
    deriv2[1] += d2Weight * src.point[1];
    deriv2[2] += d2Weight * src.point[2];
  }

  Real point[3], deriv1[3], deriv2[3];
};

void VisualizationViaOBJ(const std::vector<LimitFrame>& samples, std::ostream& os)
{ // Visualization with Maya : print a MEL script that generates particles
  // at the location of the limit vertices

  int nsamples = (int)samples.size();

  os << "# file -f -new;\n";

  // Output particle positions for the tangent
  os << "# particle -n deriv1 ";
  os << boost::format("# Number of particles %d\n") % nsamples;
  for (int sample = 0; sample < nsamples; ++sample)
  {
    Real const* pos = samples[sample].point;
    os << boost::format("v %f %f %f\n") % pos[0] % pos[1] % pos[2];
  }

  for (int sample = 0; sample < nsamples; ++sample)
  {
    Real const* tan1 = samples[sample].deriv1;
    Real const* tan2 = samples[sample].deriv2;
    Imath::Vec3<Real> _tan1(tan1[0], tan1[1], tan1[2]);
    Imath::Vec3<Real> _tan2(tan2[0], tan2[1], tan2[2]);
    Imath::Vec3<Real> vn = _tan1.cross(_tan2);
    vn.normalize();
    os << boost::format("vn %f %f %f\n") % vn[0] % vn[1] % vn[2];
  }
}

void VisualizationViaVTU(const std::vector<LimitFrame>& samples, const std::string& vtuFilename)
{
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();

  vtkNew<vtkFloatArray> normals;
  normals->SetName("Normals");
  normals->SetNumberOfComponents(3);

  vtkNew<vtkFloatArray> deriv1;
  deriv1->SetName("Derivative1");
  deriv1->SetNumberOfComponents(3);

  vtkNew<vtkFloatArray> deriv2;
  deriv2->SetName("Derivative2");
  deriv2->SetNumberOfComponents(3);

  int nsamples = (int)samples.size();

  for (int sample = 0; sample < nsamples; ++sample)
  {
    Real const* pos = samples[sample].point;
    points->InsertNextPoint ( pos[0], pos[1], pos[2] );

    Real const* tan1 = samples[sample].deriv1;
    Imath::Vec3<Real> _tan1(tan1[0], tan1[1], tan1[2]);
    _tan1.normalize();
    deriv1->InsertNextTuple3(tan1[0], tan1[1], tan1[2]);

    Real const* tan2 = samples[sample].deriv2;
    Imath::Vec3<Real> _tan2(tan2[0], tan2[1], tan2[2]);
    _tan2.normalize();
    deriv2->InsertNextTuple3(tan2[0], tan2[1], tan2[2]);

    Imath::Vec3<Real> normal = _tan1.cross(_tan2).normalize();
    std::cout << normal << "\n";
    normals->InsertNextTuple3(normal.x,normal.y,normal.z);
  }

  vtkNew<vtkUnstructuredGrid> ug;

  ug->SetPoints(points);

  ug->GetPointData()->SetNormals(normals);
  ug->GetPointData()->AddArray(deriv1);
  ug->GetPointData()->AddArray(deriv2);

  vtkNew<vtkXMLUnstructuredGridWriter> writer;
  writer->SetFileName(vtuFilename.c_str());
  writer->SetInputData(ug);
  writer->Write();
}

//------------------------------------------------------------------------------
int main(int argc, char** argv)
{

  if (argc != 3)
  {
    std::cerr << "Usage: app <level> <obj>\n";
    return EXIT_FAILURE;
  }
  int maxPatchLevel = atoi(argv[1]);
  std::ifstream objstream(argv[2]);
  std::stringstream objbuffer;
  objbuffer << objstream.rdbuf();

  const Shape* shape = Shape::parseObj(objbuffer.str().c_str(), Scheme::kCatmark);

  // Generate a Far::TopologyRefiner (see tutorial_1_1 for details).
  Far::TopologyRefiner* refiner = createTopologyRefiner(shape);

  // Patches are constructed from adaptively refined faces, but the processes
  // of constructing the PatchTable and of applying adaptive refinement have
  // historically been separate.  Adaptive refinement is applied purely to
  // satisfy the needs of the desired PatchTable, so options associated with
  // adaptive refinement should be derived from those specified for the
  // PatchTable.  This is not a strict requirement, but it will avoid
  // problems arising from specifying/coordinating the two independently
  // (especially when dealing with face-varying patches).

  // Initialize options for the PatchTable:
  //
  // Choose patches adaptively refined to level 3 since the sharpest crease
  // in the shape is 3.0f (in g_creaseweights[]), and include the inf-sharp
  // crease option just to illustrate the need to syncronize options.
  //
  // int maxPatchLevel = 3;

  Far::PatchTableFactory::Options patchOptions(maxPatchLevel);
  patchOptions.SetPatchPrecision<Real>();
  patchOptions.useInfSharpPatch = true;
  patchOptions.generateVaryingTables = false;
  patchOptions.endCapType = Far::PatchTableFactory::Options::ENDCAP_GREGORY_BASIS;

  // Initialize corresonding options for adaptive refinement:
  Far::TopologyRefiner::AdaptiveOptions adaptiveOptions(maxPatchLevel);

  bool assignAdaptiveOptionsExplicitly = false;
  if (assignAdaptiveOptionsExplicitly)
  {
    adaptiveOptions.useInfSharpPatch = true;
  }
  else
  {
    // Be sure patch options were intialized with the desired max level
    adaptiveOptions = patchOptions.GetRefineAdaptiveOptions();
  }
  assert(adaptiveOptions.useInfSharpPatch == patchOptions.useInfSharpPatch);

  // Apply adaptive refinement and construct the associated PatchTable to
  // evaluate the limit surface:
  refiner->RefineAdaptive(adaptiveOptions);

  Far::PatchTable const* patchTable = Far::PatchTableFactory::Create(*refiner, patchOptions);

  // Compute the total number of points we need to evaluate the PatchTable.
  // Approximations at irregular or extraordinary features require the use
  // of additional points associated with the patches that are referred to
  // as "local points" (i.e. local to the PatchTable).
  int nRefinerVertices = refiner->GetNumVerticesTotal();
  int nLocalPoints = patchTable->GetNumLocalPoints();

  // Create a buffer to hold the position of the refined verts and
  // local points, then copy the coarse positions at the beginning.
  std::vector<Vertex> verts(nRefinerVertices + nLocalPoints);
  std::memcpy(&verts[0], shape->verts.data(), shape->GetNumVertices() * 3 * sizeof(Real));

  // Adaptive refinement may result in fewer levels than the max specified.
  int nRefinedLevels = refiner->GetNumLevels();

  // Interpolate vertex primvar data : they are the control vertices
  // of the limit patches (see tutorial_1_1 for details)
  Far::PrimvarRefinerReal<Real> primvarRefiner(*refiner);

  Vertex* src = &verts[0];
  for (int level = 1; level < nRefinedLevels; ++level)
  {
    Vertex* dst = src + refiner->GetLevel(level - 1).GetNumVertices();
    primvarRefiner.Interpolate(level, src, dst);
    src = dst;
  }

  // Evaluate local points from interpolated vertex primvars.
  if (nLocalPoints)
  {
    patchTable->GetLocalPointStencilTable<Real>()->UpdateValues(
      &verts[0], &verts[nRefinerVertices]);
  }

  // Create a Far::PatchMap to help locating patches in the table
  Far::PatchMap patchmap(*patchTable);

  // Create a Far::PtexIndices to help find indices of ptex faces.
  Far::PtexIndices ptexIndices(*refiner);

  // Generate random samples on each ptex face
  int nsamplesPerFace = 200, nfaces = ptexIndices.GetNumFaces();

  std::vector<LimitFrame> samples(nsamplesPerFace * nfaces);

  srand(static_cast<int>(2147483647));

  Real pWeights[20], dsWeights[20], dtWeights[20];

  for (int face = 0, count = 0; face < nfaces; ++face)
  {

    for (int sample = 0; sample < nsamplesPerFace; ++sample, ++count)
    {

      Real s = (Real)rand() / (Real)RAND_MAX, t = (Real)rand() / (Real)RAND_MAX;

      // Locate the patch corresponding to the face ptex idx and (s,t)
      Far::PatchTable::PatchHandle const* handle = patchmap.FindPatch(face, s, t);
      assert(handle);

      // Evaluate the patch weights, identify the CVs and compute the limit frame:
      patchTable->EvaluateBasis(*handle, s, t, pWeights, dsWeights, dtWeights);

      Far::ConstIndexArray cvs = patchTable->GetPatchVertices(*handle);

      LimitFrame& dst = samples[count];
      dst.Clear();
      for (int cv = 0; cv < cvs.size(); ++cv)
      {
        dst.AddWithWeight(verts[cvs[cv]], pWeights[cv], dsWeights[cv], dtWeights[cv]);
      }
    }
  }

  std::ofstream output_stream("particles.obj");
  VisualizationViaOBJ(samples,output_stream);
  output_stream.close();

  VisualizationViaVTU(samples, "particles.vtu");

  delete refiner;
  delete patchTable;
  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
static Far::TopologyRefiner* createTopologyRefiner(const Shape* shape)
{

  typedef Far::TopologyDescriptor Descriptor;

  Sdc::SchemeType type = OpenSubdiv::Sdc::SCHEME_CATMARK;

  Sdc::Options options;
  options.SetVtxBoundaryInterpolation(Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);

  Descriptor desc;
  desc.numVertices = shape->GetNumVertices();
  desc.numFaces = shape->GetNumFaces();
  desc.numVertsPerFace = shape->nvertsPerFace.data();
  desc.vertIndicesPerFace = shape->faceverts.data();
  /*
  desc.numCreases = g_ncreases;
  desc.creaseVertexIndexPairs = g_creaseverts;
  desc.creaseWeights = g_creaseweights;
*/
  // Instantiate a Far::TopologyRefiner from the descriptor.
  Far::TopologyRefiner* refiner = Far::TopologyRefinerFactory<Descriptor>::Create(
    desc, Far::TopologyRefinerFactory<Descriptor>::Options(type, options));

  return refiner;
}
