/*
Copyright 2017 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "lullaby/util/mesh_util.h"
#include "gtest/gtest.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "lullaby/util/vertex.h"
#include "lullaby/generated/tests/portable_test_macros.h"

namespace lull {
namespace {

constexpr float kEpsilon = 1.0E-5f;

using DeformFn = std::function<mathfu::vec3(const mathfu::vec3&)>;

template <typename T>
void ApplyDeformationToList(DeformFn deform, std::vector<T>* list) {
  CHECK_EQ(sizeof(T) % sizeof(float), 0);
  const size_t stride = sizeof(T) / sizeof(float);
  ApplyDeformation(reinterpret_cast<float*>(list->data()),
                   stride * list->size(), stride, deform);
}

TEST(TesselatedQuadDeathTest, SanityChecks) {
  constexpr const char* kNotEnoughVertsMessage = "Failed to reserve";

  // We need at least 2 verts in each dimension.
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadIndices(
          /* num_verts_x = */ 1, /* num_verts_y = */ 2, /* corner_verts = */ 0),
      kNotEnoughVertsMessage);
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadIndices(
          /* num_verts_x = */ 2, /* num_verts_y = */ 1, /* corner_verts = */ 0),
      kNotEnoughVertsMessage);
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadVertices<VertexPT>(
          /* size_x = */ 1, /* size_y = */ 1, /* num_verts_x = */ 1,
          /* num_verts_y = */ 2,
          /* corner_radius = */ 0, /* corner_verts = */ 0),
      kNotEnoughVertsMessage);
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadVertices<VertexPT>(
          /* size_x = */ 1, /* size_y = */ 1, /* num_verts_x = */ 2,
          /* num_verts_y = */ 1,
          /* corner_radius = */ 0, /* corner_verts = */ 0),
      kNotEnoughVertsMessage);

  // We need at least 4 verts in each dimension if we have rounded corners.
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadIndices(
          /* num_verts_x = */ 2, /* num_verts_y = */ 4, /* corner_verts = */ 2),
      kNotEnoughVertsMessage);
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadIndices(
          /* num_verts_x = */ 4, /* num_verts_y = */ 2, /* corner_verts = */ 2),
      kNotEnoughVertsMessage);
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadVertices<VertexPT>(
          /* size_x = */ 1, /* size_y = */ 1, /* num_verts_x = */ 2,
          /* num_verts_y = */ 4,
          /* corner_radius = */ 1, /* corner_verts = */ 2),
      kNotEnoughVertsMessage);
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadVertices<VertexPT>(
          /* size_x = */ 1, /* size_y = */ 1, /* num_verts_x = */ 4,
          /* num_verts_y = */ 2,
          /* corner_radius = */ 1, /* corner_verts = */ 2),
      kNotEnoughVertsMessage);

  // Check that we're not asking for negative corner vertices.
  constexpr const char* kNegativeCornerVertsMessage =
      "Must have >= 0 corner vertices.";
  PORT_EXPECT_DEBUG_DEATH(CalculateTesselatedQuadIndices(
                              /* num_verts_x = */ 2, /* num_verts_y = */ 2,
                              /* corner_verts = */ -2),
                          kNegativeCornerVertsMessage);
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadVertices<VertexPT>(
          /* size_x = */ 1, /* size_y = */ 1, /* num_verts_x = */ 2,
          /* num_verts_y = */ 2,
          /* corner_radius = */ 1, /* corner_verts = */ -1),
      kNegativeCornerVertsMessage);

  // Check that negatively-sized quads are not allowed.
  constexpr const char* kNegativeSizeMessage =
      "Size of quad has to be >= than 0.0";
  PORT_EXPECT_DEBUG_DEATH(
      CalculateTesselatedQuadVertices<VertexPT>(
          /* size_x = */ -1, /* size_y = */ -1, /* num_verts_x = */ 2,
          /* num_verts_y = */ 2,
          /* corner_radius = */ 0, /* corner_verts = */ 0),
      kNegativeSizeMessage);
}

