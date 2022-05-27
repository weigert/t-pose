# t-pose

(triangulation pose)

Two-View Pose Estimation and Direct-Mesh Scene Reconstruction from Image Triangulation

**Disclaimer**: This repository is a proof of concept / prototype for testing purposes. I am publishing it now in case anybody is interested. The methodology is still being improved and the repository is still being cleaned. The state of progress and improvements is explained below.

<img alt="Example Interpolation of a Warping between two Views" src="https://github.com/weigert/t-pose/blob/main/screenshots/interpolate.gif">

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

#### Mesh Reconstruction

This is the aspect with which I am currently having the most difficulties. Given two views, the scene is strictly only reconstructable up to a projective ambiguity without supplying additional scene or camera information.

A fundamental matrix can be computed, but the accuracy is still difficult to determine at the moment and without explicitly resolving the projective ambiguity it is difficult to gauge and debug.

Nonetheless, an implementation that should give a 3D reconstruction is provided but I am not currently satisfied with it and it still requires tuning, particularly in recognizing erronious warpings and the management of occlusion.

## Usage

Note that this repository is still a mess and I haven't yet decided on a data io structure for working with data sets.

Written in C++.

Build the programs with `make all`.

Requires:
- [TinyEngine](https://github.com/weigert/TinyEngine)
- OpenCV (only for two small aspects - I plan on removing this dependency)

### Reading

`/experiments/`: Smaller programs for testing aspects of the pipeline
`/programs/`: Main pipeline for generating scene reconstructions
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
