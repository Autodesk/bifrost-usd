//-
// Copyright 2023 Autodesk, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//+

#include <BifrostHydra/Translators/GeometryFn.h>

#include <Bifrost/Geometry/GeoProperty.h>
#include <Bifrost/Geometry/Primitives.h>
#include <Bifrost/Object/bifrost_what_is.h>

#include <pxr/base/gf/quath.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/imaging/hd/basisCurvesSchema.h>
#include <pxr/imaging/hd/dataSource.h>
#include <pxr/imaging/hd/instanceCategoriesSchema.h>
#include <pxr/imaging/hd/instancedBySchema.h>
#include <pxr/imaging/hd/instancer.h>
#include <pxr/imaging/hd/instancerTopologySchema.h>
#include <pxr/imaging/hd/meshSchema.h>
#include <pxr/imaging/hd/primvarsSchema.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hd/xformSchema.h>

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

using BifrostGeoIndices  = Amino::Array<Bifrost::Geometry::Index>;
using BifrostFloat3Array = Amino::Array<Bifrost::Math::float3>;

namespace {

VtVec3fArray GetVec3fArray(const Bifrost::Object& object,
                           const Amino::String&   name) {
    VtVec3fArray result;
    const auto   data =
        Bifrost::Geometry::getDataGeoPropValues<Bifrost::Math::float3>(object, name);
    if (data) {
        result.reserve(data->size());

        for (size_t i = 0; i < data->size(); ++i) {
            result.emplace_back((*data)[i].x, (*data)[i].y, (*data)[i].z);
        }
    }
    return result;
}

VtIntArray GetIntArray(const Bifrost::Object& object,
                       const Amino::String&   name) {
    VtIntArray result;
    auto data = Bifrost::Geometry::getDataGeoPropValues<Bifrost::Geometry::Index>(
        object, name);

    if (data) {
        result = VtIntArray{data->begin(), data->end()};
    }
    return result;
}

VtIntArray BifrostOffsetsToUsdVertexCount(
    Amino::Ptr<BifrostGeoIndices>& offsets) {
    using SizeType                              = decltype(offsets->size());
    const SizeType    face_vertex_indices_count = offsets->size() - 1;
    BifrostGeoIndices face_vertex_count;
    face_vertex_count.resize(face_vertex_indices_count);
    VtIntArray vt_face_vertex_count(face_vertex_indices_count);
    // Cumulative substract on array
    for (SizeType i = 0; i < face_vertex_indices_count; ++i) {
        vt_face_vertex_count[i] = (*offsets)[i + 1] - (*offsets)[i];
    }

    return vt_face_vertex_count;
}

class Matrix4dFromObjectDataSource : public HdMatrixDataSource {
public:
    HD_DECLARE_DATASOURCE(Matrix4dFromObjectDataSource)

    bool GetContributingSampleTimesForInterval(
        Time /*startTime*/,
        Time /*endTime*/,
        std::vector<Time>* /*outSampleTimes*/) override {
        (void)m_object;
        return true;
    }

    VtValue GetValue(Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    GfMatrix4d GetTypedValue(Time /*shutterOffset*/) override {
        (void)m_object;
        // GfMatrix4d m = GfMatrix4d(1).SetTranslateOnly(p[_index]);
        // return GfMatrix4d(1).SetScale(_scale) * m;
        return GfMatrix4d(1);
    }

private:
    explicit Matrix4dFromObjectDataSource(const Bifrost::Object& object)
        : m_object(object) {}

    const Bifrost::Object& m_object;
};

HdContainerDataSourceHandle BuildXfromDataSource(
    const Bifrost::Object& object) {
    return HdXformSchema::Builder()
        .SetMatrix(Matrix4dFromObjectDataSource::New(object))
        .SetResetXformStack(HdRetainedTypedSampledDataSource<bool>::New(true))
        .Build();
}

HdContainerDataSourceHandle BuildMeshPrimvarsDataSource(
    const Bifrost::Object& object) {
    auto displayColor = BifrostHd::GetDisplayColor(object);

    if (displayColor.empty()) {
        return HdRetainedContainerDataSource::New(
            HdPrimvarsSchemaTokens->points,
            HdPrimvarSchema::Builder()
                .SetPrimvarValue(
                    HdRetainedTypedSampledDataSource<VtVec3fArray>::New(
                        BifrostHd::GetPoints(object)))
                .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->vertex))
                .SetRole(HdPrimvarSchema::BuildRoleDataSource(
                    HdPrimvarSchemaTokens->point))
                .Build());
    } else {
        return HdRetainedContainerDataSource::New(
            HdPrimvarsSchemaTokens->points,
            HdPrimvarSchema::Builder()
                .SetPrimvarValue(
                    HdRetainedTypedSampledDataSource<VtVec3fArray>::New(
                        BifrostHd::GetPoints(object)))
                .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->vertex))
                .SetRole(HdPrimvarSchema::BuildRoleDataSource(
                    HdPrimvarSchemaTokens->point))
                .Build(),

