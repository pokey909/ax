#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>
#include <iomanip>
#include <iostream>

namespace AudioX {

	struct StreamInfo {
		StreamInfo() :
		namelookupTime(0.0),
		connectTime(0.0),
		sslHandshakeTime(0.0),
		startTransferTime(0.0),
		redirectTime(0.0),
		totalTime(0.0),
		numRedirects(0),
		avgDownloadSpeed(0.0),
		headerSize(0),
		requestSize(0),
		sslVerifyResult(0),
		cdnCached(false)
		{}

		std::string contentType;
		std::string url;
		std::string effectiveUrl;
		double namelookupTime;
		double connectTime;
		double sslHandshakeTime;
		double startTransferTime;
		double redirectTime;
		double totalTime;

		long numRedirects;

		double avgDownloadSpeed;
		long headerSize;
		long requestSize;
		long sslVerifyResult;

		bool cdnCached;

		void print() {
			using namespace std;
			const int w = 25;
			cout << std::left << std::fixed << std::setprecision(4);
			cout << "--------------------------------------------------------------\n";
			cout << setw(w) << "Content-type" << contentType.c_str() << "\n";
			cout << setw(w) << "URL" << url.c_str() << "\n";
			cout << setw(w) << "Effective URL" << effectiveUrl.c_str() << "\n";
			cout << setw(w) << "Namelookup" << namelookupTime << "s\n";
			cout << setw(w) << "Connect" << connectTime << "s\n";
			cout << setw(w) << "SSL handshake" << sslHandshakeTime << "s\n";
			cout << setw(w) << "StartTransfer" << startTransferTime << "s\n";
			cout << setw(w) << "Redirect" << redirectTime << "s\n";
			cout << setw(w) << ">>>TotalTime<<<" << totalTime << "s\n";
			cout << setw(w) << "# Redirect" << numRedirects << "\n";
			cout << setw(w) << std::setprecision(2) << "avg. DL speed (kB/s)" << avgDownloadSpeed / 1000.0 << " kB/s\n";
			cout << setw(w) << std::setprecision(2) << "Header size" << headerSize << " Bytes\n";
			cout << setw(w) << "Request size" << requestSize << " Bytes\n";
			cout << setw(w) << "SSLVerify" << sslVerifyResult << "\n";
			cout << setw(w) << "CDN cached" << (cdnCached ? "Yes" : "no") << "\n";
			cout << "--------------------------------------------------------------\n\n";
		}
	};

	class Stream {
	public:
		Stream();
		Stream(std::streamsize maxSize);

		size_t read(char* buffer, std::streamsize maxSize);
		void read(std::vector<char>& buffer);

		void write(const char* data, std::streamsize size);

		inline std::streamsize size() const { return gptrEnd; }

		inline std::streamsize available() const { return gptrEnd - gptrNext; }
		inline void markEos() { eosFlag = true; waitDataCond.notify_all(); }
		inline bool eos() const { return eosFlag; }

		StreamInfo& streamInfo() { return m_info; }

		void reset();

		bool waitReadyRead(std::streamsize minBytesAvail = 1);

	private:
		std::vector<char> buf;
		bool eosFlag;
		std::streamsize gptrNext, gptrEnd;
		std::mutex waitMutex;
		std::condition_variable waitDataCond;
		StreamInfo m_info;
	};

}