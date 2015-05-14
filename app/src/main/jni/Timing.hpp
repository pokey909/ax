#pragma once
#include <chrono>
#include <string>
#include <iostream>

class Timing {
public:
	Timing(const std::string& name) : m_name(name), start(std::chrono::high_resolution_clock::now())
	{}

	~Timing() {
		using namespace std::chrono;
		auto end = std::chrono::high_resolution_clock::now();
		duration<double> time_span = duration_cast<duration<double>>(end - start);
		std::cout << "[" << m_name << "] " << time_span.count() << "\n";
	}
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
	std::string m_name;
};


