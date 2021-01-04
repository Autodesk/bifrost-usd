//-
// Copyright 2022 Autodesk, Inc.
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

/// \file Enum.h
/// \brief Bifrost USD enum types.

#ifndef USD_ENUM_H_
#define USD_ENUM_H_

namespace BifrostUsd {

/// \page Enum Bifrost USD enum types.
///

/// \section SdfVariability
///
/// An enum that xxx identifies variability types for attributes.
/// Variability indicates whether the attribute may vary over time and
/// value coordinates, and if its value comes through authoring or
/// or from its owner.
///
/// <ul>
///     <li><b>Varying.</b> Varying attributes may be directly
///            authored, animated and affected on by Actions.  They are the
///            most flexible.
///     <li><b>Uniform.</b> Uniform attributes may be authored
///            only with non-animated values (default values).  They cannot
///            be affected by Actions, but they can be connected to other
///            Uniform attributes.
/// </ul>
enum /*@cond true*/ class AMINO_ANNOTATE(
    "Amino::Enum") /*@endcond*/ SdfVariability : int {
    Varying,
    Uniform
};

/// \section SdfValueTypeName
///
/// An enum for the SdfValueTypeNames.
enum /*@cond true*/ class AMINO_ANNOTATE(
    "Amino::Enum") /*@endcond*/ SdfValueTypeName : int {
    Asset,
    AssetArray,
    Bool,
    BoolArray,
    Color3f,
    Color3fArray,
    Double,
    DoubleArray,
    Double2,
    Double2Array,
    Double3,
    Double3Array,
    Double4,
    Double4Array,
    Float,
    FloatArray,
    Float2,
    Float2Array,
    Float3,
    Float3Array,
    Float4,
    Float4Array,
    Int,
    IntArray,
    Int64,
    Int64Array,
    Normal3f,
    Normal3fArray,
    Quatd,
    QuatdArray,
    Quatf,
    QuatfArray,
    Quath,
    QuathArray,
    String,
    StringArray,
    TexCoord2f,
    TexCoord2fArray,
    Token,
    TokenArray,
    UChar,
    UCharArray,
    UInt,
    UIntArray,
    UInt64,
    UInt64Array
};

/// \section UsdGeomPrimvarInterpolation
///
/// An enum for the UsdGeomPrimvarInterpolations.
enum /*@cond true*/ class AMINO_ANNOTATE("Amino::Enum") /*@endcond*/
    UsdGeomPrimvarInterpolation : int {
        PrimVarConstant,
        PrimVarUniform,
        PrimVarVarying,
        PrimVarVertex,
        PrimVarFaceVarying
    };

/// \section VolumeFieldFormat
///
/// An enum that identifies the volume field format.
///
/// <b>VolumeFieldFormat:</b>
/// <ul>
///     <li><b>OpenVDB.</b> For OpenVDB field primitive.
///     <li><b>Field3D.</b> For Field3D field primitive.
/// </ul>
enum /*@cond true*/ class AMINO_ANNOTATE(
    "Amino::Enum") /*@endcond*/ VolumeFieldFormat : int {
    OpenVDB,
    Field3D
};

/// \section ImageablePurpose
///
/// An enum that contain the purposes for a USDImageable.
enum /*@cond true*/ class AMINO_ANNOTATE(
    "Amino::Enum") /*@endcond*/ ImageablePurpose : int {
    Default,
    Render,
    Proxy,
    Guide
};

/// \section MaterialBindingStrength
///
/// An enum to specify if a material binding on a prim is weaker or stronger
/// that bindings that apper lower in namespace.
enum /*@cond true*/ class AMINO_ANNOTATE(
    "Amino::Enum") /*@endcond*/ MaterialBindingStrength : int {
    FallbackStrength,
    StrongerThanDescendants,
    WeakerThanDescendants
};

/// \section MaterialPurpose
///
/// An enum to specify the material purpose in the material binding attribute
/// set on the object.
enum /*@cond true*/ class AMINO_ANNOTATE(
    "Amino::Enum") /*@endcond*/ MaterialPurpose : int {
    All,
    Preview,
    Full
};

/// \section ExpansionRule
///
/// An enum that contain the expansion rule for a USD Collection.
enum /*@cond true*/ class AMINO_ANNOTATE(
    "Amino::Enum") /*@endcond*/ ExpansionRule : int {
    Default,
    ExplicitOnly,
    ExpandPrims,
    ExpandPrimsAndProperties
};

} // namespace BifrostUsd

#endif // USD_ENUM_H_
