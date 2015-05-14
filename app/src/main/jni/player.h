#pragma once

#include <string>
#include <memory>
#include <gst/gst.h>
#include <chrono>

class Player_impl;
class Player {
	std::unique_ptr<Player_impl> d;
public:
	Player();
	~Player();

	// Interface
	void play(std::string filename);	
	void pause();
	void stop();
	void seek(std::chrono::milliseconds ms);
};

