#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>

using namespace cv;

void detectAndDraw(Mat& img, CascadeClassifier& cascade,
	CascadeClassifier& nestedCascade,
	double scale, bool tryflip);

int main()
{
	int sockSrv;
	socklen_t client_len;
	int  n;
	char buffer[256];
	const int blocksize = 7200;
//	const int blocksize = 57600;
	struct recvbuf
	{
		char buf[blocksize];
		int flag;
	};
	struct recvbuf data;

/*建立socket,采用UDP协议*/
	if((sockSrv = socket(AF_INET, SOCK_DGRAM, 0))<0)
	{
	perror ("socket");
        exit(1);
	}
	else{printf("sock successful!\n");}
/*创建UDP套接口*/

/*创建服务端和客户端*/
	struct sockaddr_in addrSrv;
	struct sockaddr_in addrClient;
/*初始化服务器端地址*/
	memset(addrSrv.sin_zero, 0x00, 8);
	memset(&addrSrv,0,sizeof(struct sockaddr_in));
	addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(34567);

	client_len = sizeof(struct sockaddr_in);

/* 绑定套接口 */
  	if(bind(sockSrv,(struct sockaddr*)&addrSrv,sizeof(struct sockaddr_in))<0)
  	{
    		perror("Server Bind Failed:");
    		exit(1);
  	}
	else{printf("bind successful!\nwaiting for connect...\n");}

	int nRecvBuf = 1024 * 1024 * 10;//10M
	setsockopt(sockSrv, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));

	CascadeClassifier cascade, nestedCascade;
	//训练好的文件名称，放置在可执行文件同目录下
	cascade.load("../cascade/hogcascade_pedestrians.xml");
	nestedCascade.load("../cascade/haarcascade_frontalface_alt.xml");
	Mat mFrame;
	IplImage* pFrame = NULL, *frame_copy = 0;
	n = recvfrom(sockSrv, buffer, sizeof(buffer), 0, (struct sockaddr *)&addrClient, &client_len);
		if(n>0)
		{
			printf("Client:%s %u connected: %s\n", inet_ntoa(addrClient.sin_addr), ntohs(addrClient.sin_port), buffer);
			CvCapture* pCapture = cvCreateCameraCapture(0);
			CvSize size = cvSize(320, 240);
			cvSetCaptureProperty(pCapture, CV_CAP_PROP_FRAME_WIDTH , size.width);
      			cvSetCaptureProperty(pCapture, CV_CAP_PROP_FRAME_HEIGHT , size.height);
			while(1)
			{
				pFrame = cvQueryFrame(pCapture);
				if (!pFrame)break;
				mFrame = pFrame;
				detectAndDraw(mFrame, cascade, nestedCascade, 2, 0);
				char* img = pFrame->imageData;
				for (int i = 0; i<32; i++)                    //640*480*3= 921600 320*240 = 76800
				{
					for (int k = 0; k<blocksize; k++)
					{
						data.buf[k] = img[i*blocksize + k];
					}
					if (i == 31)
					{
						data.flag = 2;
					}
					else
					{
						data.flag = 1;
					}
					n = sendto(sockSrv, (char*)(&data), sizeof(data), 0, (struct sockaddr*)&addrClient, client_len);
				}
				cvReleaseImage(&frame_copy);
				sleep(0.05);
			}
			cvReleaseCapture(&pCapture);
		}
	close(sockSrv);
}


