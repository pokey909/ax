#pragma once

#include <string>
#include <memory>
#include <chrono>

class Player_impl;
class Player {
	std::unique_ptr<Player_impl> d;
public:
	Player();
	~Player();

	// Interface
	void play(const char* filename);	
	void pause();
	void stop();
	void seek(std::chrono::milliseconds ms);
};

