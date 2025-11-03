#pragma once
#include <vector>
#include "calib_data_io.hpp"
struct LUTGrid { std::vector<double> zoom_bins, focus_bins; };
struct LUTParam { std::vector<double> fx,fy,cx,cy,k1,k2,p1,p2,k3; int zCount=0,fCount=0; };
void initLUTFromGuess(const InitialGuess& guess, const LUTGrid& grid, LUTParam& lut);
void bilinearLUT(const LUTGrid& grid, const LUTParam& lut, double zoom, double focus, double& fx,double& fy,double& cx,double& cy);