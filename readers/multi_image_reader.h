/**
 * \file multi_image_reader.h Multi image reader 
 */
// Created by amourao on 27-06-2019.
#pragma once

#include <fstream>
#include <iostream>
#include <vector>

// #include <cereal/archives/binary.hpp>

#include "../structs/frame_struct.h"
#include "../utils/image_decoder.h"
#include "image_reader.h"
#include "ireader.h"

namespace moetsi::ssp {

class MultiImageReader: public IReader {
private:
  unsigned int frame_counter_;

  std::vector<std::shared_ptr<IReader>> readers_;
  std::vector<std::shared_ptr<FrameStruct>> current_frame_internal_;

public:
  MultiImageReader(std::vector<std::string> filename);
  ~MultiImageReader();

  void Reset();

  void GoToFrame(unsigned int frame_id);

  bool HasNextFrame();

  void NextFrame();

  std::vector<std::shared_ptr<FrameStruct>> GetCurrentFrame();

  unsigned int GetCurrentFrameId();

  std::vector<FrameType> GetType();

  unsigned int GetFps();
};

} // namespace moetsi::ssp
