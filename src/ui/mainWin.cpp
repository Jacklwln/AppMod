#include "Viewer.h"
#include "mainWin.h"
#include "Coarse.h"
#include "GroundTruth.h"
#include "ModelLight.h"
#include "I2SAlgorithms.h"
#include "ProjOptimize.h"
#include <QGLViewer/manipulatedFrame.h>
#include "../Image-2-Shape/src/CameraPose/CameraPose.h"
#include "../Image-2-Shape/src/CameraPose/FocalLength.h"

MainWin::MainWin()
{
    setupUi(this); 

    //viewer->makeCurrent();
    //viewer->setParent(centralwidget);
    //viewer_img->context()->create(viewer->context());

    //viewer = new Viewer(centralwidget);
    //viewer->setObjectName(QStringLiteral("viewer"));

    //viewer_img = new Viewer(centralwidget);
    //viewer_img->setObjectName(QStringLiteral("viewer_img"));
    ////viewer->setMinimumSize(QSize(800, 600));
    ////viewer->setMaximumSize(QSize(800, 600));

    //main_grid_layout = new QGridLayout(centralwidget);
    //setObjectName(QStringLiteral("main_grid_layout"));

    //main_grid_layout->addWidget(viewer, 0, 0, 1, 1);
    //main_grid_layout->addWidget(viewer_img, 0, 1, 1, 1);

    connect(action_Load_Model, SIGNAL(triggered()), this, SLOT(loadModel()));
    connect(action_Load_LightingBall, SIGNAL(triggered()), this, SLOT(loadLightingBall()));
    connect(action_Snap_Shot, SIGNAL(triggered()), this, SLOT(snapShot()));
    connect(action_Load_S2I_Transform, SIGNAL(triggered()), this, SLOT(loadS2ITransform()));
    connect(action_Fix_Camera, SIGNAL(triggered()), this, SLOT(fixCamera()));
    connect(action_Init_Light, SIGNAL(triggered()), this, SLOT(initLight()));
    connect(action_Check_Visible, SIGNAL(triggered()), this, SLOT(checkVisibleVertices()));
    connect(action_Reset_Screen, SIGNAL(triggered()), this, SLOT(resetScreen()));
    //connect(action_Update_Light, SIGNAL(triggered()), this, SLOT(updateLight()));
    connect(action_Compute_Normal, SIGNAL(triggered()), this, SLOT(computeNormal()));
    connect(action_Update_Geometry, SIGNAL(triggered()), this, SLOT(updateGeometry()));
	  connect(action_Export_OBJ, SIGNAL(triggered()), this, SLOT(exportOBJ()));
    connect(action_Render, SIGNAL(triggered()), this, SLOT(renderTexture()));
    connect(m_pushButton_set_para, SIGNAL(clicked()), this, SLOT(setOptParatoModel()));
    connect(m_pushButton_Run, SIGNAL(clicked()), this, SLOT(runAll()));
    connect(action_Compute_All, SIGNAL(triggered()), this, SLOT(computeAll()));

	connect(action_Clear_Selected_Points, SIGNAL(triggered()), this, SLOT(clearSelectedPoints()));
	connect(action_Compute_3, SIGNAL(triggered()), this, SLOT(computeCameraPose()));
	connect(action_Load_2D_3D_points, SIGNAL(triggered()), this, SLOT(loadPoints()));
	connect(action_Line_Mode_On_Off, SIGNAL(triggered()), this, SLOT(lineMode()));
	connect(action_Compute_Focal_Length, SIGNAL(triggered()), this, SLOT(computeFocalLength()));
	connect(action_Compute_Camera_Pose, SIGNAL(triggered()), this, SLOT(getPose()));


    this->show();

    coarse_model = nullptr;
    gt_model = nullptr;
    lighting_ball = nullptr;
    img_part_alg = new ImagePartAlg;
    img_part_alg_thread = new QThread;
    feature_guided = new FeatureGuided;

    connect(this, SIGNAL(callComputeInitLight(Coarse *, Viewer *)), img_part_alg, SLOT(computeInitLight(Coarse *, Viewer *)));
    connect(this, SIGNAL(callUpdateLight(Coarse *, Viewer *)), img_part_alg, SLOT(updateLight(Coarse *, Viewer *)));
    connect(this, SIGNAL(callComputeNormal(Coarse *, Viewer *)), img_part_alg, SLOT(computeNormal(Coarse *, Viewer *)));
    connect(this, SIGNAL(callRunWholeIter(Coarse *, Viewer *)), img_part_alg, SLOT(runWholeIter(Coarse *, Viewer *)));

    connect(this, SIGNAL(callComputeBRDFLightNormal(Coarse *, Viewer *)), img_part_alg, SLOT(solveRenderEqAll(Coarse *, Viewer *)));
    connect(img_part_alg, SIGNAL(refreshScreen()), this, SLOT(refreshScreen()));

    img_part_alg->moveToThread(img_part_alg_thread);

    img_part_alg_thread->start();

	//pointsSelect = false;
	//isCompute = false;
}

