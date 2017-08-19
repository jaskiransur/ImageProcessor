// LuminosityCalculator.cpp : Defines the entry point for the console application.
//

#include <vector>
#include <set>
#include <string>
#include <thread>
#include <numeric>

#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <opencv\ml.h>
#include <opencv\cxcore.h>
#include <boost\filesystem.hpp>

#include "ImageProcessor.h"

using namespace std;
using namespace cv;
using namespace boost::filesystem;

namespace detail
{
	const int MAX_THREADS = 100;
	std::recursive_timed_mutex threadMutex;
	void WorkerThread(std::weak_ptr<ImageProcessor> imageProcessor)
	{
		if (threadMutex.try_lock_for(std::chrono::milliseconds(1000)))
		{
			if (imageProcessor.expired())
				threadMutex.unlock();
			else
			{
				auto sharedImageProcessor = imageProcessor.lock();
				std::cout << sharedImageProcessor->File() << std::endl;
				threadMutex.unlock();

				try
				{
					sharedImageProcessor->ConvertVideoToFrames();
#ifdef USE_BGR
					sharedImageProcessor->CalculateLuminosityUsingBGR();
#else
					sharedImageProcessor->CalculateLuminosityUsingHSV();
#endif

					if (threadMutex.try_lock_for(std::chrono::milliseconds(1000)))
					{
						std::cout << "Average Luminosity for " << sharedImageProcessor->File().filename().string().c_str()<< "is"<< sharedImageProcessor->AvgLuminosity()<<std::endl;
						threadMutex.unlock();
					}
				}
				catch (std::exception & ex)
				{
					std::cerr << "exception " << ex.what() << "while executing thread " << getThreadNum() << std::endl;
					std::cerr << "exiting gracefully" << std::endl;
				}
			}
		}
	}
}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		cerr << "Invalid number of arguments" << endl;
		cerr << "Usage: <prg> <Path to video files> <number of threads>" << endl;
		return -1;
	}

	char* pathToVideoDirectory = argv[1];
	size_t numberOfThreads = std::atol(argv[2]);

	//ensure valid path for video file
	auto videoDirPath = boost::filesystem::path(pathToVideoDirectory);

	if (!boost::filesystem::exists(videoDirPath))
		throw std::invalid_argument("Invalid path to video directory");

	//ensure number of threads are within the range of min and max
	if (numberOfThreads <1 || numberOfThreads >::detail::MAX_THREADS)
		throw std::invalid_argument("Invalid number of threads specified");

	//get video files paths (and count as well)
	std::vector<path> files;
	for_each(directory_iterator(videoDirPath), directory_iterator(),
		[&files](const path& file) 
	{
		std::cout << file.string().c_str() << std::endl;
		files.emplace_back(file); 
	});

	//let's start the worker threads
	std::map<size_t, vector<thread>> threads;

	//need to keep the context
	using BatchToFramesMap = std::map<size_t, vector<std::shared_ptr<ImageProcessor>>>;
	BatchToFramesMap frames;

	//calculate the number of batch needed to process all the files with specified number of threads
	size_t numberOfFiles = files.size();
	const size_t batches = static_cast<size_t>(ceil(static_cast<double>(numberOfFiles) / static_cast<double>(numberOfThreads)));

	//create threads in batches where each batch = std::min(numberOfFiles, numberOfThreads) number of threads 
	for (size_t batch = 0; batch < batches; ++batch)
	{
		const size_t workerThreads= std::min(numberOfFiles, numberOfThreads);

		for (size_t index = 0; index < workerThreads; ++index)
		{

			cout << "starting thread #" << index << "for batch " << batch << endl;
			if (::detail::threadMutex.try_lock_for(std::chrono::milliseconds(1000)))
			{
				std::shared_ptr<ImageProcessor> imageProcessor = std::make_shared<ImageProcessor>(files[index]);

				frames[batch].emplace_back(std::move(imageProcessor));
				::detail::threadMutex.unlock();
			}

			auto&& threadContext = frames[batch][index];
			threads[batch].emplace_back(thread(::detail::WorkerThread, threadContext));
		}
		numberOfFiles -= workerThreads;

		for (size_t index = 0; index < workerThreads; ++index)
		{
			if (threads[batch][index].joinable())
				threads[batch][index].join();
		}
	}

	//get average luminisites for each video file 
	std::map<unsigned int, std::string> avgVideoLuminosities; 
	for_each(frames.cbegin(), frames.cend(),
		[&avgVideoLuminosities](BatchToFramesMap::const_reference pair)
	{
		std::for_each(pair.second.cbegin(), pair.second.cend(), 
			[&avgVideoLuminosities](const std::shared_ptr<ImageProcessor>& processor)
		{
			avgVideoLuminosities.emplace(processor->AvgLuminosity(), processor->File().filename().string());
		});
	});

	std::cout << "Minimum luminance value " << avgVideoLuminosities.begin()->first << "recorded for video file" << avgVideoLuminosities.begin()->second << std::endl;
	std::cout << "Maximum luminance value " << avgVideoLuminosities.rbegin()->first << "recorded for video file" << avgVideoLuminosities.rbegin()->second << std::endl;

	unsigned int init = 0;
	unsigned int sumVideosLuminosity = std::accumulate(avgVideoLuminosities.cbegin(), avgVideoLuminosities.cend(), init, 
		[](unsigned int initValue, const std::pair<unsigned int, std::string>& secondPair)
	{
		return initValue + secondPair.first;
	});

	std::cout << "Mean luminance value " << sumVideosLuminosity/avgVideoLuminosities.size()<< std::endl;
	auto medianIndex = static_cast<unsigned int>(avgVideoLuminosities.size() / 2);
	std::cout << "Median luminance value " << avgVideoLuminosities.find(medianIndex)->first<< std::endl;
    return 0;
}

