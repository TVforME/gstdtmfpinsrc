/*
 * DTMF PIN Detection Test Program
 */

#include <gst/gst.h>
#include <glib.h>
#include <string.h>
#include <stdio.h>

static void execute_function(const gchar *function_name);
static gboolean unlock_front_door_func(void);
static gboolean activate_alarm_func(void);
static gboolean emergency_shutdown_func(void);
static gboolean test_mode_func(void);
static gboolean guest_access_func(void);
static gboolean admin_mode_func(void);
static gboolean reset_system_func(void);
static gboolean hash_test_pin_func(void);
static gboolean test_abcd_mode_func(void);
static gboolean mixed_digit_test_func(void);
static gboolean extended_pin_test_func(void);
static void pad_added_handler (GstElement * src, GstPad * new_pad, gpointer user_data);

typedef struct {
    const gchar *function_name;
    gboolean (*function)(void);
    const gchar *description;
} FunctionMapping;

static FunctionMapping function_map[] = {
    {"unlock_front_door", unlock_front_door_func, "Unlocks the front door"},
    {"activate_alarm", activate_alarm_func, "Activates the security alarm"},
    {"emergency_shutdown", emergency_shutdown_func, "Performs emergency shutdown"},
    {"test_mode", test_mode_func, "Enters test mode"},
    {"guest_access", guest_access_func, "Grants guest access"},
    {"admin_mode", admin_mode_func, "Enters admin mode"},
    {"reset_system", reset_system_func, "Resets the system"},
    {"hash_test_pin", hash_test_pin_func, "Tests hash PIN functionality"},
    {"test_abcd_mode", test_abcd_mode_func, "Tests ABCD DTMF digits"},
    {"mixed_digit_test", mixed_digit_test_func, "Tests mixed digit PINs"},
    {"extended_pin_test", extended_pin_test_func, "Tests extended PIN codes"},
    {NULL, NULL, NULL}
};

static gboolean
bus_call (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  GMainLoop *loop = (GMainLoop *) user_data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_ELEMENT:{
      const GstStructure *structure = gst_message_get_structure (msg);
      
      if (structure && gst_structure_has_name (structure, "pin-detected")) {
        const gchar *pin, *function;
        gboolean valid;

        if (gst_structure_get (structure,
                "pin", G_TYPE_STRING, &pin,
                "function", G_TYPE_STRING, &function,
                "valid", G_TYPE_BOOLEAN, &valid, NULL)) {
          
          if (valid) {
            g_print ("\n");
            g_print ("═════════════════════════════════════════════════════════════\n");
            g_print ("✅ VALID PIN DETECTED: %s -> %s\n", pin, function);
            g_print ("═════════════════════════════════════════════════════════════\n");
            execute_function(function);
          } else {
            g_print ("\n❌ INVALID PIN: %s\n", pin);
          }
        }
      }
      break;
    }

    case GST_MESSAGE_WARNING:{
      gchar *debug;
      GError *err;
      gst_message_parse_warning (msg, &err, &debug);
      g_printerr ("⚠️  WARNING: %s (%s)\n", err->message, debug);
      g_free (debug);
      g_error_free (err);
      break;
    }

    case GST_MESSAGE_ERROR:{
      gchar *debug;
      GError *err;
      gst_message_parse_error (msg, &err, &debug);
      g_printerr ("❌ ERROR: %s (%s)\n", err->message, debug);
      g_free (debug);
      g_error_free (err);
      g_main_loop_quit (loop);
      break;
    }

    case GST_MESSAGE_EOS:
      g_print ("\n🏁 End of stream reached\n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_STATE_CHANGED:{
      GstState old_state, new_state, pending_state;
      gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
      if (GST_MESSAGE_SRC (msg) == GST_OBJECT (user_data)) {
        g_print ("🎯 Pipeline state changed from %s to %s\n",
            gst_element_state_get_name (old_state),
            gst_element_state_get_name (new_state));
      }
      break;
    }

    default:
      break;
  }

  return TRUE;
}

