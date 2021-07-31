/**
 * \file idecoder.h Frame decoder interface
 */
// Created by amourao on 12-09-2019.
#pragma once

#include "../structs/frame_struct.h"
#include <opencv2/core/mat.hpp>

namespace moetsi::ssp {

/**
 * IDecoder abstract decoder interface
 */
class IDecoder {
public:
  /** Virtual destructor */
  virtual ~IDecoder() {}
  /**
   * Extract an opencv image from a FrameStruct
   * \input data FrameStruct
   */
  virtual cv::Mat Decode(FrameStruct& data) = 0;
};

/**
 * IDecoder factory.
 * \param config configuration
 * \return IDecoder instance
 */
std::shared_ptr<IDecoder> IDecoderFactory(const std::string & config);

} // moetsi::ssp
