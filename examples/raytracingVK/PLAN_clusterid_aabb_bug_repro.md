# Plan: Reproduce `rayQueryGetIntersectionClusterIdNV` Register Corruption with AABBs

## Bug Report (from Axel)

> "I believe `rayQueryGetIntersectionClusterIdNV` corrupts registers when the
> current intersection is with an AABB, not triangle geo. At least that's the
> only working theory I currently have for an extremely strange GPU crash. But
> it goes away if I don't do this.
>
> I wouldn't be surprised if no one tested this. It's obviously not very useful
> to do this, but I put the call in because in that path we still were filling
> a struct that had `cluster_id` in it."

**Root cause hypothesis:** Calling `rayQueryGetIntersectionClusterIdNV` on a
committed/candidate intersection that came from an AABB (procedural) geometry
rather than triangle geometry corrupts GPU registers, leading to hangs or
crashes.

---

## Current State of the Sample

| File | Role |
|---|---|
| `raytracingVK.h` | Class declaration; holds BLAS/TLAS, pipeline, descriptors, test geometry buffers |
| `raytracingVK.cpp` | Descriptor sets, prepare/render loop, command buffer recording |
| `raytracingVK_rt.cpp` | BLAS/TLAS creation, ray tracing pipeline, SBT, enabled features |
| `raytracingVK_util.cpp` | Uniform buffers, resize, `createTestGeometry()` (a 5-pointed star) |
| `raygen.rgen` | Ray generation shader: casts primary rays via `traceRayEXT` |
| `closesthit.rchit` | Closest-hit shader: unpacks triangle vertices, does N dot L lighting |
| `anyhit.rahit` | Any-hit shader: wireframe effect via barycentric edge test |
| `miss.rmiss` | Miss shader: sky gradient |

The sample currently:
- Uses **only triangle geometry** (a star or a glTF model).
- Has a single BLAS (triangles) and a single-instance TLAS.
- Uses a **ray tracing pipeline** (`vkCmdTraceRaysKHR`), not ray queries.
- Does **not** enable any NV cluster or ray query extensions.

---

## Reproduction Strategy

### Phase 1 -- Add AABB (Procedural) Geometry to the Scene

**Goal:** Have both triangle and AABB geometry in the TLAS so rays can hit
either type.

#### 1.1 Create AABB geometry buffer

- Define a set of `VkAabbPositionsKHR` structs (axis-aligned bounding boxes).
- Place them around the existing star geometry so that primary rays from the
  camera will hit some AABBs and some triangles.
- Suggested layout: scatter 4-8 small AABBs at known positions (e.g. a ring
  around the star, or a grid behind it).

#### 1.2 Build a second BLAS for the AABBs

- Create a new `AccelerationStructure aabbBottomLevelAS`.
- Use `VK_GEOMETRY_TYPE_AABBS_KHR` instead of `VK_GEOMETRY_TYPE_TRIANGLES_KHR`.
- Fill `VkAccelerationStructureGeometryAabbsDataKHR` with the AABB buffer
  device address and stride.
- Build the BLAS the same way we build the triangle BLAS.

#### 1.3 Add both BLASes as instances in the TLAS

- Currently `createTopLevelAccelerationStructure()` creates one instance
  pointing at `bottomLevelAS`.
- Change to two instances:
  - Instance 0: triangle BLAS (existing star) -- `instanceShaderBindingTableRecordOffset = 0`
  - Instance 1: AABB BLAS -- `instanceShaderBindingTableRecordOffset = 1`
    (points at a separate hit group that includes an intersection shader)

#### 1.4 Add an intersection shader for the AABBs

- Write `intersection.rint`: a trivial AABB intersection shader that reports
  a hit at `gl_RayTminEXT` (or computes a proper ray-box intersection).
- Add a second closest-hit shader (`closesthit_aabb.rchit`) that colors AABB
  hits distinctly (e.g. solid blue) so we can visually confirm they are being
  hit.
- Add a new hit group to the pipeline:
  ```
  SBT layout:
    [0] raygen
    [1] miss
    [2] hit (triangles): closesthit.rchit + anyhit.rahit
    [3] hit (AABBs):     closesthit_aabb.rchit + intersection.rint
  ```
- Update `createShaderBindingTables()` to account for the additional hit
  group entry.

#### 1.5 Verification checkpoint

At this point the sample should render **both** the star (triangles, wireframe)
and the AABBs (solid blue boxes). If it renders correctly, Phase 1 is done.

---

### Phase 2 -- Add Cluster Acceleration Structure Support

