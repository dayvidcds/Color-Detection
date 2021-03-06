// ConsoleApplication4.cpp : define o ponto de entrada para o aplicativo do console.
//

#include "stdafx.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/opencv.hpp"
#include <iostream>

using namespace std;
using namespace cv;

#define NUM_PIXELS_AMOSTRA 20

struct MouseParams {
	Mat img;
	int x;
	int y;
	int r, g, b;
	Scalar RGB;
	Mat imgROI;
};

Scalar getColor(Mat ROI) {

	int r = 0, g = 0, b = 0;

	blur(ROI, ROI, Size(1, 1));

	for (int x = 0; x < ROI.cols; x++) {
		for (int y = 0; y < ROI.rows; y++) {
			r += (int)(ROI).at<Vec3b>(y, x)[0];
			g += (int)(ROI).at<Vec3b>(y, x)[1];
			b += (int)(ROI).at<Vec3b>(y, x)[2];
		}
	}

	const int const_multiplicativa = ROI.cols * ROI.rows;

	r = r / const_multiplicativa;
	g = g / const_multiplicativa;
	b = b / const_multiplicativa;

	return Scalar(r, g, b);

}

void getPixels(MouseParams &selection) {

	Mat imgROI;

	//cout << &selection << endl;

	Rect ROI(selection.x - NUM_PIXELS_AMOSTRA / 2, selection.y - NUM_PIXELS_AMOSTRA / 2, NUM_PIXELS_AMOSTRA, NUM_PIXELS_AMOSTRA);
	imgROI = selection.img(ROI);

	const int temp = 100 - NUM_PIXELS_AMOSTRA;

	resize(imgROI, imgROI, Size(imgROI.rows + temp, imgROI.cols + temp), CV_INTER_LINEAR);

	selection.imgROI = imgROI;

	//cout << "imgROI: " << &imgROI << endl;
	//cout << "selection: " << &selection.imgROI << endl;

	selection.RGB = getColor(imgROI);

}

void mouseEvent(int evt, int x, int y, int flags, void *param) {

	MouseParams *mp = (MouseParams*)param;

	mp->x = x;
	mp->y = y;

	if (evt == CV_EVENT_LBUTTONDOWN) {

		mp->r = (int)(mp->img).at<Vec3b>(y, x)[0];
		mp->g = (int)(mp->img).at<Vec3b>(y, x)[1];
		mp->b = (int)(mp->img).at<Vec3b>(y, x)[2];

		//cout << mp << endl;

		getPixels(*mp);

		/*cout <<	"Posicao: (" << x << "," << y << ")" << endl << "cor: (" <<
		mp->r << ", "<<
		mp->g << ", "<<
		mp->b << ")" << endl;
		*/
	}

}

void transform(Mat img, Scalar color) {

	Mat imgThresholded, imgHSV;
	const int thresh = 40;
	RNG rng(12345);

	//Scalar min = Scalar(color[2] - thresh, color[1] - thresh, color[0] - thresh);
	//Scalar max = Scalar(color[2] + thresh, color[1] + thresh, color[0] + thresh);

	Scalar min = Scalar(color[2], color[1], color[0]) * 0.8f;
	Scalar max = Scalar(color[2], color[1], color[0]) * 1.5f;
	
	//int min = ((color[0] + color[1] + color[2]) - thresh) / 3;
	//int max = ((color[0] + color[1] + color[2]) + thresh) / 3;

	blur(img, img, Size(1, 1));
	cvtColor(img, imgHSV, COLOR_BGR2HSV);

	//threshold(imgHSV, imgHSV, 50, 255, THRESH_BINARY);

	inRange(imgHSV, Scalar(19, 84, 58), Scalar(75, 255, 133), imgThresholded);

	//cout << "COR:       " << color * 0.8 << endl;
	//cout << "COR_MODIF: " << color * 1.3 << endl;

	// remover pequenos objetos do primeiro plano
	erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));


	dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

	//cvtColor(imgThresholded, imgThresholded, COLOR_BGR2GRAY);

	//Canny(imgThresholded, imgThresholded, 50, 100, 3, true);

	vector <vector<Point>> contornos;
	vector <Vec4i> hierarquia;

	findContours(imgThresholded, contornos, hierarquia, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

	Mat saida = Mat::zeros(imgThresholded.size(), CV_8UC3);
	vector <vector<Point>> vertices(contornos.size());
	vector <Rect> retangulos(contornos.size());

	for (size_t i = 0; i < contornos.size(); i++) {
		String texto;
		approxPolyDP(Mat(contornos[i]), vertices[i], 3, true); // Aproximação poligonal
		retangulos[i] = boundingRect(Mat(vertices[i])); // Caixa de contorno para contorno
		Scalar cor = Scalar(0, 255, 0); // Gerando cor
		drawContours(img, vertices, (int)i, cor, 2, LINE_8, hierarquia, 0, Point());
		//putText(img, "Bolinha", Point(retangulos[i].tl().x, retangulos[i].tl().y), FONT_HERSHEY_PLAIN, 1.5, color, 1, LINE_AA);
		//circle(img, Point(retangulos[i].tl().x, (retangulos[i].tl().y + retangulos[i].height) / 2), 4, cor, -1, 8, 0);
	}
	

	imshow("Imagem modificada", imgThresholded);

}

int main(int argc, char** argv) {

	Mat img;
	VideoCapture cap(0);
	MouseParams mp;

	mp.x = 1;
	mp.y = 1;

	cap.read(mp.imgROI);

	resize(mp.imgROI, mp.imgROI, Size(100, 100), CV_INTER_LINEAR);

	if (!cap.isOpened()) {
		cout << "Camera nao disponivel" << endl;
		return -1;
	}

	while (1) {

		bool t = cap.read(img);

		if (!t) {
			cout << "Erro de leitura" << endl;
			break;
		}

		namedWindow("Imagem", 1);

		mp.img = img;

		setMouseCallback("Imagem", mouseEvent, &mp);

		mp.imgROI.copyTo(mp.img(Rect(5, 1, 100, 100)));

		rectangle(mp.img, Rect(130, 1, 100, 100), mp.RGB, -1, CV_AA);	
		rectangle(mp.img, Rect(250, 1, 100, 100), Scalar(mp.r, mp.g, mp.b), -1, CV_AA);
		rectangle(mp.img, Rect(mp.x - 15, mp.y - 15, 30, 30), Scalar(0, 255, 0));

		putText(mp.img, "Cor desejada", Point(2, 118), FONT_HERSHEY_PLAIN, 1, Scalar(0, 255, 0), 1, LINE_AA);
		putText(mp.img, "Cor obtida", Point(132, 118), FONT_HERSHEY_PLAIN, 1, Scalar(0, 255, 0), 1, LINE_AA);
		putText(mp.img, "Sem media", Point(252, 118), FONT_HERSHEY_PLAIN, 1, Scalar(0, 255, 0), 1, LINE_AA);

		transform(mp.img, mp.RGB);

		imshow("Imagem", mp.img);

		if (waitKey(30) == 27) {
			break;
		}

	}

	return 0;
}