MainWin::~MainWin()
{
    delete img_part_alg;
    delete img_part_alg_thread;
    delete feature_guided;
}

void MainWin::loadModel()
{
    QString filter;
    filter = "obj file (*.obj)";

    QDir dir;
    QString fileName = QFileDialog::getOpenFileName(this, QString(tr("Open Obj File")), dir.absolutePath(), filter);
    if (fileName.isEmpty() == true) return;


    std::string model_file_path = fileName.toStdString();
    std::string model_file_name = model_file_path.substr(model_file_path.find_last_of('/') + 1);
    model_file_path = model_file_path.substr(0, model_file_path.find_last_of('/'));
    int model_id = atoi(model_file_path.substr(model_file_path.find_last_of('/') + 1).c_str());
    model_file_path = model_file_path.substr(0, model_file_path.find_last_of('/') + 1);

    if (coarse_model != nullptr) 
        delete coarse_model;
    coarse_model = new Coarse(model_id, model_file_path, model_file_name);

    viewer->getModel(coarse_model);
    viewer->resetCamera(coarse_model);
    coarse_model->setRenderer(viewer);
    viewer->setBackGroundImage(QString::fromStdString(
      model_file_path + std::to_string(model_id) + "/photo.png"));

	otherViewer->getModel(coarse_model);
	otherViewer->resetCamera(coarse_model);
	otherViewer->setBackGroundImage(QString::fromStdString(
      model_file_path + std::to_string(model_id) + "/photo.png"));

	viewer->viewer2 = otherViewer;

    // make an output dir
    char time_postfix[50];
    time_t current_time = time(NULL);
    strftime(time_postfix, sizeof(time_postfix), "_%Y%m%d-%H%M%S", localtime(&current_time));
    std::string outptu_file_path = coarse_model->getDataPath() + "/output" + time_postfix;
    dir.mkpath(QString(outptu_file_path.c_str()));
    coarse_model->setOutputPath(outptu_file_path);

    setOptParatoModel();

    this->snapShot();

    //coarse_model->exportPtRenderInfo(233);
    //coarse_model->drawFaceNormal();

    //if (gt_model == nullptr)
    //{
    //    gt_model = new Groundtruth(coarse_model);
    //    viewer_img->getModel(gt_model);
    //    gt_model->setRenderer(viewer_img);
    //}
    //coarse_model->setGtModelPtr(gt_model);
}

void MainWin::loadLightingBall()
{
    //QString filter;
    //filter = "obj file (*.obj)";

    //QDir dir;
    //QString fileName = QFileDialog::getOpenFileName(this, QString(tr("Open Obj File")), dir.absolutePath(), filter);
    //if (fileName.isEmpty() == true) return;


    //std::string model_file_path = fileName.toStdString();
    //std::string model_file_name = model_file_path.substr(model_file_path.find_last_of('/') + 1);
    //model_file_path = model_file_path.substr(0, model_file_path.find_last_of('/'));
    //int model_id = atoi(model_file_path.substr(model_file_path.find_last_of('/') + 1).c_str());
    //model_file_path = model_file_path.substr(0, model_file_path.find_last_of('/') + 1);

    //if (lighting_ball != nullptr) 
    //    delete lighting_ball;
    //lighting_ball = new Model(model_id, model_file_path, model_file_name);

    ////viewer->getModel(coarse_model);
    //lighting_ball->setRenderer(viewer_img);

    //lighting_ball->getModelLightObj()->getOutsideLight() = coarse_model->getLightRec();
    //lighting_ball->getModelLightObj()->getOutsideSampleMatrix() = coarse_model->getModelLightObj()->getSampleMatrix();

    //lighting_ball->computeBrightness();
    //viewer_img->getModel(lighting_ball);

}

void MainWin::exportOBJ()
{
	coarse_model->exportOBJ();

    //coarse_model->setInit();
}