**Goal:** Enable `VK_NV_ray_tracing_cluster_acceleration_structure` so that
`rayQueryGetIntersectionClusterIdNV` is available.

#### 2.1 Enable required extensions

In `enableExtensions()` / `getEnabledFeatures()`:

- Add `VK_NV_ray_tracing_cluster_acceleration_structure` to the device
  extension list.
- Chain `VkPhysicalDeviceClusterAccelerationStructureFeaturesNV` into the
  device creation pNext chain and enable `clusterAccelerationStructure`.

#### 2.2 Build cluster-aware BLAS (optional, for triangle geo)

- Use `VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_CLUSTER_ACCELERATION_STRUCTURE_BIT_NV`
  when building the triangle BLAS, if the extension requires it for cluster
  IDs to be valid.
- The AABB BLAS does **not** get cluster structures (this is the whole point:
  calling the cluster ID intrinsic on a non-cluster, non-triangle intersection).

---

### Phase 3 -- Switch to Ray Queries and Call `rayQueryGetIntersectionClusterIdNV`

**Goal:** Use `GL_EXT_ray_query` (inline ray tracing) so we can call the
cluster ID intrinsic in a compute or fragment shader.

#### 3.1 Enable ray query extensions

- Add `VK_KHR_ray_query` device extension.
- Chain `VkPhysicalDeviceRayQueryFeaturesKHR` with `rayQuery = VK_TRUE`.
- Add `GL_EXT_ray_query` and `GL_NV_ray_tracing_cluster_acceleration_structure`
  to shader `#extension` directives.

#### 3.2 Write a ray query compute shader

Create `rayquery_cluster_test.comp`:

```glsl
#version 460
#extension GL_EXT_ray_query : require
#extension GL_NV_ray_tracing_cluster_acceleration_structure : enable

layout(binding = 0) uniform accelerationStructureEXT tlas;
layout(binding = 1, rgba8) uniform image2D outImage;
layout(binding = 2) uniform CameraProperties {
    mat4 viewInverse;
    mat4 projInverse;
    vec4 lightPos;
} cam;

layout(local_size_x = 8, local_size_y = 8) in;

void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size  = imageSize(outImage);
    if (any(greaterThanEqual(pixel, size))) return;

    vec2 uv = (vec2(pixel) + 0.5) / vec2(size);
    vec2 d  = uv * 2.0 - 1.0;

    vec3 origin    = (cam.viewInverse * vec4(0, 0, 0, 1)).xyz;
    vec4 target    = cam.projInverse * vec4(d, 1, 1);
    vec3 direction = normalize((cam.viewInverse * vec4(normalize(target.xyz / target.w), 0)).xyz);

    rayQueryEXT rq;
    rayQueryInitializeEXT(rq, tlas, gl_RayFlagsNoneEXT, 0xFF,
                          origin, 0.001, direction, 10000.0);

    vec3 color = vec3(0.0);

    while (rayQueryProceedEXT(rq))
    {
        if (rayQueryGetIntersectionTypeEXT(rq, false) ==
            gl_RayQueryCandidateIntersectionAABBEXT)
        {
            // *** THE BUG TRIGGER ***
            // Calling rayQueryGetIntersectionClusterIdNV on an AABB
            // intersection -- this is suspected to corrupt registers.
            uint clusterId = rayQueryGetIntersectionClusterIdNV(rq, false);

            // Confirm the hit so it becomes committed
            rayQueryGenerateIntersectionEXT(rq, rayQueryGetIntersectionTEXT(rq, false));
        }
    }

    if (rayQueryGetIntersectionTypeEXT(rq, true) !=
        gl_RayQueryCommittedIntersectionNoneEXT)
    {
        // Also try calling on committed intersection
        uint clusterId = rayQueryGetIntersectionClusterIdNV(rq, true);

        bool isAABB = (rayQueryGetIntersectionTypeEXT(rq, true) ==
                       gl_RayQueryCommittedIntersectionGeneratedEXT);

        if (isAABB)
            color = vec3(0.0, 0.0, 1.0);   // Blue = AABB hit
        else
            color = vec3(1.0, 1.0, 0.0);   // Yellow = triangle hit

        // Use clusterId to prevent the compiler from optimizing it away
        color *= float(clusterId == 0u ? 1 : 1);
    }

    imageStore(outImage, pixel, vec4(color, 1.0));
}
```

Key points:
- We call `rayQueryGetIntersectionClusterIdNV(rq, false)` on a **candidate
  AABB intersection** -- this is the exact scenario Axel described.
