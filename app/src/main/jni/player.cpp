#include "player.h"
#include <iostream>
#include <gst/gst.h>

using namespace AudioX;

Player::Player() : m_stream(std::make_shared<AudioX::Stream>()) {
}

Player::~Player() {
  g_print("exiting\n"); 
  stop();
}

void Player::play(const char* filename) {
  http.cancel();
  m_stream = std::make_shared<AudioX::Stream>();
  m_stream->setUrl(filename);
  std::string s = filename;
  http.newRequest(s, m_stream);
  m_backend.start(m_stream);
  g_print("AFTER START");
}

void Player::pause() {
  m_backend.pause();
}

void Player::stop() {
  m_backend.stop();
  http.cancel();
}

void Player::seek(uint64_t ms) {
  http.cancel();
  m_stream->seek(ms);
  http.newRequest(m_stream->streamInfo().url, m_stream);
}