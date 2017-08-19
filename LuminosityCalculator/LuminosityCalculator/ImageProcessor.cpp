#include "ImageProcessor.h"
#include "RAII.hpp"
#include <opencv2/opencv.hpp>
#include <chrono>
#include <algorithm>
#include <numeric>


namespace detail
{
	const int BEST_QUALITY = 100;
	struct VideoCapturer
	{
		VideoCapture operator()(const path& videoFilePath)
		{
			return VideoCapture(videoFilePath.string());
		}
	};

	struct VideoReleaser
	{
		void operator()(VideoCapture& cap)
		{
			cap.release(); 
		}
	};

	struct NoLocker
	{
		void operator()(const std::recursive_timed_mutex*) const
		{
			//do nothing, raii's constructor can't return
		}
	};

	struct Unlocker
	{
		void operator()(std::recursive_timed_mutex* timedMutex)
		{
			if (timedMutex)
				timedMutex->unlock();
		}
	};

	uchar CalcLuminosity(uchar b, uchar g, uchar r)
	{
		return static_cast<uchar>( 0.0722 * b + 0.7152 * g  + 0.2126 * r);
	}
}

ImageProcessor::ImageProcessor(const path& _videoFilePath)
	: videoFilePath_(_videoFilePath)
{
}

ImageProcessor::ImageProcessor(const ImageProcessor& other)
	: avgVideoLuminosity(other.avgVideoLuminosity)
	, videoFilePath_(other.videoFilePath_)
	, frames_(other.frames_)
	, avgFrameLuminosity_(other.avgFrameLuminosity_)
	, luminosities_(other.luminosities_)
{

}

ImageProcessor& ImageProcessor::operator=(ImageProcessor & other)
{
	std::lock_guard<std::recursive_timed_mutex> guard(mutex_);
	std::swap(avgVideoLuminosity, other.avgVideoLuminosity);
	videoFilePath_.swap(other.videoFilePath_);
	frames_.swap(other.frames_);
	avgFrameLuminosity_.swap(other.avgFrameLuminosity_);
	luminosities_.swap(other.luminosities_);
	return *this;
}

ImageProcessor::ImageProcessor(ImageProcessor&& other)
	: avgVideoLuminosity(std::move(other.avgVideoLuminosity))
	, videoFilePath_(std::move(other.videoFilePath_))
	, frames_(std::move(other.frames_))
	, avgFrameLuminosity_(std::move(other.avgFrameLuminosity_))
	, luminosities_(std::move(other.luminosities_))
{
}

ImageProcessor & ImageProcessor::operator=(ImageProcessor && other)
{
	std::lock_guard<std::recursive_timed_mutex> guard(mutex_);
	avgVideoLuminosity = std::move(other.avgVideoLuminosity);
	videoFilePath_=std::move(other.videoFilePath_);
	frames_=std::move(other.frames_);
	avgFrameLuminosity_=std::move(other.avgFrameLuminosity_);
	luminosities_=std::move(other.luminosities_);
	return *this;
}

const path& ImageProcessor::File() const
{
	std::lock_guard<std::recursive_timed_mutex> guard(mutex_);
	return videoFilePath_;
}

path ImageProcessor::File() 
{
	std::lock_guard<std::recursive_timed_mutex> guard(mutex_);
	return videoFilePath_;
}

const vector<Mat>& ImageProcessor::Frames() const
{
	std::lock_guard<std::recursive_timed_mutex> guard(mutex_);
	return frames_;
}

vector<Mat>& ImageProcessor::Frames() 
{
	std::lock_guard<std::recursive_timed_mutex> guard(mutex_);
	return frames_;
}

