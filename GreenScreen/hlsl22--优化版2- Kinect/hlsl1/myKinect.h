#pragma once
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>  //ofstream���ͷ�ļ�
#include <iostream>
using namespace std;

//#include <conio.h>//��ⰴ���¼�
//#include <Windows.h>
//#include <Ole2.h>

#include <Kinect.h>


const int depthwidth = 512;
const int depthheight = 424;
const int colorwidth = 1920;
const int colorheight = 1080;

bool initKinect();
void getKinectData(unsigned char * colorFlow, unsigned short * depthFlow);