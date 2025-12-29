/*
 * GStreamer - DTMF PIN Source Detection Plugin for the Repeater Project
 *
 * Copyright 2025 Robert Hensel VK3DG vk3dg@gmail.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * SECTION:element-dtmfpinsrc
 * @title: dtmfpinsrc
 * @short_description: Detects DTMF PIN codes and emits function names
 *
 * This element will detect DTMF tones, match them against configured PIN codes,
 * and emit messages with the corresponding function names when a valid PIN is entered.
 * It supports inter-digit timeout and entry timeout, with bus message emission.
 *
 * The plugin emits GStreamer bus messages for PIN detection events:
 *
 * * gchar `pin`: The detected PIN code
 * * gchar `function`: The function name associated with the PIN
 * * gboolean `valid`: Whether the PIN was valid
 *
 * Properties:
 * * gchar `config-file`: Path to the PIN configuration file
 * * guint `inter-digit-timeout`: Timeout between digits in milliseconds (default: 3000)
 * * guint `entry-timeout`: Timeout for complete PIN entry in milliseconds (default: 10000)
 * * gboolean `pass-through`: Allow input audio to pass through to output (default: FALSE)
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstdtmfpinsrc.h"

#include <string.h>
#include <time.h>
#include <gst/audio/audio.h>
#include <unistd.h>

GST_DEBUG_CATEGORY (dtmf_pin_src_debug);
#define GST_CAT_DEFAULT (dtmf_pin_src_debug)

/* Pad templates - input accepts 8000Hz for spandsp compatibility */
static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (S16) ", "
        "rate = (int) 8000, "
        "channels = (int) { 1, 2 }")
    );

/* Output pad can handle multiple rates for flexibility */
static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (S16) ", "
        "rate = (int) { 8000, 44100, 48000 }, "
        "channels = (int) { 1, 2 }")
    );

/* Properties */
enum
{
  PROP_0,
  PROP_CONFIG_FILE,
  PROP_INTER_DIGIT_TIMEOUT,
  PROP_ENTRY_TIMEOUT,
  PROP_PASS_THROUGH
};

static void gst_dtmf_pin_src_finalize (GObject * object);
static void gst_dtmf_pin_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_dtmf_pin_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_dtmf_pin_src_set_caps (GstBaseTransform * trans,
    GstCaps * incaps, GstCaps * outcaps);
static GstFlowReturn gst_dtmf_pin_src_transform_ip (GstBaseTransform * trans,
    GstBuffer * buf);
static gboolean gst_dtmf_pin_src_sink_event (GstBaseTransform * trans,
    GstEvent * event);

static gboolean load_pin_config (GstDtmfPinSrc * self, const gchar * filename);
static void reset_pin_entry (GstDtmfPinSrc * self);
static gboolean check_pin_match (GstDtmfPinSrc * self);
static void emit_pin_detected_message (GstDtmfPinSrc * self, const gchar * pin,
    const gchar * function, gboolean valid);

static gboolean check_timeouts_continuously (GstDtmfPinSrc * self);
static void start_timeout_checking (GstDtmfPinSrc * self);
static void stop_timeout_checking (GstDtmfPinSrc * self);
static void gst_dtmf_pin_src_state_reset (GstDtmfPinSrc * self);
static void process_dtmf_digit (GstDtmfPinSrc * self, gchar digit);

G_DEFINE_TYPE (GstDtmfPinSrc, gst_dtmf_pin_src, GST_TYPE_BASE_TRANSFORM);

