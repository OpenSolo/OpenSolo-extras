/* GStreamer
 *
 * Common code for GStreamer unittests
 *
 * Copyright (C) 2004,2006 Thomas Vander Stichele <thomas at apestaart dot org>
 * Copyright (C) 2008 Thijs Vermeir <thijsvermeir@gmail.com>
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
/**
 * SECTION:gstcheck
 * @short_description: Common code for GStreamer unit tests
 *
 * These macros and functions are for internal use of the unit tests found
 * inside the 'check' directories of various GStreamer packages.
 */

#include "gstcheck.h"

GST_DEBUG_CATEGORY (check_debug);

/* logging function for tests
 * a test uses g_message() to log a debug line
 * a gst unit test can be run with GST_TEST_DEBUG env var set to see the
 * messages
 */

gboolean _gst_check_threads_running = FALSE;
GList *thread_list = NULL;
GMutex *mutex;
GCond *start_cond;              /* used to notify main thread of thread startups */
GCond *sync_cond;               /* used to synchronize all threads and main thread */

GList *buffers = NULL;
GMutex *check_mutex = NULL;
GCond *check_cond = NULL;

/* FIXME 0.11: shouldn't _gst_check_debug be static? Not used anywhere */
gboolean _gst_check_debug = FALSE;
gboolean _gst_check_raised_critical = FALSE;
gboolean _gst_check_raised_warning = FALSE;
gboolean _gst_check_expecting_log = FALSE;

static void gst_check_log_message_func
    (const gchar * log_domain, GLogLevelFlags log_level,
    const gchar * message, gpointer user_data)
{
  if (_gst_check_debug) {
    g_print ("%s", message);
  }
}

static void gst_check_log_critical_func
    (const gchar * log_domain, GLogLevelFlags log_level,
    const gchar * message, gpointer user_data)
{
  if (!_gst_check_expecting_log) {
    g_print ("\n\nUnexpected critical/warning: %s\n", message);
    fail ("Unexpected critical/warning: %s", message);
  }

  if (_gst_check_debug) {
    g_print ("\nExpected critical/warning: %s\n", message);
  }

  if (log_level & G_LOG_LEVEL_CRITICAL)
    _gst_check_raised_critical = TRUE;
  if (log_level & G_LOG_LEVEL_WARNING)
    _gst_check_raised_warning = TRUE;
}

static gint
sort_plugins (GstPlugin * a, GstPlugin * b)
{
  int ret;

  ret = strcmp (gst_plugin_get_source (a), gst_plugin_get_source (b));
  if (ret == 0)
    ret = strcmp (gst_plugin_get_name (a), gst_plugin_get_name (b));
  return ret;
}

static void
print_plugins (void)
{
  GList *plugins, *l;

  plugins = gst_default_registry_get_plugin_list ();
  plugins = g_list_sort (plugins, (GCompareFunc) sort_plugins);
  for (l = plugins; l != NULL; l = l->next) {
    GstPlugin *plugin = GST_PLUGIN (l->data);

    if (strcmp (gst_plugin_get_source (plugin), "BLACKLIST") != 0) {
      GST_LOG ("%20s@%s", gst_plugin_get_name (plugin),
          GST_STR_NULL (gst_plugin_get_filename (plugin)));
    }
  }
  gst_plugin_list_free (plugins);
}

/* initialize GStreamer testing */
void
gst_check_init (int *argc, char **argv[])
{
  gst_init (argc, argv);

  GST_DEBUG_CATEGORY_INIT (check_debug, "check", 0, "check regression tests");

  if (g_getenv ("GST_TEST_DEBUG"))
    _gst_check_debug = TRUE;

  g_log_set_handler (NULL, G_LOG_LEVEL_MESSAGE, gst_check_log_message_func,
      NULL);
  g_log_set_handler (NULL, G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING,
      gst_check_log_critical_func, NULL);
  g_log_set_handler ("GStreamer", G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING,
      gst_check_log_critical_func, NULL);
  g_log_set_handler ("GLib-GObject", G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING,
      gst_check_log_critical_func, NULL);
  g_log_set_handler ("Gst-Phonon", G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING,
      gst_check_log_critical_func, NULL);

  print_plugins ();

  check_cond = g_cond_new ();
  check_mutex = g_mutex_new ();
}

