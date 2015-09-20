#ifndef Model_H
#define Model_H

#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <limits>

#include <Eigen\Eigen>
#include <Eigen\Sparse>
#include <cv.h>
#include <highgui.h>

#include "kdtree.h" // to make life easier...

class Viewer;
class ModelLight;
class Bound;
class Ray;
class Light;

class Model
{
public:
    typedef std::vector<float> VectorF;
    typedef std::vector<unsigned int> Facelist;
    typedef std::vector<float> Colorlist;
    typedef std::vector<float> Normallist;
    typedef std::vector<std::vector<int>> FaceAdjlist;
    typedef std::vector<std::vector<int>> VertexAdjlist;
    typedef Eigen::Vector3i Vector3i;
    typedef Eigen::Vector3f Vector3f;
    typedef std::vector<Vector3i> VectorVec3i;
    typedef std::vector<Vector3f> VectorVec3f;
    typedef std::pair<int, int> UDEdge;

    

public:
    Model();
    ~Model();

    Model(const int id, const std::string path, const std::string name);
    void passData(VectorF &vertices, Facelist &faces, Colorlist &colors);
	void exportOBJ(int cur_iter = 0);
    void setInit();

    void drawFaceNormal();

    void computeLight();
    void computeBrightness();
    void computeModelVisbs();
    void computeVisbs(Eigen::Vector3f &point, Eigen::Vector3f &normal, std::vector<bool> &visb);
    void computeVisbs(Eigen::Vector3f &point, Eigen::Vector3f &normal, Eigen::VectorXf &visb);
    void computeVisbs(int face_id, std::vector<bool> &visb);
    void computeVisbs(int face_id, Eigen::VectorXf &visb);
    void computeVerVisbs(int pt_id, Eigen::VectorXf &visb);
	void computeFaceNormal();
    void computeVertexNormal();
    void updateBSPtree();
    void exportPtRenderInfo(int pt_id);

    void passCameraPara(float c_modelview[16], float c_projection[16], int c_viewport[4]);
    void passRenderImgInfo(cv::Mat &zImg, cv::Mat &primitiveID, cv::Mat &rImg);
    void passVerticesVisbleStatus(std::vector<bool> &visble);
    bool getWorldCoord(Eigen::Vector3f rimg_coord, Eigen::Vector3f &w_coord);
    void getPtNormalInFace(Eigen::Vector3f &pt, int face_id, Eigen::Vector3f &normal);
    int getClosestVertexId(float world_pos[3], int x, int y);
    int getClosestVertexId(float world_pos[3]);
    void getProjRay(float proj_ray[3], int x, int y);
    void getCameraOri(float camera_ori[3]);
    inline void setRenderer(Viewer *viewer){ renderer = viewer; };
    inline Viewer* getRenderer() { return renderer; };

    inline ModelLight *getModelLightObj(){ return model_light; };
    inline std::vector<int> *getFaceAdj(int face_id){ return &model_faces_adj[face_id]; };
    inline Bound* getBounds(){ return model_bounds; };
    inline std::vector<unsigned int> *getFaceList(){ return &model_faces; };
    inline std::vector<float> *getVertexList(){ return &model_vertices; };
    inline std::vector<float> *getNormalList(){ return &model_normals; };
	inline std::vector<float> *getFaceNormalList(){ return &model_face_normals; };
    inline std::vector<float> *getColors() { return &model_colors; };
    inline std::vector<float> *getRhoList(){ return &model_rhos; };
    inline std::vector<std::vector<int>> *getVertexShareFaces(){ return &model_vertices_share_faces; };
    inline std::vector<int> *getVertexShareFaces(int vertex_id){ return &model_vertices_share_faces[vertex_id]; };
    inline std::vector<std::vector<int>> *getVertexAdj(){ return &model_vertex_adj; };
    inline std::vector<Eigen::Vector3i> *getFaceListCompact(){ return &model_faces_compact; };
    inline Eigen::Matrix<float, 4, 3> &getRhoSpclr(){ return rho_specular; };
    inline std::vector<float> &getTextCoord(){ return model_text_coord; };
    inline void setShadowOff(){shadow_on = false;};
    inline void setShadowOn(){shadow_on = true;};

    inline cv::Mat &getRImg(){ return r_img; };
    inline cv::Mat &getRBGRAImg() { return rBGRA_img; };
    inline cv::Mat &getPrimitiveIDImg(){ return primitive_ID; };
    inline cv::Mat &getZImg() { return z_img; };
    inline cv::Mat &getRMask() { return mask_rimg; };
    inline std::vector<float> &getRhoSpecular(){ return model_rhos; };
    inline std::vector<float> &getRhodIrr(){ return model_brightness; };

    inline std::string getDataPath(){ return data_path + std::to_string(ID); };
    inline void setOutputPath(std::string &path) { output_path = path; };
    inline std::string getOutputPath() { return output_path; };
    inline std::string getFileName() { return file_name; };

    inline Eigen::Matrix4f& getProjectInvMat() { return m_inv_modelview_projection; };

public:
	CvPoint3D32f modelCentroid;
	double modelRadius;

protected:
    bool loadOBJ(const std::string name, const std::string base_path);
    bool getUnprojectPt(float winx, float winy, float winz, float object_coord[3]);
    bool getProjectPt(float object_coord[3], float &winx, float &winy);
    void computeBaryCentreCoord(float pt[3], float v0[3], float v1[3], float v2[3], float lambd[3]);
    void buildFaceAdj();
    void buildVertexShareFaces();
    void buildVertexAdj();
    void buildFaceListCompact();
    bool shareEdge(int face0, int face1);
    void computeBounds();

protected:
    int ID;
    std::string data_path;
    std::string file_name;
    std::string output_path;

    // model basic attributes
    VectorF model_vertices;
    VectorVec3f model_vertices_compact;
    Facelist model_faces;
    VectorVec3i model_faces_compact;
    VectorF model_vertices_init;

    // color attributes
    Colorlist model_colors; // not use anymore, we use texture mapping
    Colorlist model_rhos; // rho specular
    Colorlist model_brightness; // rho d irradiance
    Eigen::Matrix<float, 4, 3> rho_specular;// it stores as BGR
    std::vector<float> model_text_coord;

    // normals
    Normallist model_normals;
    Normallist model_normals_init;
    Normallist model_face_normals;

    // auxiliary attributes
    FaceAdjlist model_faces_adj;// face adjacent list
    FaceAdjlist model_vertices_share_faces;// vertex one-ring faces
    VertexAdjlist model_vertex_adj;// vertex adjacent list
    
    Bound* model_bounds;

    // Lighting
    ModelLight *model_light;
    std::vector<std::vector<bool>> model_visbs;
	  Ray* ray_cast;
    kdtree::KDTree* model_kdTree;
    kdtree::KDTreeArray model_kdTree_data;
    bool shadow_on;

    // information from renderer
    cv::Mat z_img;
    cv::Mat primitive_ID;
    cv::Mat rBGRA_img;
    cv::Mat r_img;
    cv::Mat mask_rimg;
    std::vector<bool> v_vis_stat_in_r_img;
    Viewer *renderer;

    // information of camera
    Eigen::Matrix4f m_modelview;
    Eigen::Matrix4f m_projection;
    Eigen::Matrix4f m_inv_modelview_projection;
    Eigen::Vector4i m_viewport;

};

#endif