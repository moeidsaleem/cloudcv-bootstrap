/**********************************************************************************
* CloudCV Boostrap - A starter template for Node.js with OpenCV bindings.
*                    This project lets you to quickly prototype a REST API
*                    in a Node.js for a image processing service written in C++.
*
* Author: Eugene Khvedchenya <ekhvedchenya@gmail.com>
*
* More information:
*  - https://cloudcv.io
*  - http://computer-vision-talks.com
*
**********************************************************************************/
#include <node.h>
#include <v8.h>
#include <nan.h>

#include "framework/Logger.hpp"
#include "ImageSource.hpp"
#include "Algorithm.hpp"
#include "framework/marshal/marshal.hpp"




namespace cloudcv
{
    class ImageSource::ImageSourceImpl
    {
    public:
        virtual ~ImageSourceImpl() = default;
        virtual cv::Mat getImage(int flags) = 0;
    };

    class FileImageSource : public ImageSource::ImageSourceImpl
    {
    public:
        FileImageSource(const std::string& imagePath)
            : mFilePath(imagePath)
        {            
        }

        cv::Mat getImage(int flags) override
        {
            return cv::imread(mFilePath, flags);
        }

    private:
        std::string mFilePath;
    };

    class BufferImageSource : public ImageSource::ImageSourceImpl
    {
    public:
        BufferImageSource(v8::Local<v8::Object> imageBuffer)
            : mImageBuffer(imageBuffer)
        {            
            mImageData    = node::Buffer::Data(imageBuffer);
            mImageDataLen = node::Buffer::Length(imageBuffer);
            LOG_TRACE_MESSAGE("[Buffer]" << mImageDataLen);        
        }

        virtual ~BufferImageSource()
        {
            mImageBuffer.Reset();
        }

        cv::Mat getImage(int flags) override
        {
            return cv::imdecode(cv::_InputArray(mImageData, mImageDataLen), flags);
        }

    private:
        Nan::Persistent<v8::Object>       mImageBuffer;
        char                   * mImageData;
        size_t                   mImageDataLen;
    };

    ImageSource::ImageSource()
    {

    }

    ImageSource::ImageSource(std::shared_ptr<ImageSourceImpl> impl)
        : m_impl(impl)
    {
    }

    cv::Mat ImageSource::getImage(int flags /* = cv::IMREAD_COLOR */)
    {
        if (m_impl.get() != nullptr)
        {
            return m_impl->getImage(flags);
        }

        throw std::runtime_error("Image is empty");
    }

    ImageSource ImageSource::CreateImageSource(const std::string& filepath)
    {
        LOG_TRACE_MESSAGE("ImageSource [File]:" << filepath);
        return ImageSource(std::shared_ptr<ImageSourceImpl>(new FileImageSource(filepath)));
    }

    ImageSource ImageSource::CreateImageSource(v8::Local<v8::Value> bufferOrString)
    {
        if (node::Buffer::HasInstance(bufferOrString))
            return CreateImageSource(bufferOrString->ToObject());

        if (bufferOrString->IsString())
            return CreateImageSource(marshal<std::string>(bufferOrString->ToString()));

        throw serialization::MarshalTypeMismatchException("Invalid input argument type. Cannot create ImageSource");
    }

    ImageSource ImageSource::CreateImageSource(v8::Local<v8::Object> imageBuffer)
    {
        LOG_TRACE_MESSAGE("ImageSource [Buffer]");        
        return ImageSource(std::shared_ptr<ImageSourceImpl>(new BufferImageSource(imageBuffer)));
    }

}