/* Element class initialization */
static void
gst_dtmf_pin_src_class_init (GstDtmfPinSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseTransformClass *gstbasetransform_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasetransform_class = (GstBaseTransformClass *) klass;

  gobject_class->finalize = gst_dtmf_pin_src_finalize;
  gobject_class->set_property = gst_dtmf_pin_src_set_property;
  gobject_class->get_property = gst_dtmf_pin_src_get_property;

  gstbasetransform_class->set_caps = GST_DEBUG_FUNCPTR (gst_dtmf_pin_src_set_caps);
  gstbasetransform_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_dtmf_pin_src_transform_ip);
  gstbasetransform_class->sink_event =
      GST_DEBUG_FUNCPTR (gst_dtmf_pin_src_sink_event);

  /* Install properties */
  g_object_class_install_property (gobject_class, PROP_CONFIG_FILE,
      g_param_spec_string ("config-file", "Config File",
          "Path to the PIN configuration file", "codes.pin",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_INTER_DIGIT_TIMEOUT,
      g_param_spec_uint ("inter-digit-timeout", "Inter-Digit Timeout",
          "Timeout between DTMF digits in milliseconds", 1000, 60000, 3000,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_ENTRY_TIMEOUT,
      g_param_spec_uint ("entry-timeout", "Entry Timeout",
          "Timeout for complete PIN entry in milliseconds", 1000, 60000, 10000,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PASS_THROUGH,
      g_param_spec_boolean ("pass-through", "Pass Through",
          "Allow input audio to pass through to output", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /* Add pad templates */
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sinktemplate));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&srctemplate));

  /* Set element details */
  gst_element_class_set_static_metadata (gstelement_class,
      "DTMF PIN Detection", "Filter/Analyzer/Audio",
      "Detects DTMF tones and compares the tone char against a list of pins in codes.pin file. Emits function name if pin is valid via bus messages",
      "DTMF PIN Detection Plugin <http://github.com/TVforME/gstreamer/gstdtmfpinsrc>");
}

/* Element initialization */
static void
gst_dtmf_pin_src_init (GstDtmfPinSrc * self)
{
  gst_base_transform_set_passthrough (GST_BASE_TRANSFORM (self), TRUE);
  gst_base_transform_set_gap_aware (GST_BASE_TRANSFORM (self), TRUE);

  /* Initialize DTMF state */
  self->dtmf_state = NULL;

  /* Initialize PIN configuration */
  self->pin_count = 0;
  self->config_file = g_strdup ("codes.pin");

  /* Initialize PIN entry state */
  memset (self->pin_buffer, 0, sizeof (self->pin_buffer));
  self->pin_position = 0;

  /* Initialize timers */
  self->inter_digit_timer = g_timer_new ();
  self->entry_timer = g_timer_new ();
  self->last_digit_timer = g_timer_new ();
  self->last_digit_interval = 0.0;

  /* Set default timeouts */
  self->inter_digit_timeout = 3000;    /* 3 seconds */
  self->entry_timeout = 10000;         /* 10 seconds */
  self->timeout_check_id = 0;

  /* Initialize pass-through (disabled by default) */
  self->pass_through = FALSE;

  /* Start continuous timeout checking */
  start_timeout_checking (self);

  /* Load default PIN configuration */
  load_pin_config (self, self->config_file);

  /* Start timers */
  g_timer_start (self->inter_digit_timer);
  g_timer_start (self->entry_timer);
}

/* Finalize */
static void
gst_dtmf_pin_src_finalize (GObject * object)
{
  GstDtmfPinSrc *self = GST_DTMF_PIN_SRC (object);

  if (self->dtmf_state)
    dtmf_rx_free (self->dtmf_state);

  if (self->config_file)
    g_free (self->config_file);

  if (self->inter_digit_timer)
    g_timer_destroy (self->inter_digit_timer);

  if (self->entry_timer)
    g_timer_destroy (self->entry_timer);

  if (self->last_digit_timer)
    g_timer_destroy (self->last_digit_timer);

  /* Stop timeout checking */
  stop_timeout_checking (self);

  G_OBJECT_CLASS (gst_dtmf_pin_src_parent_class)->finalize (object);
}

