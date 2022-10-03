#ifndef mtots_m_sdl_adev_h
#define mtots_m_sdl_adev_h

#include "mtots_m_sdl_common.h"


/**********************************************************
 * AudioDevice.queue()
 *********************************************************/

static ubool implAudioDeviceQueue(i16 argCount, Value *args, Value *out) {
  ObjAudioDevice *ad = (ObjAudioDevice*)AS_OBJ(args[-1]);
  ObjByteArray *ba = AS_BYTE_ARRAY(args[0]);
  if (SDL_QueueAudio(ad->handle, ba->buffer, ba->size) != 0) {
    runtimeError("Failed to queue SDL audio: %s", SDL_GetError());
    return UFALSE;
  }
  return UTRUE;
}

static TypePattern argsAudioDeviceQueue[] = {
  { TYPE_PATTERN_BYTE_ARRAY },
};

static CFunction funcAudioDeviceQueue = {
  implAudioDeviceQueue, "queue",
  sizeof(argsAudioDeviceQueue)/sizeof(TypePattern), 0,
  argsAudioDeviceQueue,
};

/**********************************************************
 * AudioDevice.pause()
 *********************************************************/

static ubool implAudioDevicePause(i16 argCount, Value *args, Value *out) {
  ObjAudioDevice *ad = (ObjAudioDevice*)AS_OBJ(args[-1]);
  int pauseOn = AS_NUMBER(args[0]);
  SDL_PauseAudioDevice(ad->handle, pauseOn);
  return UTRUE;
}

static TypePattern argsAudioDevicePause[] = {
  { TYPE_PATTERN_NUMBER },
};

static CFunction funcAudioDevicePause = {
  implAudioDevicePause, "pause",
  sizeof(argsAudioDevicePause)/sizeof(TypePattern), 0,
  argsAudioDevicePause,
};

/**********************************************************
 * AudioDevice.getQueuedSize()
 *********************************************************/

static ubool implAudioDeviceGetQueuedSize(
    i16 argCount, Value *args, Value *out) {
  ObjAudioDevice *ad = (ObjAudioDevice*)AS_OBJ(args[-1]);
  *out = NUMBER_VAL(SDL_GetQueuedAudioSize(ad->handle));
  return UTRUE;
}

static CFunction funcAudioDeviceGetQueuedSize = {
  implAudioDeviceGetQueuedSize, "getQueuedSize", 0 };

/**********************************************************
 * -- the descriptor --
 *********************************************************/

static CFunction *audioDeviceMethods[] = {
  &funcAudioDeviceQueue,
  &funcAudioDevicePause,
  &funcAudioDeviceGetQueuedSize,
  NULL,
};

NativeObjectDescriptor descriptorAudioDevice = {
  nopBlacken, nopFree, NULL, NULL, NULL, sizeof(ObjAudioDevice), "AudioDevice",
  audioDeviceMethods };

#endif/*mtots_m_sdl_adev_h*/
