//
// Created by amourao on 26-06-2019.
//

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#define SSP_EXPORT __declspec(dllexport)
#else
#include <unistd.h>
#define SSP_EXPORT
#endif

#include <k4a/k4a.h>
#include <opencv2/imgproc.hpp>
#include <zmq.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

#include "../utils/logger.h"

#include "../readers/network_reader.h"
#include "../utils/kinect_utils.h"

class BodyTracker
{
public:
    BodyTracker(int port);
    ~BodyTracker();

    int update();
    int getBodyCount() const;
    int getBodiesStruct(k4abt_body_t* pBodies, int n) const;
    int getBodies(k4abt_skeleton_t* pSkeletons, int* pIds, int n) const;

private:
    NetworkReader* m_reader;
    k4a::calibration m_sensor_calibration;
    bool m_calibration_set;
    k4abt::tracker m_tracker;
    std::unordered_map<std::string, std::shared_ptr<IDecoder>> m_decoders;
    std::vector< k4abt_body_t> m_bodies;
    mutable std::mutex m_mutex;
};

BodyTracker::BodyTracker(int port)
{
    spdlog::set_level(spdlog::level::debug);
    av_log_set_level(AV_LOG_QUIET);

    m_calibration_set = false;

    m_reader = new NetworkReader(port);
    m_reader->init();
}

BodyTracker::~BodyTracker()
{
    delete m_reader;
}

int BodyTracker::update()
{
  try {
    if (m_reader->HasNextFrame()) {
      m_reader->NextFrame();
      std::vector<FrameStruct> f_list = m_reader->GetCurrentFrame();
      for (FrameStruct f : f_list) {
        std::string decoder_id = f.stream_id + std::to_string(f.sensor_id);

        if (f.camera_calibration_data.type == 0 && m_calibration_set == false) {
          const k4a_depth_mode_t d = static_cast<const k4a_depth_mode_t>(f.camera_calibration_data.extra_data[0]);
          const k4a_color_resolution_t r =
            static_cast<const k4a_color_resolution_t>(f.camera_calibration_data.extra_data[1]);

          m_sensor_calibration = k4a::calibration::get_from_raw(
            reinterpret_cast<char *>(&f.camera_calibration_data.data[0]),
            f.camera_calibration_data.data.size(), d, r);

          m_calibration_set = true;
          m_tracker = k4abt::tracker::create(m_sensor_calibration);
        }
      }

      k4a::capture sensor_capture = k4a::capture::create();
      FrameStructToK4A(f_list, sensor_capture, m_decoders);

      if (!m_tracker.enqueue_capture(sensor_capture)) {
        // It should never hit timeout when K4A_WAIT_INFINITE is set.
        spdlog::error("Error adding capture to tracker process queue timeout!");
        return -1;
      }

      k4abt::frame body_frame = m_tracker.pop_result();
      if (body_frame != nullptr) {
        m_mutex.lock();
        size_t num_bodies = body_frame.get_num_bodies();
        m_bodies.resize(num_bodies);

        // Store bodies
        for (size_t i = 0; i < num_bodies; i++)
          m_bodies[i] = body_frame.get_body(i);
        m_mutex.unlock();
      }
      else {
        spdlog::error("Pop body frame result time out!!");
        return -1;
      }
    }
  }
  catch (std::exception &e) {
    spdlog::error(e.what());
    return -1;
  }

  return 0;
}

int BodyTracker::getBodyCount() const
{
  m_mutex.lock();
  int num = m_bodies.size();
  m_mutex.unlock();

  return num;
}

int BodyTracker::getBodiesStruct(k4abt_body_t* pBodies, int n) const
{
  // If the number has changed since call to getBodyCount
  // Fill at most n bodies

  m_mutex.lock();
  int num = std::min(n, (int)m_bodies.size());
  memcpy(pBodies, &m_bodies[0], num * sizeof(k4abt_body_t));
  m_mutex.unlock();

  return num;
}

int BodyTracker::getBodies(k4abt_skeleton_t* pSkeletons, int* pIds, int n) const
{
  // If the number has changed since call to getBodyCount
  // Fill at most n bodies

  m_mutex.lock();
  int num = std::min(n, (int)m_bodies.size());
  for (int i = 0; i < num; i++)
  {
    pIds[i] = m_bodies[i].id;
    pSkeletons[i] = m_bodies[i].skeleton;
  }
  m_mutex.unlock();

  return num;
}

BodyTracker *gTracker = NULL;

extern "C" SSP_EXPORT int open_k4a(int port)
{
    if (gTracker != NULL)
        return -1;

    gTracker = new BodyTracker(port);

    return 0;
}

extern "C" SSP_EXPORT int close_k4a(int port)
{
    if (gTracker == NULL)
        return -1;

    delete gTracker;
    gTracker = NULL;
    return 0;
}

extern "C" SSP_EXPORT int update_k4a()
{
    if (gTracker == NULL)
        return -1;

    gTracker->update();

    return 0;
}

