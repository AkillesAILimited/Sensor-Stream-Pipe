/**
 * \file image_reader.h Image reader
 */
// Created by amourao on 27-06-2019.
#pragma once

#include <fstream>
#include <iostream>
#include <vector>

#include "../structs/frame_struct.h"
#include "../utils/image_decoder.h"
#include "ireader.h"

namespace moetsi::ssp {

class ImageReader : public IReader {
private:
  unsigned int frame_counter_;
  unsigned int fps_;
  std::string scene_desc_;
  unsigned int sensor_id_;
  unsigned int device_id_;
  FrameType frame_type_;
  std::string stream_id_;

  std::shared_ptr<CodecParamsStruct> codec_params_struct_;

  std::shared_ptr<FrameStruct> current_frame_internal_;

  std::vector<std::string> frame_lines_;

  std::vector<unsigned char> ReadFile(std::string &filename);

  std::shared_ptr<FrameStruct> CreateFrameStruct(unsigned int frame_id);

public:
  ImageReader(std::string filename);
  ~ImageReader();

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