void MainWin::snapShot()
{
    if (coarse_model)
        viewer->getSnapShot(coarse_model, true);

}

void MainWin::loadS2ITransform()
{
    if (coarse_model)
        coarse_model->loadS2ITransform();
}

void MainWin::fixCamera()
{
    viewer->fixCamera();
}

void MainWin::initLight()
{
    if (coarse_model)
    {
        emit callComputeInitLight(coarse_model, viewer);

        // update coarse_model's rho and color
        // pass new color to viewer to display

    }
}

void MainWin::checkVisibleVertices()
{
    viewer->checkVisibleVertices(coarse_model);
}

void MainWin::resetScreen()
{
	//emit callNLoptTest();

    viewer->resetScreen();
    viewer->getModel(coarse_model);
    viewer->setShowModel(true);
}

void MainWin::updateLight()
{
    //if (coarse_model)
    //{
    //    emit callUpdateLight(coarse_model, viewer);

    //    // update coarse_model's rho and color
    //    // pass new color to viewer to display

    //}

	//Eigen::VectorXf v_test(coarse_model->getModelLightObj()->getNumSamples());
	//viewer->checkVertexVisbs(0, coarse_model, v_test);

	//std::ofstream f_v(coarse_model->getDataPath() + "/first_visb_test.mat");
	//if (f_v)
	//{
	//	f_v << v_test;

	//	f_v.close();
	//}

	coarse_model->updateVertexRho();
    renderTexture();
}

void MainWin::computeNormal()
{
    if (coarse_model)
    {
        emit callComputeNormal(coarse_model, viewer);
    }
}

void MainWin::updateGeometry()
{
    if (coarse_model)
    {
        GeometryPartAlg geoAlg;
        geoAlg.updateWithExNormal(coarse_model);

        coarse_model->computeLight();
        viewer->getModel(coarse_model);
        this->refreshScreen();
    }
}

void MainWin::refreshScreen()
{
    viewer->UpdateGLOutside();
}

void MainWin::setOptParatoModel()
{
    int int_paras[3];
    
    int_paras[0] = m_spinBox_iter_num->value();
    int_paras[1] = m_spinBox_deform_iter->value();
    int_paras[2] = m_spinBox_cluster_num->value();


    double double_paras[13];

    double_paras[0] = m_spinBox_BRDF_Light_sfs->value();
    double_paras[1] = m_spinBox_Light_Reg->value();
    double_paras[2] = m_spinBox_cluster_smooth->value();
    double_paras[3] = m_spinBox_norm_sfs->value();
    double_paras[4] = m_spinBox_norm_smooth->value();
    double_paras[5] = m_spinBox_norm_normalized->value();
    double_paras[6] = m_spinBox_k_strech->value();
    double_paras[7] = m_spinBox_k_bend->value();
    double_paras[8] = m_spinBox_deform_normal->value();
    double_paras[9] = m_spinBox_vertical_move->value();
    double_paras[10] = m_spinBox_rho_smooth->value();
    double_paras[11] = m_spinBox_rho_s_r->value();
    double_paras[12] = m_spinBox_norm_prior->value();

    if (coarse_model)
        coarse_model->getParaObjPtr()->setOptParameter(3, int_paras, 13, double_paras);
}

void MainWin::runAll()
{
    if (coarse_model)
    {
        emit callRunWholeIter(coarse_model, viewer);
    }
}

void MainWin::renderTexture()
{
    viewer->resetScreen();
    viewer->getModelWithTexture(coarse_model, coarse_model->getRhoImg());
    viewer->setShowModel(true);
    viewer->UpdateGLOutside();

    viewer->setSnapshotFormat("PNG");

    char time_postfix[50];
    time_t current_time = time(NULL);
    strftime(time_postfix, sizeof(time_postfix), "_%Y%m%d-%H%M%S", localtime(&current_time));
    std::string file_time_postfix = time_postfix;

    viewer->saveSnapshot(QString((coarse_model->getOutputPath()+"/ren_img" + file_time_postfix + ".png").c_str()));
}

