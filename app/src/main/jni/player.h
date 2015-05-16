#pragma once

#include <string>
#include <memory>
#include <chrono>
#include "Stream.h"
#include "GstAudioBackend.h"
#include "Downloader.h"

//class Player_impl;
class Player {
//	std::unique_ptr<Player_impl> d;
public:
	Player();
	~Player();

	// Interface
	void play(const char* filename);	
	void pause();
	void stop();
	void seek(uint64_t ms);

private:
	AudioX::GstAudioBackend m_backend;
	AudioX::Downloader http;
	std::shared_ptr<AudioX::Stream> m_stream;
};