/* message checking */
void
gst_check_message_error (GstMessage * message, GstMessageType type,
    GQuark domain, gint code)
{
  GError *error;
  gchar *debug;

  fail_unless (GST_MESSAGE_TYPE (message) == type,
      "message is of type %s instead of expected type %s",
      gst_message_type_get_name (GST_MESSAGE_TYPE (message)),
      gst_message_type_get_name (type));
  gst_message_parse_error (message, &error, &debug);
  fail_unless_equals_int (error->domain, domain);
  fail_unless_equals_int (error->code, code);
  g_error_free (error);
  g_free (debug);
}

/* helper functions */
GstFlowReturn
gst_check_chain_func (GstPad * pad, GstBuffer * buffer)
{
  GST_DEBUG ("chain_func: received buffer %p", buffer);
  buffers = g_list_append (buffers, buffer);

  g_mutex_lock (check_mutex);
  g_cond_signal (check_cond);
  g_mutex_unlock (check_mutex);

  return GST_FLOW_OK;
}

/* setup an element for a filter test with mysrcpad and mysinkpad */
GstElement *
gst_check_setup_element (const gchar * factory)
{
  GstElement *element;

  GST_DEBUG ("setup_element");

  element = gst_element_factory_make (factory, factory);
  fail_if (element == NULL, "Could not create a '%s' element", factory);
  ASSERT_OBJECT_REFCOUNT (element, factory, 1);
  return element;
}

void
gst_check_teardown_element (GstElement * element)
{
  GST_DEBUG ("teardown_element");

  fail_unless (gst_element_set_state (element, GST_STATE_NULL) ==
      GST_STATE_CHANGE_SUCCESS, "could not set to null");
  ASSERT_OBJECT_REFCOUNT (element, "element", 1);
  gst_object_unref (element);
}

/* FIXME: set_caps isn't that useful
 */
GstPad *
gst_check_setup_src_pad (GstElement * element,
    GstStaticPadTemplate * tmpl, GstCaps * caps)
{
  GstPad *srcpad;

  srcpad = gst_check_setup_src_pad_by_name (element, tmpl, "sink");
  if (caps)
    fail_unless (gst_pad_set_caps (srcpad, caps), "could not set caps on pad");
  return srcpad;
}

GstPad *
gst_check_setup_src_pad_by_name (GstElement * element,
    GstStaticPadTemplate * tmpl, const gchar * name)
{
  GstPad *srcpad, *sinkpad;

  /* sending pad */
  srcpad = gst_pad_new_from_static_template (tmpl, "src");
  GST_DEBUG_OBJECT (element, "setting up sending pad %p", srcpad);
  fail_if (srcpad == NULL, "Could not create a srcpad");
  ASSERT_OBJECT_REFCOUNT (srcpad, "srcpad", 1);

  sinkpad = gst_element_get_static_pad (element, name);
  if (sinkpad == NULL)
    sinkpad = gst_element_get_request_pad (element, name);
  fail_if (sinkpad == NULL, "Could not get sink pad from %s",
      GST_ELEMENT_NAME (element));
  ASSERT_OBJECT_REFCOUNT (sinkpad, "sinkpad", 2);
  fail_unless (gst_pad_link (srcpad, sinkpad) == GST_PAD_LINK_OK,
      "Could not link source and %s sink pads", GST_ELEMENT_NAME (element));
  gst_object_unref (sinkpad);   /* because we got it higher up */
  ASSERT_OBJECT_REFCOUNT (sinkpad, "sinkpad", 1);

  return srcpad;
}

void
gst_check_teardown_pad_by_name (GstElement * element, const gchar * name)
{
  GstPad *pad_peer, *pad_element;

  /* clean up floating src pad */
  pad_element = gst_element_get_static_pad (element, name);
  /* We don't check the refcount here since there *might* be
   * a pad cache holding an extra reference on pad_element.
   * To get to a state where no pad cache will exist,
   * we first unlink that pad. */
  pad_peer = gst_pad_get_peer (pad_element);

  if (pad_peer) {
    if (gst_pad_get_direction (pad_element) == GST_PAD_SINK)
      gst_pad_unlink (pad_peer, pad_element);
    else
      gst_pad_unlink (pad_element, pad_peer);

    /* caps could have been set, make sure they get unset */
    gst_pad_set_caps (pad_peer, NULL);
  }

  /* pad refs held by both creator and this function (through _get) */
  ASSERT_OBJECT_REFCOUNT (pad_element, "element pad_element", 2);
  gst_object_unref (pad_element);
  /* one more ref is held by element itself */

  if (pad_peer) {
    /* pad refs held by both creator and this function (through _get_peer) */
    ASSERT_OBJECT_REFCOUNT (pad_peer, "check pad_peer", 2);
    gst_object_unref (pad_peer);
    gst_object_unref (pad_peer);
  }
}

