#include "Preprocess.h"
#include "Parameter.h"
#include "dirent.h"
#include "time.h"
#include <fstream>

using namespace cv;

Preprocess::Preprocess() {
	_nx = 9;
	_ny = 6;
}
Mat Preprocess::read_resize(string name, int scale) {
	Mat src = imread(name);
	resize(src, src, Size(src.cols / scale, src.rows / scale));
	return src;
}
Mat Preprocess::read_undistort_resize(string name, int scale) {
	Mat src = imread(name);
	//imwrite("org.jpg", src);
	Mat clb = calibration(src);
	//imwrite("test1_2.jpg", clb);
	resize(clb, clb, Size(src.cols / scale, src.rows / scale));
	return clb;
}
int Preprocess::preprocess() {
	
	string filename1 = "intrinsic";
	string filename2 = "distCoeffs";
	ifstream ifile1((filename1 + ".yml").c_str());
	ifstream ifile2((filename2 + ".yml").c_str());
	if ((bool)ifile1 && (bool)ifile2) {
		FileStorage storage1(filename1 + ".yml", FileStorage::READ);
		FileStorage storage2(filename2 + ".yml", FileStorage::READ);
		storage1[filename1] >> _intrinsic;
		storage1.release();
		storage2[filename2] >> _distCoeffs;
		storage2.release();
		return 1;
	}

	Size patternSize(_nx, _ny);
	vector<Point3f> obj;
	vector<Point2f> corners;
	char name[100];

	for (int i = 0; i < _ny; i++) {
		for (int j = 0; j < _nx; j++) {
			obj.push_back(Point3f(i, j, 0.0f));
		}
	}

	// prepare object and image points

	string inputDirectory = "camera_cal";
	DIR *directory = opendir(inputDirectory.c_str());
	struct dirent *dirent = NULL;
	if (directory == NULL) {
		cout << "Cannot open Input Folder" << endl;
		return -1;
	}
	while ((dirent = readdir(directory)) != NULL) {
		string fileName = inputDirectory + "\\" + string(dirent->d_name);
		Mat rawImage = imread(fileName.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
		if (rawImage.data == NULL) {
			cout << "Cannot Open Image" << endl;
			continue;
		}
		bool patternfound = findChessboardCorners(rawImage, patternSize, corners);
		if (patternfound) {
			cornerSubPix(rawImage, corners, Size(11, 11), Size(-1, -1), TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 30, 0.1));
			drawChessboardCorners(rawImage, patternSize, Mat(corners), patternfound);
			//imshow("find corner",cur);
			//waitKey();
			_image_points.push_back(corners);
			_object_points.push_back(obj);
		}
	}
	closedir(directory);

	// prepare intrinsic and distCoeffs matrix

	_intrinsic = Mat(3, 3, CV_32FC1);
	vector<Mat> rvecs;
	vector<Mat> tvecs;
	_intrinsic.ptr<float>(0)[0] = 1;
	_intrinsic.ptr<float>(1)[1] = 1;
	calibrateCamera(_object_points, _image_points, Size(IMG_COL_SIZE, IMG_ROW_SIZE), _intrinsic, _distCoeffs, rvecs, tvecs);

	FileStorage storage1(filename1 + ".yml", FileStorage::WRITE);
	FileStorage storage2(filename2 + ".yml", FileStorage::WRITE);
	storage1 << filename1 << _intrinsic;
	storage1.release();
	storage2 << filename2 << _distCoeffs;
	storage2.release();

	return 0;
}
Mat Preprocess::calibration(Mat img) {
	Mat processedImg;
	undistort(img, processedImg, _intrinsic, _distCoeffs);
	//imshow("Original", img);
	//imshow("Processed", processedImg);

	return processedImg;
}