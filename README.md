# t-pose

(triangulation pose)

Two-View Pose Estimation and Direct-Mesh Scene Reconstruction from Image Triangulation

Note: This is a rough proof of concept / prototype and this repo ist still sloppy, being cleaned.
This currently also does not include a bundle adjustment step. A number of potential improvements are listed below.

## Concept

Surfaces of equal material and orientation typically exhibit the same color when projected on images due to lighting. Therefore, triangulations of images are natural choices for representing surface geometry.

Since projections of triangles from 3D into 2D preserve the triangle geometry, an image triangulation can be directly converted to a 3D mesh if the 3D positions of the vertices can be determined. To do this, a standard multiview geometry approach is used, by warping the triangulation from one image onto another.

For a calibrated camera, this allows for direct computation of camera pose and metric vertex positions and mesh.

In a manner of speaking, this uses the energy minimization over triangular surfaces of uniform orientation to extract triangulation vertices as features.

## Implementation

1. Generate triangulation  for image A, optimizing a cost function and triangulation topology
2. Warp triangulation onto image B, also by optimizing a cost function (preserving topology)
3. Compute fundamental matrix, unproject points, visualize mesh

Alternatively, you can generate triangulations for image A and B and use a two-way consistent warping (with barycentrics) to improve the warping quality and make it converge more consistently.

![triangulation](https://github.com/weigert/t-pose/blob/main/screenshots/triangulate.gif)

![warping](https://github.com/weigert/t-pose/blob/main/screenshots/warp.gif)

![interpolate](https://github.com/weigert/t-pose/blob/main/screenshots/view.gif)

(thanks to my roommate aaron for t-posing lol)

Some boundary artifacts can be seen in the warped image.

Optionally, if the fundamental matrix is known (e.g. by other feature matching techniques), then the warping can utilize the epipolar geometry to warp more optimally and the mesh comes out better.

The triangulation can be accelerated by using a good initial guess, e.g. delaunay triangulating interest points. This is not necessary, the triangulations that the system finds without any assumptions are very good.

## Usage

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

## Credits

Nicholas McDonald, 2022

## To-Do

### Short Term

- Clean IO System / Ratio Setting / Window-Size Setting
- Normalize the Cost Functions Properly
    Note: Difficult because of atomic operation limitation
- Properly Separate out the Parameters (e.g. energy weights)
- Better Storage Format Handling

### Mid Term

- Fix Flip Oscillation Somehow ( Flip Decay? ). This is the main slowing factor.
- Implement Linear Gradient Cost Function for improved fitting!
- Texture Mip-Mapping for Improved Speed and Accuracy

### Long Term

- Topology Optimization / Detachment During Warping
    To Handle Occlusion!
