/* GStreamer
 * Copyright (C) 2005 Andy Wingo <wingo@pobox.com>
 *
 * simple_launch_lines.c: Unit test for simple pipelines
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gst/check/gstcheck.h>

#ifndef GST_DISABLE_PARSE

static GstElement *
setup_pipeline (const gchar * pipe_descr)
{
  GstElement *pipeline;

  pipeline = gst_parse_launch (pipe_descr, NULL);
  g_return_val_if_fail (GST_IS_PIPELINE (pipeline), NULL);
  return pipeline;
}

/* 
 * run_pipeline:
 * @pipe: the pipeline to run
 * @desc: the description for use in messages
 * @events: is a mask of expected events
 * @tevent: is the expected terminal event.
 *
 * the poll call will time out after half a second.
 */
static void
run_pipeline (GstElement * pipe, const gchar * descr,
    GstMessageType events, GstMessageType tevent, GstState target_state)
{
  GstBus *bus;
  GstMessage *message;
  GstMessageType revent;
  GstStateChangeReturn ret;

  g_assert (pipe);
  bus = gst_element_get_bus (pipe);
  g_assert (bus);

  fail_if (gst_element_set_state (pipe, target_state) ==
      GST_STATE_CHANGE_FAILURE, "Could not set pipeline %s to playing", descr);
  ret = gst_element_get_state (pipe, NULL, NULL, 10 * GST_SECOND);
  if (ret == GST_STATE_CHANGE_ASYNC) {
    g_critical ("Pipeline '%s' failed to go to PAUSED fast enough", descr);
    goto done;
  } else if ((ret != GST_STATE_CHANGE_SUCCESS)
      && (ret != GST_STATE_CHANGE_NO_PREROLL)) {
    g_critical ("Pipeline '%s' failed to go into PAUSED state (%s)", descr,
        gst_element_state_change_return_get_name (ret));
    goto done;
  }

  while (1) {
    message = gst_bus_poll (bus, GST_MESSAGE_ANY, GST_SECOND / 2);

    /* always have to pop the message before getting back into poll */
    if (message) {
      revent = GST_MESSAGE_TYPE (message);
      gst_message_unref (message);
    } else {
      revent = GST_MESSAGE_UNKNOWN;
    }

    if (revent == tevent) {
      break;
    } else if (revent == GST_MESSAGE_UNKNOWN) {
      g_critical ("Unexpected timeout in gst_bus_poll, looking for %d: %s",
          tevent, descr);
      break;
    } else if (revent & events) {
      continue;
    }
    g_critical
        ("Unexpected message received of type %d, '%s', looking for %d: %s",
        revent, gst_message_type_get_name (revent), tevent, descr);
  }

done:
  fail_if (gst_element_set_state (pipe, GST_STATE_NULL) ==
      GST_STATE_CHANGE_FAILURE, "Could not set pipeline %s to NULL", descr);
  gst_element_get_state (pipe, NULL, NULL, GST_CLOCK_TIME_NONE);
  gst_object_unref (pipe);

  gst_bus_set_flushing (bus, TRUE);
  gst_object_unref (bus);
}

