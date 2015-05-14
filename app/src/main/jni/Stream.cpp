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

	void Stream::reset() {
		buf.clear();
		gptrNext = 0;
		gptrEnd = 0;
		eosFlag = false;
		m_info = StreamInfo();
	}

	size_t Stream::read(char* buffer, std::streamsize maxSize) {
		std::streamsize n = (std::min)(gptrEnd - gptrNext, maxSize);
		memcpy(buffer, &buf[gptrNext], n);
		gptrNext += n;
		return n;
	}
	void Stream::read(std::vector<char>& buffer) {
		buffer.insert(buffer.end(), &buf[0] + gptrNext, &buf[0] + gptrEnd);
		gptrNext = gptrEnd;
	}

	void Stream::write(const char* data, std::streamsize size) {
		buf.insert(buf.end(), data, data + size);
		gptrEnd += size;
		waitDataCond.notify_one();
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