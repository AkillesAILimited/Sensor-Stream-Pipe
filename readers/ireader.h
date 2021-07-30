/**
 * \file ireader.h Reader interface to SSP
 */
// Created by amourao on 14-08-2019.
#pragma once

#include "../structs/frame_struct.h"

namespace moetsi::ssp {

/**
 * Setup logging
 * \param level logging level
 * \param file logging file
 */
void SetupLogging(std::string &level, std::string &file);

/**
 * SSP reader interface - abstract class.
 * Question: BLOCKING operations ????
 */ 
class IReader {

public:
  /** Destructor */
  virtual ~IReader() {}

  /** Get current frame data */
  virtual std::vector<std::shared_ptr<FrameStruct>> GetCurrentFrame() = 0;

  /** 
   * Get frame types
   * \return a vector of FrameType, listing available data types 
   */
  virtual std::vector<FrameType> GetType() = 0;

  /**
   * Check if there is a next frame
   * \return true if there is a next frame
   */
  virtual bool HasNextFrame() = 0;

  /** Go to next frame */
  virtual void NextFrame() = 0;

  /** Reset this reader */
  virtual void Reset() = 0;

  /** 
   * Go to a given frame
   * \param frame_id target frame number
   */
  virtual void GoToFrame(unsigned int frame_id) = 0;

  /**
   * Get current frame number
   * \return current frame number.
   */ 
  virtual unsigned int GetCurrentFrameId() = 0;

  /**
   * Get indicative FPS in frame per second.
   * \return the FPS number
   */ 
  virtual unsigned int GetFps() = 0;
};

/**
 * IReader factory
 * \param config configuration
 * \return an IReader instance
 */
std::shared_ptr<IReader> IReaderFactory(const std::string & config); // TODO implement

} // namespace moetsi::ssp