#include "GstAudioBackend.h"
#include <gst/app/gstappsrc.h>
#include <memory>
#include <iostream>

GST_DEBUG_CATEGORY (playbin2); 
#define GST_CAT_DEFAULT playbin2 

#define APP_SRC_CAPS "audio/mpeg" 
#define CHUNK_SIZE 1024 * 10   /* Amount of bytes we are sending in each buffer */ 

using namespace AudioX;

GstAudioBackend::GstAudioBackend() : m_sourceid(0), m_offset(0) {
	init();
}

GstAudioBackend::~GstAudioBackend() {
    stop();
    std::lock_guard<std::mutex> lock(m_handlerMutex); // be sure the handler is not running when we connect/disconnect it
    g_main_loop_quit(m_mainLoop);
    m_mainThread.join();
    gst_object_unref (m_pipeline); 
}

void GstAudioBackend::start(std::weak_ptr<Stream> source) {
	m_source = source;
	gst_element_set_state (m_pipeline, GST_STATE_PLAYING);
}

void GstAudioBackend::stop() {
	gst_element_set_state (m_pipeline, GST_STATE_NULL); 
	m_source.reset();
}

void GstAudioBackend::pause() {
	gst_element_set_state (m_pipeline, GST_STATE_PAUSED); 
}

void GstAudioBackend::seek(uint64_t off) {
	gst_element_seek_simple(m_pipeline, GST_FORMAT_BYTES, GST_SEEK_FLAG_FLUSH, (gint64)off);
}

/*** Private section ***/

void GstAudioBackend::init() {
    GST_DEBUG_CATEGORY_INIT (playbin2, "AudioBackend", 0, "GStreamer Audio Backend");
      
	//gst_parse_launch ("playbin uri=appsrc://", NULL);
	m_pipeline = gst_element_factory_make ("playbin", NULL);
	g_object_set (m_pipeline, "uri", "appsrc://", NULL);
	
	g_signal_connect (m_pipeline, "deep-notify::source", G_CALLBACK (GstAudioBackend::on_source_setup), this);
	//g_signal_connect (data.pipeline, "source-setup", G_CALLBACK (on_source_setup), &data);

	/* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
	GstBus* bus = gst_element_get_bus (m_pipeline);
	gst_bus_add_signal_watch (bus);
	g_signal_connect (G_OBJECT (bus), "message::error", G_CALLBACK(on_error_cb), this);
	gst_object_unref (bus);

	m_mainLoop = g_main_loop_new (NULL, FALSE);
	m_mainThread = std::move(std::thread([this](){ g_main_loop_run (m_mainLoop); }));
      
}

/* GStreamer callbacks */

void GstAudioBackend::on_source_setup(GObject * object, GObject * orig, GParamSpec * pspec, GstAudioBackend * thiz) {
	GST_DEBUG ("got appsrc %p", thiz->m_appsrc);

	g_object_get (orig, pspec->name, &thiz->m_appsrc, NULL);
	gst_util_set_object_arg (G_OBJECT (thiz->m_appsrc), "stream-type", "seekable");

	g_signal_connect (thiz->m_appsrc, "need-data", 	G_CALLBACK (GstAudioBackend::on_start_feed), thiz);
	g_signal_connect (thiz->m_appsrc, "enough-data", 	G_CALLBACK (GstAudioBackend::on_stop_feed),  thiz);
	g_signal_connect (thiz->m_appsrc, "seek-data", 	G_CALLBACK (GstAudioBackend::seek_cb), 		 thiz);
	g_object_set 	 (thiz->m_appsrc, "size", 		(gint64)2000000, NULL);
}

void GstAudioBackend::on_start_feed(GstElement *source, guint size, GstAudioBackend *thiz) {
	std::lock_guard<std::mutex> lock(thiz->m_handlerMutex); // be sure no handler is not running when we connect/disconnect it
    if (thiz->m_sourceid == 0) { 
      g_print ("Start feeding\n"); 
      thiz->m_sourceid = g_idle_add ((GSourceFunc) GstAudioBackend::on_push_data, thiz); 
    }
}

void GstAudioBackend::on_stop_feed(GstElement *source, GstAudioBackend *thiz) {
	std::lock_guard<std::mutex> lock(thiz->m_handlerMutex); // be sure no handler is not running when we connect/disconnect it
	if (thiz->m_sourceid != 0) { 
		g_print ("Stop feeding\n"); 
		g_source_remove (thiz->m_sourceid); 
		thiz->m_sourceid = 0; 
	} 
}

void GstAudioBackend::on_error_cb(GstBus *bus, GstMessage *msg, GstAudioBackend *thiz) {
    GError *err; 
    gchar *debug_info; 
    
    /* Print error details on the screen */ 
    gst_message_parse_error (msg, &err, &debug_info); 
    g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message); 
    g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none"); 
    g_clear_error (&err); 
    g_free (debug_info); 
    
    g_main_loop_quit (thiz->m_mainLoop); 
}

gboolean GstAudioBackend::seek_cb(GstElement* appsrc, guint64 position, GstAudioBackend* thiz) {
//    thiz->m_offset = position;
    return TRUE;
}

// helper callback to handle data transfers from the source to appsrc
gboolean GstAudioBackend::on_push_data(GstAudioBackend* thiz) {
	std::lock_guard<std::mutex> lock(thiz->m_handlerMutex);
    GstBuffer *buffer; 
    GstFlowReturn ret; 

    if (auto source = thiz->m_source.lock()) {
	    // Read stream from a file into GstBuffer 
	    // if (!data->fileStream.read((char*)map.data, CHUNK_SIZE)) 
		const size_t numBytes = source->available();
	    if (numBytes >= CHUNK_SIZE) {
			source->streamInfo().print();
			GstMapInfo map; 

			/* Create a new empty buffer */ 
			buffer = gst_buffer_new_and_alloc (numBytes); 
			gst_buffer_map (buffer, &map, GST_MAP_WRITE);

			const guint64 offset = source->pos();
			size_t n = source->read((char*)map.data, numBytes);
			GST_BUFFER_OFFSET (buffer) = offset;
			GST_BUFFER_OFFSET_END (buffer) = offset + n;

			/* Push the buffer into the appsrc */ 
			g_signal_emit_by_name (thiz->m_appsrc, "push-buffer", buffer, &ret); 
			/* Free the buffer now that we are done with it */ 
			gst_buffer_unref (buffer); 

			if (ret != GST_FLOW_OK) { 
				/* We got some error, stop sending data */ 
				g_printerr ("Error while pushing buffer: %d\n", ret); 
				return FALSE; 
			} 
			return TRUE; 
	    }  else if (source->eos() && source->available() == 0) {
			std::cout << "Emitting EOS\n";
			/* we are EOS, send end-of-stream */
			g_signal_emit_by_name (thiz->m_appsrc, "end-of-stream", &ret);
			return FALSE;      
	    } else {
			// std::cout << "Not enough data. " << data->httpStream->available() << "/" << CHUNK_SIZE << "\n";
			return TRUE;
	    }
	} else {
			std::cout << "Emitting EOS because source stream is gone.\n";
			g_signal_emit_by_name (thiz->m_appsrc, "end-of-stream", &ret);
			return FALSE;      
	}
}