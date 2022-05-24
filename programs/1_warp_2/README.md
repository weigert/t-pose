# warp 2

Hierarchical Two-Way Consistent Warping

From two images A, B we generate two triangulations T(A), T(B) at different resolutions.

To warp these triangulations, we fit at every resolution to the other image T'(A), T'(B).

After each warping step, we can take the warping T'(A) to warp T(B) and restart the warp.
Same the other way around. After a few iterations, the warpings should be consistent.

This also allows us to compute a discrepancy and pick points which are not warped well.
