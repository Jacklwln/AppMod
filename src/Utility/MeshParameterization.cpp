#include "MeshParameterization.h"
#include "Model.h"
#include "Shape.h"
#include "obj_writer.h"
#include "CurvesUtility.h"

#include <cv.h>
#include <set>

MeshParameterization::MeshParameterization()
{
  this->init();
}

MeshParameterization::~MeshParameterization()
{

}

void MeshParameterization::init()
{
  cut_shape = nullptr;
}

void MeshParameterization::doMeshParameterization(std::shared_ptr<Model> model)
{
  this->cutMesh(model);
  this->prepareCutShape(model);
  this->findBoundary();
  this->computeBaryCentericPara();
  this->saveParameterization(model->getOutputPath());
}

void MeshParameterization::saveParameterization(std::string file_path)
{
  STLVectorf vertex_list;
  const STLVectorf& UV_list = cut_shape->getUVCoord();
  for (size_t i = 0; i < UV_list.size() / 2; ++i)
  {
    vertex_list.push_back(UV_list[2 * i + 0]);
    vertex_list.push_back(UV_list[2 * i + 1]);
    vertex_list.push_back(0.0f);
  }

  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  tinyobj::shape_t obj_shape;

  obj_shape.mesh.positions = vertex_list;
  obj_shape.mesh.indices = cut_shape->getFaceList();
  shapes.push_back(obj_shape);
  WriteObj(file_path + "/parameterization.obj", shapes, materials);
}

void MeshParameterization::cutMesh(std::shared_ptr<Model> model)
{
  cv::Mat& primitive_ID_img = model->getPrimitiveIDImg();
  std::set<int> visible_faces;
  for (int i = 0; i < primitive_ID_img.rows; ++i)
  {
    for (int j = 0; j < primitive_ID_img.cols; ++j)
    {
      int face_id = primitive_ID_img.at<int>(i, j);
      if (face_id >= 0)
      {
        visible_faces.insert(face_id);
      }
    }
  }

  const AdjList& f_adjList = model->getShapeFaceAdjList();
  std::set<int> hole_faces;
  do
  {
    hole_faces.clear();
    for (auto i : visible_faces)
    {
      for (int j = 0; j < 3; ++j)
      {
        int adj_f = f_adjList[i][j];
        if (visible_faces.find(adj_f) == visible_faces.end())
        {
          int fs[2];
          if (f_adjList[adj_f][0] == i) { fs[0] = f_adjList[adj_f][1]; fs[1] = f_adjList[adj_f][2]; }
          else if (f_adjList[adj_f][1] == i) { fs[0] = f_adjList[adj_f][0]; fs[1] = f_adjList[adj_f][2]; }
          else if (f_adjList[adj_f][2] == i) { fs[0] = f_adjList[adj_f][0]; fs[1] = f_adjList[adj_f][1]; }

          if (visible_faces.find(fs[0]) != visible_faces.end() || visible_faces.find(fs[1]) != visible_faces.end())
          {
            hole_faces.insert(adj_f);
          }
        }
      }
    }

    visible_faces.insert(hole_faces.begin(), hole_faces.end());
  } while (!hole_faces.empty());

  cut_face_list.clear();
  const FaceList& ori_face_list = model->getShapeFaceList();
  for (auto i : visible_faces)
  {
    cut_face_list.push_back(ori_face_list[3 * i + 0]);
    cut_face_list.push_back(ori_face_list[3 * i + 1]);
    cut_face_list.push_back(ori_face_list[3 * i + 2]);
  }

  //std::vector<tinyobj::shape_t> shapes;
  //std::vector<tinyobj::material_t> materials;
  //tinyobj::shape_t obj_shape;

  //obj_shape.mesh.positions = model->getShapeVertexList();
  //obj_shape.mesh.indices = cut_face_list;
  //shapes.push_back(obj_shape);
  //WriteObj(model->getOutputPath() + "/cutface.obj", shapes, materials);
}

void MeshParameterization::prepareCutShape(std::shared_ptr<Model> model)
{
  //get all parameterization related vertices
  const FaceList& face_list = model->getShapeFaceList();

  // vertex_set store the vertex id map from old full mesh to new cut mesh
  vertex_set.clear();
  for (auto i : cut_face_list)
  {
    vertex_set.push_back(i);
  }
  std::sort(vertex_set.begin(), vertex_set.end());
  vertex_set.erase(std::unique(vertex_set.begin(), vertex_set.end()), vertex_set.end());
  vertex_set.shrink_to_fit();

  const VertexList& vertex_list = model->getShapeVertexList();
  VertexList new_vertex_list;
  for (auto i : vertex_set)
  {
    new_vertex_list.push_back(vertex_list[3 * i + 0]);
    new_vertex_list.push_back(vertex_list[3 * i + 1]);
    new_vertex_list.push_back(vertex_list[3 * i + 2]);
  }
  FaceList new_face_list;
  for (auto i : cut_face_list)
  {
    size_t id = std::distance(vertex_set.begin(), std::find(vertex_set.begin(), vertex_set.end(), i));
    new_face_list.push_back(id);
  }
  STLVectorf UVList(vertex_set.size() * 2, 0.0f);

  cut_shape.reset(new Shape);
  cut_shape->init(new_vertex_list, new_face_list, UVList);
}

