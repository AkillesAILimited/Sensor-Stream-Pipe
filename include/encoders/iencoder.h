/**
 * \file iencoder.h IEncoder definition: frame encoder
 */
// Created by amourao on 12-09-2019.
#pragma once

#include "../structs/frame_struct.h"

namespace moetsi::ssp {

/**
 * IEncoder abstract encoder class 
 */
class IEncoder {

public:
  /** Virtual destructor */
  virtual ~IEncoder() {}

  /** 
   * Add a frame struct
   * \param frame_struct FrameStruct to add
   */
  virtual void AddFrameStruct(std::shared_ptr<FrameStruct> &frame_struct) = 0;

  /**
   * Go to next packet
   */
  virtual void NextPacket() = 0;

  /**
   * Check if there is a next packet
   * \return true if there is a next packet
   */
  virtual bool HasNextPacket() = 0;

  /**
   * Get current encoded frame
   * \return current encoded frame
   */
  virtual std::shared_ptr<FrameStruct> CurrentFrameEncoded() = 0;

  /**
   * Get current frame in its original format 
   * \return current frame in its original format
   */
  virtual std::shared_ptr<FrameStruct> CurrentFrameOriginal() = 0;

  /**
   * Get codec parameters
   * \return codec parameters
   */
  virtual std::shared_ptr<CodecParamsStruct> GetCodecParamsStruct() = 0;

  /**
   * Get FPS
   * \return FPS in frame per second
   */
  virtual unsigned int GetFps() = 0;
};

/**
 * IEncoder factory
 * \param config configuration
 * \return IEncoder instance
 */
std::shared_ptr<IEncoder> IEncoderFactory(const std::string & config);

}