void MainWin::computeAll()
{
    //if (coarse_model)
    //{
    //    emit callComputeBRDFLightNormal(coarse_model, viewer);
    //}

    // test tele-reg

  //tele2d *teleRegister = new tele2d( 50, 0.02,1 ) ;

  //CURVES curves ;
  //std::vector<std::vector<int>> group ;
  //std::vector<int2> endps ;

	
  //teleRegister->load_Curves( "curves.txt", curves, group, endps );


  //teleRegister->init( curves, group, endps  ) ;


  ////// Uncomment the 3 lines below, you can directly run the registration and save the result.
  ////teleRegister->runRegister() ;
  ////teleRegister->outputResCurves( "rescurves.txt") ;
  ////return 0;



  //teleRegister->setInputField() ; // only for visualization

  QString filter;
  filter = "image file (*.png)";

  QDir dir;
  QString fileName = QFileDialog::getOpenFileName(this, QString(tr("Open Obj File")), dir.absolutePath(), filter);
  if (fileName.isEmpty() == true) return;


  std::string fileSource = fileName.toStdString();
  std::string fileTarget = fileSource.substr(0, fileSource.find_last_of('/') + 1) + "featureP.png";
  feature_guided->initImages(fileSource, fileTarget);
  feature_guided->initRegister();
  feature_guided->initVisualization(viewer_img);

  ProjOptimize proj_opt;
  proj_opt.updateShape(feature_guided, coarse_model);
}

void MainWin::computeCameraPose()
{
	coarse_model->getSelectedPoints();
	if(coarse_model->imgpts->size() != viewer->objpts.size())
		cout << "The number of selected image points is not equal to the number of corresponding 3D points." << endl;
	else if(coarse_model->imgpts->size() != 6 || viewer->objpts.size() != 6)
		cout << "The number of selected points is not equal to 6." << endl;
	else
	{
		CameraPose cp;
		cp.getCameraPose(*(coarse_model->imgpts),viewer->objpts);

	//	//GLdouble mvm[16];
	//  //Eigen::Matrix4d camera_pose;
	//	//camera_pose << cp.rotation.at<double>(0,0), cp.rotation.at<double>(0,1), cp.rotation.at<double>(0,2), cp.translation.at<double>(0,0),
	//	//	           cp.rotation.at<double>(1,0), cp.rotation.at<double>(1,1), cp.rotation.at<double>(1,2), cp.translation.at<double>(1,0),
	//	//			   cp.rotation.at<double>(2,0), cp.rotation.at<double>(2,1), cp.rotation.at<double>(2,2), cp.translation.at<double>(2,0),
	//	//			   0, 0, 0, 1;
	//	//Eigen::Matrix4d camera_pose_inv = camera_pose.inverse();

	//	//mvm[0] = camera_pose_inv(0, 0);
	//	//mvm[1] = camera_pose_inv(0, 1);
	//	//mvm[2] = camera_pose_inv(0, 2);
	//	//mvm[3] = camera_pose_inv(0, 3);
	//	//mvm[4] = camera_pose_inv(1, 0);
	//	//mvm[5] = camera_pose_inv(1, 1);
	//	//mvm[6] = camera_pose_inv(1, 2);
	//	//mvm[7] = camera_pose_inv(1, 3);
	//	//mvm[8] = camera_pose_inv(2, 0);
	//	//mvm[9] = camera_pose_inv(2, 1);
	//	//mvm[10] = camera_pose_inv(2, 2);
	//	//mvm[11] = camera_pose_inv(2, 3);
	//	//mvm[12] = camera_pose_inv(3, 0);
	//	//mvm[13] = camera_pose_inv(3, 1);
	//	//mvm[14] = camera_pose_inv(3, 2);
	//	//mvm[15] = camera_pose_inv(3, 3);
	//	//viewer->camera()->setFromModelViewMatrix(mvm);
	//	/*mvm[0] = 0;
	//	mvm[1] = 0;
	//	mvm[2] = -1;
	//	mvm[3] = 10;
	//	mvm[4] = 0;
	//	mvm[5] = 1;
	//	mvm[6] = 0;
	//	mvm[7] = 0;
	//	mvm[8] = 1;
	//	mvm[9] = 0;
	//	mvm[10] = 0;
	//	mvm[11] = 0;
	//	mvm[12] = 0;
	//	mvm[13] = 0;
	//	mvm[14] = 0;
	//	mvm[15] = 1;
	//	viewer->camera()->setFromModelViewMatrix(mvm);*/
	//	/*GLint viewport[4];
	//	viewer->camera()->getViewport(viewport);
	//	Mat vp = Mat_<double>::zeros(4,1);
	//	vp.at<double>(0,0) = viewport[0];
	//	vp.at<double>(1,0) = viewport[1];
	//	vp.at<double>(2,0) = viewport[2];
	//	vp.at<double>(3,0) = viewport[3];
	//	std::cout << " The viewport is " << vp << std::endl;*/
	//	
	//	qreal pm[12];
	//	pm[0] = cp.projectionMatrix.at<double>(0,0);
	//	pm[1] = cp.projectionMatrix.at<double>(0,1);
	//	pm[2] = cp.projectionMatrix.at<double>(0,2);
	//	pm[3] = cp.projectionMatrix.at<double>(0,3);
	//	pm[4] = cp.projectionMatrix.at<double>(1,0);
	//	pm[5] = cp.projectionMatrix.at<double>(1,1);
	//	pm[6] = cp.projectionMatrix.at<double>(1,2);
	//	pm[7] = cp.projectionMatrix.at<double>(1,3);
	//	pm[8] = cp.projectionMatrix.at<double>(2,0);
	//	pm[9] = cp.projectionMatrix.at<double>(2,1);
	//	pm[10] = cp.projectionMatrix.at<double>(2,2);
	//	pm[11] = cp.projectionMatrix.at<double>(2,3);

	//	viewer->camera()->setFromProjectionMatrix(pm);


	//	//viewer->camera()->position();
	//	//std::cout<<"camera orie: "<<viewer->camera()->orientation()<<"\n";
	//	//std::cout<<"clipping: " << viewer->camera()->zNear()<<"\t"<<viewer->camera()->zFar()<<"\n";

		cout << "The rotation matrix is " << endl << cp.rotation << endl;
		cout << "The translation vector is " << endl << cp.translation << endl;
		cout << "The projection Matrix is " << endl << cp.projectionMatrix << endl;
	}
}

