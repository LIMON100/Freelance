#include "lut.hpp"
static int idx2D(int fIdx, int zIdx, int zCount){ return fIdx*zCount + zIdx; }
void initLUTFromGuess(const InitialGuess&, const LUTGrid& g, LUTParam& lut){
    lut.zCount=g.zoom_bins.size(); lut.fCount=g.focus_bins.size(); int N=lut.zCount*lut.fCount;
    lut.fx.assign(N,1000.0); lut.fy.assign(N,1000.0); lut.cx.assign(N,640.0); lut.cy.assign(N,360.0);
    lut.k1.assign(N,0.0); lut.k2.assign(N,0.0); lut.p1.assign(N,0.0); lut.p2.assign(N,0.0); lut.k3.assign(N,0.0);
}
void bilinearLUT(const LUTGrid& g, const LUTParam& lut, double z, double f, double& fx,double& fy,double& cx,double& cy){
    auto clamp=[](double v,double lo,double hi){return v<lo?lo:(v>hi?hi:v);};
    z = clamp(z, g.zoom_bins.front(), g.zoom_bins.back());
    f = clamp(f, g.focus_bins.front(), g.focus_bins.back());
    int zi=0,zj=g.zoom_bins.size()-1; for(int i=0;i<(int)g.zoom_bins.size()-1;++i) if(z>=g.zoom_bins[i]&&z<=g.zoom_bins[i+1]){zi=i;zj=i+1;break;}
    int fi=0,fj=g.focus_bins.size()-1; for(int i=0;i<(int)g.focus_bins.size()-1;++i) if(f>=g.focus_bins[i]&&f<=g.focus_bins[i+1]){fi=i;fj=i+1;break;}
    double z0=g.zoom_bins[zi], z1=g.zoom_bins[zj], f0=g.focus_bins[fi], f1=g.focus_bins[fj];
    double tz=(z1==z0)?0:(z-z0)/(z1-z0), tf=(f1==f0)?0:(f-f0)/(f1-f0);
    auto at=[&](const std::vector<double>& A,int F,int Z){return A[idx2D(F,Z,lut.zCount)];};
    auto bil=[&](const std::vector<double>& A){double v00=at(A,fi,zi), v01=at(A,fi,zj), v10=at(A,fj,zi), v11=at(A,fj,zj);
        double v0=v00*(1-tz)+v01*tz, v1=v10*(1-tz)+v11*tz; return v0*(1-tf)+v1*tf;};
    fx=bil(lut.fx); fy=bil(lut.fy); cx=bil(lut.cx); cy=bil(lut.cy);
}