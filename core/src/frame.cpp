/**
 * @file frame.cpp
 * @brief Implementation of Frame Class functions 
 * @version 0.2
 * @date 10/02/2018
 * @author Victor Garcia
 */

#include "../include/frame.hpp"

using namespace std;
using namespace cv;

namespace m2d //!< mosaic 2d namespace
{

// See description in header file
Frame::Frame(Mat _img, bool _pre, int _width, int _height)
{

	bound_points = vector<vector<Point2f>>(3);
	grid_points = vector<vector<Point2f>>(2);
	good_points = vector<vector<Point2f>>(2);

	const float cx = 639.5, cy = 359.5;
	const float fx = 1101, fy = 1101;

	const float k1 = -0.359, k2 = 0.279, k3 = -0.16;
	const float p1 = 0, p2 = 0;

	Mat camera_matrix = (Mat1d(3, 3) << fx, 0, cx, 0, fy, cy, 0, 0, 1);
	Mat distortion_coeff = (Mat1d(1, 5) << k1, k2, p1, p2, k3);

	if (_img.size().width != _width || _img.size().height != _height)
		resize(_img, _img, Size(_width, _height));

	undistort(_img, color, camera_matrix, distortion_coeff);

	cvtColor(color, gray, CV_BGR2GRAY);
	if (_pre)
	{
		imgChannelStretch(gray, gray, 1, 99, Mat());
	}

	bound_rect = Rect2f(0, 0, (float)_width, (float)_height);
	// corner points
	bound_points[PERSPECTIVE].push_back(Point2f(0, 0));
	bound_points[PERSPECTIVE].push_back(Point2f(_width, 0));
	bound_points[PERSPECTIVE].push_back(Point2f(_width, _height));
	bound_points[PERSPECTIVE].push_back(Point2f(0, _height));
	// center point
	bound_points[PERSPECTIVE].push_back(Point2f(_width / 2, _height / 2));

	bound_points[EUCLIDEAN] = bound_points[PERSPECTIVE];

	H = Mat::eye(3, 3, CV_64F);
	E = Mat::eye(3, 3, CV_64F);
}

Frame::~Frame()
{
	H.release();
	E.release();
	gray.release();
	color.release();
	descriptors.release();
	grid_points.clear();
	good_points.clear();
	bound_points.clear();
	keypoints.clear();
	neighbors.clear();
}

Frame *Frame::clone()
{
	// CARE: must pass apply_pre instead of true
	Frame *new_frame = new Frame(color, true);

	new_frame->frame_error = frame_error;
	new_frame->descriptors = descriptors.clone();
	//new_frame->gray = gray.clone();
	new_frame->H = H.clone();
	new_frame->E = E.clone();
	new_frame->bound_rect = bound_rect;
	new_frame->bound_points = bound_points;
	new_frame->grid_points = grid_points;
	new_frame->good_points = good_points;	
	new_frame->keypoints = keypoints;
	new_frame->neighbors = neighbors;

	return new_frame;
}

// See description in header file
void Frame::resetFrame()
{

	H = Mat::eye(3, 3, CV_64F);

	bound_points[PERSPECTIVE][0] = Point2f(0, 0);
	bound_points[PERSPECTIVE][1] = Point2f(color.cols, 0);
	bound_points[PERSPECTIVE][2] = Point2f(color.cols, color.rows);
	bound_points[PERSPECTIVE][3] = Point2f(0, color.rows);

	bound_points[PERSPECTIVE][4] = Point2f(color.cols / 2, color.rows / 2);
	bound_rect = Rect2f(0, 0, (float)color.cols, (float)color.rows);

	grid_points[NEXT].clear();

	neighbors.clear();
}

// See description in header file
bool Frame::isGoodFrame()
{
	float deformation, area, keypoints_area;
	float semi_diag[4], ratio[2];

	for (int i = 0; i < 4; i++)
	{
		// 5th point correspond to center of image
		// Getting the distance between corner points to the center (all semi diagonal distances)
		semi_diag[i] = getDistance(bound_points[PERSPECTIVE][i], bound_points[PERSPECTIVE][4]);
	}
	// ratio beween semi diagonals
	ratio[0] = max(semi_diag[0] / semi_diag[2], semi_diag[2] / semi_diag[0]);
	ratio[1] = max(semi_diag[1] / semi_diag[3], semi_diag[3] / semi_diag[1]);

	// Area of distorted images
	area = contourArea(bound_points[PERSPECTIVE]);

	// enclosing area with good keypoints
	keypoints_area = boundAreaKeypoints();

	// 3 initial threshold value, must be ajusted in future tests
	if (area > 1.5 * color.cols * color.rows)
		return false;
	// 4 initial threshold value, must be ajusted in future tests
	if (ratio[0] > 1.6 || ratio[1] > 1.6)
		return false;
	if (keypoints_area < 0.2 * color.cols * color.rows)
		return false;

	return true;
}

// See description in header file
float Frame::boundAreaKeypoints()
{
	vector<Point2f> hull;

	convexHull(grid_points[PREV], hull);

	return contourArea(hull);
}

void Frame::setHReference(Mat _H, int _ref)
{
	perspectiveTransform(bound_points[_ref], bound_points[_ref], _H);
	if (_ref == PERSPECTIVE)
	{
		if (grid_points[PREV].size())
			perspectiveTransform(grid_points[PREV], grid_points[PREV], _H);
		if (grid_points[NEXT].size())
			perspectiveTransform(grid_points[NEXT], grid_points[NEXT], _H);

		updateBoundRect();
		H = _H * H;
	}

}

void Frame::updateBoundRect()
{
	float top = TARGET_HEIGHT, bottom = 0, left = TARGET_WIDTH, right = 0;

	for (Point2f point : bound_points[PERSPECTIVE])
	{
		if (point.x < left)
			left = point.x;
		if (point.y < top)
			top = point.y;
		if (point.x > right)
			right = point.x;
		if (point.y > bottom)
			bottom = point.y;
	}

	bound_rect.x = left;
	bound_rect.y = top;
	bound_rect.width = right - left;
	bound_rect.height = bottom - top;
}

bool Frame::haveKeypoints()
{
	return keypoints.size() > 0 ? true : false;
}
}