#include "Stream.h"

#include <algorithm>

namespace AudioX {

	Stream::Stream() : eosFlag(false) {
		size_t maxSize = 2000000;
		buf.reserve(maxSize); // 2mb reserve
		gptrNext = 0;
		gptrEnd = 0;
	}

	Stream::Stream(std::streamsize maxSize) : eosFlag(false) {
		buf.reserve(maxSize); // 2mb reserve
		gptrNext = 0;
		gptrEnd = 0;
	}

	void Stream::setUrl(const std::string& url) {
		m_info.url = url;
	}
	
	void Stream::reset() {
		gptrNext = 0;
		gptrEnd = 0;
		eosFlag = false;
		buf.clear();
		m_info = StreamInfo();
	}

	size_t Stream::read(char* buffer, std::streamsize maxSize) {
		std::streamsize n = (std::min)(gptrEnd - gptrNext, maxSize);
		if (n > 0) {
			memcpy(buffer, &buf[gptrNext], n);
			gptrNext += n;
		} else {
			n = 0;
		}
		return n;
	}
	void Stream::read(std::vector<char>& buffer) {
		std::streamsize n = gptrEnd - gptrNext;
		if (n > 0) {
			buffer.insert(buffer.end(), &buf[0] + gptrNext, &buf[0] + gptrEnd);
			gptrNext = gptrEnd;
		}
	}

	void Stream::write(const char* data, std::streamsize size) {
		buf.insert(buf.end(), data, data + size);
		gptrEnd += size;
		waitDataCond.notify_one();
	}

	void Stream::seek(std::streamsize off) {
		if (off > buf.size()) {
			buf.resize(off);
			gptrEnd = off;
		}
		gptrNext = off;
	}

	bool Stream::waitReadyRead(std::streamsize minBytesAvail) {
		if (available() < minBytesAvail && !eos()) {
			std::unique_lock<std::mutex> lock(waitMutex);
			waitDataCond.wait(lock, [&]{ return (available() >= minBytesAvail) || eos(); });
		}
		// only return false if nothing available and eos is reached
		return !(!available() && eos());
	}

}