glm::vec3 vertices3d[12][3] = {
      {glm::vec3(1.0f, -1.0f, 3.0f), glm::vec3(1.0f, 1.0f, 3.0f),
       glm::vec3(1.0f, 1.0f, 5.0f)},
      {glm::vec3(1.0f, -1.0f, 3.0f), glm::vec3(1.0f, -1.0f, 5.0f),
       glm::vec3(1.0f, 1.0f, 5.0f)},
      {glm::vec3(-1.0f, 1.0f, 3.0f), glm::vec3(-1.0f, -1.0f, 3.0f),
       glm::vec3(1.0f, 1.0f, 3.0f)},
      {glm::vec3(-1.0f, -1.0f, 3.0f), glm::vec3(1.0f, -1.0f, 3.0f),
       glm::vec3(1.0f, 1.0f, 3.0f)},
  };
  raster3DTriangle(vertices3d[0], 255, 0, 0);
  raster3DTriangle(vertices3d[1], 200, 0, 0);
  raster3DTriangle(vertices3d[2], 0, 255, 0);
  raster3DTriangle(vertices3d[3], 0, 200, 0);