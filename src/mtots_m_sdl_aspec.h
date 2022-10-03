#ifndef mtots_m_sdl_aspec_h
#define mtots_m_sdl_aspec_h

#include "mtots_m_sdl_common.h"

#include <string.h>

static ObjAudioSpec *newAudioSpec() {
  ObjAudioSpec *spec = NEW_NATIVE(ObjAudioSpec, &descriptorAudioSpec);
  memset(&spec->data, 0, sizeof(SDL_AudioSpec));
  spec->data.callback = NULL;
  return spec;
}

static ubool implAudioSpec(i16 argCount, Value *args, Value *out) {
  *out = OBJ_VAL(newAudioSpec());
  return UTRUE;
}

static CFunction funcAudioSpec = { implAudioSpec, "AudioSpec", 0 };

static ubool audioSpecGetField(ObjNative *n, ObjString *key, Value *out) {
  ObjAudioSpec *audioSpec = (ObjAudioSpec*)n;
  if (key == string_freq) {
    *out = NUMBER_VAL(audioSpec->data.freq);
    return UTRUE;
  }
  if (key == string_format) {
    *out = NUMBER_VAL(audioSpec->data.format);
    return UTRUE;
  }
  if (key == string_channels) {
    *out = NUMBER_VAL(audioSpec->data.channels);
    return UTRUE;
  }
  if (key == string_samples) {
    *out = NUMBER_VAL(audioSpec->data.samples);
    return UTRUE;
  }
  return UFALSE;
}

static ubool audioSpecSetField(ObjNative *n, ObjString *key, Value in) {
  ObjAudioSpec *audioSpec = (ObjAudioSpec*)n;
  if (!IS_NUMBER(in)) {
    panic("AudioSpec.%s requires a number value but got %s",
      key->chars, getKindName(in));
    return UFALSE;
  }
  if (key == string_freq) {
    audioSpec->data.freq = AS_NUMBER(in);
    return UTRUE;
  }
  if (key == string_format) {
    audioSpec->data.format = AS_NUMBER(in);
    return UTRUE;
  }
  if (key == string_channels) {
    audioSpec->data.channels = AS_NUMBER(in);
    return UTRUE;
  }
  if (key == string_samples) {
    audioSpec->data.samples = AS_NUMBER(in);
    return UTRUE;
  }
  return UFALSE;
}

NativeObjectDescriptor descriptorAudioSpec = {
  nopBlacken, nopFree,
  audioSpecGetField, audioSpecSetField, &funcAudioSpec,
  sizeof(ObjAudioSpec), "AudioSpec" };

#endif/*mtots_m_sdl_aspec_h*/
