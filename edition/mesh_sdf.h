#ifndef VOXEL_MESH_SDF_H
#define VOXEL_MESH_SDF_H

#include "../storage/voxel_buffer_internal.h"
#include "../util/math/vector3f.h"
#include "../util/math/vector3i.h"
#include "../util/span.h"
#include "../util/tasks/threaded_task.h"

#include <atomic>
#include <memory>
#include <vector>

namespace zylann::voxel::mesh_sdf {

// Utilities to generate a signed distance field from a 3D triangle mesh.

struct Triangle {
	// Vertices to provide from the mesh.
	Vector3f v1;
	Vector3f v2;
	Vector3f v3;

	// Values precomputed with `prepare_triangles()`.
	Vector3f v21;
	Vector3f v32;
	Vector3f v13;
	Vector3f nor;
	Vector3f v21_cross_nor;
	Vector3f v32_cross_nor;
	Vector3f v13_cross_nor;
	float inv_v21_length_squared;
	float inv_v32_length_squared;
	float inv_v13_length_squared;
	float inv_nor_length_squared;
};

struct Chunk {
	Vector3i pos;
	std::vector<const Chunk *> near_chunks;
	std::vector<const Triangle *> triangles;
};

struct ChunkGrid {
	std::vector<Chunk> chunks;
	Vector3i size;
	Vector3f min_pos;
	float chunk_size;
};

class GenMeshSDFSubBoxTask : public IThreadedTask {
public:
	struct SharedData {
		std::vector<Triangle> triangles;
		std::atomic_int pending_jobs;
		VoxelBufferInternal buffer;
		Vector3f min_pos;
		Vector3f max_pos;
		ChunkGrid chunk_grid;
		bool use_chunk_grid = false;
		bool boundary_sign_fix = false;
	};

	std::shared_ptr<SharedData> shared_data;
	Box3i box;

	void run(ThreadedTaskContext ctx) override;

	// Called when `pending_jobs` reaches zero.
	virtual void on_complete() {}

	void apply_result() override {}
};

// Computes a representation of the mesh that's more optimal to compute distance to triangles.
bool prepare_triangles(Span<const Vector3> vertices, Span<const int> indices, std::vector<Triangle> &triangles,
		Vector3f &out_min_pos, Vector3f &out_max_pos);

// Partitions triangles of the mesh such that we can reduce the number of triangles to check when evaluating the SDF.
// Space is subdivided in a grid of chunks. Triangles overlapping chunks are listed, then each chunk finds which other
// non-empty chunks are close to it. The amount of subvidisions should be carefully chosen: too low will cause less
// triangles to be skipped, too high will make partitionning slower.
void partition_triangles(
		int subdiv, Span<const Triangle> triangles, Vector3f min_pos, Vector3f max_pos, ChunkGrid &chunk_grid);

// A naive method to get a sampled SDF from a mesh, by checking every triangle at every cell. It's accurate, but much
// slower than other techniques, but could be used as a CPU-based alternative, for less
// realtime-intensive tasks. The mesh must be closed, otherwise the SDF will contain errors.
void generate_mesh_sdf_full(Span<float> sdf_grid, const Vector3i res, Span<const Triangle> triangles,
		const Vector3f min_pos, const Vector3f max_pos);

// Compute the SDF faster by partitionning triangles, while retaining the same accuracy as if all triangles
// were checked. With Suzanne mesh subdivided once with 3900 triangles and `subdiv = 32`, it's about 8 times fasterthan
// checking every triangle on every cell.
void generate_mesh_sdf_partitioned(Span<float> sdf_grid, const Vector3i res, Span<const Triangle> triangles,
		const Vector3f min_pos, const Vector3f max_pos, int subdiv);

// Generates an approximation.
// Subdivides the grid into nodes spanning 4*4*4 cells each.
// If a node's corner distances are close to the surface, the SDF is fully evaluated. Otherwise, it is interpolated.
// Tests with Suzanne show it is 2 to 3 times faster than the basic naive method, with only minor quality decrease.
// It's still quite slow though.
void generate_mesh_sdf_approx_interp(Span<float> sdf_grid, const Vector3i res, Span<const Triangle> triangles,
		const Vector3f min_pos, const Vector3f max_pos);

// TODO Untested
// Generates an approximation.
// Calculates a thin hull of accurate SDF values, then propagates it across each axis using manhattan distance.
void generate_mesh_sdf_approx_sweep(Span<float> sdf_grid, const Vector3i res, Span<const Triangle> triangles,
		const Vector3f min_pos, const Vector3f max_pos);

Vector3i auto_compute_grid_resolution(const Vector3f box_size, int cell_count);

struct CheckResult {
	bool ok;
	struct BadCell {
		Vector3i grid_pos;
		Vector3f mesh_pos;
		unsigned int closest_triangle_index;
	};
	BadCell cell0;
	BadCell cell1;
};

// Checks if SDF variations are legit. The difference between two neighboring cells cannot be higher than the distance
// between those two cells. This is intented at proper SDF, not approximation or scaled ones.
CheckResult check_sdf(
		Span<const float> sdf_grid, Vector3i res, Span<const Triangle> triangles, Vector3f min_pos, Vector3f max_pos);

// The current method provides imperfect signs. Due to ambiguities, sometimes patches of cells get the wrong sign.
// This function attemps to correct these.
// Assumes the sign on the edge of the box is positive and use a floodfill.
// If we start from the rough SDF we had, we could do a floodfill that considers unexpected sign change as fillable,
// while an expected sign change would properly stop the fill.
// However, this workaround won't fix signs inside the volume.
// I thought of using this with an completely unsigned distance field instead, however I'm not sure if it's possible to
// accurately tell when when the sign is supposed to flip (i.e when we cross the surface).
void fix_sdf_sign_from_boundary(Span<float> sdf_grid, Vector3i res, Vector3f min_pos, Vector3f max_pos);

} // namespace zylann::voxel::mesh_sdf

#endif // VOXEL_MESH_SDF_H