void detectAndDraw(Mat& img, CascadeClassifier& cascade,
	CascadeClassifier& nestedCascade,
	double scale, bool tryflip)
{
	int i = 0;
	double t = 0;
	//建立用于存放人脸的向量容器
	vector<Rect> faces, faces2;
	//定义一些颜色，用来标示不同的人脸
	const static Scalar colors[] = { CV_RGB(0, 0, 255),
		CV_RGB(0, 128, 255),
		CV_RGB(0, 255, 255),
		CV_RGB(0, 255, 0),
		CV_RGB(255, 128, 0),
		CV_RGB(255, 255, 0),
		CV_RGB(255, 0, 0),
		CV_RGB(255, 0, 255) };
	//建立缩小的图片，加快检测速度
	//nt cvRound (double value) 对一个double型的数进行四舍五入，并返回一个整型数！
	Mat gray, smallImg(cvRound(img.rows / scale), cvRound(img.cols / scale), CV_8UC1);
	//转成灰度图像，Harr特征基于灰度图
	cvtColor(img, gray, CV_BGR2GRAY);
	//改变图像大小，使用双线性差值
	resize(gray, smallImg, smallImg.size(), 0, 0, INTER_LINEAR);
	//变换后的图像进行直方图均值化处理
	equalizeHist(smallImg, smallImg);

	//程序开始和结束插入此函数获取时间，经过计算求得算法执行时间
	t = (double)cvGetTickCount();
	//检测人脸
	//detectMultiScale函数中smallImg表示的是要检测的输入图像为smallImg，faces表示检测到的人脸目标序列，1.1表示
	//每次图像尺寸减小的比例为1.1，2表示每一个目标至少要被检测到3次才算是真的目标(因为周围的像素和不同的窗口大
	//小都可以检测到人脸),CV_HAAR_SCALE_IMAGE表示不是缩放分类器来检测，而是缩放图像，Size(30, 30)为目标的
	//最小最大尺寸
	cascade.detectMultiScale(smallImg, faces,
		1.1, 2, 0
		//|CV_HAAR_FIND_BIGGEST_OBJECT
		//|CV_HAAR_DO_ROUGH_SEARCH
		| CV_HAAR_SCALE_IMAGE
		,
		Size(30, 30));
	//如果使能，翻转图像继续检测
	if (tryflip)
	{
		flip(smallImg, smallImg, 1);
		cascade.detectMultiScale(smallImg, faces2,
			1.1, 2, 0
			//|CV_HAAR_FIND_BIGGEST_OBJECT
			//|CV_HAAR_DO_ROUGH_SEARCH
			| CV_HAAR_SCALE_IMAGE
			,
			Size(30, 30));
		for (vector<Rect>::const_iterator r = faces2.begin(); r != faces2.end(); r++)
		{
			faces.push_back(Rect(smallImg.cols - r->x - r->width, r->y, r->width, r->height));
		}
	}
	t = (double)cvGetTickCount() - t;
	//   qDebug( "detection time = %g ms\n", t/((double)cvGetTickFrequency()*1000.) );
	for (vector<Rect>::const_iterator r = faces.begin(); r != faces.end(); r++, i++)
	{
		Mat smallImgROI;
		vector<Rect> nestedObjects;
		Point center;
		Scalar color = colors[i % 8];
		int radius;

		double aspect_ratio = (double)r->width / r->height;
		if (0.75 < aspect_ratio && aspect_ratio < 1.3)
		{
			//标示人脸时在缩小之前的图像上标示，所以这里根据缩放比例换算回去
			center.x = cvRound((r->x + r->width*0.5)*scale);
			center.y = cvRound((r->y + r->height*0.5)*scale);
			radius = cvRound((r->width + r->height)*0.25*scale);
			circle(img, center, radius, color, 3, 8, 0);
		}
		else
			rectangle(img, cvPoint(cvRound(r->x*scale), cvRound(r->y*scale)),
			cvPoint(cvRound((r->x + r->width - 1)*scale), cvRound((r->y + r->height - 1)*scale)),
			color, 3, 8, 0);
		if (nestedCascade.empty())
			continue;
		smallImgROI = smallImg(*r);
		//同样方法检测人眼
		nestedCascade.detectMultiScale(smallImgROI, nestedObjects,
			1.1, 2, 0
			//|CV_HAAR_FIND_BIGGEST_OBJECT
			//|CV_HAAR_DO_ROUGH_SEARCH
			//|CV_HAAR_DO_CANNY_PRUNING
			| CV_HAAR_SCALE_IMAGE
			,
			Size(30, 30));
		for (vector<Rect>::const_iterator nr = nestedObjects.begin(); nr != nestedObjects.end(); nr++)
		{
			center.x = cvRound((r->x + nr->x + nr->width*0.5)*scale);
			center.y = cvRound((r->y + nr->y + nr->height*0.5)*scale);
			radius = cvRound((nr->width + nr->height)*0.25*scale);
			circle(img, center, radius, color, 3, 8, 0);
		}
	}
}

