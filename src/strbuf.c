/* strbuf.c - buffer de texto crescente (ver strbuf.h). */
#include "strbuf.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Garante espaco para mais "extra" bytes alem do terminador. */
static bool reserve(StrBuf *buffer, size_t extra)
{
    size_t needed;
    size_t capacity;
    char *grown;

    if (buffer->failed) {
        return false;
    }
    if (extra > SIZE_MAX - buffer->length - 1) {
        buffer->failed = true;
        return false;
    }
    needed = buffer->length + extra + 1;
    if (needed <= buffer->capacity) {
        return true;
    }
    capacity = buffer->capacity == 0 ? 256 : buffer->capacity;
    while (capacity < needed) {
        if (capacity > SIZE_MAX / 2) {
            capacity = needed;
            break;
        }
        capacity *= 2;
    }
    grown = realloc(buffer->data, capacity);
    if (grown == NULL) {
        buffer->failed = true;
        return false;
    }
    buffer->data = grown;
    buffer->capacity = capacity;
    return true;
}

void strbuf_append_bytes(StrBuf *buffer, const char *bytes, size_t length)
{
    if (buffer == NULL || bytes == NULL || !reserve(buffer, length)) {
        return;
    }
    memcpy(buffer->data + buffer->length, bytes, length);
    buffer->length += length;
    buffer->data[buffer->length] = '\0';
}

void strbuf_append(StrBuf *buffer, const char *text)
{
    if (text != NULL) {
        strbuf_append_bytes(buffer, text, strlen(text));
    }
}

void strbuf_appendf(StrBuf *buffer, const char *format, ...)
{
    va_list args;
    int needed;

    if (buffer == NULL || buffer->failed) {
        return;
    }
    va_start(args, format);
    needed = vsnprintf(NULL, 0, format, args);
    va_end(args);
    if (needed < 0 || !reserve(buffer, (size_t)needed)) {
        if (buffer != NULL) {
            buffer->failed = true;
        }
        return;
    }
    va_start(args, format);
    vsnprintf(buffer->data + buffer->length, (size_t)needed + 1, format, args);
    va_end(args);
    buffer->length += (size_t)needed;
}

void strbuf_append_buffer(StrBuf *buffer, const StrBuf *other)
{
    if (other != NULL && other->data != NULL) {
        strbuf_append_bytes(buffer, other->data, other->length);
    }
    if (other != NULL && other->failed && buffer != NULL) {
        buffer->failed = true;
    }
}

const char *strbuf_text(const StrBuf *buffer)
{
    return (buffer == NULL || buffer->data == NULL) ? "" : buffer->data;
}

bool strbuf_failed(const StrBuf *buffer)
{
    return buffer == NULL || buffer->failed;
}

void strbuf_clear(StrBuf *buffer)
{
    if (buffer != NULL && buffer->data != NULL) {
        buffer->length = 0;
        buffer->data[0] = '\0';
    }
}

void strbuf_destroy(StrBuf *buffer)
{
    if (buffer != NULL) {
        free(buffer->data);
        *buffer = (StrBuf){0};
    }
}
