#pragma once

#include <thread>
#include <mutex>
//#include "opencv2/opencv.hpp"

class Freenect2Camera
{
	enum Status
	{
		CAPTURING,
		STOPPED
	};

public:
	Freenect2Camera()
		: m_color_frame()
		, m_depth_frame()
		, m_infra_frame()
		, m_rawColor()
		, m_rawBigDepth()
		, m_frame_width(0)
		, m_frame_height(0)
		, m_depth_fx(0)
		, m_depth_fy(0)
		, m_depth_ppx(0)
		, m_depth_ppy(0)
		, m_frames_ready(false)
		, m_thread(nullptr)
		, m_mtx()
		, m_status(STOPPED)
	{
	}

	~Freenect2Camera()
	{
		if (m_status == CAPTURING)
		{
			stop();
		}
	}

	void start();

	void stop();

	bool ready()
	{
		return m_frames_ready;
	}

	void frames(float * colorArray, float * bigDepthArray);

	void frames(float * colorArray, float * depthArray, float * infraredArray);

	//void frames(cv::Mat &color, cv::Mat &depth, cv::Mat &infra, float & fullColor);

	//void frames(cv::Mat &color, cv::Mat &depth, cv::Mat &infra);

	//void frames(cv::Mat &color, cv::Mat &depth);

	int width()
	{
		return m_frame_width;
	}

	int height()
	{
		return m_frame_height;
	}

	float fx()
	{
		return m_depth_fx;
	}

	float fy()
	{
		return m_depth_fy;
	}

	float ppx()
	{
		return m_depth_ppx;
	}

	float ppy()
	{
		return m_depth_ppy;
	}

private:
	void captureLoop();

	float* m_color_frame;
	float* m_depth_frame;
	float* m_infra_frame;

	float *m_rawColor;
	float *m_rawBigDepth;

	int m_frame_width;
	int m_frame_height;

	float m_depth_fx;
	float m_depth_fy;
	float m_depth_ppx;
	float m_depth_ppy;

	Status m_status;
	bool m_frames_ready;
	std::thread *m_thread;
	std::mutex m_mtx;
};
