========================================================================
    CONSOLE APPLICATION : [LuminosityCalculator] Project Overview
========================================================================

Binary LuminosityCalculator.exe
Usage: LuminosityCalculator.exe <Videos Director Path> <Number of Threads>

Errors: Input of an invalid video directory path or threads more than 100 

The video files are processed using opencv library. Each video file is converted to frames/images
For each frame, the following calculation is done to compute Luminosity
1. Using HSV method, the V component contains the brightness of a pixel and can be used to 
ascertain luminosity
	The following conversion is used to convert a frame to an hsv frame. 
	An HSV channel is split into H, S, V values, and the V component is copied to the luminosity vector
	
	Mat hsvImg;
	cvtColor(*frame, hsvImg, CV_BGR2HSV);

	Mat channels[3];
	cv::split(hsvImg, channels);
	//v component contains the luminosity
	uchar* start = channels[2].data
	
			
2. Another method which calculates relative Luminosity is based on the following formulae
LuminosityCalculator.exe
 Luminosity of a pixel = 0.0722 * B + 0.7152 * G  + 0.2126 * R
 

The calculation of Luminosity are done 
LuminosityCalculator.cpp
    
	This is the file that contain main function and processes the input arguments.
	It creates the threads based on the number of threads requested
	
	If threads specified are more than files, then the threads equal to number of files are created
	if less number of  threads are specified than the number of files, then the files are processed in 
	batches of number of threads
	
Data structures used
	using BatchToFramesMap = std::map<size_t, vector<std::shared_ptr<ImageProcessor>>>;
	BatchToFramesMap frames;
	A map of batch number to the vector of image processing object. 
	Each image processing object works on a single video files; more explained in ImageProcessor 

	std::map<size_t, vector<thread>> threads;	
	A map of batch number and vector of threads created. 
	Create threads in batches where each batch = std::min(numberOfFiles, numberOfThreads) number of threads 
	
	Worker thread
	void WorkerThread(std::weak_ptr<ImageProcessor> imageProcessor)
	Each video file is processed by a single thread and it takes an input of std::weak_ptr<ImageProcessor> object
	The thread is started in critical section, and after verifying the validity of shared_ptr, it is unlock to do processing on a shared_ptr
		
	if (threadMutex.try_lock_for(std::chrono::milliseconds(1000)))
	{
		...
		auto sharedImageProcessor = imageProcessor.lock();
		threadMutex.unlock();
		...
	}
	
	sharedImageProcessor  already contains the video file path from at the time of its creation, when a thread is started
	A video is converted to its frame
	
				sharedImageProcessor->ConvertVideoToFrames();
	If the code is compiled with USE_BGR preprocessor, then it uses the BGR method and else (default), it uses HSV method to calculate Luminosity
	
#ifdef USE_BGR
				sharedImageProcessor->CalculateLuminosityUsingBGR();
#else
				sharedImageProcessor->CalculateLuminosityUsingHSV();
#endif

	ImageProcessor.h, ImageProcessor.cpp
	
	Each frame is stored in frames_ data structure
	
Data Structures

	Video file path
	path videoFilePath_
	
	Contains average Luminosity of a video, the calculated values are cached for ascertaining averages of all videos at later time
	unsigned int avgVideoLuminosity = std::numeric_limits<unsigned int>::min();
	
	Each image from a video is stored in a opencv's datastructure called Mat, which contains the information about the property of each pixel
	std::vector<Mat> frames_;
	
	Each frame's average luminosity is stored in vector of unsigned int
	std::vector<unsigned int> avgFrameLuminosity_;
	
	Each frames's luminosity values for the whole video is stored in luminosities_ datastructure
	std::vector<vector<uchar>> luminosities_;

	
	ImageProcessor instance is created at the time of creation of a thread. The instance contains path to the video file.
	It processes video file by extracting the frames from video. It uses OpenCV APIs to achieve that.
	
	VideoCapture cap(videoFilePath_.string().c_str());
	
	//cap.get(CV_CAP_PROP_FRAME_COUNT) contains the number of frames_ in the video;
	for (int frameNum = 0; frameNum < cap.get(CV_CAP_PROP_FRAME_COUNT); ++frameNum)
	{
		Mat frame;
		cap >> frame; // get the next frame from video

		frames_.emplace_back(std::move(frame));
	}
	
	ConvertVideoToFrames takes a video path and convert it to a vector of Mat object. Each entry in the vector corresponds to a single frame.
	void ConvertVideoToFrames();

	Values can be calculated based on HSV method or BGR method
	void CalculateLuminosityUsingHSV();
	
	void CalculateLuminosityUsingBGR()
	
	all the frames can also be saved to an image directory for a video file. Each image is stored with a number based on its frame number
	void SaveFrames(const string& outputDir)
/////////////////////////////////////////////////////////////////////////////
Other notes:
    
	Please contact jaskiranjit@gmail.com, if you have any question. 

//////////////////////////////////////////////////////////////////////////