TEST(TesselatedQuad, CheckVerticesNoCorners) {
  const float size_x = 2.f;
  const float size_y = 4.f;
  const int verts_x = 5;
  const int verts_y = 7;

  const auto vertices = CalculateTesselatedQuadVertices<VertexPT>(
      size_x, size_y, verts_x, verts_y, 0.f, 0);

  EXPECT_EQ(static_cast<int>(vertices.size()), verts_x * verts_y);

  const int bottom_left_ind = 0;
  const int top_left_ind = verts_y - 1;
  const int top_right_ind = (verts_x)*verts_y - 1;
  const int bottom_right_ind = (verts_x - 1) * verts_y;

  const mathfu::vec3 bottom_left = GetPosition(vertices[bottom_left_ind]);
  const mathfu::vec3 top_left = GetPosition(vertices[top_left_ind]);
  const mathfu::vec3 top_right = GetPosition(vertices[top_right_ind]);
  const mathfu::vec3 bottom_right = GetPosition(vertices[bottom_right_ind]);

  EXPECT_NEAR(bottom_left.x, -size_x / 2.f, kEpsilon);
  EXPECT_NEAR(top_left.x, -size_x / 2.f, kEpsilon);
  EXPECT_NEAR(top_right.x, size_x / 2.f, kEpsilon);
  EXPECT_NEAR(bottom_right.x, size_x / 2.f, kEpsilon);

  EXPECT_NEAR(bottom_left.y, -size_y / 2.f, kEpsilon);
  EXPECT_NEAR(top_left.y, size_y / 2.f, kEpsilon);
  EXPECT_NEAR(top_right.y, size_y / 2.f, kEpsilon);
  EXPECT_NEAR(bottom_right.y, -size_y / 2.f, kEpsilon);

  EXPECT_NEAR(bottom_left.z, 0, kEpsilon);
  EXPECT_NEAR(top_left.z, 0, kEpsilon);
  EXPECT_NEAR(top_right.z, 0, kEpsilon);
  EXPECT_NEAR(bottom_right.z, 0, kEpsilon);
}

TEST(TesselatedQuad, CheckIndicesNoCorners) {
  const int verts_x = 5;
  const int verts_y = 7;
  const auto indices = CalculateTesselatedQuadIndices(verts_x, verts_y, 0);
  EXPECT_EQ(static_cast<int>(indices.size()),
            (verts_x - 1) * (verts_y - 1) * 6);
}

