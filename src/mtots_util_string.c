#include "mtots_util_string.h"

#include "mtots_util_error.h"

#include <stdlib.h>
#include <string.h>

#define STRING_SET_MAX_LOAD 0.75

typedef struct StringSet {
  String **strings;
  size_t capacity, occupied;
} StringSet;

static StringSet allStrings;

static u32 hashString(const char *key, size_t length) {
  /* FNV-1a as presented in the Crafting Interpreters book */
  size_t i;
  u32 hash = 2166136261u;
  for (i = 0; i < length; i++) {
    hash ^= (u8) key[i];
    hash *= 16777619;
  }
  return hash;
}

static String **stringSetFindEntry(const char *chars, size_t length, u32 hash) {
  u32 index = hash & (allStrings.capacity - 1);
  for (;;) {
    String **entry = &allStrings.strings[index];
    String *str = *entry;
    if (str == NULL ||
        (str->length == length &&
        str->hash == hash &&
        memcmp(str->chars, chars, length) == 0)) {
      return entry;
    }
    index = (index + 1) & (allStrings.capacity - 1);
  }
}

String *internString(const char *chars, size_t length) {
  if (allStrings.occupied + 1 > allStrings.capacity * STRING_SET_MAX_LOAD) {
    size_t oldCap = allStrings.capacity;
    size_t newCap = oldCap < 8 ? 8 : oldCap * 2;
    size_t i;
    String **oldStrings = allStrings.strings;
    String **newStrings = (String**)malloc(sizeof(String*) * newCap);
    String **entry;
    for (i = 0; i < newCap; i++) {
      newStrings[i] = NULL;
    }
    allStrings.strings = newStrings;
    allStrings.capacity = newCap;
    allStrings.occupied = 0;
    for (i = 0; i < oldCap; i++) {
      String *oldString = oldStrings[i];
      if (oldString == NULL) {
        continue;
      }
      entry = stringSetFindEntry(oldString->chars, oldString->length, oldString->hash);
      if (*entry) {
        assertionError();
      }
      *entry = oldString;
      allStrings.occupied++;
    }
    free(oldStrings);
  }

  {
    u32 hash = hashString(chars, length);
    String **entry = stringSetFindEntry(chars, length, hash);
    String *string;
    if (*entry) {
      return *entry;
    }
    allStrings.occupied++;
    string = (String*)malloc(sizeof(String));
    string->isMarked = UFALSE;
    string->length = length;
    string->hash = hash;
    string->chars = (char*)malloc(length + 1);
    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';
    *entry = string;
    return string;
  }
}

String *internCString(const char *string) {
  return internString(string, strlen(string));
}

String *internOwnedString(char *chars, size_t length) {
  /* TODO actually reuse the memory provided by the caller */
  String *string = internString(chars, length);
  free(chars);
  return string;
}

void freeUnmarkedStrings() {
  size_t i, cap = allStrings.capacity;
  String **oldEntries = allStrings.strings;
  String **newEntries = (String**)malloc(sizeof(String*) * cap);
  for (i = 0; i < cap; i++) {
    newEntries[i] = NULL;
  }
  allStrings.occupied = 0;
  allStrings.strings = newEntries;
  for (i = 0; i < cap; i++) {
    String *str = oldEntries[i];
    if (str) {
      if (str->isMarked) {
        String **entry = stringSetFindEntry(str->chars, str->length, str->hash);
        if (*entry) {
          assertionError();
        }
        *entry = str;
        str->isMarked = UFALSE;
        allStrings.occupied++;
      } else {
        free(str->chars);
        free(str);
        oldEntries[i] = NULL;
      }
    }
  }
  free(oldEntries);
}