            HdTokens->displayColor,
            HdPrimvarSchema::Builder()
                .SetIndexedPrimvarValue(
                    HdRetainedTypedSampledDataSource<VtVec3fArray>::New(
                        displayColor))
                .SetIndices(HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    BifrostHd::GetFaceVertexIndices(object)))
                .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->faceVarying))
                .SetRole(HdPrimvarSchema::BuildRoleDataSource(
                    HdPrimvarSchemaTokens->color))
                .Build());
    }
}

HdContainerDataSourceHandle BuildMeshTopologyDataSource(
    const Bifrost::Object& object) {
    auto face_offsets =
        Bifrost::Geometry::getDataGeoPropValues<Bifrost::Geometry::Index>(
            object, Bifrost::Geometry::sFaceOffsets);

    if (!face_offsets) {
        return nullptr;
    }

    using IntArrayDataSource = HdRetainedTypedSampledDataSource<VtIntArray>;

    IntArrayDataSource::Handle fvcDs =
        IntArrayDataSource::New(BifrostOffsetsToUsdVertexCount(face_offsets));

    IntArrayDataSource::Handle fviDs = IntArrayDataSource::New(
        GetIntArray(object, Bifrost::Geometry::sFaceVertices));

    return HdMeshSchema::Builder()
        .SetTopology(HdMeshTopologySchema::Builder()
                         .SetFaceVertexCounts(fvcDs)
                         .SetFaceVertexIndices(fviDs)
                         .Build())
        .Build();
}

HdContainerDataSourceHandle BuildBasisCurveTopologyDataSource(
    const Bifrost::Object& object) {
    auto strands_offsets =
        Bifrost::Geometry::getDataGeoPropValues<Bifrost::Geometry::Index>(
            object, Bifrost::Geometry::sStrandOffsets);

    if (!strands_offsets) {
        return nullptr;
    }

    using IntArrayDataSource = HdRetainedTypedSampledDataSource<VtIntArray>;

    IntArrayDataSource::Handle cvcDs = IntArrayDataSource::New(
        BifrostOffsetsToUsdVertexCount(strands_offsets));

    IntArrayDataSource::Handle ciDs = IntArrayDataSource::New(
        GetIntArray(object, Bifrost::Geometry::sFaceVertices));

    return HdBasisCurvesSchema::Builder()
        .SetTopology(HdBasisCurvesTopologySchema::Builder()
                         .SetCurveVertexCounts(cvcDs)
                         .SetCurveIndices(ciDs)
                         .Build())
        .Build();
}

HdContainerDataSourceHandle BuildBasisCurvePrimvarsDataSource(
    const Bifrost::Object& object) {
    return HdRetainedContainerDataSource::New(
        HdTokens->displayColor,
        HdPrimvarSchema::Builder()
            .SetPrimvarValue(
                HdRetainedTypedSampledDataSource<VtVec3fArray>::New(
                    BifrostHd::GetDisplayColor(object)))
            .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                HdPrimvarSchemaTokens->varying))
            .SetRole(HdPrimvarSchema::BuildRoleDataSource(
                HdPrimvarSchemaTokens->color))
            .Build(),

        HdPrimvarsSchemaTokens->points,
        HdPrimvarSchema::Builder()
            .SetPrimvarValue(
                HdRetainedTypedSampledDataSource<VtVec3fArray>::New(
                    BifrostHd::GetPoints(object)))
            .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                HdPrimvarSchemaTokens->vertex))
            .SetRole(HdPrimvarSchema::BuildRoleDataSource(
                HdPrimvarSchemaTokens->point))
            .Build(),

        HdTokens->widths,
        HdPrimvarSchema::Builder()
            .SetPrimvarValue(
                HdRetainedTypedSampledDataSource<VtFloatArray>::New(
                    BifrostHd::GetWidth(object)))
            .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                HdPrimvarSchemaTokens->vertex))
            .SetRole(HdPrimvarSchema::BuildRoleDataSource(TfToken{}))
            .Build()

    );
}

