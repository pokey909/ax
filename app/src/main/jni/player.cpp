#include "player.h"

#include <gst/app/gstappsrc.h>

#include <thread>
#include <fstream>

#include <iostream>

#include "Stream.h"
#include "Downloader.h"

#define STREAM_FILENAME "/Users/alexanderlenhardt/test.mp3" 
#define CHUNK_SIZE 1024 * 10   /* Amount of bytes we are sending in each buffer */ 
//#define APP_SRC_CAPS "video/mpegts,systemstream=true,packet-size=188" 
#define APP_SRC_CAPS "audio/mpeg" 

GST_DEBUG_CATEGORY (playbin2); 
#define GST_CAT_DEFAULT playbin2 

using namespace AudioX;

struct CustomData { 

    std::thread mainThread;
    
    GstElement *pipeline;
    GstElement *app_source; 

    std::shared_ptr<Stream> httpStream;

    std::ifstream fileStream;
    std::streampos fileLen;
    size_t totalBytesRead; 
    guint64 offset;

    guint sourceid;        /* To control the GSource */ 
    gulong need_data_id;
    gulong enough_data_id;

    GMainLoop *main_loop;  /* GLib's Main Loop */ 
    Player_impl* thiz;

    CustomData() :
      pipeline(NULL), app_source(NULL), fileLen(0),
      totalBytesRead(0), offset(0), sourceid(0), need_data_id(0),
      enough_data_id(0), main_loop(NULL), thiz(nullptr), httpStream(std::make_shared<Stream>()) {}
  };


class Player_impl {
  /* Structure to contain all our information, so we can pass it to callbacks */ 
 

  CustomData data;
  std::mutex m_handlerMutex;
  bool m_started;
  Downloader http;

public:
  Player_impl() : m_started(false) {
      GST_DEBUG_CATEGORY_INIT (playbin2, "pipeline", 0, "appsrc pipeline example");
      
      /* Build the pipeline */
      g_print("launch playbin2\n");
      data.pipeline = gst_parse_launch ("playbin uri=appsrc://", NULL);
      g_signal_connect (data.pipeline, "source-setup", G_CALLBACK (on_source_setup), &data);
      
      /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
      GstBus* bus = gst_element_get_bus (data.pipeline);
      gst_bus_add_signal_watch (bus);
      g_signal_connect (G_OBJECT (bus), "message::error", G_CALLBACK(on_error_cb), &data);
      gst_object_unref (bus);

      // http.newRequest("http://www.noiseaddicts.com/samples/3926.mp3", data.httpStream);
      // ("http://www.noiseaddicts.com/samples/3926.mp3");
      /* Create a GLib Main Loop and set it to run */

      g_print("create main loop\n");
      data.main_loop = g_main_loop_new (NULL, FALSE);
      data.mainThread = std::move(std::thread([this](){
          g_print("run main loop\n");
          g_main_loop_run (data.main_loop);
      }));
      
  }

  ~Player_impl() {
    std::lock_guard<std::mutex> lock(m_handlerMutex); // be sure the handler is not running when we connect/disconnect it
    stop();
    g_main_loop_quit(data.main_loop);
    data.mainThread.join();
    gst_object_unref (data.pipeline); 
  }

  void start(const std::string& uri) {
    stop();

    data.httpStream = std::make_shared<Stream>();
    data.httpStream->streamInfo().url = uri;

    g_print("connect appsrc signals\n");
    data.need_data_id = g_signal_connect (data.app_source, "need-data", G_CALLBACK (&Player_impl::on_start_feed), &data);
    data.enough_data_id = g_signal_connect (data.app_source, "enough-data", G_CALLBACK (&Player_impl::on_stop_feed), &data);
    g_signal_connect (data.app_source, "seek-data", G_CALLBACK (&Player_impl::seek_cb), &data);
    g_object_set (data.app_source, "size", (gint64) data.fileLen, NULL);

    /* Start playing the pipeline */
    g_print("pipeline set to PLAYING\n");
    
    data.httpStream = std::make_shared<Stream>();
    http.newRequest(uri, data.httpStream);

    gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
  }

  void stop() {
    g_print("pipeline STOPPED\n");
    http.cancel();
    gst_element_set_state (data.pipeline, GST_STATE_NULL); 
  }