void MeshParameterization::findBoundary()
{
  // find all boundaries
  const STLVectori& edge_connectivity = cut_shape->getEdgeConnectivity();
  const FaceList& face_list = cut_shape->getFaceList();
  std::vector<Edge> boundary_edges;
  int inner_index[6] = {0, 1, 1, 2, 2, 0};
  for (size_t i = 0; i < edge_connectivity.size(); ++i)
  {
    if (edge_connectivity[i] == -1)
    {
      // a boundary edge
      int face_id = i / 3;
      int inner_id = i % 3;
      int v0 = face_list[3 * face_id + inner_index[2 * inner_id + 0]];
      int v1 = face_list[3 * face_id + inner_index[2 * inner_id + 1]];
      boundary_edges.push_back(Edge(v0, v1));
    }
  }
  std::vector<STLVectori> boundary_lines;
  CurvesUtility::mergeShapeEdges(boundary_edges, boundary_lines);
  // use the longest boundary for parameterization
  size_t longest_id = 0;
  size_t longest_len = std::numeric_limits<size_t>::min();
  for (size_t i = 0; i < boundary_lines.size(); ++i)
  {
    if (boundary_lines[i].size() > longest_len)
    {
      longest_len = boundary_lines[i].size();
      longest_id = i;
    }
  }
  boundary_loop = boundary_lines[longest_id];
}

void MeshParameterization::computeBaryCentericPara()
{
  // map boundary loop to unit circle in texture domain
  const VertexList& vertex_list = cut_shape->getVertexList();
  STLVectorf UV_list(2 * vertex_list.size() / 3, 0.0f);
  float length = 0.0;
  size_t n = boundary_loop.size();
  for (size_t i = 0; i < n; ++i)
  {
    int v_0 = boundary_loop[i];
    int v_1 = boundary_loop[(i + 1) % n];

    Vector3f diff(vertex_list[3 * v_0 + 0] - vertex_list[3 * v_1 + 0],
                  vertex_list[3 * v_0 + 1] - vertex_list[3 * v_1 + 1],
                  vertex_list[3 * v_0 + 2] - vertex_list[3 * v_1 + 2]);

    length += diff.norm();
  }
  float l = 0.0;
  for (size_t i = 0; i < n; ++i)
  {
    float angle = l / length * 2.0 * M_PI;
    UV_list[2 * boundary_loop[i] + 0] = 0.5 * cos(angle) + 0.5;
    UV_list[2 * boundary_loop[i] + 1] = 0.5 * sin(angle) + 0.5;

    int v_0 = boundary_loop[i];
    int v_1 = boundary_loop[(i + 1) % n];

    Vector3f diff(vertex_list[3 * v_0 + 0] - vertex_list[3 * v_1 + 0],
                  vertex_list[3 * v_0 + 1] - vertex_list[3 * v_1 + 1],
                  vertex_list[3 * v_0 + 2] - vertex_list[3 * v_1 + 2]);
    
    l += diff.norm();
  }

  // setup matrix and rhs
  // 1. delete boundary loop vertex from the vertex list
  STLVectori free_vertices;
  for (size_t i = 0; i < vertex_list.size() / 3; ++i)
  {
    if (std::find(boundary_loop.begin(), boundary_loop.end(), i) == boundary_loop.end())
    {
      free_vertices.push_back(i);
    }
  }

  // 2. fill matrix
  const AdjList& v_adjlist = cut_shape->getVertexAdjList();
  size_t N = free_vertices.size();
  SparseMatrix A(N, N);
  TripletList A_triplets;
  std::vector<VectorXf> b(2, VectorXf(N));
  std::vector<VectorXf> x(2, VectorXf(N));
  std::map<int, float> row;
  std::map<int, float>::iterator r_it;
  for (size_t i = 0; i < N; ++i)
  {
    row.clear();
    this->computeLaplacianWeight(free_vertices[i], row);
    b[0][i] = 0.0; b[1][i] = 0.0;

    for (r_it = row.begin(); r_it != row.end(); ++r_it)
    {
      if (std::find(boundary_loop.begin(), boundary_loop.end(), r_it->first) != boundary_loop.end())
      {
        b[0][i] -= r_it->second * UV_list[2 * r_it->first + 0];
        b[1][i] -= r_it->second * UV_list[2 * r_it->first + 1];
      }
      else
      {
        size_t idx = std::distance(free_vertices.begin(), std::find(free_vertices.begin(), free_vertices.end(), r_it->first));
        A_triplets.push_back(Triplet(i, idx, r_it->second));
      }
    }
  }

  A.setFromTriplets(A_triplets.begin(), A_triplets.end());
  Eigen::SimplicialLLT<SparseMatrix> solver(A);

  x[0] = solver.solve(b[0]);
  x[1] = solver.solve(b[1]);

  for (size_t i = 0; i < N; ++i)
  {
    UV_list[2 * free_vertices[i] + 0] = x[0][i];
    UV_list[2 * free_vertices[i] + 1] = x[1][i];
  }

  cut_shape->setUVCoord(UV_list);
}

