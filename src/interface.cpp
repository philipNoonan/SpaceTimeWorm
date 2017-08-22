#include "interface.h"
#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/registration.h>
#include <iostream>
//#include <libfreenect2/packet_pipeline.h>




void Freenect2Camera::start()
{
	if (m_status == STOPPED)
	{
		m_status = CAPTURING;
		m_thread = new std::thread(&Freenect2Camera::captureLoop, this);
	}
}

void Freenect2Camera::stop()
{
	if (m_status == CAPTURING)
	{
		m_status = STOPPED;

		if (m_thread->joinable())
		{
			m_thread->join();
		}

		m_thread = nullptr;
	}
}

void Freenect2Camera::frames(float * colorAray, float * bigDepthArray)
{
	//m_mtx.lock();

	//memcpy_S the arrays to the pointers passed in here

	memcpy_s(colorAray, 1920 * 1080 * 4, m_rawColor, 1920 * 1080 * 4);
	memcpy_s(bigDepthArray, 1920 * 1082 * 4, m_rawBigDepth, 1920 * 1082 * 4);



	//m_mtx.unlock();
	m_frames_ready = false;
}

//
//
//void Freenect2Camera::frames(cv::Mat &color, cv::Mat &depth, cv::Mat &infra, float &fullcolor)
//{
//	m_mtx.lock();
//
//	//cv::cvtColor(m_color_frame, color, CV_BGRA2BGR);
//
//	m_depth_frame.convertTo(depth, CV_16UC1);
//	m_infra_frame.convertTo(infra, CV_16UC1);
//
//	color = cv::Mat(1080, 1920, CV_8UC4, rawColor);
//
//	//fullcolor = **rawColor;
//
//	m_mtx.unlock();
//
//	//cv::Mat newColor = cv::Mat(1080, 1920, CV_8UC4, rawColor);
//	//cv::imshow("col", newColor);
//	//cv::waitKey(1);
//
//	m_frames_ready = false;
//}
//
void Freenect2Camera::frames(float * color, float * depth, float * infra)
{
	//m_mtx.lock();

	//cv::cvtColor(m_color_frame, color, CV_BGRA2BGR);
	//color = cv::Mat(1080, 1920, CV_8UC4, rawColor);

	//m_depth_frame.convertTo(depth, CV_16UC1);
	//m_infra_frame.convertTo(infra, CV_16UC1);

	//m_mtx.unlock();
	memcpy_s(color, m_frame_width * m_frame_height * 4, m_color_frame, m_frame_width * m_frame_height * 4);
	memcpy_s(depth, m_frame_width * m_frame_height * 4, m_depth_frame, m_frame_width * m_frame_height * 4);
	memcpy_s(infra, m_frame_width * m_frame_height * 4, m_infra_frame, m_frame_width * m_frame_height * 4);

	m_frames_ready = false;
}
//
//void Freenect2Camera::frames(cv::Mat &color, cv::Mat &depth)
//{
//	m_mtx.lock();
//
//	cv::cvtColor(m_color_frame, color, CV_BGRA2BGR);
//	m_depth_frame.convertTo(depth, CV_16UC1);
//
//	m_mtx.unlock();
//
//	m_frames_ready = false;
//}

void Freenect2Camera::captureLoop()
{
	m_frame_width = 512;
	m_frame_height = 424;

	m_rawColor = new float[1920 * 1080];
	m_rawBigDepth = new float[1920 * 1082];

	m_color_frame = new float[m_frame_width * m_frame_height];
	m_depth_frame = new float[m_frame_width * m_frame_height];
	m_infra_frame = new float[m_frame_width * m_frame_height];


	//cv::Size2i frame_size(m_frame_width, m_frame_height);

	libfreenect2::Freenect2 freenect2;
	libfreenect2::Freenect2Device *dev = 0;
	libfreenect2::PacketPipeline *pipeline = 0;

	//pipeline = new libfreenect2::CpuPacketPipeline();
	pipeline = new libfreenect2::OpenGLPacketPipeline();
	//pipeline = new libfreenect2::OpenCLPacketPipeline(0);
	//pipeline = new libfreenect2::CudaPacketPipeline(0);

	if (freenect2.enumerateDevices() == 0)
	{
		std::cout << "no device connected!" << std::endl;
		return;
	}

	std::string serial = freenect2.getDefaultDeviceSerialNumber();
	dev = freenect2.openDevice(serial, pipeline);

	if (dev == 0)
	{
		std::cout << "failure opening device!" << std::endl;
		return;
	}

	int types = libfreenect2::Frame::Color | libfreenect2::Frame::Ir | libfreenect2::Frame::Depth;
	libfreenect2::SyncMultiFrameListener listener(types);
	libfreenect2::FrameMap frames;

	dev->setColorFrameListener(&listener);
	dev->setIrAndDepthFrameListener(&listener);

	if (!dev->start())
		return;

	m_depth_fx = dev->getIrCameraParams().fx;
	m_depth_fy = dev->getIrCameraParams().fy;
	m_depth_ppx = dev->getIrCameraParams().cx;
	m_depth_ppy = dev->getIrCameraParams().cy;

	libfreenect2::Registration* registration = new libfreenect2::Registration(dev->getIrCameraParams(), dev->getColorCameraParams());
	libfreenect2::Frame undistorted(m_frame_width, m_frame_height, 4), registered(m_frame_width, m_frame_height, 4), bigDepth(1920, 1082, 4);
	int colorDepthIndex[512 * 424];


	while (m_status == CAPTURING)
	{
		if (!listener.waitForNewFrame(frames, 10 * 1000)) // 10 sconds
		{
			std::cout << "timeout!" << std::endl;
			return;
		}
		libfreenect2::Frame *rgb = frames[libfreenect2::Frame::Color];
		libfreenect2::Frame *ir = frames[libfreenect2::Frame::Ir];
		libfreenect2::Frame *depth = frames[libfreenect2::Frame::Depth];
		

		registration->apply(rgb, depth, &undistorted, &registered, true, &bigDepth, colorDepthIndex);
		//registration->apply(rgb, depth, &undistorted, &registered);

		//m_mtx.lock();
		//cv::Mat temp1 = cv::Mat(1082, 1920, CV_32F, bigDepth.data);
		//cv::Mat temp2;
		//temp1.convertTo(temp2, CV_16UC1);


		//cv::imshow("rgb1", 50 * temp2);
		//cv::waitKey(1);

		memcpy_s(m_rawColor, 1920 * 1080 * 4, rgb->data, 1920 * 1080 * 4);
		memcpy_s(m_rawBigDepth, 1920 * 1082 * 4, bigDepth.data, 1920 * 1082 * 4);

		memcpy_s(m_color_frame, m_frame_width * m_frame_height * 4, registered.data, m_frame_width * m_frame_height * 4);
		memcpy_s(m_depth_frame, m_frame_width * m_frame_height * 4, undistorted.data, m_frame_width * m_frame_height * 4);
		memcpy_s(m_infra_frame, m_frame_width * m_frame_height * 4, ir->data, m_frame_width * m_frame_height * 4);

		//m_mtx.unlock();

		m_frames_ready = true;

		listener.release(frames);




	}

	dev->stop();
	dev->close();

	delete registration;
	delete m_rawColor;
	delete m_rawBigDepth;

	delete m_color_frame;
	delete m_depth_frame;
	delete m_infra_frame;


}
