#ifndef FACELANDMARK_H
#define FACELANDMARK_H

#include "FaceDetection.hpp"

namespace my {

    class FaceLandmark : public my::FaceDetection {
        public:
            FaceLandmark(std::string modelPath);
            virtual ~FaceLandmark() = default; 

            virtual void runInference();

            virtual cv::Point getFaceLandmarkAt(int index) const;

            virtual std::vector<cv::Point> getAllFaceLandmarks() const;

            virtual std::vector<float> loadOutput(int index = 0) const;


        private:
            my::ModelLoader m_landmarkModel;

    };
}

#endif // FACELANDMARK_H