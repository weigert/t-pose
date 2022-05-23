# t-pose experiments

A number of test programs for aspects of the main reconstruction pipeline.

    triangulation
      render_direct - a direct method for rendering a triangulation
      render_indirect - an indirect method for rendering a triangulation (more flexible using SSBOs, allows for compute shader vertex position feedback)

    fit
      color_constant - for a triangulation, fit an average color using shaders
      energy_constant - for a triangulation, compute per-triangle cost function

    optimize
      gradient_descent - basic gradient descent for the cost function to shift vertex positions (note: no topological optimization)
      optimize_topology - gradient descent with toplogical optimization
      no_geometry_shader - reimplementation with indirect method and no geometry shaders

    reconstruct
      using only point-match data and camera calibration matrix compute a 3D metric pointcloud