void MainWin::clearSelectedPoints()
{
	std::cout << "Clear all selected points." << endl;
	coarse_model->getSelectedPoints();
	while(!(coarse_model->imgpts->empty()))
		coarse_model->imgpts->pop_back();
	while(!(viewer->objpts.empty()))
		viewer->objpts.pop_back();
}

void MainWin::loadPoints()
{
	while(!(viewer->pts2d.empty()))
		viewer->pts2d.pop_back();
	while(!(viewer->pts3d.empty()))
		viewer->pts3d.pop_back();
	coarse_model->getSelectedPoints();
	for(std::vector<CvPoint2D32f>::iterator it = coarse_model->imgpts->begin();it != coarse_model->imgpts->end();it ++)
		viewer->pts2d.push_back(*it);
	for(std::vector<CvPoint3D32f>::iterator it = viewer->objpts.begin();it != viewer->objpts.end();it ++)
		viewer->pts3d.push_back(*it);

  ProjOptimize proj_opt;
  proj_opt.updateShape(feature_guided, coarse_model);
}

void MainWin::lineMode()
{
	if(select_mode == 1)
		select_mode = 2;
	else
		select_mode = 1;
	std::cout << "Points select mode is changed." << std::endl;
}

void MainWin::computeFocalLength()
{
	coarse_model->getSelectedPoints();
	if(coarse_model->imgpts->size() != 8)
		cout << "The number of selected image points is not equal to 8." << endl;
	else
	{
		fl.computeFocalLength(*(coarse_model->imgpts));
		std::cout << "The Focal Length is " << fl.focalLength << std::endl;
		GLfloat m[16];
		std::cout << "Current Projection matrix is: \n";
		viewer->camera()->getProjectionMatrix(m);
		for (int i = 0; i < 16; i++)
		{
			std::cout << m[i] << "\t";
		}
		std::cout << "\n";
		viewer->camera()->setSceneRadius(fl.focalLength);
		viewer->camera()->setFieldOfView(2 * atan(coarse_model->photo_height / 2 / fl.focalLength));
		viewer->camera()->setFlySpeed(0.5);
		std::cout << "Current Projection matrix is: \n";
		viewer->camera()->getProjectionMatrix(m);
		for (int i = 0; i < 16; i++)
		{
			std::cout << m[i] << "\t";
		}
		std::cout << "\n";
		//viewer->camera()->setPosition(qglviewer::Vec(0,0,-fl.focalLength));
		//viewer->camera()->setViewDirection(qglviewer::Vec(0,0,-1));
		//viewer->camera()->setUpVector(qglviewer::Vec(0,1,0));
		//viewer->camera()->setZClippingCoefficient(1.7);
		//viewer->camera()->setZNearCoefficient(1e-4);
		//viewer->camera()->setSceneCenter(qglviewer::Vec(0,0,0));
		//viewer->camera()->setSceneRadius(fl.focalLength);
	}
	
}

