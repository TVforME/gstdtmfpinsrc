/*
 * GStreamer - DTMF PIN Source Detection
 *
 * Copyright 2024 DTMF PIN Detection Plugin
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
 *
 */

#ifndef __GST_DTMF_PIN_SRC_H__
#define __GST_DTMF_PIN_SRC_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>

#include <spandsp.h>

G_BEGIN_DECLS

#define GST_TYPE_DTMF_PIN_SRC \
  (gst_dtmf_pin_src_get_type())
#define GST_DTMF_PIN_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), \
  GST_TYPE_DTMF_PIN_SRC,GstDtmfPinSrc))
#define GST_DTMF_PIN_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), \
  GST_TYPE_DTMF_PIN_SRC,GstDtmfPinSrcClass))
#define GST_IS_DTMF_PIN_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DTMF_PIN_SRC))
#define GST_IS_DTMF_PIN_SRC_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DTMF_PIN_SRC))

typedef struct _GstDtmfPinSrc GstDtmfPinSrc;
typedef struct _GstDtmfPinSrcClass GstDtmfPinSrcClass;

#define MAX_PIN_LENGTH 16
#define MAX_PINS 100
#define PIN_BUFFER_SIZE 64

typedef struct {
  gchar pin[MAX_PIN_LENGTH + 1];
  gchar function[256];
} PinEntry;

struct _GstDtmfPinSrc
{
  GstBaseTransform parent;

  /* DTMF detection state */
  dtmf_rx_state_t *dtmf_state;

  /* PIN configuration */
  PinEntry pins[MAX_PINS];
  gint pin_count;
  gchar *config_file;

  /* PIN entry state */
  gchar pin_buffer[PIN_BUFFER_SIZE];
  gint pin_position;

  /* Timeout handling */
  GTimer *inter_digit_timer;
  GTimer *entry_timer;
  GTimer *last_digit_timer;  /* Track timing of last DTMF digit */
  guint inter_digit_timeout;
  guint entry_timeout;
  guint timeout_check_id;
  gdouble last_digit_interval;  /* Time since last digit (ms) */

  /* Audio pass-through control */
  gboolean pass_through;
};

struct _GstDtmfPinSrcClass
{
  GstBaseTransformClass parent_class;
};

GType gst_dtmf_pin_src_get_type (void);

GST_ELEMENT_REGISTER_DECLARE (dtmfpinsrc);

G_END_DECLS

#endif /* __GST_DTMF_PIN_SRC_H__ */