void
gst_check_teardown_src_pad (GstElement * element)
{
  gst_check_teardown_pad_by_name (element, "sink");
}

/* FIXME: set_caps isn't that useful; might want to check if fixed,
 * then use set_use_fixed or somesuch */
GstPad *
gst_check_setup_sink_pad (GstElement * element, GstStaticPadTemplate * tmpl,
    GstCaps * caps)
{
  GstPad *sinkpad;

  sinkpad = gst_check_setup_sink_pad_by_name (element, tmpl, "src");
  if (caps)
    fail_unless (gst_pad_set_caps (sinkpad, caps), "Could not set pad caps");
  return sinkpad;
}

GstPad *
gst_check_setup_sink_pad_by_name (GstElement * element,
    GstStaticPadTemplate * tmpl, const gchar * name)
{
  GstPad *srcpad, *sinkpad;

  /* receiving pad */
  sinkpad = gst_pad_new_from_static_template (tmpl, "sink");
  GST_DEBUG_OBJECT (element, "setting up receiving pad %p", sinkpad);
  fail_if (sinkpad == NULL, "Could not create a sinkpad");

  srcpad = gst_element_get_static_pad (element, name);
  if (srcpad == NULL)
    srcpad = gst_element_get_request_pad (element, name);
  fail_if (srcpad == NULL, "Could not get source pad from %s",
      GST_ELEMENT_NAME (element));
  gst_pad_set_chain_function (sinkpad, gst_check_chain_func);

  GST_DEBUG_OBJECT (element, "Linking element src pad and receiving sink pad");
  fail_unless (gst_pad_link (srcpad, sinkpad) == GST_PAD_LINK_OK,
      "Could not link %s source and sink pads", GST_ELEMENT_NAME (element));
  gst_object_unref (srcpad);    /* because we got it higher up */
  ASSERT_OBJECT_REFCOUNT (srcpad, "srcpad", 1);

  GST_DEBUG_OBJECT (element, "set up srcpad, refcount is 1");
  return sinkpad;
}

void
gst_check_teardown_sink_pad (GstElement * element)
{
  gst_check_teardown_pad_by_name (element, "src");
}

/**
 * gst_check_drop_buffers:
 *
 * Unref and remove all buffers that are in the global @buffers GList,
 * emptying the list.
 *
 * Since: 0.10.18
 */
void
gst_check_drop_buffers (void)
{
  while (buffers != NULL) {
    gst_buffer_unref (GST_BUFFER (buffers->data));
    buffers = g_list_delete_link (buffers, buffers);
  }
}

/**
 * gst_check_caps_equal:
 * @caps1: first caps to compare
 * @caps2: second caps to compare
 *
 * Compare two caps with gst_caps_is_equal and fail unless they are
 * equal.
 *
 * Since: 0.10.18
 */
void
gst_check_caps_equal (GstCaps * caps1, GstCaps * caps2)
{
  gchar *name1 = gst_caps_to_string (caps1);
  gchar *name2 = gst_caps_to_string (caps2);

  fail_unless (gst_caps_is_equal (caps1, caps2),
      "caps ('%s') is not equal to caps ('%s')", name1, name2);
  g_free (name1);
  g_free (name2);
}

/**
 * gst_check_element_push_buffer_list:
 * @element_name: name of the element that needs to be created
 * @buffer_in: a list of buffers that needs to be puched to the element
 * @buffer_out: a list of buffers that we expect from the element
 * @last_flow_return: the last buffer push needs to give this GstFlowReturn
 *
 * Create an @element with the factory with the name and push the buffers in
 * @buffer_in to this element. The element should create the buffers equal to
 * the buffers in @buffer_out. We only check the caps, size and the data of the
 * buffers. This function unrefs the buffers in the two lists.
 * The last_flow_return parameter indicates the expected flow return value from
 * pushing the final buffer in the list.
 * This can be used to set up a test which pushes some buffers and then an
 * invalid buffer, when the final buffer is expected to fail, for example.
 * 
 * Since: 0.10.18
 */