/* Property setter */
static void
gst_dtmf_pin_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDtmfPinSrc *self = GST_DTMF_PIN_SRC (object);

  switch (prop_id) {
    case PROP_CONFIG_FILE:
      if (self->config_file)
        g_free (self->config_file);
      self->config_file = g_value_dup_string (value);
      load_pin_config (self, self->config_file);
      break;
    case PROP_INTER_DIGIT_TIMEOUT:
      self->inter_digit_timeout = g_value_get_uint (value);
      break;
    case PROP_ENTRY_TIMEOUT:
      self->entry_timeout = g_value_get_uint (value);
      break;
    case PROP_PASS_THROUGH:
      self->pass_through = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* Property getter */
static void
gst_dtmf_pin_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstDtmfPinSrc *self = GST_DTMF_PIN_SRC (object);

  switch (prop_id) {
    case PROP_CONFIG_FILE:
      g_value_set_string (value, self->config_file);
      break;
    case PROP_INTER_DIGIT_TIMEOUT:
      g_value_set_uint (value, self->inter_digit_timeout);
      break;
    case PROP_ENTRY_TIMEOUT:
      g_value_set_uint (value, self->entry_timeout);
      break;
    case PROP_PASS_THROUGH:
      g_value_set_boolean (value, self->pass_through);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* Set caps */
static gboolean
  gst_dtmf_pin_src_set_caps (GstBaseTransform * trans,
      GstCaps * incaps, GstCaps * outcaps)
{
  GstDtmfPinSrc *self = GST_DTMF_PIN_SRC (trans);
  GstStructure *s;
  gint rate, channels;
  gboolean success = TRUE;

  /* Log caps information for debugging */
  GST_DEBUG_OBJECT (self, "Input caps: %" GST_PTR_FORMAT, incaps);
  GST_DEBUG_OBJECT (self, "Output caps: %" GST_PTR_FORMAT, outcaps);

  /* Extract and verify input caps parameters */
  if (incaps) {
    s = gst_caps_get_structure (incaps, 0);
    if (s) {
      if (gst_structure_get_int (s, "rate", &rate)) {
        GST_DEBUG_OBJECT (self, "Input sample rate: %d Hz", rate);
        /* Verify sample rate is 8000Hz for proper DTMF detection */
        if (rate != 8000) {
          GST_WARNING_OBJECT (self, "Sample rate is %d Hz, 8000 Hz is recommended for DTMF", rate);
        }
      }
      if (gst_structure_get_int (s, "channels", &channels)) {
        GST_DEBUG_OBJECT (self, "Input channels: %d", channels);
      }
    }
  }

  /* Extract and verify output caps parameters */
  if (outcaps) {
    s = gst_caps_get_structure (outcaps, 0);
    if (s) {
      if (gst_structure_get_int (s, "rate", &rate)) {
        GST_DEBUG_OBJECT (self, "Output sample rate: %d Hz", rate);
      }
      if (gst_structure_get_int (s, "channels", &channels)) {
        GST_DEBUG_OBJECT (self, "Output channels: %d", channels);
      }
    }
  }

  /* Initialize DTMF state if not already done */
  if (!self->dtmf_state) {
    self->dtmf_state = dtmf_rx_init (NULL, NULL, NULL);
    if (!self->dtmf_state) {
      GST_ERROR_OBJECT (self, "Failed to initialize DTMF detector");
      success = FALSE;
    } else {
      GST_DEBUG_OBJECT (self, "DTMF detector initialized");
    }
  }

  return success;
}
/* Transform in-place */
static GstFlowReturn
gst_dtmf_pin_src_transform_ip (GstBaseTransform * trans, GstBuffer * buf)
{
  GstDtmfPinSrc *self = GST_DTMF_PIN_SRC (trans);
  gint dtmf_count;
  gchar dtmfbuf[MAX_DTMF_DIGITS] = "";
  gint i;
  GstMapInfo map;

  if (GST_BUFFER_IS_DISCONT (buf))
    gst_dtmf_pin_src_state_reset (self);
  if (GST_BUFFER_FLAG_IS_SET (buf, GST_BUFFER_FLAG_GAP))
    return GST_FLOW_OK;

  gst_buffer_map (buf, &map, GST_MAP_READ);

  dtmf_rx (self->dtmf_state, (gint16 *) map.data, map.size / 2);

  dtmf_count = dtmf_rx_get (self->dtmf_state, dtmfbuf, MAX_DTMF_DIGITS);

  if (dtmf_count) {
    GST_DEBUG_OBJECT (self, "Got %d DTMF events: %s", dtmf_count, dtmfbuf);
  } else {
    GST_LOG_OBJECT (self, "Got no DTMF events");
  }

  gst_buffer_unmap (buf, &map);

  /* Process each DTMF digit */
  for (i = 0; i < dtmf_count; i++) {
    process_dtmf_digit (self, dtmfbuf[i]);
  }

  /* Handle pass-through */
  if (!self->pass_through) {
    /* If pass-through is disabled, replace audio with silence */
    GstMapInfo map_info;
    
    if (gst_buffer_map (buf, &map_info, GST_MAP_WRITE)) {
      memset (map_info.data, 0, map_info.size);
      gst_buffer_unmap (buf, &map_info);
    }
    
    return GST_FLOW_OK;
  }
  
  /* Pass-through enabled - let audio continue */
  return GST_FLOW_OK;
}

/* Sink event handler */
static gboolean
gst_dtmf_pin_src_sink_event (GstBaseTransform * trans, GstEvent * event)
{
  GstDtmfPinSrc *self = GST_DTMF_PIN_SRC (trans);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
      gst_dtmf_pin_src_state_reset (self);
      break;
    default:
      break;
  }

  return GST_BASE_TRANSFORM_CLASS (gst_dtmf_pin_src_parent_class)->sink_event (trans, event);
}

/* Load PIN configuration from file */
static gboolean
load_pin_config (GstDtmfPinSrc * self, const gchar * filename)
{
  FILE *file;
  gchar line[512];
  gint line_num = 0;

  file = fopen (filename, "r");
  if (!file) {
    GST_WARNING_OBJECT (self, "Could not open PIN config file: %s", filename);
    return FALSE;
  }

  self->pin_count = 0;

  while (fgets (line, sizeof (line), file) && self->pin_count < MAX_PINS) {
    line_num++;

    /* Remove trailing newline */
    line[strcspn (line, "\r\n")] = 0;

    /* Skip empty lines and comments */
    if (line[0] == '\0' || line[0] == ';')
      continue;

    /* Parse PIN=function format */
    gchar *equal = strchr (line, '=');
    if (!equal) {
      GST_WARNING_OBJECT (self, "Invalid line %d: missing '='", line_num);
      continue;
    }

    *equal = '\0';
    gchar *pin = g_strstrip (line);
    gchar *function = g_strstrip (equal + 1);

    if (strlen (pin) == 0 || strlen (function) == 0) {
      GST_WARNING_OBJECT (self, "Invalid line %d: empty PIN or function", line_num);
      continue;
    }

    if (strlen (pin) > MAX_PIN_LENGTH) {
      GST_WARNING_OBJECT (self, "Line %d: PIN too long (max %d)", line_num,
          MAX_PIN_LENGTH);
      continue;
    }

    strncpy (self->pins[self->pin_count].pin, pin, MAX_PIN_LENGTH);
    strncpy (self->pins[self->pin_count].function, function, 255);
    self->pins[self->pin_count].function[255] = '\0';
    self->pin_count++;

    GST_INFO_OBJECT (self, "Loaded PIN: %s -> %s", pin, function);
  }

  fclose (file);
  GST_INFO_OBJECT (self, "Loaded %d PIN codes from %s", self->pin_count, filename);
  return TRUE;
}

/* Reset PIN entry state */
static void
reset_pin_entry (GstDtmfPinSrc * self)
{
  memset (self->pin_buffer, 0, sizeof (self->pin_buffer));
  self->pin_position = 0;
  g_timer_start (self->inter_digit_timer);
  g_timer_start (self->entry_timer);
  GST_DEBUG_OBJECT (self, "PIN entry reset");
}

/* Check if current PIN buffer matches any configured PIN */
static gboolean
check_pin_match (GstDtmfPinSrc * self)
{
  gint i;

  for (i = 0; i < self->pin_count; i++) {
    if (strcmp (self->pin_buffer, self->pins[i].pin) == 0) {
      GST_INFO_OBJECT (self, "PIN matched: %s -> %s", self->pin_buffer,
          self->pins[i].function);
      emit_pin_detected_message (self, self->pin_buffer,
          self->pins[i].function, TRUE);
      return TRUE;
    }
  }

  GST_INFO_OBJECT (self, "No match for PIN: %s", self->pin_buffer);
  emit_pin_detected_message (self, self->pin_buffer, NULL, FALSE);
  return FALSE;
}

/* Emit bus message for PIN detection */
static void
emit_pin_detected_message (GstDtmfPinSrc * self, const gchar * pin,
    const gchar * function, gboolean valid)
{
  GstStructure *structure;
  GstMessage *message;

  structure = gst_structure_new ("pin-detected", "pin", G_TYPE_STRING, pin,
      "function", G_TYPE_STRING, function ? function : "", "valid",
      G_TYPE_BOOLEAN, valid, NULL);

  message = gst_message_new_element (GST_OBJECT (self), structure);
  gst_element_post_message (GST_ELEMENT (self), message);

  GST_DEBUG_OBJECT (self, "Emitted pin-detected message: pin=%s function=%s valid=%d",
      pin, function ? function : "", valid);
}

/* Process a single DTMF digit */
static void
process_dtmf_digit (GstDtmfPinSrc * self, gchar digit)
{
  gdouble elapsed;

  /* Update timing tracking */
  if (self->last_digit_timer) {
    elapsed = g_timer_elapsed (self->last_digit_timer, NULL) * 1000.0;
    self->last_digit_interval = elapsed;
    g_timer_start (self->last_digit_timer);
  }

  GST_DEBUG_OBJECT (self, "Processing digit: %c (current buffer: '%s')", digit,
      self->pin_buffer);

  /* Add digit to buffer if there's space */
  if (self->pin_position < PIN_BUFFER_SIZE - 1) {
    self->pin_buffer[self->pin_position++] = digit;
    self->pin_buffer[self->pin_position] = '\0';

    /* Check if this matches any PIN */
    if (check_pin_match (self)) {
      /* PIN matched - reset buffer */
      reset_pin_entry (self);
    } else {
      /* No match - keep accumulating */
      g_timer_start (self->inter_digit_timer);
    }
  } else {
    /* Buffer full - reset */
    GST_WARNING_OBJECT (self, "PIN buffer full, resetting");
    reset_pin_entry (self);
  }
}

/* Continuous timeout checking */
static gboolean
check_timeouts_continuously (GstDtmfPinSrc * self)
{
  gdouble inter_digit_elapsed, entry_elapsed;

  if (!self->inter_digit_timer || !self->entry_timer)
    return TRUE;

  inter_digit_elapsed = g_timer_elapsed (self->inter_digit_timer, NULL) * 1000.0;
  entry_elapsed = g_timer_elapsed (self->entry_timer, NULL) * 1000.0;

  /* Check inter-digit timeout */
  if (self->pin_position > 0 && inter_digit_elapsed >= self->inter_digit_timeout) {
    GST_INFO_OBJECT (self,
        "Inter-digit timeout: %.0fms >= %ums (PIN: '%s')", inter_digit_elapsed,
        self->inter_digit_timeout, self->pin_buffer);
    
    /* Emit timeout message if there's a partial PIN */
    if (strlen (self->pin_buffer) > 0) {
      emit_pin_detected_message (self, self->pin_buffer, NULL, FALSE);
    }
    
    reset_pin_entry (self);
  }

  /* Check entry timeout */
  if (entry_elapsed >= self->entry_timeout) {
    GST_INFO_OBJECT (self, "Entry timeout: %.0fms >= %ums", entry_elapsed,
        self->entry_timeout);
    reset_pin_entry (self);
  }

  return TRUE;
}

/* Start timeout checking */
static void
start_timeout_checking (GstDtmfPinSrc * self)
{
  if (!self->timeout_check_id) {
    self->timeout_check_id = g_timeout_add (100,
        (GSourceFunc) check_timeouts_continuously, self);
    GST_DEBUG_OBJECT (self, "Started continuous timeout checking");
  }
}

/* Stop timeout checking */
static void
stop_timeout_checking (GstDtmfPinSrc * self)
{
  if (self->timeout_check_id) {
    g_source_remove (self->timeout_check_id);
    self->timeout_check_id = 0;
    GST_DEBUG_OBJECT (self, "Stopped continuous timeout checking");
  }
}

/* State reset helper */
static void
gst_dtmf_pin_src_state_reset (GstDtmfPinSrc * self)
{
  reset_pin_entry (self);
  if (self->dtmf_state) {
    dtmf_rx_release (self->dtmf_state);
    self->dtmf_state = dtmf_rx_init (NULL, NULL, NULL);
  }
}

/* Plugin initialization */
static gboolean
plugin_init (GstPlugin * plugin)
{
#ifdef HAVE_CONFIG_H
  GST_INFO ("DTMFPINSRC Plugin - Built on %s at %s", BUILD_DATE, BUILD_TIME);
#endif

  return gst_element_register (plugin, "dtmfpinsrc", GST_RANK_NONE,
      GST_TYPE_DTMF_PIN_SRC);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    dtmfpinsrc,
    "DTMF Pin compare and execute Plugin",
    plugin_init, VERSION, "LGPL", PACKAGE, 
    "http://github.com/TVforME/gstreamer/gstdtmfpinsrc")