- We also call it on the **committed** intersection (which may be an AABB
  that was generated via `rayQueryGenerateIntersectionEXT`).
- We use the returned `clusterId` value so the compiler cannot dead-code
  eliminate the call.

#### 3.3 Create compute pipeline and dispatch

- Create a separate `VkPipeline` (compute) with the ray query shader.
- Reuse the same descriptor set layout (TLAS + storage image + UBO).
- Dispatch with `vkCmdDispatch(cmdBuffer, ceil(width/8), ceil(height/8), 1)`.
- Add a toggle (`useRayQueryPath = true/false`) so we can easily compare the
  ray tracing pipeline path vs. the ray query compute path.

---

### Phase 4 -- Observe & Validate

#### 4.1 Expected outcomes

| Scenario | Expected behavior if bug exists |
|---|---|
| Ray query hits **triangle** geo, calls `rayQueryGetIntersectionClusterIdNV` | Works fine (cluster ID may be 0 or valid) |
| Ray query hits **AABB** geo, calls `rayQueryGetIntersectionClusterIdNV` on **candidate** | GPU hang / crash / visual corruption |
| Ray query hits **AABB** geo, calls `rayQueryGetIntersectionClusterIdNV` on **committed** | GPU hang / crash / visual corruption |
| Same setup but **without** the `rayQueryGetIntersectionClusterIdNV` call | Works fine (control case) |

#### 4.2 Diagnostic toggles

Add runtime or compile-time toggles so we can narrow down exactly which call
path triggers the issue:

- `ENABLE_CLUSTER_ID_ON_CANDIDATE_AABB` -- call on candidate AABB intersection
- `ENABLE_CLUSTER_ID_ON_COMMITTED_AABB` -- call on committed AABB intersection
- `ENABLE_CLUSTER_ID_ON_TRIANGLE` -- call on triangle intersection (control)

#### 4.3 Validation layers & debug tooling

- Run with `VK_LAYER_KHRONOS_validation` enabled.
- Enable GPU-assisted validation if available
  (`VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT`).
- Check for device-lost errors via `VK_EXT_device_fault` if supported.
- Monitor for TDR (Timeout Detection and Recovery) on Windows.

---

## Implementation Order (files to modify/create)

### C++ Host Code

| Step | File | Change |
|---|---|---|
| 1 | `raytracingVK.h` | Add `AccelerationStructure aabbBottomLevelAS`, AABB buffer, compute pipeline members, `useRayQueryPath` flag |
| 2 | `raytracingVK_util.cpp` | Add `createAABBGeometry()` to generate AABB buffer |
| 3 | `raytracingVK_rt.cpp` | Modify `createBottomLevelAccelerationStructure()` to also build AABB BLAS; modify TLAS to include both instances; add second hit group with intersection shader; enable cluster + ray query extensions in `getEnabledFeatures()` |
| 4 | `raytracingVK.cpp` | Add compute pipeline creation; add toggle to dispatch ray query compute vs. ray tracing pipeline; update descriptor sets for compute |
| 5 | `raytracingVK.cpp` | Cleanup: destroy new resources in destructor |

### Shaders (new files)

| File | Purpose |
|---|---|
| `intersection.rint` | Intersection shader for AABB procedural geometry |
| `closesthit_aabb.rchit` | Closest-hit shader for AABB hits (distinct color) |
| `rayquery_cluster_test.comp` | Compute shader with ray query + `rayQueryGetIntersectionClusterIdNV` calls on AABB intersections |

### Shaders (modified)

| File | Change |
|---|---|
| `compile.sh` | Add compilation commands for new shaders |

---

## Risk & Notes

- **Driver support:** `VK_NV_ray_tracing_cluster_acceleration_structure` is an
  NV-specific extension. The test must run on an NVIDIA GPU with a driver
  version that exposes this extension.
- **Cluster BLAS is optional for the repro:** The bug may trigger even without
  actually building cluster acceleration structures -- the key is calling the
  intrinsic when the intersection type is AABB.
- **Minimal repro variant:** If we want the smallest possible repro, we could
  skip the triangle geometry entirely and have a scene with **only** AABBs,
  then fire a single ray query and call `rayQueryGetIntersectionClusterIdNV`.
  This removes variables and makes the crash more attributable.
- **Fallback if ray query path doesn't crash:** Try the same call pattern
  inside the ray tracing pipeline (closest-hit shader for AABBs) using
  `GL_EXT_ray_tracing` instead of `GL_EXT_ray_query`, though the intrinsic
  name suggests it is ray-query specific.