HdContainerDataSourceHandle NewMeshDataSource(const Bifrost::Object& object) {
    return HdRetainedContainerDataSource::New(
        HdMeshSchemaTokens->mesh, BuildMeshTopologyDataSource(object),
        HdPrimvarsSchemaTokens->primvars, BuildMeshPrimvarsDataSource(object),
        HdXformSchemaTokens->xform, BuildXfromDataSource(object));
}

HdContainerDataSourceHandle NewBasisCurveDataSource(
    const Bifrost::Object& object) {
    return HdRetainedContainerDataSource::New(
        HdBasisCurvesSchemaTokens->basisCurves,
        BuildBasisCurveTopologyDataSource(object),
        HdPrimvarsSchemaTokens->primvars,
        BuildBasisCurvePrimvarsDataSource(object), HdXformSchemaTokens->xform,
        BuildXfromDataSource(object));
}

HdContainerDataSourceHandle BuildInstancerTranslateDataSource(
    const Bifrost::Object& object) {
    return HdPrimvarSchema::Builder()
        .SetPrimvarValue(HdRetainedTypedSampledDataSource<VtVec3fArray>::New(
            BifrostHd::GetPoints(object)))
        .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
            HdPrimvarSchemaTokens->instance))
        .SetRole(
            HdPrimvarSchema::BuildRoleDataSource(HdPrimvarSchemaTokens->vector))
        .Build();
}

HdContainerDataSourceHandle BuildInstancerRotateDataSource(
    const Bifrost::Object& object) {
    auto         rotationsSize = BifrostHd::GetPoints(object).size();
    VtQuathArray rotations{rotationsSize, GfQuath{1, 0, 0, 0}};

    return HdPrimvarSchema::Builder()
        .SetPrimvarValue(
            HdRetainedTypedSampledDataSource<VtQuathArray>::New(rotations))
        .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
            HdPrimvarSchemaTokens->instance))
        .SetRole(HdPrimvarSchema::BuildRoleDataSource(TfToken{""}))
        .Build();
}

HdContainerDataSourceHandle BuildInstancerScaleDataSource(
    const Bifrost::Object& object) {
    auto         scaleSize = BifrostHd::GetPoints(object).size();
    VtVec3fArray scales{scaleSize, GfVec3f{1, 1, 1}};

    return HdPrimvarSchema::Builder()
        .SetPrimvarValue(
            HdRetainedTypedSampledDataSource<VtVec3fArray>::New(scales))
        .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
            HdPrimvarSchemaTokens->instance))
        .SetRole(HdPrimvarSchema::BuildRoleDataSource(TfToken{""}))
        .Build();
}

HdContainerDataSourceHandle BuildInstancerInstancedByDataSource() {
    return HdInstancedBySchema::Builder().Build();
}

class InstanceCategoriesVectorDataSource : public HdVectorDataSource {
public:
    HD_DECLARE_DATASOURCE(InstanceCategoriesVectorDataSource)

    InstanceCategoriesVectorDataSource() {}

    size_t GetNumElements() override { return 0; }

    HdDataSourceBaseHandle GetElement(size_t /*element*/) override {
        return nullptr;
    }
};

HdContainerDataSourceHandle GetInstancerInstanceCategoriesDataSource() {
    return HdInstanceCategoriesSchema::Builder()
        .SetCategoriesValues(InstanceCategoriesVectorDataSource::New())
        .Build();
}

HdContainerDataSourceHandle NewPointInstancerPrimvarsDataSource(
    const Bifrost::Object& object) {
    return HdRetainedContainerDataSource::New(
        HdInstancerTokens->rotate, BuildInstancerRotateDataSource(object),
        HdInstancerTokens->scale, BuildInstancerScaleDataSource(object),
        HdInstancerTokens->translate,
        BuildInstancerTranslateDataSource(object));
}