  void seek(gint64 sec) {
    size_t off = (size_t)(0.5 * data.fileLen);
    gst_element_seek_simple(data.pipeline, GST_FORMAT_BYTES, GST_SEEK_FLAG_FLUSH, (gint64)off);
  }

protected:
  gboolean push_data (CustomData *data) { 
    GstBuffer *buffer; 
    GstFlowReturn ret; 

    // Read stream from a file into GstBuffer 
    // if (!data->fileStream.read((char*)map.data, CHUNK_SIZE)) 
    if (data->httpStream->available() >= CHUNK_SIZE) {
      data->httpStream->streamInfo().print();
      std::vector<char> buf;
      GstMapInfo map; 
  
      /* Create a new empty buffer */ 
      buffer = gst_buffer_new_and_alloc (CHUNK_SIZE); 
      gst_buffer_map (buffer, &map, GST_MAP_WRITE);

      size_t n = data->httpStream->read((char*)map.data, CHUNK_SIZE);
      GST_BUFFER_OFFSET (buffer) = data->offset;
      GST_BUFFER_OFFSET_END (buffer) = data->offset + n;
      data->offset += n;

      /* Push the buffer into the appsrc */ 
      g_signal_emit_by_name (data->app_source, "push-buffer", buffer, &ret); 
      
      /* Free the buffer now that we are done with it */ 
      gst_buffer_unref (buffer); 
      
      if (ret != GST_FLOW_OK) { 
        /* We got some error, stop sending data */ 
       g_printerr ("Error while pushing buffer: %d\n", ret); 
       return FALSE; 
      } 

      return TRUE; 
    }  else if (data->httpStream->eos() && data->httpStream->available() == 0) {
      std::cout << "Emitting EOS\n";
      /* we are EOS, send end-of-stream */
      g_signal_emit_by_name (data->app_source, "end-of-stream", &ret);
      return FALSE;      
    } else {
      // std::cout << "Not enough data. " << data->httpStream->available() << "/" << CHUNK_SIZE << "\n";
      return TRUE;
    }
  }

private:
  /* This function is called when playbin2 has created the appsrc element, so we have 
   * a chance to configure it. */ 
  void source_setup (GstElement *pipeline, GstElement *source, CustomData *cdata) { 
    GstCaps *appsrc_caps; 
    g_print ("Source has been created. Configuring.\n"); 
    data.app_source = source; 
    gst_util_set_object_arg (G_OBJECT (data.app_source), "stream-type", "seekable");
    g_object_set (G_OBJECT (data.app_source), "format", GST_FORMAT_BYTES, NULL);
    // g_object_set (G_OBJECT (data.app_source), "min-latency", 0,
    //   "max-latency", 1, NULL);
  }


  /* This signal callback triggers when appsrc needs data. Here, we add an idle handler 
   * to the mainloop to start pushing data into the appsrc */ 
  void start_feed (GstElement *source, guint size, CustomData *data) { 
    if (data->sourceid == 0) { 
      g_print ("Start feeding\n"); 
      data->sourceid = g_idle_add ((GSourceFunc) Player_impl::on_push_data, data); 
    } 
  } 
    
 
  /* This callback triggers when appsrc has enough data and we can stop sending. 
   * We remove the idle handler from the mainloop */ 
  void stop_feed (GstElement *source, CustomData *data) { 
    if (data->sourceid != 0) { 
      g_print ("Stop feeding\n"); 
      g_source_remove (data->sourceid); 
      data->sourceid = 0; 
    } 
  } 

  /* This function is called when an error message is posted on the bus */ 
  void error_cb (GstBus *bus, GstMessage *msg, CustomData *data) { 
    GError *err; 
    gchar *debug_info; 
    
    /* Print error details on the screen */ 
    gst_message_parse_error (msg, &err, &debug_info); 
    g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message); 
    g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none"); 
    g_clear_error (&err); 
    g_free (debug_info); 
    
    g_main_loop_quit (data->main_loop); 
  } 

  static gboolean seek_cb(GstElement* appsrc, guint64 position, CustomData* app) {
    app->offset = position;
    return TRUE;
  }

  static gboolean on_push_data(CustomData *data) {
    return data->thiz->push_data(data);
  }

  static void on_source_setup(GstElement *pipeline, GstElement *source, CustomData *data) {
    data->thiz->source_setup(pipeline, source, data);
  }
  static void on_start_feed(GstElement *source, guint size, CustomData *data) {
    std::lock_guard<std::mutex> lock(data->thiz->m_handlerMutex); // be sure the handler is not running when we connect/disconnect it
    data->thiz->start_feed(source, size, data);
  }
  static void on_stop_feed(GstElement *source, CustomData *data) {
   std::lock_guard<std::mutex> lock(data->thiz->m_handlerMutex); // be sure the handler is not running when we connect/disconnect it
   data->thiz->stop_feed(source, data);
  }
  static void on_error_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
    data->thiz->error_cb(bus, msg, data);
  }

//  friend void on_source_setup(GstElement *pipeline, GstElement *source, CustomData *data);
//  friend void on_start_feed(GstElement *source, guint size, CustomData *data);
//  friend void on_stop_feed(GstElement *source, CustomData *data);
//  friend void on_error_cb(GstBus *bus, GstMessage *msg, CustomData *data);
};

Player::Player() : d(new Player_impl()) {
}

Player::~Player() {
  g_print("exiting\n"); 
}

void Player::play(std::string filename) {
  d->start(filename);
}

void Player::pause() {
  std::cout << "Not implemented\n";
}

void Player::stop() {
  d->stop();
}

void Player::seek(std::chrono::milliseconds ms) {
  d->seek(ms.count() * 1000);
}