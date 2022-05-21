# todo

How do I get the actual 3D locations out?
Does the fact that I have a triangulation constrain the problem somehow that makes it easier to solve or not?

## LLE Approach

LLE is useful because it is invariant to rigid transformations.

1. Every node can be locally linear embedded in both projects, warped and unwarped
2. In 2D space, this will yield two sets of weights for warped and unwarped
3. Can I use this collective information to solve for the 3D positions, assuming that both sets of weights are correct, but just embedded?

That would be sick.

This would basically enforce the local structure.

The neighborhood structure approximation is constant in 3D
and warped in 2D, take the two warped version and determine the missing component in 3D.

### More Details

We assume the higher dimensional space is 3D.
Each point is reconstructed from it's neighbors in a locally tangent space, i.e. the surface is the manifold.

Thus we have the weights.

Projecting this down, we then got projected weights.

We can compute these weights too using LLE for both warpings.

Then we try to recover the depth by assuming that these two warpings are somehow consistent with the 3D weights.

Problem: LLE does not like it if the space is 3 dimensional, but every point HAS to have at least 3 neighbors. This is sub-optimal.

## Distance Based Formulation

What if instead, I assumed in 3D space the distances from points to their neighbors were defined??

Does this uniquely let me recover their positions in space?