/* FIXME 0.11: rename this function now that there's GstBufferList? */
void
gst_check_element_push_buffer_list (const gchar * element_name,
    GList * buffer_in, GList * buffer_out, GstFlowReturn last_flow_return)
{
  GstCaps *sink_caps;
  GstCaps *src_caps = NULL;
  GstElement *element;
  GstPad *pad_peer;
  GstPad *sink_pad = NULL;
  GstPad *src_pad;
  GstBuffer *buffer;

  /* check that there are no buffers waiting */
  gst_check_drop_buffers ();
  /* create the element */
  element = gst_check_setup_element (element_name);
  fail_if (element == NULL, "failed to create the element '%s'", element_name);
  fail_unless (GST_IS_ELEMENT (element), "the element is no element");
  /* create the src pad */
  buffer = GST_BUFFER (buffer_in->data);

  fail_unless (GST_IS_BUFFER (buffer), "There should be a buffer in buffer_in");
  src_caps = GST_BUFFER_CAPS (buffer);
  src_pad = gst_pad_new (NULL, GST_PAD_SRC);
  gst_pad_set_caps (src_pad, src_caps);
  pad_peer = gst_element_get_static_pad (element, "sink");
  fail_if (pad_peer == NULL);
  fail_unless (gst_pad_link (src_pad, pad_peer) == GST_PAD_LINK_OK,
      "Could not link source and %s sink pads", GST_ELEMENT_NAME (element));
  gst_object_unref (pad_peer);
  /* activate the pad */
  gst_pad_set_active (src_pad, TRUE);
  GST_DEBUG ("src pad activated");
  /* don't create the sink_pad if there is no buffer_out list */
  if (buffer_out != NULL) {
    gchar *temp;

    GST_DEBUG ("buffer out detected, creating the sink pad");
    /* get the sink caps */
    sink_caps = GST_BUFFER_CAPS (GST_BUFFER (buffer_out->data));
    fail_unless (GST_IS_CAPS (sink_caps), "buffer out don't have caps");
    temp = gst_caps_to_string (sink_caps);

    GST_DEBUG ("sink caps requested by buffer out: '%s'", temp);
    g_free (temp);
    fail_unless (gst_caps_is_fixed (sink_caps), "we need fixed caps");
    /* get the sink pad */
    sink_pad = gst_pad_new (NULL, GST_PAD_SINK);
    fail_unless (GST_IS_PAD (sink_pad));
    gst_pad_set_caps (sink_pad, sink_caps);
    /* get the peer pad */
    pad_peer = gst_element_get_static_pad (element, "src");
    fail_unless (gst_pad_link (pad_peer, sink_pad) == GST_PAD_LINK_OK,
        "Could not link sink and %s source pads", GST_ELEMENT_NAME (element));
    gst_object_unref (pad_peer);
    /* configure the sink pad */
    gst_pad_set_chain_function (sink_pad, gst_check_chain_func);
    gst_pad_set_active (sink_pad, TRUE);
  }
  fail_unless (gst_element_set_state (element,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");
  /* push all the buffers in the buffer_in list */
  fail_unless (g_list_length (buffer_in) > 0, "the buffer_in list is empty");
  while (buffer_in != NULL) {
    GstBuffer *next_buffer = GST_BUFFER (buffer_in->data);

    fail_unless (GST_IS_BUFFER (next_buffer),
        "data in buffer_in should be a buffer");
    /* remove the buffer from the list */
    buffer_in = g_list_remove (buffer_in, next_buffer);
    if (buffer_in == NULL) {
      fail_unless (gst_pad_push (src_pad, next_buffer) == last_flow_return,
          "we expect something else from the last buffer");
    } else {
      fail_unless (gst_pad_push (src_pad, next_buffer) == GST_FLOW_OK,
          "Failed to push buffer in");
    }
  }
  fail_unless (gst_element_set_state (element,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS, "could not set to null");
  /* check that there is a buffer out */
  fail_unless_equals_int (g_list_length (buffers), g_list_length (buffer_out));
  while (buffers != NULL) {
    GstBuffer *new = GST_BUFFER (buffers->data);
    GstBuffer *orig = GST_BUFFER (buffer_out->data);

    GST_LOG ("orig buffer: size %u, caps %" GST_PTR_FORMAT,
        GST_BUFFER_SIZE (orig), GST_BUFFER_CAPS (orig));
    GST_LOG ("new  buffer: size %u, caps %" GST_PTR_FORMAT,
        GST_BUFFER_SIZE (new), GST_BUFFER_CAPS (new));
    GST_MEMDUMP ("orig buffer", GST_BUFFER_DATA (orig), GST_BUFFER_SIZE (orig));
    GST_MEMDUMP ("new  buffer", GST_BUFFER_DATA (new), GST_BUFFER_SIZE (new));

    /* remove the buffers */
    buffers = g_list_remove (buffers, new);
    buffer_out = g_list_remove (buffer_out, orig);
    fail_unless (GST_BUFFER_SIZE (orig) == GST_BUFFER_SIZE (new),
        "size of the buffers are not the same");
    fail_unless (memcmp (GST_BUFFER_DATA (orig), GST_BUFFER_DATA (new),
            GST_BUFFER_SIZE (new)) == 0, "data is not the same");
    gst_check_caps_equal (GST_BUFFER_CAPS (orig), GST_BUFFER_CAPS (new));
    gst_buffer_unref (new);
    gst_buffer_unref (orig);
  }
  /* teardown the element and pads */
  gst_pad_set_active (src_pad, FALSE);
  gst_check_teardown_src_pad (element);
  gst_pad_set_active (sink_pad, FALSE);
  gst_check_teardown_sink_pad (element);
  gst_check_teardown_element (element);
}

/**
 * gst_check_element_push_buffer:
 * @element_name: name of the element that needs to be created
 * @buffer_in: push this buffer to the element
 * @buffer_out: compare the result with this buffer
 *
 * Create an @element with the factory with the name and push the
 * @buffer_in to this element. The element should create one buffer
 * and this will be compared with @buffer_out. We only check the caps
 * and the data of the buffers. This function unrefs the buffers.
 * 
 * Since: 0.10.18
 */
void
gst_check_element_push_buffer (const gchar * element_name,
    GstBuffer * buffer_in, GstBuffer * buffer_out)
{
  GList *in = NULL;
  GList *out = NULL;

  in = g_list_append (in, buffer_in);
  out = g_list_append (out, buffer_out);

  gst_check_element_push_buffer_list (element_name, in, out, GST_FLOW_OK);
}

void
gst_check_abi_list (GstCheckABIStruct list[], gboolean have_abi_sizes)
{
  if (have_abi_sizes) {
    gboolean ok = TRUE;
    gint i;

    for (i = 0; list[i].name; i++) {
      if (list[i].size != list[i].abi_size) {
        ok = FALSE;
        g_print ("sizeof(%s) is %d, expected %d\n",
            list[i].name, list[i].size, list[i].abi_size);
      }
    }
    fail_unless (ok, "failed ABI check");
  } else {
    const gchar *fn;

    if ((fn = g_getenv ("GST_ABI"))) {
      GError *err = NULL;
      GString *s;
      gint i;

      s = g_string_new ("\nGstCheckABIStruct list[] = {\n");
      for (i = 0; list[i].name; i++) {
        g_string_append_printf (s, "  {\"%s\", sizeof (%s), %d},\n",
            list[i].name, list[i].name, list[i].size);
      }
      g_string_append (s, "  {NULL, 0, 0}\n");
      g_string_append (s, "};\n");
      if (!g_file_set_contents (fn, s->str, s->len, &err)) {
        g_print ("%s", s->str);
        g_printerr ("\nFailed to write ABI information: %s\n", err->message);
      } else {
        g_print ("\nWrote ABI information to '%s'.\n", fn);
      }
      g_string_free (s, TRUE);
    } else {
      g_print ("No structure size list was generated for this architecture.\n");
      g_print ("Run with GST_ABI environment variable set to output header.\n");
    }
  }
}

gint
gst_check_run_suite (Suite * suite, const gchar * name, const gchar * fname)
{
  SRunner *sr;
  gchar *xmlfilename = NULL;
  gint nf;

  sr = srunner_create (suite);

  if (g_getenv ("GST_CHECK_XML")) {
    /* how lucky we are to have __FILE__ end in .c */
    xmlfilename = g_strdup_printf ("%sheck.xml", fname);

    srunner_set_xml (sr, xmlfilename);
  }

  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  g_free (xmlfilename);
  srunner_free (sr);
  return nf;
}

gboolean
_gst_check_run_test_func (const gchar * func_name)
{
  const gchar *gst_checks;
  gboolean res = FALSE;
  gchar **funcs, **f;

  gst_checks = g_getenv ("GST_CHECKS");

  /* no filter specified => run all checks */
  if (gst_checks == NULL || *gst_checks == '\0')
    return TRUE;

  /* only run specified functions */
  funcs = g_strsplit (gst_checks, ",", -1);
  for (f = funcs; f != NULL && *f != NULL; ++f) {
    if (g_pattern_match_simple (*f, func_name)) {
      res = TRUE;
      break;
    }
  }
  g_strfreev (funcs);
  return res;
}
