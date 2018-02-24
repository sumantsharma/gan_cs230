// OS_OPTICALSTIMULATOR.CPP
// ------------------------------------------------------------------------
// DESCRIPTION: generates synthetic scenes of the space environment 
// ------------------------------------------------------------------------
// AUTHOR: Connor Beierle
//         2016-06-21 CRB  Created
//         2016-06-24 CRB  Added LoadConfigFromMAT(), directory currently hardcoded
//         2016-06-24 CRB  Added SimulateStarImage(q) and its dependencies
//         2017-03-21 CRB  Fixed SimulateNSO to properly superimpose SO and NSO
//         2017-03-24 CRB  Made proper interface to handle arbitrary base directories
//         2018-01-30 CRB: major overhaul
// ------------------------------------------------------------------------
// COPYRIGHT: 2016 SLAB Group
//            OS Function
// ------------------------------------------------------------------------

#include "mex.h"
#include "mat.h"
#include "os_opticalstimulator.hpp"

using namespace std;

OpticalStimulator::OpticalStimulator(int Nu, int Nv, double ppx, double ppy, double fx, double fy,
                                     double magThresh, double halfFOV, Matrix R_vbs2os) : 
    m_Nu(Nu),
    m_Nv(Nv),
    m_ppx(ppx),
    m_ppy(ppy),
    m_fx(fx),
    m_fy(fy),
    m_hsc(magThresh, halfFOV),
    m_R_vbs2os(R_vbs2os),
    m_gl(Nu, Nv, ppx, ppy, fx, fy)
{
}

int OpticalStimulator::getNu()                 { return m_Nu;       };
int OpticalStimulator::getNv()                 { return m_Nv;       };
double OpticalStimulator::getPpx()             { return m_ppx;      };
double OpticalStimulator::getPpy()             { return m_ppy;      };
double OpticalStimulator::getFx()              { return m_fx;       };
double OpticalStimulator::getFy()              { return m_fy;       };
const Hipparcos& OpticalStimulator::getHSC()   { return m_hsc;      };
const Matrix& OpticalStimulator::getR_vbs2os() { return m_R_vbs2os; };

Matrix OpticalStimulator::Pixel2UnitVector(Matrix& uv, int dist){
    
    int N = uv.nRows();
    Matrix xyz(N,3);
    double x, y, z, norm;
    double cx = m_gl.m_camera.Nu / 2.0;
    double cy = m_gl.m_camera.Nv / 2.0;
    
    for(int i=0;i<N;i++){
        switch(dist){
            case 0:
                x = (uv(i,0) - cx) / m_gl.m_camera.fx;
                y = (uv(i,1) - cy) / m_gl.m_camera.fy;
                z = 1.0;
                norm = pow(x*x + y*y + z*z, 0.5);
                xyz(i,0) = x/norm;
                xyz(i,1) = y/norm;
                xyz(i,2) = z/norm;
                break;
            default:
                printf("Pixel2UnitVector: Error\n");
        }
    }
    
    return xyz;
}
            
Matrix OpticalStimulator::UnitVector2Pixel(Matrix& xyz, int dist){
    int N = xyz.nRows();
    Vector xn_pow(5), yn_pow(5);
    double xn, yn;
    Matrix uv(N,2);
    
    double cx = m_gl.m_camera.Nu / 2.0;
    double cy = m_gl.m_camera.Nv / 2.0;
    double fpx = m_gl.m_camera.fx / m_gl.m_camera.dx;
    double fpy = m_gl.m_camera.fy / m_gl.m_camera.dy;
    
    //"STAR TRACKER REAL-TIME HARDWARE IN THE LOOP TESTING USING OPTICAL STAR SIMULATOR" (AAS 11-260)
    Matrix AB(1,25);
    int col;
    
    for(int i=0;i<N;i++){
        xn = xyz(i,0) / xyz(i,2);                                          // small angle approximation xn = x/z
        yn = xyz(i,1) / xyz(i,2);                                          // small angle approximation yn = y/z
        switch(dist){
            case 0:
                uv(i,0) = fpx*xn + cx;
                uv(i,1) = fpy*yn + cy;
                break;
            case 2:
                for(int power=0;power<5;power++){
                    xn_pow(power) = pow(xn,power);
                    yn_pow(power) = pow(yn,power);
                }
                
                for(int k=0;k<5;k++){
                    for(int j=0;j<5;j++){
                        col = 5*k + j;
                        //AB(0,col) = pow(xn,k) * pow(yn,j);
                        AB(0,col) = xn_pow(k) * yn_pow(j);
                    }
                }
                uv(i,0) = (double) ((AB*m_C)(0,0));
                uv(i,1) = (double) ((AB*m_D)(0,0));
                break;
            default:
                printf("UnitVector2Pixel: Error\n");              
        }
    }
   
    return uv;
}