GST_START_TEST (test_rtp_payloaders)
{
  const gchar *s;

  /* FIXME: going to playing would be nice, but thet leads to lot of failure */
  GstState target_state = GST_STATE_PAUSED;

  /* we use is-live here to avoid preroll */
#define PIPELINE_STRING(bufcount, bufsize, pl, dpl) "fakesrc is-live=true num-buffers=" bufcount " filltype=2 sizetype=2 sizemax=" bufsize " ! " pl " ! " dpl " ! fakesink"
#define DEFAULT_BUFCOUNT "5"
#define DEFAULT_BUFSIZE "16"

  s = PIPELINE_STRING (DEFAULT_BUFCOUNT, DEFAULT_BUFSIZE, "rtpilbcpay",
      "rtpilbcdepay");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);

  s = PIPELINE_STRING (DEFAULT_BUFCOUNT, DEFAULT_BUFSIZE, "rtpgsmpay",
      "rtpgsmdepay");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);

  /* This one needs a bit different buffer size than others or it doesn't work. */
  s = PIPELINE_STRING (DEFAULT_BUFCOUNT, "52", "rtpamrpay", "rtpamrdepay");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);

  s = PIPELINE_STRING (DEFAULT_BUFCOUNT, DEFAULT_BUFSIZE, "rtppcmapay",
      "rtppcmadepay");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);

  s = PIPELINE_STRING (DEFAULT_BUFCOUNT, DEFAULT_BUFSIZE, "rtppcmupay",
      "rtppcmudepay");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);

  s = PIPELINE_STRING (DEFAULT_BUFCOUNT, DEFAULT_BUFSIZE, "rtpmpapay",
      "rtpmpadepay");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);

  s = PIPELINE_STRING (DEFAULT_BUFCOUNT, DEFAULT_BUFSIZE, "rtph263ppay",
      "rtph263pdepay");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);

  s = PIPELINE_STRING (DEFAULT_BUFCOUNT, DEFAULT_BUFSIZE, "rtph263pay",
      "rtph263depay");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);

  s = PIPELINE_STRING (DEFAULT_BUFCOUNT, DEFAULT_BUFSIZE, "rtph264pay",
      "rtph264depay");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);

  s = PIPELINE_STRING (DEFAULT_BUFCOUNT, DEFAULT_BUFSIZE, "rtpL16pay",
      "rtpL16depay");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);

  s = PIPELINE_STRING (DEFAULT_BUFCOUNT, DEFAULT_BUFSIZE, "rtpmp2tpay",
      "rtpmp2tdepay");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);

  s = PIPELINE_STRING (DEFAULT_BUFCOUNT, DEFAULT_BUFSIZE, "rtpmp4vpay",
      "rtpmp4vdepay");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);

  s = PIPELINE_STRING (DEFAULT_BUFCOUNT, DEFAULT_BUFSIZE, "rtpmp4gpay",
      "rtpmp4gdepay");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);

  /* Cannot be tested with fakesrc becouse speex payloader requires a valid header?! */
  /*
     s = PIPELINE_STRING(DEFAULT_BUFCOUNT, DEFAULT_BUFSIZE, "rtpspeexpay", "rtpspeexdepay");
     run_pipeline (setup_pipeline (s), s,
     GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
     GST_MESSAGE_UNKNOWN, target_state);
   */

  s = PIPELINE_STRING (DEFAULT_BUFCOUNT, DEFAULT_BUFSIZE, "rtptheorapay",
      "rtptheoradepay");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);

  s = PIPELINE_STRING (DEFAULT_BUFCOUNT, DEFAULT_BUFSIZE, "rtpvorbispay",
      "rtpvorbisdepay");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);

  /*s = FAKESRC " ! ! rtpac3depay ! " FAKESINK */
  /*s = FAKESRC " ! ! asteriskh263 ! " FAKESINK; */
  /*s = FAKESRC " ! ! rtpmpvdepay ! " FAKESINK; */
  /*s = FAKESRC " ! ! rtpmp4adepay ! " FAKESINK; */
  /*s = FAKESRC " ! ! rtpsv3vdepay ! " FAKESINK; */
}

GST_END_TEST
GST_START_TEST (test_video_encoders_decoders)
{
  const gchar *s;
  GstState target_state = GST_STATE_PLAYING;

  /* no is-live on the source because we actually want to preroll since
   * run_pipeline only goes into PAUSED */
#define ENC_DEC_PIPELINE_STRING(bufcount, enc, dec) "videotestsrc num-buffers=" bufcount " ! " enc " ! " dec " ! fakesink"
#define DEFAULT_BUFCOUNT "5"

  s = ENC_DEC_PIPELINE_STRING (DEFAULT_BUFCOUNT, "jpegenc", "jpegdec");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);

  s = ENC_DEC_PIPELINE_STRING (DEFAULT_BUFCOUNT, "pngenc", "pngdec");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);

  s = ENC_DEC_PIPELINE_STRING (DEFAULT_BUFCOUNT, "smokeenc", "smokedec");
  run_pipeline (setup_pipeline (s), s,
      GST_MESSAGE_ANY & ~(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING),
      GST_MESSAGE_UNKNOWN, target_state);
}

GST_END_TEST
#endif /* #ifndef GST_DISABLE_PARSE */
static Suite *
simple_launch_lines_suite (void)
{
  Suite *s = suite_create ("Pipelines");
  TCase *tc_chain = tcase_create ("linear");

  /* time out after 60s, not the default 3 */
  tcase_set_timeout (tc_chain, 60);

  suite_add_tcase (s, tc_chain);
#ifndef GST_DISABLE_PARSE
  tcase_add_test (tc_chain, test_rtp_payloaders);
  tcase_add_test (tc_chain, test_video_encoders_decoders);
#endif
  return s;
}

int
main (int argc, char **argv)
{
  int nf;

  Suite *s = simple_launch_lines_suite ();
  SRunner *sr = srunner_create (s);

  gst_check_init (&argc, &argv);

  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  return nf;
}
