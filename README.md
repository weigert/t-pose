# t-pose

(triangulation pose)

Two-View Pose Estimation and Direct-Mesh Scene Reconstruction from Image Triangulation

The animation below was generated from only 2 images using the method described below and implemented in this repository.

<img alt="Example Interpolation of a Warping between two Views, with Texture Mapping" src="https://github.com/weigert/t-pose/blob/main/screenshots/warp_texture.gif">

**Disclaimer**: This repository is a proof of concept / prototype for testing purposes. I am publishing it now in case anybody is interested. The methodology is still being improved and the repository is still being cleaned. The state of progress and improvements is explained below.

## Basic Concept

Surfaces of equal material and orientation typically exhibit the same color when projected in images due to lighting. Therefore, triangulations of images are natural choices for representing surface geometry.

Projections of triangles from 3D into 2D preserve straight lines and thus triangle geometry. Thus, an image triangulation can be directly converted to a 3D mesh if the 3D positions of the vertices can be computed.

To do this, a multi-view geometry approach is applied, where "feature matches" are used to reconstruct the 3D positions. To generate these features, 2D image triangulations are warped between two images of two different views of the same object.

For calibrated cameras, this allows for direct computation of camera pose, metric vertex positions and thus an object mesh.

### Details

The main algorithm consists of the following steps:

1. Triangulate Images
2. Warp Triangulations
3. Reconstruct Mesh

#### Triangulation

Given two images, we compute a triangulation for each using a minimum energy based approach. Each triangle computes the average color of the image underneath and a cost function can be computed by the distance of the average to the true value. Vertex positions are shifted along the gradient of this cost function. This is implemented using shaders on the GPU (no geometry shaders).

<img alt="Las Meninas by Diego Velazquez, Triangulated with 3000 Triangles (Surfaces Only)" src="https://github.com/weigert/t-pose/blob/main/screenshots/meninas.png" width="48%"> <img alt="Las Meninas by Diego Velazquez, Triangulated with 3000 Triangles (With Lines)" src="https://github.com/weigert/t-pose/blob/main/screenshots/meninas_lines.png" width="48%">

The topology of the triangulation is optimized using a topological optimization strategy, by flipping edges, splitting triangles at their centroid, collapsing short edges and pruning flattened triangles on the domain boundary.

The topological minimum energy approach leads to good approximations of the base image at various levels of detail, defining a triangulation hierarchy for which the approximation becomes increasingly detailed as the number of triangles increases.

The triangulations that this system finds from zero-assumptions are very good and give clean boundaries, such that the vertices are well fit to image features.

<img alt="Triangulation Animation of a Sneaker" src="https://github.com/weigert/t-pose/blob/main/screenshots/shoeA.gif">

#### Warping

The coarse-to-fine triangulations can be used to warp one image onto the other with better convergence. Barycentric coordinates on triangles in 3D are also preserved under projective transformations, which allows for a simple hierarchical warping algorithm.

Starting from the most coarse triangulation `T0(A)` for image `A`, we can warp it into image `B` by minimizing the cost function (i.e. moving vertices along gradient) without altering the topology of `T0(A)`. This gives us vertex positions for all triangles before and after warping (`V0` -> `V0'`). For the next triangulation `T1(A)`, we can compute the barycentric coordiantes of the vertices `V1` in terms of the triangles of `T0(A)` and vertices `V0`, and recover their "warped" cartesian coordiantes from the vertices `V0'`. This acts as the next-best-guess initial condition for warping `V1` -> `V1'`, consistent with the previous warping step. This process is repeated for increasingly fine triangulations.

Finally, this method can be made two-way consistent for two images `A` and `B`.

Any vertex warped by `TN(A)` -> `TN(A)'` should be warped identically by `TN(B)'` -> `TN(B)`. This is enforced by making the initial condition of a warping not the forward warping of the previous more coarse step, but the reverse warping of the previous more coarse step of the other image's triangulation.

<img alt="Two-Way Consistent Warping between two Views" src="https://github.com/weigert/t-pose/blob/main/screenshots/warp.gif">

This leads to incredibly clean warpings which accurately predict the positions of key-points as triangulation vertices.

<img alt="Example Interpolation of a Warping between two Views" src="https://github.com/weigert/t-pose/blob/main/screenshots/interpolate.gif">

#### Mesh Reconstruction

This is the aspect with which I am currently having the most difficulties. Given two views, the scene is strictly only reconstructable up to a projective ambiguity without supplying additional scene or camera information.

A fundamental matrix can be computed, but the accuracy is still difficult to determine at the moment and without explicitly resolving the projective ambiguity it is difficult to gauge and debug.

