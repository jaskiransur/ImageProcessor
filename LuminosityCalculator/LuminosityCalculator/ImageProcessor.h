
#ifndef image_processor_h
#define image_processor_h
#include<string>
#include <vector>
#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <opencv\ml.h>
#include <opencv\cxcore.h>
#include <mutex>
#include <vector>
#include <limits>

#include<boost\filesystem.hpp>

using namespace std;
using namespace boost::filesystem;
using namespace cv;

class ImageProcessor
{
public:
	/*! @brief @brief Image processor constructor
	*   @param[in]: path to a video file to be processed
	*/
	ImageProcessor(const path& _file);

	/*! @brief Image processor copy constructor
	*   @param[in]: ImageProcessor object to copy from 
	*/
	ImageProcessor(const ImageProcessor& other);

	/*! @brief Image processor assignment operator 
	*   @param[in]: ImageProcessor object to copy from 
	*/
	ImageProcessor& operator= (ImageProcessor& other);


	/*! @brief Image processor move constructor
	*	@param[in]: ImageProcessor object to move 
	*/
	ImageProcessor(ImageProcessor&& other);
	
	/*! @brief Image processor move assignment operator 
	*	param[in]: ImageProcessor object to move 
	*/
	ImageProcessor& operator= (ImageProcessor&& other);

	~ImageProcessor() = default;

	/*! @brief Video file, which is processed by this class
	* 	Convert the file to Images/frames, which can be used to calculate BGR/HSV and
	* 	Luminosity of an image/frame
	*	@return const reference to path of video file
	*/

	const path& File() const;

	/*! @brief Video file, which is processed by this class
	*	Convert the file to Images/frames, which can be used to calculate BGR/HSV and
	*	Luminosity of an image/frame
	*	@return copy of path of video file
	*/
	path File();

	/*! @brief Frames/Images extracted from a video file 
	* 	Convert the file to Images/frames, which can be used to calculate BGR/HSV and
	* 	Luminosity of an image/frame
	* 	@return frames as a const reference to container of cv::Mat object
	*/
	const vector<Mat>& Frames() const;

	/*! @brief Frames/Images extracted from a video file 
	* 	Convert the file to Images/frames, which can be used to calculate BGR/HSV and
	* 	Luminosity of an image/frame
	* 	@return frames as a reference to container of cv::Mat object
	*/
	vector<Mat>& Frames();

	/*! @brief Avg luminosity calculated using V component of HSV/ or using BGR's relative luminosity formulae
	*	@return avg luminosity of a frame/image
	*/
	unsigned int AvgLuminosity(unsigned int frame) const;

	/*! @brief Avg luminosity calculated using V component of HSV/ or using BGR's relative luminosity formulae
	*	@return avg luminosity of video
	*/
	unsigned int AvgLuminosity() const;

	/*! @brief Save frames/images extracted from a video to an output folder
	* the images are named after the frame numbers
	*/
	void SaveFrames(const string& outputDir);

	/*! @brief Convert a video to frames/images
	*	The images are stored in frame container 
	*	@return: <none>
	*/
	void ConvertVideoToFrames();
	/*! @brief Convert a video to frames/images
	* the images are stored in frame container 
	*/
	void CalculateLuminosityUsingHSV();
	void CalculateLuminosityUsingBGR();
private:
	path videoFilePath_;
	mutable unsigned int avgVideoLuminosity = std::numeric_limits<unsigned int>::min();
	std::vector<Mat> frames_;
	mutable std::vector<unsigned int> avgFrameLuminosity_;
	std::vector<vector<uchar>> luminosities_;
	mutable std::recursive_timed_mutex mutex_;
};

#endif //ImageProcessor