TEST(TesselatedQuad, CheckVerticesWithCorners) {
  const float size_x = 8.f;
  const float size_y = 4.f;
  const float half_size_x = size_x / 2.f;
  const float half_size_y = size_y / 2.f;
  const int verts_x = 8;
  const int verts_y = 4;
  const float corner_radius = 1.f;
  const int corner_verts = 1;
  const auto vertices = CalculateTesselatedQuadVertices<VertexPT>(
      size_x, size_y, verts_x, verts_y, corner_radius, corner_verts);

  const int vertex_count = ((verts_x * verts_y) - 4 + (corner_verts * 4));
  EXPECT_EQ(static_cast<int>(vertices.size()), vertex_count);
  // From radiused corner vertices we expect that:
  //  a) The minimum and maximum x value will be -/+ size_x / 2
  //  b) The minimum and maximum y value will be -/+ size_y / 2
  //  c) The minimum and maximum u value will be [0, 1], and at the appropriate
  //      position in extremes in x.
  //  d) The minimum and maximum v value will be [0, 1], and at the appropriate
  //      position in extremes in y.
  //  e) all z values are nonzero
  //  f) no vertex having a min or max in one dimension will have a min or
  //     max in the other dimension (meaning that the actual corners are not
  //     within the geometry)
  float min_x_value = size_x;
  float max_x_value = -size_x;
  float min_y_value = size_y;
  float max_y_value = -size_y;
  float min_u_value = 1.0f;
  float max_u_value = 0.0f;
  float min_v_value = 1.0f;
  float max_v_value = 0.0f;
  for (int i = 0; i < vertex_count; ++i) {
    const int ind = i;
    // Validate x value assumptions.
    min_x_value = std::min(min_x_value, vertices[ind].x);
    max_x_value = std::max(max_x_value, vertices[ind].x);
    if (std::abs(vertices[ind].x - half_size_x) < kEpsilon) {
      // The y value should not also be near its extreme.
      EXPECT_GT(std::abs(vertices[ind].y - half_size_y), kEpsilon);
      // We are at an extreme of x, make sure that the u value corresponds.
      if (vertices[ind].x < 0) {
        EXPECT_NEAR(vertices[ind].u0, 0.f, kEpsilon);
      } else {
        EXPECT_NEAR(vertices[ind].u0, 1.f, kEpsilon);
      }
    }

    // Validate y value assumptions.
    min_y_value = std::min(min_y_value, vertices[ind].y);
    max_y_value = std::max(max_y_value, vertices[ind].y);
    if (std::abs(vertices[ind].y - half_size_y) < kEpsilon) {
      // The x value should not also be near its extreme.
      EXPECT_GT(std::abs(vertices[ind].x - half_size_x), kEpsilon);
      // We are at the extreme of y, make sure that the v value corresponds.
      if (vertices[ind].y < 0) {
        EXPECT_NEAR(vertices[ind].v0, 1.f, kEpsilon);
      } else {
        EXPECT_NEAR(vertices[ind].v0, 0.f, kEpsilon);
      }
    }

    // The z value should always be very near zero.
    EXPECT_NEAR(vertices[ind].z, 0, kEpsilon);

    // Validate u value assumptions.
    min_u_value = std::min(min_u_value, vertices[ind].u0);
    max_u_value = std::max(max_u_value, vertices[ind].u0);

    // Validate v value assumptions.
    min_v_value = std::min(min_v_value, vertices[ind].v0);
    max_v_value = std::max(max_v_value, vertices[ind].v0);
  }

  // Check computed extrema for correctness.
  EXPECT_NEAR(-half_size_x, min_x_value, kEpsilon);
  EXPECT_NEAR(half_size_x, max_x_value, kEpsilon);
  EXPECT_NEAR(-half_size_y, min_y_value, kEpsilon);
  EXPECT_NEAR(half_size_y, max_y_value, kEpsilon);
  EXPECT_NEAR(0.f, min_u_value, kEpsilon);
  EXPECT_NEAR(1.f, max_u_value, kEpsilon);
  EXPECT_NEAR(0.f, min_v_value, kEpsilon);
  EXPECT_NEAR(1.f, max_v_value, kEpsilon);
}

TEST(TesselatedQuad, CheckIndicesWithCorners) {
  const int verts_x = 17;
  const int verts_y = 7;
  const int corner_verts = 11;
  const auto indices =
      CalculateTesselatedQuadIndices(verts_x, verts_y, corner_verts);
  EXPECT_EQ(
      static_cast<int>(indices.size()),
      ((verts_x - 1) * (verts_y - 1) * 6) - 24 + (12 * (corner_verts + 1)));
}

TEST(TesselatedQuad, CornerMask) {
  const int verts_x = 17;
  const int verts_y = 7;
  const int corner_verts = 11;
  const auto indices =
      CalculateTesselatedQuadIndices(verts_x, verts_y, corner_verts);
  const auto verts = CalculateTesselatedQuadVertices<VertexPT>(
      /* size_x = */ 1, /* size_y = */ 1, verts_x, verts_y,
      /* corner_radius = */ 1, corner_verts,
      /* corner_mask = */ CornerMask::kNone);

  // Make sure that all the indices are valid.
  for (const auto& index : indices) {
    EXPECT_GE(index, 0U);
    EXPECT_LT(index, verts.size());
  }
}

static const int kIndicesPerQuad = 6;
static const int kIndicesPerTriangle = 3;
static const int kCornersPerQuad = 4;

TEST(TesselatedQuad, VertexIndexCountsSquareCorners) {
  const int verts_x = 5;
  const int verts_y = 7;
  const size_t vertex_count = GetTesselatedQuadVertexCount(verts_x, verts_y, 0);
  EXPECT_EQ(static_cast<int>(vertex_count), verts_x * verts_y);
  const size_t index_count = GetTesselatedQuadIndexCount(verts_x, verts_y, 0);
  EXPECT_EQ(static_cast<int>(index_count),
            (verts_x - 1) * (verts_y - 1) * kIndicesPerQuad);
}

