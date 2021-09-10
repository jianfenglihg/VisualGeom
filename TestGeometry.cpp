#include <random>
#include "TestGeometry.h"
#include <math.h>

void test::generateVisualData(int camNums, int worldPointsNum){
    double radius = 8;
    double PI = 3.1415926;
    double fx = 1.0;
    double fy = 1.0;
    double cx = 10.0;
    double cy = 10.0;
    Eigen::Matrix3d K;
    K << fx, 0, cx, 0, fy, cy, 0, 0, 1;

    for(int n=0; n<camNums; n++){
        double theta = n*2*PI/camNums;
        Eigen::Matrix3d R;
        R = Eigen::AngleAxisd(theta, Eigen::Vector3d::UnitZ());
        Eigen::Vector3d t = Eigen::Vector3d(radius*cos(theta)-radius, radius*sin(theta), 1*sin(2*theta));
        _cameras.push_back(Camera(R,t));
    }

    std::default_random_engine generator;
    std::uniform_real_distribution<double> xy_rand(-4.0,4.0);
    std::uniform_real_distribution<double> z_rand(8.0,10.0);
    std::uniform_real_distribution<double> noisy_rand(3e-3, 5e-3);

    for(int n=0; n<worldPointsNum; n++){
        double X = xy_rand(generator);
        double Y = xy_rand(generator);
        double Z = z_rand(generator);
        _world_points.push_back(Eigen::Vector3d(X,Y,Z));
    }

    for(int i=0;i<camNums;i++){
        Eigen::Matrix3d Rcw = _cameras[i].Rwc.transpose();
        Eigen::Vector3d tcw = -Rcw * _cameras[i].twc;
        _cameras[i]._K = K;
        for(int j=0;j<worldPointsNum;j++){
            Eigen::Vector3d camPoint = Rcw * _world_points[j] + tcw;
            camPoint = K * camPoint;
            double x = camPoint.x();
            double y = camPoint.y();
            double z = camPoint.z();
            _cameras[i].observed_points.push_back(Eigen::Vector3d(x/z,y/z,1.0));
        }
    }
    std::cout<<"generate visual data finished!"<<std::endl;
}


void test::testTriangulation(){
    std::cout<<"**********************test triangulation*********************"<<std::endl;
    std::cout<<"the ground-truth of world points:"<<std::endl;
    for(int i=0;i<_world_points.size();i++){
        std::cout<<_world_points[i].x()<<" "<<_world_points[i].y()<<" "<<_world_points[i].z()<<std::endl;
    }
    std::cout<<"\n"<<std::endl;
    std::vector<Eigen::Vector3d> triangualted_points;

    for(int i=0;i<_world_points.size();i++){
        unsigned long observed_dim = _cameras.size() * 2;
        MatXX A(MatXX::Zero(observed_dim,4));
        for(int j=0;j<_cameras.size();j++){
            Eigen::Vector3d camPointNorm = _cameras[j]._K.inverse() * _cameras[j].observed_points[i];
            Eigen::Matrix3d Rcw = _cameras[j].Rwc.transpose();
            Eigen::Vector3d tcw = -Rcw * _cameras[j].twc;
            MatXX P(MatXX::Zero(3,4));
            P.block(0,0,3,3) = Rcw;
            P.block(0,3,3,1) = tcw;
            /* A.block(2*j,0,1,4) = _cameras[j].observed_points[i].x() * P.block(2,0,1,4) - P.block(0,0,1,4); */
            A.block(2*j,0,1,4) = camPointNorm.x() * P.block(2,0,1,4) - P.block(0,0,1,4);
            /* A.block(2*j+1,0,1,4) = _cameras[j].observed_points[i].y() * P.block(2,0,1,4) - P.block(1,0,1,4); */
            A.block(2*j+1,0,1,4) = camPointNorm.y() * P.block(2,0,1,4) - P.block(1,0,1,4);
        }
        Eigen::JacobiSVD<Eigen::MatrixXd> svd(A.transpose()*A, Eigen::ComputeThinU | Eigen::ComputeThinV);
        Eigen::Vector4d u4 = svd.matrixU().col(3);
        if(u4(3)!=0){
            triangualted_points.push_back(Eigen::Vector3d(u4(0)/u4(3), u4(1)/u4(3), u4(2)/u4(3)));
        }
        else{
            triangualted_points.push_back(Eigen::Vector3d(0,0,0));
        }
    }
    std::cout<<"the estimated of world points:"<<std::endl;
    for(int i=0;i<triangualted_points.size();i++){
        std::cout<<triangualted_points[i].x()<<" "<<triangualted_points[i].y()<<" "<<triangualted_points[i].z()<<std::endl;
    }
    std::cout<<"******************triangulation end*************************\n\n\n"<<std::endl;
}