static void
execute_function(const gchar *function_name)
{
    int i;
    gboolean found = FALSE;
    
    g_print("🔍 Looking for function: '%s'\n", function_name);
    
    for (i = 0; function_map[i].function_name != NULL; i++) {
        if (strcmp(function_map[i].function_name, function_name) == 0) {
            g_print("📋 Description: %s\n", function_map[i].description);
            g_print("⚡ Executing function...\n");
            if (function_map[i].function()) {
                g_print("✓ Function executed successfully\n");
            } else {
                g_print("✗ Function execution failed\n");
            }
            found = TRUE;
            break;
        }
    }
    
    if (!found) {
        g_print("⚠️  Warning: No function defined for '%s'\n", function_name);
    }
}

static gboolean unlock_front_door_func(void)
{
    g_print("  🚪 UNLOCKING FRONT DOOR...\n");
    g_print("  → Access granted\n");
    g_print("  → Door unlocked\n");
    return TRUE;
}

static gboolean activate_alarm_func(void)
{
    g_print("  🚨 ACTIVATING ALARM...\n");
    g_print("  → Security system armed\n");
    g_print("  → Alarm activated\n");
    return TRUE;
}

static gboolean emergency_shutdown_func(void)
{
    g_print("  🆘 EMERGENCY SHUTDOWN...\n");
    g_print("  → Stopping all services\n");
    g_print("  → System shutting down\n");
    return TRUE;
}

static gboolean test_mode_func(void)
{
    g_print("  🧪 ENTERING TEST MODE...\n");
    g_print("  → Test mode enabled\n");
    g_print("  → Diagnostics running\n");
    return TRUE;
}

static gboolean guest_access_func(void)
{
    g_print("  👤 GRANTING GUEST ACCESS...\n");
    g_print("  → Guest permissions granted\n");
    g_print("  → Limited access enabled\n");
    return TRUE;
}

static gboolean admin_mode_func(void)
{
    g_print("  🔑 ENTERING ADMIN MODE...\n");
    g_print("  → Admin privileges enabled\n");
    g_print("  → Full system access granted\n");
    return TRUE;
}

static gboolean reset_system_func(void)
{
    g_print("  🔄 RESETTING SYSTEM...\n");
    g_print("  → Clearing all buffers\n");
    g_print("  → System reset complete\n");
    return TRUE;
}

static gboolean hash_test_pin_func(void)
{
    g_print("  🔷 HASH PIN TEST...\n");
    g_print("  → Testing # digit functionality\n");
    g_print("  → Hash PIN working correctly\n");
    return TRUE;
}

static gboolean test_abcd_mode_func(void)
{
    g_print("  🔠 ABCD MODE TEST...\n");
    g_print("  → Testing extended DTMF digits\n");
    g_print("  → ABCD digits detected correctly\n");
    return TRUE;
}

static gboolean mixed_digit_test_func(void)
{
    g_print("  🔢 MIXED DIGIT TEST...\n");
    g_print("  → Testing numeric and alphabetic digits\n");
    g_print("  → Mixed PIN working correctly\n");
    return TRUE;
}

static gboolean extended_pin_test_func(void)
{
    g_print("  📏 EXTENDED PIN TEST...\n");
    g_print("  → Testing long PIN codes\n");
    g_print("  → Extended PIN detected correctly\n");
    return TRUE;
}