void ImageProcessor::ConvertVideoToFrames()
{
	try 
	{
		RAII<::detail::VideoCapturer, ::detail::VideoReleaser, VideoCapture, std::string> 
			raii(videoFilePath_.string().c_str());
		auto cap= raii.get();

		if (!cap.isOpened())  // check if we succeeded
		{
			throw std::runtime_error("Can not open Video file");
		}

		//cap.get(CV_CAP_PROP_FRAME_COUNT) contains the number of frames_ in the video;
		for (int frameNum = 0; frameNum < cap.get(CV_CAP_PROP_FRAME_COUNT); ++frameNum)
		{
			Mat frame;
			cap >> frame; // get the next frame from video

			if(mutex_.try_lock_for(std::chrono::microseconds(1000)))
			{
			RAII<::detail::NoLocker, ::detail::Unlocker, std::recursive_timed_mutex*> raiiMutex(&mutex_);
				frames_.emplace_back(std::move(frame));
			}
		}
	}
	catch (cv::Exception& e) 
	{
		throw std::runtime_error(e.what());
	}
}


void ImageProcessor::SaveFrames(const string& outputDir) 
{
	vector<int> compression_params;
	compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
	compression_params.push_back(::detail::BEST_QUALITY);


	std::lock_guard<std::recursive_timed_mutex> guard(mutex_);
	int frameNumber = 0;
	for (std::vector<Mat>::iterator frame = frames_.begin(); frame != frames_.end(); ++frame, ++frameNumber) 
	{
		string filePath = outputDir + to_string(static_cast<long long>(frameNumber)) + ".jpg";
		imwrite(filePath, *frame, compression_params);
	}
}

void ImageProcessor::CalculateLuminosityUsingHSV()
{
	std::lock_guard<std::recursive_timed_mutex> guard(mutex_);
	int frameNumber = 0;
	for (std::vector<Mat>::iterator frame = frames_.begin(); frame != frames_.end(); ++frame, ++frameNumber) 
	{
		if (frame->rows > 1 && frame->cols > 1)
		{
			Mat hsvImg;
			cvtColor(*frame, hsvImg, CV_BGR2HSV);

			Mat channels[3];
			cv::split(hsvImg, channels);
			uchar* start = channels[2].data;
			assert(frame->rows == channels[2].rows);
			assert(frame->cols == channels[2].cols);
			size_t size = channels[2].rows* channels[2].cols;
			vector<uchar> luminosity((frame->rows)*(frame->cols));

			for (size_t index = 0; index < size; ++index)
			{
				luminosity[index] = *start;
				++start;
			}
			luminosities_.emplace_back(luminosity);
		}
	}
}

void ImageProcessor::CalculateLuminosityUsingBGR()
{
	std::lock_guard<std::recursive_timed_mutex> guard(mutex_);
	int frameNumber = 0;
	for (std::vector<Mat>::iterator frame = frames_.begin(); frame != frames_.end(); ++frame, ++frameNumber) 
	{
		if (frame->rows > 1 && frame->cols > 1)
		{
			size_t size = frame->rows* frame->cols;
			vector<uchar> luminosity((frame->rows)*(frame->cols));

			uchar* bgr = frame->data;
			for (size_t index = 0; index < size; ++index)
			{
				const size_t indexTimesThree = index * 3;
				luminosity[index] = ::detail::CalcLuminosity(bgr[indexTimesThree + 0], bgr[indexTimesThree + 1], bgr[indexTimesThree + 2]);
			}
			luminosities_.emplace_back(luminosity);
		}
	}
}

unsigned int ImageProcessor::AvgLuminosity(unsigned int frame) const
{
	auto luminosity = luminosities_[frame];

	return luminosity.size()>0 
			? static_cast<unsigned int>(std::accumulate(luminosity.begin(), luminosity.end(), 0)/luminosity.size())
			: 0;
}

unsigned int ImageProcessor::AvgLuminosity() const
{
	if (avgVideoLuminosity != std::numeric_limits<unsigned int>::min())
		return avgVideoLuminosity;

	unsigned int sum = 0;
	avgFrameLuminosity_.resize(luminosities_.size());

	for (size_t index = 0; index < luminosities_.size(); ++index)
	{
		auto avgLuminosity = AvgLuminosity(static_cast<unsigned int>(index));
		avgFrameLuminosity_[index] = avgLuminosity;
		sum += avgLuminosity;
	}
	avgVideoLuminosity = luminosities_.size()>0
			? static_cast<unsigned int>(sum / luminosities_.size()): 0;
	return avgVideoLuminosity;
}