void test::testEightPointEpipolar(){
    std::cout<<"*************************test Eight Point Method: **********************"<<std::endl;
    for(unsigned long i=0;i<_cameras.size()-1;i++){
        double observed_dim = _world_points.size();
        MatXX A(MatXX::Zero(observed_dim,9));
        for(unsigned long j=0;j<_world_points.size();j++){
            Eigen::Vector3d norm_point1 = _cameras[i]._K.inverse() * _cameras[i].observed_points[j];
            Eigen::Vector3d norm_point2 = _cameras[i+1]._K.inverse() * _cameras[i+1].observed_points[j];
            double x1 = norm_point1.x();
            double y1 = norm_point1.y();
            double x2 = norm_point2.x();
            double y2 = norm_point2.y();
            A.row(j) << x1*x2, x1*y2, x1, y1*x2, y1*y2, y1, x2, y2, 1;
        }
        Eigen::JacobiSVD<Eigen::MatrixXd>
    }
}

void test::testPnP(){
    std::cout<<"*************************test pnp DLT solution: **********************"<<std::endl;
    for(unsigned long i=0; i<_cameras.size(); i++){
         std::cout<<"the camera "<<i<<" 's rotation matrix is Rcw:\n"<<std::endl;
         std::cout<<_cameras[i].Rwc.transpose()<<std::endl;
         std::cout<<"translation matrix tcw is:\n"<<std::endl;
         std::cout<<-_cameras[i].Rwc.transpose() * _cameras[i].twc<<"\n"<<std::endl;

         double observed_dim = _world_points.size() * 2;
         MatXX A(MatXX::Zero(observed_dim, 12));
         for(unsigned long j=0;j<_world_points.size();j++){
             Eigen::Vector3d norm_uv =_cameras[i]._K.inverse() * _cameras[i].observed_points[j];
             /* double uu = _cameras[i].observed_points[j].x(); */
             double uu = norm_uv.x();
             /* double vv = _cameras[i].observed_points[j].y(); */
             double vv = norm_uv.y();
             A.row(2*j) << _world_points[j].x(), _world_points[j].y(), _world_points[j].z(), 1,
                            0,0,0,0,
                           -uu * _world_points[j].x(), -uu * _world_points[j].y(), -uu * _world_points[j].z(), -uu;
             A.row(2*j+1) <<0,0,0,0,
                            _world_points[j].x(), _world_points[j].y(), _world_points[j].z(), 1,
                            -vv * _world_points[j].x(), -vv * _world_points[j].y(), -vv * _world_points[j].z(), -vv;
         }
         Eigen::JacobiSVD<Eigen::MatrixXd> svd(A.transpose() * A, Eigen::ComputeThinU | Eigen::ComputeThinV);
         Eigen::VectorXd u_last_col = svd.matrixU().col(11);
         Eigen::MatrixXd P(3,4);
         P << u_last_col(0), u_last_col(1), u_last_col(2), u_last_col(3),
              u_last_col(4), u_last_col(5), u_last_col(6), u_last_col(7),
              u_last_col(8), u_last_col(9), u_last_col(10), u_last_col(11);

         Eigen::Matrix3d R_estimate = P.block(0,0,3,3);
         Eigen::Vector3d t_estimate = P.col(3);

         Eigen::JacobiSVD<Eigen::MatrixXd> svd_R(R_estimate, Eigen::ComputeThinU | Eigen::ComputeThinV);
         Eigen::Matrix3d U = svd_R.matrixU();
         Eigen::Matrix3d V = svd_R.matrixV();
         Eigen::Vector3d singular = svd_R.singularValues();
         Eigen::Matrix3d R_best_estimated = U * V.transpose();

         double lambda = 3.0/singular.sum();
         lambda = lambda*(u_last_col(8)*_world_points[0].x()+u_last_col(9)*_world_points[0].y()+u_last_col(10)*_world_points[0].z()+u_last_col(11)) > 0 ? lambda : -lambda;
         R_best_estimated = lambda/abs(lambda) * R_best_estimated;
         Eigen::Vector3d t_best_estimated = lambda * t_estimate;

/*          Eigen::Matrix3d M = P.block(0,0,3,3); */
/*          Eigen::Vector3d MRC = P.col(3); */
/*          Eigen::JacobiSVD<Eigen::MatrixXd> svd_M(M, Eigen::ComputeThinU | Eigen::ComputeFullV); */
/*          Eigen::Matrix3d U = svd_M.matrixU(); */
/*          Eigen::Matrix3d V = svd_M.matrixV(); */
/*          Eigen::Matrix3d K_estimate = M * V * U.transpose(); */
/*          Eigen::Matrix3d R_estimate = U * V.transpose(); */
/*          Eigen::Vector3d t_estimate = -K_estimate.inverse() * MRC; */
/*          std::cout<<"the estimated K is :\n"<<K_estimate<<std::endl; */

         std::cout<<"the estimated R is :\n"<<R_best_estimated<<std::endl;
         std::cout<<"the estimated t is :\n"<<t_best_estimated<<std::endl;
         std::cout<<"------------------\n"<<std::endl;
         /* std::cout<<"validate estimated R:"<<R_estimate.transpose()*R_estimate<<std::endl; */
    }
}


void test::generateIMUData(){

}