void MainWin::getPose()
{
	if(fl.focalLength == 0)
		std::cout << "The focal length has not been computed!" << std::endl;
	else
	{
		coarse_model->getSelectedPoints();
		Mat cameraMatrix = Mat_<double>::zeros(3,3);
		vector<double> distCoeffs;
		Mat rvec,tvec;
		cameraMatrix.at<double>(0,0) = fl.focalLength;
		cameraMatrix.at<double>(0,2) = (coarse_model->photo_width) / 2;
		cameraMatrix.at<double>(1,1) = fl.focalLength;
		cameraMatrix.at<double>(1,2) = (coarse_model->photo_height) / 2;
		cameraMatrix.at<double>(2,2) = 1;
		
		vector<Point3f> inputObjpts;
		vector<Point2f> inputImgpts;
		for(int i = 0;i < viewer->objpts.size();i ++)
			inputObjpts.push_back(Point3f((viewer->objpts)[i].x,(viewer->objpts)[i].y,(viewer->objpts)[i].z));
		for(int j = 0;j < coarse_model->imgpts->size();j ++)
			inputImgpts.push_back(Point2f((*(coarse_model->imgpts))[j].x,(*(coarse_model->imgpts))[j].y));
		bool status = solvePnP(inputObjpts,inputImgpts,
			cameraMatrix,distCoeffs,rvec,tvec,false,CV_ITERATIVE);
		Mat rmat;
		Rodrigues(rvec,rmat);
		std::cout << "The rotation matrix is " << rmat << std::endl;
		std::cout << "The translation vector is " << tvec << std::endl;
		std::cout << "The default focal length is " << viewer->camera()->focusDistance() << std::endl;

		Eigen::Matrix4d modelView;
		modelView << rmat.at<double>(0,0),rmat.at<double>(0,1),rmat.at<double>(0,2),tvec.at<double>(0,0),
			rmat.at<double>(1,0),rmat.at<double>(1,1),rmat.at<double>(1,2),tvec.at<double>(1,0),
			rmat.at<double>(2,0),rmat.at<double>(2,1),rmat.at<double>(2,2),tvec.at<double>(2,0),
			0,0,0,1;
		Eigen::Matrix4d inv_modeView = modelView.inverse();
		GLdouble mvm[16];
		mvm[0] = modelView(0,0);
		mvm[1] = modelView(0,1);
		mvm[2] = modelView(0,2);
		mvm[3] = modelView(0,3);
		mvm[4] = modelView(1,0);
		mvm[5] = modelView(1,1);
		mvm[6] = modelView(1,2);
		mvm[7] = modelView(1,3);
		mvm[8] = modelView(2,0);
		mvm[9] = modelView(2,1);
		mvm[10] = modelView(2,2);
		mvm[11] = modelView(2,3);
		mvm[12] = modelView(3,0);
		mvm[13] = modelView(3,1);
		mvm[14] = modelView(3,2);
		mvm[15] = modelView(3,3);
		/*mvm[0] = inv_modeView(0,0);
		mvm[1] = inv_modeView(0,1);
		mvm[2] = inv_modeView(0,2);
		mvm[3] = inv_modeView(0,3);
		mvm[4] = inv_modeView(1,0);
		mvm[5] = inv_modeView(1,1);
		mvm[6] = inv_modeView(1,2);
		mvm[7] = inv_modeView(1,3);
		mvm[8] = inv_modeView(2,0);
		mvm[9] = inv_modeView(2,1);
		mvm[10] = inv_modeView(2,2);
		mvm[11] = inv_modeView(2,3);
		mvm[12] = inv_modeView(3,0);
		mvm[13] = inv_modeView(3,1);
		mvm[14] = inv_modeView(3,2);
		mvm[15] = inv_modeView(3,3);*/
		//viewer->camera()->setFromModelViewMatrix(mvm);
		viewer->camera()->setFocusDistance(fl.focalLength);
		viewer->camera()->setPosition(qglviewer::Vec(0,0,-fl.focalLength));
		viewer->camera()->setViewDirection(qglviewer::Vec(0,0,-1));
		viewer->camera()->setUpVector(qglviewer::Vec(0,1,0));
		viewer->camera()->setZClippingCoefficient(1.7);
		viewer->camera()->setZNearCoefficient(0.0);
		viewer->camera()->setSceneCenter(qglviewer::Vec(0,0,0));
		viewer->camera()->setSceneRadius(fl.focalLength);
 	}
}