Nonetheless, an implementation that should give a 3D reconstruction is provided but I am not currently satisfied with it and it still requires tuning, particularly in recognizing erronious warpings and the management of occlusion.

#### RGB-D Meshing

*Note: This part of the repo is merely a proof of concept and depends on a library which I have not yet released.*

The 2D triangulation can also be used to generate accurate meshes from RGB-D data. The image is first triangulated, and the depth data is fit to the triangles. The triangulation provides an additional constraint on the vertices of the mesh, requiring that they satisfy all (or a subset) of the plane equations. This can be solved in a least-squares sense.

<img alt="Triangulation computed for a depth-aligned RGB image" src="https://github.com/weigert/t-pose/blob/main/screenshots/rgbd-triangulation.jpg">

The above image is the triangulation of a depth-aligned RGB-D image.

<img alt="Deprojected Pointcloud from RGB-D Dataset, Showing Surface Normals" src="https://github.com/weigert/t-pose/blob/main/screenshots/rgbd-pointcloud.jpg">

A pointcloud can be computed from the depth values (given camera intrinsics). Accurate surface normals improve the reconstruction quality (these were computed using a custom algorithm) The data-set used for this was generated using an Intel Realsense L515 camera.

<img alt="Computed 3D Mesh from RGB-D dataset using planar triangulation" src="https://github.com/weigert/t-pose/blob/main/screenshots/rgbd-mesh.gif">

The resulting mesh fits well to the data, is extremely fast to compute and does not require the computation signed distance functions. Different method are possible for fitting the mesh vertices ideally to the data, and I am currently working on figuring out what works best. Additionally, occlusion needs to be detected by potentially splitting a vertex into two. Currently, I assume full water-tightness.

The triangulation thus provides a convenient structure for directly applying statistical methods to the mesh fitting.

## Usage

Note that this repository is still a mess and I haven't yet decided on a data io structure for working with data sets. This will follow in the future.

Nevertheless, you can build the programs which have inconsistent interfaces.

Written in C++.

Build each program with `make all`.

Dependencies:
- [TinyEngine](https://github.com/weigert/TinyEngine)
- OpenCV (only for two small aspects - I plan on removing this dependency)

Run the programs using `./main`, the programs will let you know what input they want.

Some programs generate output triangulation `.tri` files, which are a storage format for the triangulations (which will be overhauled). These are stored in the root `output` folder, so some programs will ask for a dataset name to write them to (e.g. triangulation program, warping program).

### Reading

`/experiments/`: Smaller programs for testing aspects of the pipeline
`/programs/`: Main pipeline for generating scene reconstructions
`/resource/`: Just some testing data. Not linked to the programs in any way.
`/output/`: Data output storage folder
`/source/`: Main header files

## Future Work and Improvements

### Top-Level

The estimation of the fundamental matrix needs to be automated by detecting relevant and non-relevant triangles. I predict that this will primarily be related to the gradients of the cost function.

If the fundamental matrix is known, the warping can exploit the epipolar geometry to warp more optimally. This was implemented in a previous version, but I decided to drop this assumption.

Triangulations can be accelerated using a good initial guess, e.g. by delaunay triangulating interest points. This is not necessary, the triangulations that the system finds without any assumptions are very good.

There are still some issues with oscillation of the optimizer which I would like to fix. Sometimes the optimization gets trapped due to the gradient approximation.

A linear gradient cost function can be used to improve the goodness of fit and lower the triangulation energy more, but the constant color fitting is currently still sufficiently good. This could help triangle fitting / feature detection in sparse areas.

Performance improvements might be achievable by using texture mipmapping so that the coarseness of the image scales with the coarseness of the triangulation.

Finally, the main potential for this method comes from topological alterations during the warping step. By allowing for the collapsing of edges and creation (or specifically duplication) of vertices / detachment of triangles, this system has the potential to deal with occlusion explicitly and elegantly. This is a difficult subject that warrants some planning though.

### General

- Better Triangulation Storage Format and Data IO
- Proper Parameter Separation / Loading
- Cost Function Normalization (very hard because of integer atomics)

## Credits

Nicholas McDonald, 2022

The approach to image triangulation was originally inspired by ["Stylized Image Triangulation", Kay Lawonn and Tobias Guenther (2018)](https://cgl.ethz.ch/publications/papers/paperLaw18a.php). I reimplemented the concept entirely on my own taking cues from their paper.

A number of general improvements were made, including removing the dependency on geometry shaders, removing the delaunay triangulation requirement (which improved the fit greatly), and removing the one-ring energy term which shifted the vertices sufficiently to make 3D reconstruction infeasible.