extern "C" SSP_EXPORT int getBodyCount()
{
    return gTracker->getBodyCount();
}

extern "C" SSP_EXPORT int getBodiesStruct(k4abt_body_t* pBodies, int n)
{
  return gTracker->getBodiesStruct(pBodies, n);
}

extern "C" SSP_EXPORT int getBodies(k4abt_skeleton_t* pSkeletons, int* pIds, int n)
{
    return gTracker->getBodies(pSkeletons, pIds, n);
}

void PrintBodyInformation(k4abt_body_t body) {
  std::cout << "Body ID: " << body.id << std::endl;
  for (int i = 0; i < (int)K4ABT_JOINT_COUNT; i++) {
    k4a_float3_t position = body.skeleton.joints[i].position;
    k4a_quaternion_t orientation = body.skeleton.joints[i].orientation;
    printf("Joint[%d]: Position[mm] ( %f, %f, %f ); Orientation ( %f, %f, %f, "
           "%f) \n",
           i, position.v[0], position.v[1], position.v[2], orientation.v[0],
           orientation.v[1], orientation.v[2], orientation.v[3]);
  }
}

void PrintBodyIndexMapMiddleLine(k4a::image body_index_map) {
  uint8_t *body_index_map_buffer = body_index_map.get_buffer();

  assert(body_index_map.get_stride_bytes() ==
         body_index_map.get_width_pixels());

  int middle_line_num = body_index_map.get_height_pixels() / 2;
  body_index_map_buffer = body_index_map_buffer +
                          middle_line_num * body_index_map.get_width_pixels();

  std::cout << "BodyIndexMap at Line " << middle_line_num << ":" << std::endl;
  for (int i = 0; i < body_index_map.get_width_pixels(); i++) {
    std::cout << (int)*body_index_map_buffer << ", ";
    body_index_map_buffer++;
  }
  std::cout << std::endl;
}

#ifndef SSP_PLUGIN
int main(int argc, char *argv[]) {

  spdlog::set_level(spdlog::level::debug);
  av_log_set_level(AV_LOG_QUIET);

  srand(time(NULL));

  try {

    if (argc < 2) {
      std::cerr << "Usage: ssp_client <port> (<log level>) (<log file>)"
                << std::endl;
      return 1;
    }
    std::string log_level = "debug";
    std::string log_file = "";

    if (argc > 2)
      log_level = argv[2];
    if (argc > 3)
      log_file = argv[3];

    int port = std::stoi(argv[1]);
    NetworkReader reader(port);

    reader.init();

    k4a::calibration sensor_calibration;
    bool calibration_set = false;
    k4abt::tracker tracker;

    std::unordered_map<std::string, std::shared_ptr<IDecoder>> decoders;

    while (reader.HasNextFrame()) {
      reader.NextFrame();
      std::vector<FrameStruct> f_list = reader.GetCurrentFrame();
      for (FrameStruct f : f_list) {
        std::string decoder_id = f.stream_id + std::to_string(f.sensor_id);

        if (f.camera_calibration_data.type == 0 && calibration_set == false) {

          const k4a_depth_mode_t d = static_cast<const k4a_depth_mode_t>(
              f.camera_calibration_data.extra_data[0]);
          const k4a_color_resolution_t r =
              static_cast<const k4a_color_resolution_t>(
                  f.camera_calibration_data.extra_data[1]);

          sensor_calibration = k4a::calibration::get_from_raw(
              reinterpret_cast<char *>(&f.camera_calibration_data.data[0]),
              f.camera_calibration_data.data.size(), d, r);

          calibration_set = true;

          tracker = k4abt::tracker::create(sensor_calibration);
        }
      }

      k4a::capture sensor_capture = k4a::capture::create();
      FrameStructToK4A(f_list, sensor_capture, decoders);

      if (!tracker.enqueue_capture(sensor_capture)) {
        // It should never hit timeout when K4A_WAIT_INFINITE is set.
        spdlog::error("Error adding capture to tracker process queue timeout!");
        exit(1);
      }

      k4abt::frame body_frame = tracker.pop_result();
      if (body_frame != nullptr) {
        size_t num_bodies = body_frame.get_num_bodies();
        spdlog::info("{} bodies are detected!", num_bodies);

        for (size_t i = 0; i < num_bodies; i++) {
          k4abt_body_t body = body_frame.get_body(i);
          PrintBodyInformation(body);
        }

        k4a::image body_index_map = body_frame.get_body_index_map();
        if (body_index_map != nullptr) {
          PrintBodyIndexMapMiddleLine(body_index_map);
        } else {
          spdlog::error("Failed to generate bodyindex map!");
        }
      } else {
        spdlog::error("Pop body frame result time out!!");
        break;
      }
    }

  } catch (std::exception &e) {
    spdlog::error(e.what());
  }

  return 0;
}
#endif