Vector OpticalStimulator::Magnitude2RGB(double mag)
{
    // placeholder - linear approx for digital count (dc)
    double x1 = 4;                    // visual magnitude (bright)
    double x2 = 9;                    // visual magnitude (dim)
    double y1 = 1;                    // digital count (bright)
    double y2 = 0;                    // digital count (dim)
    double a = (y2-y1)/(x2-x1);       // slope
    double b = y1 - a*x1;             // vertical intercept

    // interpolate
    double DC = a*mag + b;            // interpolated digital count
    Vector rgb(DC,DC,DC);
    return rgb;
}

void OpticalStimulator::DrawSO(const Vector& q_eci2vbs)
{
    // render stars
    Matrix R_eci2vbs = Quaternion2Rotation(q_eci2vbs);
    Vector z_eci = R_eci2vbs.Row(2);                  // camera boresight vector expressed in (ECI) frame
    std::vector<SO> stars = m_hsc.StarsInFOV(z_eci);
    for(auto so : stars){
        Vector n_vbs = R_eci2vbs * so.v_eci;          // unit vector to SO expressed in (VBS) frame
        // TODO: insert warping here
        Vector rgb = Magnitude2RGB(so.mag);
        m_gl.DrawRGBStar(n_vbs, rgb);
    }
}

void OpticalStimulator::RenderQuat(const Vector& q_eci2vbs){
    m_gl.ClearScreen();
    DrawSO(q_eci2vbs);
    m_gl.SwapBuffers();
}

void OpticalStimulator::FarRangeAON(Matrix& so_vbs, Matrix& so_rgb, Matrix& nso_vbs, Matrix& nso_rgb){

    // reset screen
    m_gl.ClearScreen();

    // render SO
    for(int i=0; i < so_vbs.nRows(); i++)
        m_gl.DrawRGBStar(so_vbs.Row(i), so_rgb.Row(i));
    
    // render NSO
    for(int i=0; i < nso_vbs.nRows(); i++)
        m_gl.DrawRGBStar(nso_vbs.Row(i), nso_rgb.Row(i));
        
    // swap buffers
    m_gl.SwapBuffers();
}

void OpticalStimulator::RenderTango(const S3& s3){
    
    // reset screen
    m_gl.ClearScreen();
    
    // Update Sun
    m_gl.m_sun.r_vbs = s3.r_Vo2So_vbs;
    m_gl.m_sun.on = true;
        
    // Draw TANGO
    m_gl.m_tango.r_vbs = s3.r_Vo2To_vbs;
    m_gl.m_tango.q_vbs2body = s3.q_vbs2tango;
    m_gl.DrawCAD(m_gl.m_tango);
    
    // Draw TANGO triad
    m_gl.m_triad.r_vbs = m_gl.m_tango.r_vbs;
    m_gl.m_triad.q_vbs2body = m_gl.m_tango.q_vbs2body;
    m_gl.DrawCAD(m_gl.m_triad);
    
    // Draw Earth
    m_gl.m_earth.r_vbs = s3.r_Vo2Eo_vbs;
    m_gl.m_earth.q_vbs2body = s3.q_vbs2ecef;
    m_gl.DrawCAD(m_gl.m_earth);

    // render SO
    DrawSO(s3.q_eci2vbs);
        
    // Swap Buffers
    m_gl.SwapBuffers();
}