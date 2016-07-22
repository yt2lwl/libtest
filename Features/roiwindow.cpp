#include "roiwindow.h"

#include <QString>
namespace qm{


RoiWindow_1d::~RoiWindow_1d()
{

}

RoiWindow_1d::RoiWindow_1d(int i_x0, int i_x1, int i_roi_x0, int i_roi_x1) :
m_x0(i_x0), m_x1(i_x1),
m_roi_x0(i_roi_x0), m_roi_x1(i_roi_x1){
}

void RoiWindow_1d::set(int i_x0, int i_x1, int i_roi_x0, int i_roi_x1){
	m_x0 = i_x0; m_x1 = i_x1;
	m_roi_x0 = i_roi_x0; m_roi_x1 = i_roi_x1;
}

void RoiWindow_2d::set_along_x(const RoiWindow_1d &window1d){
	memcpy(&m_x0, &window1d.m_x0, 4 * sizeof(int));
}

void RoiWindow_2d::set_along_y(const RoiWindow_1d &window1d){
	memcpy(&m_y0, &window1d.m_x0, 4 * sizeof(int));
}
}

