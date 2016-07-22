#ifndef ROIWINDOW_H
#define ROIWINDOW_H

namespace qm{
	//roi window 1d
	class RoiWindow_1d
	{
	public:
		//RoiWindow_1d(QObject *parent);
		~RoiWindow_1d();
		// window boundaries
		int m_x0, m_x1;
		// ROI boundaries
		int m_roi_x0, m_roi_x1;

		RoiWindow_1d(){}
		RoiWindow_1d(int i_x0, int i_x1, int i_roi_x0, int i_roi_x1);
		void set(int i_x0, int i_x1, int i_roi_x0, int i_roi_x1);
	};



	// a window with a region of interest in two dimensions
	class RoiWindow_2d
	{

	public:
		// along x
		int m_x0, m_x1;
		int m_roi_x0, m_roi_x1;
		// along y
		int m_y0, m_y1;
		int m_roi_y0, m_roi_y1;

		RoiWindow_2d(){}
		void set_along_x(const RoiWindow_1d &window1d);
		void set_along_y(const RoiWindow_1d &window1d);
	};
}

#endif // ROIWINDOW_H