void MeshParameterization::computeLaplacianWeight(int v_id, std::map<int, float>& weight)
{
  const STLVectori& v_ring = cut_shape->getVertexAdjList()[v_id];
  const VertexList& vertex_list = cut_shape->getVertexList();
  float wi = 0.0f;

  for (size_t i = 0; i < v_ring.size(); ++i)
  {
    STLVectori share_vertex;
    this->findShareVertex(v_id, v_ring[i], share_vertex);

    float wij = 0.0f;
    if (share_vertex.size() == 2) 
    {
      wij = computeWij(&vertex_list[3 * v_id], &vertex_list[3 * v_ring[i]], 
        &vertex_list[3 * share_vertex[0]], &vertex_list[3 * share_vertex[1]]);
    }
    else wij = computeWij(&vertex_list[3 * v_id], &vertex_list[3 * v_ring[i]], &vertex_list[3*share_vertex[0]]);

    weight[v_ring[i]] = -wij;
    wi += wij;
  }
  weight[v_id] = wi;
}

void MeshParameterization::findShareVertex(int pi, int pj, STLVectori& share_vertex)
{
  const STLVectori& pi_f_ring = cut_shape->getVertexShareFaces()[pi];
  const STLVectori& pj_f_ring = cut_shape->getVertexShareFaces()[pj];

  STLVectori faces;
  std::set_intersection(pi_f_ring.begin(), pi_f_ring.end(), pj_f_ring.begin(), pj_f_ring.end(), back_inserter(faces));

  const FaceList& face_list = cut_shape->getFaceList();
  for (size_t i = 0; i < faces.size(); ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      if (face_list[3 * faces[i] + j] != pi && face_list[3 * faces[i] + j] != pj)
      {
        share_vertex.push_back(face_list[3 * faces[i] + j]);
        break;
      }
    }
  }

  if (share_vertex.size() > 2)
  {
    std::cout << "share vertices number warning: " << share_vertex.size() << std::endl;
  }
}

float MeshParameterization::computeWij(const float *p1, const float *p2, const float *p3, const float *p4)
{
  float e1 = sqrt((p1[0]-p2[0])*(p1[0]-p2[0])+(p1[1]-p2[1])*(p1[1]-p2[1])+(p1[2]-p2[2])*(p1[2]-p2[2]));
  float e2 = sqrt((p1[0]-p3[0])*(p1[0]-p3[0])+(p1[1]-p3[1])*(p1[1]-p3[1])+(p1[2]-p3[2])*(p1[2]-p3[2]));
  float e3 = sqrt((p3[0]-p2[0])*(p3[0]-p2[0])+(p3[1]-p2[1])*(p3[1]-p2[1])+(p3[2]-p2[2])*(p3[2]-p2[2]));
  float alpha_cos = fabs((e3*e3+e2*e2-e1*e1)/(2*e3*e2));
  float beta_cos = 0;
  if (p4 != nullptr) {
    float e4 = sqrt((p1[0]-p4[0])*(p1[0]-p4[0])+(p1[1]-p4[1])*(p1[1]-p4[1])+(p1[2]-p4[2])*(p1[2]-p4[2]));
    float e5 = sqrt((p4[0]-p2[0])*(p4[0]-p2[0])+(p4[1]-p2[1])*(p4[1]-p2[1])+(p4[2]-p2[2])*(p4[2]-p2[2]));
    beta_cos = fabs((e4*e4+e5*e5-e1*e1)/(2*e4*e5));
  }
  return ((alpha_cos/sqrt(1-alpha_cos*alpha_cos))+(beta_cos/sqrt(1-beta_cos*beta_cos)))/2;
}