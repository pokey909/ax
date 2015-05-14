#include "Downloader.h"
#include "curl_option.h"

#include <thread>
#include <locale>

typedef int(AudioX::Downloader::*write_callback) (void *, size_t, size_t, void *);
typedef size_t(AudioX::Downloader::*header_callback)(char *, size_t, size_t, void *);

namespace AudioX {
	using curl::curl_easy;
	using curl::make_option;

	// progress callback
	int Downloader::prog_cb(void* p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
		bool* abort = static_cast<bool*>(p);
		return *abort ? !CURLE_OK : CURLE_OK;
	}

	// write callback
	int Downloader::write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
		Downloader* thiz = static_cast<struct Downloader*>(userp);
		size_t realsize = size * nmemb;
		if (realsize > 0) {
			if (auto ss = thiz->m_stream.lock()) {
				ss->write((const char*)contents, realsize);
				StreamInfo& si = ss->streamInfo();
				std::chrono::milliseconds dt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - thiz->startTime);
				si.avgDownloadSpeed = (1000.0 * ss->size()) / dt.count();
			}
			else {
				std::cout << "Downloader: stream object is gone. Aborting download.\n";
				thiz->abort = true;
				return 0;
			}
		}
		return realsize;
	}

	size_t Downloader::header_cb(char *buffer, size_t size, size_t nitems, void *userdata) {
		static const std::string prefixContentType("Content-Type: ");
		static const std::string prefixCache("X-Cache: ");
		Downloader* info = static_cast<Downloader*>(userdata);
		if (auto ss = info->m_stream.lock()) {
			StreamInfo& si = ss->streamInfo();
			std::string arg(buffer);
			if (!arg.compare(0, prefixContentType.size(), prefixContentType)) {
				si.contentType = arg.substr(prefixContentType.size());
				si.contentType.erase(std::remove_if(si.contentType.begin(), si.contentType.end(), ::isspace), si.contentType.end());
			}
			else if (!arg.compare(0, prefixContentType.size(), prefixContentType)) {
				std::string val = arg.substr(prefixContentType.size());
				std::transform(val.begin(), val.end(), val.begin(), ::tolower);
				if (val.find("miss") != std::string::npos)
					si.cdnCached = false;
				else
					si.cdnCached = true;
				
			}
		}
		return nitems * size;
	}

	Downloader::Downloader() {
	}

	Downloader::~Downloader() {
		cancel();
	}

	void Downloader::cancel() {
		if (m_thread.joinable()) {
			std::cout << "Stopping active download (URL: " << url << ")... ";
			abort = true;
			m_thread.join();
			std::cout << "Done!\n";
		}
	}

	void Downloader::newRequest(const std::string& url, std::weak_ptr<Stream> stream) {
		cancel();
		
		m_stream = stream;
		if (auto s = m_stream.lock()) {
			s->reset();
			s->streamInfo().url = url;
		}
		else {
			std::cout << "Downloader Error: no stream object set!\n";
			return;
		}

		this->url = url;
		abort = false;

		m_thread = std::move(std::thread([this, &url]{
			easy.add<CURLOPT_URL>(url.c_str());
			easy.add<CURLOPT_FOLLOWLOCATION>(1L);
			easy.add<CURLOPT_XFERINFOFUNCTION>(&Downloader::prog_cb);
			easy.add<CURLOPT_XFERINFODATA>(&abort);
			curl_easy_setopt(easy.get_curl(), CURLOPT_WRITEFUNCTION, &write_cb);
			curl_easy_setopt(easy.get_curl(), CURLOPT_HEADERFUNCTION, &header_cb);
			easy.add<CURLOPT_WRITEDATA>(this);
			easy.add<CURLOPT_HEADERDATA>(this);
			easy.add<CURLOPT_NOPROGRESS>(0L);
			try {
				startTime = std::chrono::steady_clock::now();
				easy.perform();
			}
			catch (curl_easy_exception error) {
				// If you want to get the entire error stack we can do:
				auto errors = error.what();
				// Otherwise we could print the stack like this:
				error.print_traceback();
				// Note that the printing the stack will erase it
			}
			if (auto ss = m_stream.lock()) {
				StreamInfo& si = ss->streamInfo();
				si.namelookupTime = *(easy.get_info<double>(CURLINFO_NAMELOOKUP_TIME));
				si.connectTime = (std::max)(*(easy.get_info<double>(CURLINFO_CONNECT_TIME)) - si.namelookupTime, 0.0);
				si.sslHandshakeTime = (std::max)(*(easy.get_info<double>(CURLINFO_APPCONNECT_TIME)) - si.connectTime - si.namelookupTime, 0.0);
				si.startTransferTime = *(easy.get_info<double>(CURLINFO_STARTTRANSFER_TIME));
				si.redirectTime = *(easy.get_info<double>(CURLINFO_REDIRECT_TIME));
				si.totalTime = *(easy.get_info<double>(CURLINFO_TOTAL_TIME));
				si.numRedirects = *(easy.get_info<long>(CURLINFO_REDIRECT_COUNT));
				si.avgDownloadSpeed = *(easy.get_info<double>(CURLINFO_SPEED_DOWNLOAD));
				si.headerSize = *(easy.get_info<long>(CURLINFO_HEADER_SIZE));
				si.requestSize = *(easy.get_info<long>(CURLINFO_REQUEST_SIZE));
				si.sslVerifyResult = *(easy.get_info<long>(CURLINFO_SSL_VERIFYRESULT));

				char* ctype = NULL;
				if (curl_easy_getinfo(easy.get_curl(), CURLINFO_CONTENT_TYPE, &ctype) == CURLE_OK && ctype)
					si.contentType = ctype;
				ctype = NULL;
				if (curl_easy_getinfo(easy.get_curl(), CURLINFO_EFFECTIVE_URL, &ctype) == CURLE_OK && ctype)
					si.effectiveUrl= ctype;

				ss->markEos();	// should be replaced by something like write(SimpleStream::EOS)
			}
			else {
				abort = true;
				std::cout << "Couldnt mark EOS. Stream object gone.\n";
			}
			std::cout << "Download from " << url << " finished!\n";
		}));
	}

}
