#define NAPI_EXPERIMENTAL

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <node_api.h>
#include <napi-macros.h>
#include <pthread.h>
#include <deltachat.h>

#define NAPI_UTF8(name, val) \
  size_t name##_size = 0; \
  NAPI_STATUS_THROWS(napi_get_value_string_utf8(env, val, NULL, 0, &name##_size)); \
  name##_size++; \
  char name[name##_size]; \
  size_t name##_len; \
  NAPI_STATUS_THROWS(napi_get_value_string_utf8(env, val, (char *) &name, name##_size, &name##_len));

#define NAPI_DCN_CONTEXT() \
  dcn_context_t* dcn_context; \
  NAPI_STATUS_THROWS(napi_get_value_external(env, argv[0], (void**)&dcn_context));

#define NAPI_RETURN_UNDEFINED() \
  return 0;


// Context struct we need for some binding specific things. dcn_context_t will
// be applied to the dc_context created in dcn_context_new().
typedef struct dcn_context_t {
  dc_context_t* dc_context;
  napi_threadsafe_function napi_event_handler;
  pthread_mutex_t mutex;
} dcn_context_t;

typedef struct dcn_event_t {
  int event;
  uintptr_t data1_int;
  uintptr_t data2_int;
  char* data2_str;
} dcn_event_t;


uintptr_t dc_event_handler(dc_context_t* dc_context, int event, uintptr_t data1, uintptr_t data2)
{
  printf("dc_event_handler, event: %d\n", event);
  dcn_context_t* dcn_context = (dcn_context_t*)dc_get_userdata(dc_context);

  if (dcn_context->napi_event_handler != NULL) {
    //printf(" -> Locking mutex...\n");
    //pthread_mutex_lock(&dcn_context->mutex);
    dcn_event_t* dcn_event = calloc(1, sizeof(dcn_event_t));
    dcn_event->event = event;
    dcn_event->data1_int = data1;
    dcn_event->data2_int = data2;
    dcn_event->data2_str = (DC_EVENT_DATA2_IS_STRING(event) && data2) ? strdup((char*)data2) : NULL;
        
    napi_call_threadsafe_function(dcn_context->napi_event_handler, dcn_event, napi_tsfn_blocking);
  } else {
    printf("Warning: napi_event_handler not set :/\n");
  }

  return 0;
}

static void call_js_event_handler(napi_env env, napi_value js_callback, void* context, void* data)
{
  printf("Inside call_js_event_handler\n");
  dcn_context_t* dcn_context = (dcn_context_t*)context;
  dcn_event_t* dcn_event = (dcn_event_t*)data;

  napi_value global;
  napi_status status = napi_get_global(env, &global);

  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to get global");
  }

  const int argc = 3;
  napi_value argv[argc];
  
  status = napi_create_int32(env, dcn_event->event, &argv[0]);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to create argv[0] for event_handler arguments");
  }

  status = napi_create_int32(env, dcn_event->data1_int, &argv[1]);
  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to create argv[1] for event_handler arguments");
  }

  if (dcn_event->data2_str) {
    status = napi_create_string_utf8(env, dcn_event->data2_str, NAPI_AUTO_LENGTH, &argv[2]);
    if (status != napi_ok) {
      napi_throw_error(env, NULL, "Unable to create argv[2] for event_handler arguments");
    }
    free(dcn_event->data2_str);
  } else {
    status = napi_create_int32(env, dcn_event->data2_int, &argv[2]);
    if (status != napi_ok) {
      napi_throw_error(env, NULL, "Unable to create argv[2] for event_handler arguments");
    }
  }

  free(dcn_event);
  dcn_event = NULL;

  napi_value result;

  status = napi_call_function(
    env,
    global,
    js_callback,
    argc,
    argv,
    &result);

  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to call event_handler callback");
  }

  //pthread_mutex_unlock(&dcn_context->mutex);
  //printf(" -> Unlocked mutex...\n");
}

NAPI_METHOD(dcn_context_new) {
  
  dcn_context_t* dcn_context = calloc(1, sizeof(dcn_context_t));
  dcn_context->dc_context = NULL; 
  dcn_context->napi_event_handler = NULL;
  pthread_mutex_init(&dcn_context->mutex, NULL);
  
  dcn_context->dc_context = dc_context_new(dc_event_handler, dcn_context, NULL);

  napi_value dcn_context_napi;
  napi_status status = napi_create_external(env, dcn_context, NULL, NULL, &dcn_context_napi);

  if (status != napi_ok) {
    napi_throw_error(env, NULL, "Unable to create external dc_context object");
  }

  return dcn_context_napi;
}

NAPI_METHOD(dcn_set_event_handler) {
  NAPI_ARGV(2); //TODO: Make sure we throw a helpful error if we don't get the correct count of arguments

  NAPI_DCN_CONTEXT();

  napi_value callback = argv[1];
  napi_value async_resource_name;

  NAPI_STATUS_THROWS(napi_create_string_utf8(env, "dc_event_callback", NAPI_AUTO_LENGTH, &async_resource_name));

  //TODO: Figure out how to release threadsafe_function
  NAPI_STATUS_THROWS(napi_create_threadsafe_function(
    env,
    callback,
    0,
    async_resource_name,
    0,
    3,
    0,
    NULL, // TODO: file an issue that the finalize parameter should be optional
    dcn_context,
    call_js_event_handler,
    &dcn_context->napi_event_handler));

  NAPI_RETURN_INT32(1);
}

void* imap_thread_func(void* arg)
{
  dc_context_t* dc_context = (dc_context_t*)arg; 
 
  while (true) {
    dc_perform_imap_jobs(dc_context);
    dc_perform_imap_fetch(dc_context);
    dc_perform_imap_idle(dc_context);
  }
  
  return NULL;
}

void* smtp_thread_func(void* arg)
{
  dc_context_t* dc_context = (dc_context_t*)arg; 
  
  while (true) {
    dc_perform_smtp_jobs(dc_context);
    dc_perform_smtp_idle(dc_context);
  }
  
  return NULL;
}

NAPI_METHOD(dcn_start_threads) {
  NAPI_ARGV(2);

  NAPI_DCN_CONTEXT();

  pthread_t imap_thread;
  pthread_create(&imap_thread, NULL, imap_thread_func, dcn_context->dc_context);

  pthread_t smtp_thread;
  pthread_create(&smtp_thread, NULL, smtp_thread_func, dcn_context->dc_context);
  
  NAPI_RETURN_INT32(1);
}

NAPI_METHOD(dcn_open) {
  NAPI_ARGV(3);

  NAPI_DCN_CONTEXT();
  NAPI_UTF8(dbfile, argv[1]);
  NAPI_UTF8(blobdir, argv[2]);

  //TODO: How to handle NULL value blobdir
  printf("dcn_open %s %s\n", dbfile, blobdir);
  int status = dc_open(dcn_context->dc_context, dbfile, blobdir);
  printf("dcn_open successfully opened\n");

  NAPI_RETURN_INT32(status);
}

NAPI_METHOD(dcn_set_config) {
  NAPI_ARGV(3);

  NAPI_DCN_CONTEXT();

  NAPI_UTF8(key, argv[1]);

  NAPI_UTF8(value, argv[2]);
  
  printf("%s %s\n", key, value);

  // TODO: Investigate why this is blocking if the database is not open (hypothesis)
  int status = dc_set_config(dcn_context->dc_context, key, value);
  printf("set config\n");

  NAPI_RETURN_INT32(status);
}

NAPI_METHOD(dcn_configure) {
  NAPI_ARGV(1);

  NAPI_DCN_CONTEXT();

  dc_configure(dcn_context->dc_context);
  
  NAPI_RETURN_UNDEFINED();
}

NAPI_METHOD(dcn_is_configured) {

  NAPI_ARGV(1);

  NAPI_DCN_CONTEXT();

  int status = dc_is_configured(dcn_context->dc_context);

  NAPI_RETURN_INT32(status);
}

NAPI_INIT() {
  NAPI_EXPORT_FUNCTION(dcn_context_new);
  NAPI_EXPORT_FUNCTION(dcn_set_event_handler);
  NAPI_EXPORT_FUNCTION(dcn_start_threads);

  NAPI_EXPORT_FUNCTION(dcn_open);
  NAPI_EXPORT_FUNCTION(dcn_set_config);
  NAPI_EXPORT_FUNCTION(dcn_configure);
  NAPI_EXPORT_FUNCTION(dcn_is_configured);
} 