int
main (int argc, char *argv[])
{
  GMainLoop *loop;
  GstElement *pipeline, *source, *decoder, *converter, *resampler, *dtmfpinsrc, *sink;
  GstBus *bus;
  guint bus_watch_id;

  gst_init (&argc, &argv);

  if (argc != 3) {
    g_printerr ("\n");
    g_printerr ("╔══════════════════════════════════════════════════════════════╗\n");
    g_printerr ("║  DTMF PIN Detection Test Program                          ║\n");
    g_printerr ("╚══════════════════════════════════════════════════════════════╝\n");
    g_printerr ("\n");
    g_printerr ("Usage: %s <audio_file> <config_file>\n", argv[0]);
    g_printerr ("\n");
    g_printerr ("Arguments:\n");
    g_printerr ("  audio_file   - Path to WAV file with DTMF tones\n");
    g_printerr ("  config_file  - Path to PIN configuration file\n");
    g_printerr ("\n");
    g_printerr ("Example:\n");
    g_printerr ("  %s dtmf_test_complete.wav codes.pin\n", argv[0]);
    g_printerr ("\n");
    return -1;
  }

  loop = g_main_loop_new (NULL, FALSE);

  pipeline = gst_pipeline_new ("dtmf-test-pipeline");
  source = gst_element_factory_make ("filesrc", "file-source");
  decoder = gst_element_factory_make ("decodebin", "decoder");
  converter = gst_element_factory_make ("audioconvert", "converter");
  resampler = gst_element_factory_make ("audioresample", "resampler");
  dtmfpinsrc = gst_element_factory_make ("dtmfpinsrc", "dtmfpinsrc");
  sink = gst_element_factory_make ("autoaudiosink", "audio-output");

  if (!pipeline || !source || !decoder || !converter || !resampler || !dtmfpinsrc || !sink) {
    g_printerr ("❌ One element could not be created. Exiting.\n");
    return -1;
  }

  g_object_set (G_OBJECT (source), "location", argv[1], NULL);
  g_object_set (G_OBJECT (dtmfpinsrc), "config-file", argv[2], NULL);
  g_object_set (G_OBJECT (dtmfpinsrc), "pass-through", TRUE, NULL);

  gst_bin_add_many (GST_BIN (pipeline), source, decoder, converter, resampler, 
      dtmfpinsrc, sink, NULL);

  if (!gst_element_link (source, decoder) ||
      !gst_element_link (converter, resampler) ||
      !gst_element_link (resampler, dtmfpinsrc) ||
      !gst_element_link (dtmfpinsrc, sink)) {
    g_printerr ("❌ Elements could not be linked. Exiting.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  g_signal_connect (decoder, "pad-added", G_CALLBACK (pad_added_handler),
      converter);

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  g_print ("\n");
  g_print ("╔══════════════════════════════════════════════════════════════╗\n");
  g_print ("║  DTMF PIN Detection Test Program                          ║\n");
  g_print ("╚══════════════════════════════════════════════════════════════╝\n");
  g_print ("\n");
  g_print ("📁 Audio file: %s\n", argv[1]);
  g_print ("⚙️  Config file: %s\n", argv[2]);
  g_print ("\n");
  g_print ("🎧 Pass-through: ENABLED (you will hear the audio)\n");
  g_print ("🔊 Sample rate: 8000 Hz (required for DTMF detection)\n");
  g_print ("\n");
  g_print ("────────────────────────────────────────────────────────────────\n");
  g_print ("Press Ctrl+C to stop the test\n");
  g_print ("────────────────────────────────────────────────────────────────\n");
  g_print ("\n");

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  g_main_loop_run (loop);

  g_print ("\n");
  g_print ("────────────────────────────────────────────────────────────────\n");
  g_print ("Cleaning up...\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_source_remove (bus_watch_id);
  gst_object_unref (pipeline);
  g_main_loop_unref (loop);
  g_print ("✓ Cleanup complete\n");
  g_print ("────────────────────────────────────────────────────────────────\n");
  g_print ("\n");

  return 0;
}

static void
pad_added_handler (GstElement * src, GstPad * new_pad, gpointer user_data)
{
  GstPad *sink_pad;
  GstElement *converter = (GstElement *) user_data;
  GstPadLinkReturn ret;
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;
  const gchar *new_pad_type = NULL;

  g_print ("🔌 Received new pad '%s' from '%s'\n", GST_PAD_NAME (new_pad),
      GST_ELEMENT_NAME (src));

  new_pad_caps = gst_pad_get_current_caps (new_pad);
  new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
  new_pad_type = gst_structure_get_name (new_pad_struct);

  if (!g_str_has_prefix (new_pad_type, "audio/x-raw")) {
    g_print ("  → It has type '%s' which is not raw audio. Ignoring.\n", new_pad_type);
    if (new_pad_caps)
      gst_caps_unref (new_pad_caps);
    return;
  }

  sink_pad = gst_element_get_static_pad (converter, "sink");

  if (gst_pad_is_linked (sink_pad)) {
    g_print ("  → Sink pad is already linked. Ignoring.\n");
    gst_object_unref (sink_pad);
    if (new_pad_caps)
      gst_caps_unref (new_pad_caps);
    return;
  }

  ret = gst_pad_link (new_pad, sink_pad);
  if (GST_PAD_LINK_FAILED (ret)) {
    g_print ("  → Type is '%s' but link failed.\n", new_pad_type);
  } else {
    g_print ("  → Link succeeded (type '%s').\n", new_pad_type);
  }

  if (new_pad_caps)
    gst_caps_unref (new_pad_caps);
  gst_object_unref (sink_pad);
}