HdContainerDataSourceHandle BuildPointInstancerTopologyDataSource(
    const Bifrost::Object& object) {
    HdContainerDataSourceHandle data_source;

    const auto pointInstanceIDs = BifrostHd::GetPointInstanceIDs(object);

    // We need to flip the pointInstanceIDs: [0,1,0] -> 0: [0,2], 1: [1].
    std::vector<VtIntArray> instanceIndices;
    for (size_t i = 0; i < pointInstanceIDs.size(); ++i) {
        const int protoIndex = static_cast<int>(pointInstanceIDs[i]);
        if (size_t(protoIndex) >= instanceIndices.size()) {
            instanceIndices.resize(protoIndex + 1);
        }
        instanceIndices[protoIndex].push_back(static_cast<int>(i));
    }

    std::vector<HdDataSourceBaseHandle> indicesVec;
    for (size_t i = 0; i < instanceIndices.size(); ++i) {
        indicesVec.push_back(
            HdRetainedTypedSampledDataSource<VtArray<int>>::New(
                instanceIndices[i]));
    }

    VtArray<SdfPath> prototypes{
        SdfPath{"/instancer/prototypes/mesh/proto0_mesh_id0"}};

    using PathArrayDataSource =
        HdRetainedTypedSampledDataSource<VtArray<SdfPath>>;

    data_source = HdInstancerTopologySchema::Builder()
                      .SetPrototypes(PathArrayDataSource::New(prototypes))
                      .SetInstanceIndices(HdRetainedSmallVectorDataSource::New(
                          indicesVec.size(), indicesVec.data()))
                      // TODO(laforgg): .SetInstanceLocations()
                      .Build();

    return data_source;
}

HdContainerDataSourceHandle NewPointInstancerDataSource(
    const Bifrost::Object& object) {
    (void)BuildInstancerInstancedByDataSource();
    return HdRetainedContainerDataSource::New(
        HdInstancerTopologySchemaTokens->instancerTopology,
        BuildPointInstancerTopologyDataSource(object),
        HdXformSchemaTokens->xform, BuildXfromDataSource(object),
        HdPrimvarsSchemaTokens->primvars,
        NewPointInstancerPrimvarsDataSource(object),
        // HdInstancedBySchemaTokens->instancedBy,
        // BuildInstancerInstancedByDataSource(),
        HdInstanceCategoriesSchemaTokens->instanceCategories,
        GetInstancerInstanceCategoriesDataSource());
}

template <typename T>
VtArray<T> GetArrayFromPrimvarName(HdPrimvarsSchema& primvarsSchema,
                                   const TfToken&    name) {
    VtArray<T> result;
    auto       primvar = primvarsSchema.GetPrimvar(name);
    if (primvar.IsDefined()) {
        if (auto dataSource = primvar.GetPrimvarValue()) {
            auto value = dataSource->GetValue(0.0f);
            if (value.IsHolding<VtArray<T>>()) {
                result = value.UncheckedGet<VtArray<T>>();
            }
        }
    }
    return result;
}

template <typename T>
bool AddScalarArrayToMesh(const Amino::String&                name,
                          HdPrimvarsSchema&                   primvarsSchema,
                          Bifrost::Object& mesh) {
    auto data =
        GetArrayFromPrimvarName<T>(primvarsSchema, TfToken{name.c_str()});

    if (data.size() > 0) {
        auto obj = Bifrost::createObject();
        Bifrost::Geometry::populateDataGeoProp<T>(T{}, *obj);
        obj->setProperty(Bifrost::Geometry::sTarget,
                         Bifrost::Geometry::sPointComp);
        mesh.setProperty(name, std::move(obj));

        auto array = Amino::newMutablePtr<Amino::Array<T>>(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            (*array)[i] = data[i];
        }
        Bifrost::Geometry::setDataGeoPropValues(name, array.toImmutable(), mesh);
        return true;
    }
    return false;
}

template <typename T1, typename T2>
bool AddVec3ArrayToMesh(const Amino::String&                name,
                        HdPrimvarsSchema&                   primvarsSchema,
                        Amino::MutablePtr<Bifrost::Object>& mesh) {
    auto data =
        GetArrayFromPrimvarName<T1>(primvarsSchema, TfToken{name.c_str()});

    if (data.size() > 0) {
        auto obj = Bifrost::createObject();
        Bifrost::Geometry::populateDataGeoProp<T2>(T2{}, *obj);
        obj->setProperty(Bifrost::Geometry::sTarget,
                         Bifrost::Geometry::sPointComp);
        mesh->setProperty(name, std::move(obj));

        auto array = Amino::newMutablePtr<Amino::Array<T2>>(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            (*array)[i].x = data[i][0];
            (*array)[i].y = data[i][1];
            (*array)[i].z = data[i][2];
        }
        Bifrost::Geometry::setDataGeoPropValues(name, array.toImmutable(), *mesh);
        return true;
    }
    return false;
}
} // namespace

