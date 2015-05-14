#pragma once
#include "Stream.h"

#include <memory>
#include <string>
#include <thread>
#include <curl_easy.h>
#include <chrono>

using curl::curl_easy;

namespace AudioX {

	// passed as user data to callbacks
	//struct Info {
	//	Info(std::weak_ptr<AudioX::Stream> stream) : ss(stream), abort(false), curl(nullptr) {
	//	}
	//	std::weak_ptr<AudioX::Stream> ss;
	//	CURL* curl;
	//};

	class Downloader {
	public:
		Downloader();
		~Downloader();
		void newRequest(const std::string& url, std::weak_ptr<Stream> stream);
		void cancel();

	protected:
		static size_t header_cb(char *buffer, size_t size, size_t nitems, void *userdata);
		static int write_cb(void *contents, size_t size, size_t nmemb, void *userp);
		static int prog_cb(void* p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

	private:
		curl_easy easy;
		std::weak_ptr<Stream> m_stream;
		//std::unique_ptr<Info> m_info;
		std::thread m_thread;

		std::string url;
		bool abort;
		std::chrono::steady_clock::time_point startTime;
	};

}