TEST(TesselatedQuad, VertexIndexCountsRoundCorners) {
  const int verts_x = 5;
  const int verts_y = 7;
  const int corner_verts = 5;
  const size_t vertex_count =
      GetTesselatedQuadVertexCount(verts_x, verts_y, corner_verts);
  EXPECT_EQ(static_cast<int>(vertex_count),
            ((verts_x * verts_y) - kCornersPerQuad +
             (corner_verts * kCornersPerQuad)));
  const size_t index_count =
      GetTesselatedQuadIndexCount(verts_x, verts_y, corner_verts);
  EXPECT_EQ(static_cast<int>(index_count),
            ((verts_x - 1) * (verts_y - 1) * kIndicesPerQuad) -
                kIndicesPerQuad * kCornersPerQuad +
                (kIndicesPerTriangle * kCornersPerQuad * (corner_verts + 1)));
}

TEST(TessellatedQuad, CreateQuadMesh) {
  constexpr float kSizeX = 2.0f;
  constexpr float kSizeY = 1.5f;
  constexpr float kCornerRadius = 0.2f;
  constexpr int kNumVertsX = 5;
  constexpr int kNumVertsY = 7;
  constexpr int kNumCornerVerts = 5;
  const std::vector<VertexPT> vertices =
      CalculateTesselatedQuadVertices<VertexPT>(kSizeX, kSizeY, kNumVertsX,
                                                kNumVertsY, kCornerRadius,
                                                kNumCornerVerts);
  const std::vector<uint16_t> indices =
      CalculateTesselatedQuadIndices(kNumVertsX, kNumVertsY, kNumCornerVerts);

  MeshData mesh = CreateQuadMesh<VertexPT>(
      kSizeX, kSizeY, kNumVertsX, kNumVertsY, kCornerRadius, kNumCornerVerts);
  EXPECT_EQ(mesh.GetVertexFormat(), VertexPT::kFormat);
  EXPECT_EQ(mesh.GetNumVertices(), vertices.size());
  EXPECT_EQ(mesh.GetNumIndices(), indices.size());

  const VertexPT* vertex_data = mesh.GetMutableVertexData<VertexPT>();
  ASSERT_TRUE(vertex_data != nullptr);
  for (size_t i = 0; i < vertices.size(); ++i) {
    EXPECT_EQ(vertex_data[i].x, vertices[i].x);
    EXPECT_EQ(vertex_data[i].y, vertices[i].y);
    EXPECT_EQ(vertex_data[i].z, vertices[i].z);
    EXPECT_EQ(vertex_data[i].u0, vertices[i].u0);
    EXPECT_EQ(vertex_data[i].v0, vertices[i].v0);
  }

  const MeshData::Index* index_data = mesh.GetIndexData();
  ASSERT_TRUE(index_data != nullptr);
  for (size_t i = 0; i < indices.size(); ++i) {
    EXPECT_EQ(index_data[i], indices[i]);
  }
}

TEST(Deformation, Basic) {
  auto deform = [](const mathfu::vec3& pos) { return -2.0f * pos; };

  std::vector<mathfu::vec3> list;
  list.emplace_back(1.0f, 2.0f, 3.0f);
  list.emplace_back(4.0f, 5.0f, 6.0f);
  ApplyDeformationToList(deform, &list);
  EXPECT_EQ(list[0], mathfu::vec3(-2, -4, -6));
  EXPECT_EQ(list[1], mathfu::vec3(-8, -10, -12));
}

TEST(Deformation, ExtraDataUntouched) {
  auto deform = [](const mathfu::vec3& pos) { return -2.0f * pos; };

  std::vector<mathfu::vec4> list;
  list.emplace_back(1.0f, 2.0f, 3.0f, 4.0f);
  list.emplace_back(5.0f, 6.0f, 7.0f, 8.0f);
  ApplyDeformationToList(deform, &list);
  EXPECT_EQ(list[0], mathfu::vec4(-2, -4, -6, 4));
  EXPECT_EQ(list[1], mathfu::vec4(-10, -12, -14, 8));
}

}  // namespace
}  // namespace lull