namespace BifrostHd {

BifrostHdGeoTypes GetGeoType(const Bifrost::Object& object) {

    auto objType = Amino::whatIs(object, Bifrost::Geometry::getGeometryTypes());

    if(object.empty()) {
        return BifrostHdGeoTypes::Empty;
    } else if (objType == Bifrost::Geometry::getMeshPrototype()) {
        return BifrostHdGeoTypes::Mesh;
    } else if (objType == Bifrost::Geometry::getStrandPrototype()) {
        return BifrostHdGeoTypes::Strands;
    } else if (objType == Bifrost::Geometry::getPointCloudPrototype()) {
        return BifrostHdGeoTypes::PointCloud;
    } else if (objType == Bifrost::Geometry::getInstancesPrototype()) {
        return BifrostHdGeoTypes::PointCloud;
    }

    return BifrostHdGeoTypes::Empty;
}

size_t GetPointCount(const Bifrost::Object& object) {
    return Bifrost::Geometry::getNumberOfElements(object,
                                           Bifrost::Geometry::sPointComp);
}

VtVec3fArray GetPoints(const Bifrost::Object& object) {
    return GetVec3fArray(object, Bifrost::Geometry::sPositions);
}

VtVec3fArray GetDisplayColor(const Bifrost::Object& object) {
    static const Amino::String kPointColorStr = "point_color";
    VtVec3fArray               result = GetVec3fArray(object, kPointColorStr);
    return result;
}

VtIntArray GetFaceVertexIndices(const Bifrost::Object& object) {
    return GetIntArray(object, Bifrost::Geometry::sFaceVertices);
}

VtFloatArray GetWidth(const Bifrost::Object& object) {
    static const Amino::String kPointSizeStr = "point_size";
    VtFloatArray               result;
    const auto                 data =
        Bifrost::Geometry::getDataGeoPropValues<Amino::float_t>(object, kPointSizeStr);
    if (data) {
        result = {data->begin(), data->end()};
    }
    return result;
}

VtInt64Array GetPointIDs(const Bifrost::Object& object) {
    static const Amino::String kPointIDStr = "point_id";

    VtInt64Array result;
    const auto   data =
        Bifrost::Geometry::getDataGeoPropValues<Amino::long_t>(object, kPointIDStr);
    if (data) {
        result = {data->begin(), data->end()};
    }
    return result;
}

// instancer

VtInt64Array GetPointInstanceIDs(const Bifrost::Object& object) {
    static const Amino::String kPointInstanceIDStr = "point_instance_id";

    VtInt64Array result;
    const auto   data = Bifrost::Geometry::getDataGeoPropValues<Amino::long_t>(
        object, kPointInstanceIDStr);
    if (data) {
        result = {data->begin(), data->end()};
    }
    return result;
}

Amino::Ptr<Bifrost::Object> GetInstanceShape(const Bifrost::Object& object) {
    static const Amino::String kInstanceShapeStr = "instance_shape";

    Amino::Any shape_any = object.getProperty(kInstanceShapeStr);
    Amino::Ptr<Bifrost::Object> shapePtr;
    shapePtr = Amino::any_cast<Amino::Ptr<Bifrost::Object>>(shape_any);
    return shapePtr;
}

Amino::Ptr<Bifrost::Object> GetShapeFromId(const Bifrost::Object& obj,
                                           const Amino::long_t    id) {
    static const Amino::String kInstanceShapesStr = "instance_shapes";

    Amino::Any shape_any = obj.getProperty(kInstanceShapesStr);
    auto       shapes =
        Amino::any_cast<Amino::Ptr<Amino::Array<Amino::Ptr<Bifrost::Object>>>>(
            shape_any);
    if (shapes) {
        if (id < static_cast<Amino::long_t>(shapes->size())) {
            return (*shapes)[id];
        }
    }

    return nullptr;
}

Amino::Ptr<Bifrost::Object> GetRenderGeometry(const Bifrost::Object& object) {
    static const Amino::String kRenderGeometryStr = "render_geometry";

    Amino::Any render_geo_any = object.getProperty(kRenderGeometryStr);
    auto       render_geo =
        Amino::any_cast<Amino::Ptr<Bifrost::Object>>(render_geo_any);
    return render_geo;
}

// Bifrost to Hydra

HdSceneIndexPrim CreateHdSceneIndexMesh(const Bifrost::Object& object) {
    HdSceneIndexPrim result;
    result.primType   = HdPrimTypeTokens->mesh;
    result.dataSource = NewMeshDataSource(object);

    return result;
}

HdSceneIndexPrim CreateHdSceneIndexBasisCurves(const Bifrost::Object& object) {
    HdSceneIndexPrim result;
    result.primType   = HdPrimTypeTokens->basisCurves;
    result.dataSource = NewBasisCurveDataSource(object);

    return result;
}

HdSceneIndexPrim CreateHdSceneIndexExplicitInstancer(
    const Bifrost::Object& object) {
    HdSceneIndexPrim result;
    result.primType   = HdPrimTypeTokens->instancer;
    result.dataSource = NewPointInstancerDataSource(object);

    return result;
}

// Hydra to Bifrost

Amino::Ptr<Bifrost::Object> CreateBifrostMesh(const HdSceneIndexPrim& prim) {
    assert(prim.primType == HdPrimTypeTokens->mesh);

    auto mesh = Bifrost::createObject();

    auto primvarsSchema = HdPrimvarsSchema::GetFromParent(prim.dataSource);

    // Create "point_position" Bifrost array from Hydra "points" primvar
    Amino::MutablePtr<BifrostFloat3Array> positions;
    if (auto ptsDs = primvarsSchema.GetPrimvar(HdPrimvarsSchemaTokens->points)
                         .GetPrimvarValue()) {
        auto ptsValue = ptsDs->GetValue(0.0f);
        if (!ptsValue.IsHolding<VtArray<GfVec3f>>()) {
            std::cerr << "Couldn't get Usd mesh points" << std::endl;
            return mesh.toImmutable();
        }

        const auto& pts = ptsValue.UncheckedGet<VtArray<GfVec3f>>();
        positions       = Amino::newMutablePtr<BifrostFloat3Array>(pts.size());
        for (size_t i = 0; i < pts.size(); ++i) {
            (*positions)[i] =
                Bifrost::Math::float3{pts[i][0], pts[i][1], pts[i][2]};
        }
    }

    // Create Bifrost "face data" from Hydra mesh topology
    auto hdMesh = HdMeshSchema::GetFromParent(prim.dataSource);
    if (!hdMesh.IsDefined()) {
        std::cerr << "Couldn't get Usd mesh" << std::endl;
        return mesh.toImmutable();
    }

    auto hdMeshTopo =
        HdMeshTopologySchema::GetFromParent(hdMesh.GetContainer());
    if (!hdMeshTopo.IsDefined()) {
        std::cerr << "Couldn't get Usd mesh topology" << std::endl;
        return mesh.toImmutable();
    }

    Amino::MutablePtr<BifrostGeoIndices> faceVertices;
    auto fvi = hdMeshTopo.GetFaceVertexIndices();
    if (!fvi) {
        std::cerr << "Couldn't get Usd mesh faceVertexIndices" << std::endl;
        return mesh.toImmutable();
    }

    auto fviValue = fvi->GetTypedValue(0.0f);
    faceVertices  = Amino::newMutablePtr<BifrostGeoIndices>(fviValue.size());

    for (size_t i = 0; i < fviValue.size(); ++i) {
        (*faceVertices)[i] = fviValue[i];
    }

    Amino::MutablePtr<BifrostGeoIndices> faceOffsets;
    auto                                 fvc = hdMeshTopo.GetFaceVertexCounts();
    if (!fvc) {
        std::cerr << "couldn't get faceVertexCounts" << std::endl;
        return mesh.toImmutable();
    }

    auto   fvcValue       = fvc->GetTypedValue(0.0f);
    size_t faceOffsetsCnt = fvcValue.size() + 1;

    faceOffsets = Amino::newMutablePtr<BifrostGeoIndices>(faceOffsetsCnt);

    for (size_t i = 0; i < faceOffsetsCnt; ++i) {
        if (i == 0) {
            (*faceOffsets)[0] = 0;
        } else {
            (*faceOffsets)[i] = fvcValue[i - 1] + (*faceOffsets)[i - 1];
        }
    }

    Bifrost::Geometry::populateMesh(positions.toImmutable(),
                             faceVertices.toImmutable(),
                             faceOffsets.toImmutable(), *mesh);

    for (const auto& name : primvarsSchema.GetPrimvarNames()) {
        bool status =
            AddScalarArrayToMesh<float>(name.GetText(), primvarsSchema, *mesh);

        if (!status) {
            AddVec3ArrayToMesh<GfVec3f, Bifrost::Math::float3>(
                name.GetText(), primvarsSchema, mesh);
        }
    }

    return mesh.toImmutable();
}

} // namespace BifrostHd
