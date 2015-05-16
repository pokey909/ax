#pragma once
#include "Stream.h"
#include <gst/gst.h>
#include <thread>
#include <mutex>
#include <memory>

namespace AudioX {

class GstAudioBackend {
public:
	GstAudioBackend();
	~GstAudioBackend();

	void start(std::weak_ptr<Stream> source);
	void stop();
	void pause();
	void seek(uint64_t offs);

private:
	void init();
	void releasePipeline();
	static void on_source_setup(GObject * object, GObject * orig, GParamSpec * pspec, GstAudioBackend * thiz);
	static void on_start_feed(GstElement *source, guint size, GstAudioBackend *thiz);
	static void on_stop_feed(GstElement *source, GstAudioBackend *thiz);
	static void on_error_cb(GstBus *bus, GstMessage *msg, GstAudioBackend *thiz);
	static gboolean seek_cb(GstElement* appsrc, guint64 position, GstAudioBackend* thiz);
	static gboolean on_push_data(GstAudioBackend *data);

    std::thread m_mainThread;
    std::mutex m_handlerMutex;
	std::weak_ptr<Stream> m_source;
    GstElement *m_pipeline;
    GstElement *m_appsrc; 
    GMainLoop *m_mainLoop;
    guint m_sourceid;
    uint64_t m_offset;
};

}
