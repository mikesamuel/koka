#pragma once
#ifndef STRING_H_
#define STRING_H_
/*---------------------------------------------------------------------------
  Copyright 2020 Daan Leijen, Microsoft Corporation.

  This is free software; you can redistribute it and/or modify it under the
  terms of the Apache License, Version 2.0. A copy of the License can be
  found in the file "license.txt" at the root of this distribution.
---------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------
  Char as unicode point
--------------------------------------------------------------------------------------*/
typedef int32_t kk_char_t;

#define kk_char_replacement  ((kk_char_t)(KU32(0xFFFD)))

static inline kk_char_t kk_char_unbox(kk_box_t b, kk_context_t* ctx) {
  return (kk_char_t)kk_int32_unbox(b,ctx);
}

static inline kk_box_t kk_char_box(kk_char_t c, kk_context_t* ctx) {
  return kk_int32_box(c, ctx);
}

/*--------------------------------------------------------------------------------------
  Strings
  Always point to valid modified-UTF8 characters.
--------------------------------------------------------------------------------------*/

// A string is modified UTF8 (with encoded zeros) ending with a '0' character.
typedef struct kk_string_s {
  kk_block_t _block;
} *kk_string_t;

#define KK_STRING_SMALL_MAX (KUZ(8))
typedef struct kk_string_small_s {
  struct kk_string_s _base;
  union {
    uint64_t str_value;              
    uint8_t  str[KK_STRING_SMALL_MAX];  // UTF8 string in-place ending in 0 of at most 8 bytes
  } u;
} *kk_string_small_t;

typedef struct kk_string_normal_s {
  struct kk_string_s _base;
  size_t  length;
  uint8_t str[1];  // UTF8 string in-place of `length+1` bytes ending in 0
} *kk_string_normal_t;

typedef struct kk_string_raw_s {
  struct kk_string_s _base;
  kk_free_fun_t* free;     
  const uint8_t* cstr;   // UTF8 string of `length+1` bytes ending in 0
  size_t         length;
} *kk_string_raw_t;

extern struct kk_string_normal_s kk__static_string_empty;
#define kk_string_empty  (&kk__static_string_empty._base)

// Define string literals
#define kk_define_string_literal(decl,name,len,chars) \
  static struct { struct kk_string_s _base; size_t length; char str[len+1]; } _static_##name = \
    { { { KK_HEADER_STATIC(0,KK_TAG_STRING) } }, len, chars }; \
  decl kk_string_t name = &_static_##name._base;  


static inline kk_string_t kk_string_unbox(kk_box_t v) {
  kk_block_t* b = kk_ptr_unbox(v);
  kk_assert_internal(kk_block_tag(b) == KK_TAG_STRING_SMALL || kk_block_tag(b) == KK_TAG_STRING || kk_block_tag(b) == KK_TAG_STRING_RAW || kk_block_tag(b) == KK_TAG_BOX_ANY);
  return (kk_string_t)b;
}

static inline kk_box_t kk_string_box(kk_string_t s) {
  return kk_ptr_box(&s->_block);
}

static inline void kk_string_drop(kk_string_t str, kk_context_t* ctx) {
  kk_basetype_drop(str, ctx);
}

static inline kk_string_t kk_string_dup(kk_string_t str) {
  return kk_basetype_dup_as(kk_string_t,str);
}


/*--------------------------------------------------------------------------------------
  Strings operations
--------------------------------------------------------------------------------------*/

// Allocate a string of `len` characters. Adds a terminating zero at the end.
static inline kk_string_t kk_string_alloc_len(size_t len, const char* s, kk_context_t* ctx) {
  if (len < KK_STRING_SMALL_MAX) {
    kk_string_small_t str = kk_block_alloc_as(struct kk_string_small_s, 0, KK_TAG_STRING_SMALL, ctx);
    str->u.str_value = 0;
    if (s != NULL && len > 0) {
      memcpy(&str->u.str[0], s, len);
    }
    return &str->_base;
  }
  else {
    kk_string_normal_t str = kk_block_assert(kk_string_normal_t, kk_block_alloc_any(sizeof(struct kk_string_normal_s) - 1 /* char str[1] */ + len + 1 /* 0 terminator */, 0, KK_TAG_STRING, ctx), KK_TAG_STRING);
    if (s != NULL && len > 0) {
      memcpy(&str->str[0], s, len);
    }
    str->length = len;
    str->str[len] = 0;
    // todo: kk_assert valid UTF8 in debug mode
    return &str->_base;
  }
}

static inline kk_string_t kk_string_alloc_buf(size_t len, kk_context_t* ctx) {
  return kk_string_alloc_len(len, NULL, ctx);
}

static inline kk_string_t kk_string_alloc_dup(const char* s, kk_context_t* ctx) {
  return (s==NULL ? kk_string_alloc_len(0, "", ctx) : kk_string_alloc_len(strlen(s), s, ctx));
}

static inline kk_string_t kk_string_alloc_raw_len(size_t len, const char* s, bool free, kk_context_t* ctx) {
  if (s==NULL) return kk_string_dup(kk_string_empty);
  kk_assert_internal(s[len]==0 && strlen(s)==len);
  struct kk_string_raw_s* str = kk_block_alloc_as(struct kk_string_raw_s, 0, KK_TAG_STRING_RAW, ctx);
  str->free = (free ? &kk_free : NULL);
  str->cstr = (const uint8_t*)s;
  str->length = len;
  // todo: kk_assert valid UTF8 in debug mode
  return &str->_base;
}

static inline kk_string_t kk_string_alloc_raw(const char* s, bool free, kk_context_t* ctx) {
  if (s==NULL) return kk_string_dup(kk_string_empty);
  return kk_string_alloc_raw_len(strlen(s), s, free, ctx);
}

static inline const uint8_t* kk_string_buf_borrow(const kk_string_t str) {
  if (kk_basetype_has_tag(str,KK_TAG_STRING_SMALL)) {
    return &(kk_basetype_as_assert(kk_string_small_t, str, KK_TAG_STRING_SMALL)->u.str[0]);
  }
  else if (kk_basetype_has_tag(str,KK_TAG_STRING)) {
    return &(kk_basetype_as_assert(kk_string_normal_t, str, KK_TAG_STRING)->str[0]);
  }
  else {
    return kk_basetype_as_assert(kk_string_raw_t, str, KK_TAG_STRING_RAW)->cstr;
  }
}

static inline const char* kk_string_cbuf_borrow(const kk_string_t str) {
  return (const char*)kk_string_buf_borrow(str);
}

static inline int kk_string_cmp_cstr_borrow(const kk_string_t s, const char* t) {
  return strcmp(kk_string_cbuf_borrow(s), t);
}

static inline size_t kk_decl_pure kk_string_len_borrow(const kk_string_t str) {
  if (kk_basetype_has_tag(str,KK_TAG_STRING_SMALL)) {  
    const kk_string_small_t s = kk_basetype_as_assert(const kk_string_small_t, str, KK_TAG_STRING_SMALL);
#ifdef KK_ARCH_LITTLE_ENDIAN
    return (KK_STRING_SMALL_MAX - (kk_bits_clz64(s->u.str_value)/8));
#else
    return (KK_STRING_SMALL_MAX - (kk_bits_ctz64(s->u.str_value)/8));
#endif
  }
  else if (kk_basetype_has_tag(str,KK_TAG_STRING)) {
    return kk_basetype_as_assert(kk_string_normal_t, str, KK_TAG_STRING)->length;
  }
  else {
    return kk_basetype_as_assert(kk_string_raw_t, str, KK_TAG_STRING_RAW)->length;
  }
}

static inline kk_string_t kk_string_copy(kk_string_t str, kk_context_t* ctx) {
  if (kk_block_is_unique(&str->_block)) {
    return str;
  }
  else {
    kk_string_t tstr = kk_string_alloc_len(kk_string_len_borrow(str), kk_string_cbuf_borrow(str), ctx);
    kk_string_drop(str, ctx);
    return tstr;
  }
}

kk_decl_export kk_string_t kk_string_adjust_length(kk_string_t str, size_t newlen, kk_context_t* ctx);

/*--------------------------------------------------------------------------------------------------
  UTF8 decoding/encoding
--------------------------------------------------------------------------------------------------*/

// Is this a UTF8 continuation byte? (0x80 <= b <= 0xBF, i.e. has the form 0x10xxxxxx)
static inline bool kk_utf8_is_cont(uint8_t c) {
  return (((int8_t)c) <= -65); // we can determine this in a single _signed_ comparison
}

// Advance to the next codepoint. (does not advance past the end)
// This should not validate, but advance to the next non-continuation byte.
static inline const uint8_t* kk_utf8_next(const uint8_t* s) {
  if (*s != 0) s++;                // skip first byte except if 0
  for (; kk_utf8_is_cont(*s); s++) {} // skip continuation bytes
  return s;
}

// Retreat to the previous codepoint. 
// This should not validate, but backup to the previous non-continuation byte.
static inline const uint8_t* kk_utf8_prev(const uint8_t* s) {
  s--;                             // skip back at least 1 byte
  for (; kk_utf8_is_cont(*s); s--) {} // skip while continuation bytes
  return s;
}

// Validating UTF8 decode; careful to only read beyond s[0] if valid.
static inline kk_char_t kk_utf8_read_validate(const uint8_t* s, size_t* count) {
  uint8_t b = s[0];
  if (kk_likely(b <= 0x7F)) {
    *count = 1;
    return b;   // ASCII fast path
  }
  else if (b == 0xC0 && s[1] == 0x80) {
    *count = 2;
    return 0;   // Modified UTF8 encoded zero
  }
  // 2 byte encoding
  else if (b >= 0xC2 && b <= 0xDF && kk_utf8_is_cont(s[1])) {
    *count = 2;
    kk_char_t c = (((b & 0x1F) << 6) | (s[1] & 0x3F));
    kk_assert_internal(c >= 0x80 && c <= 0x7FF);
    return c;
  }
  // 3 byte encoding; reject overlong and UTF16 surrogate halves (0xD800 - 0xDFFF)
  else if ((b == 0xE0 && s[1] >= 0xA0 && s[1] <= 0xBF && kk_utf8_is_cont(s[2]))
    || (b >= 0xE1 && b <= 0xEC && kk_utf8_is_cont(s[1]) && kk_utf8_is_cont(s[2])))
  {
    *count = 3;
    kk_char_t c = (((b & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F));
    kk_assert_internal(c >= 0x800 && (c < 0x0D800 || c > 0xDFFF) && c <= 0xFFFF);
    return c;
  }
  // 4 byte encoding; reject overlong and out of bounds (> 0x10FFFF)
  else if ((b == 0xF0 && s[1] >= 0x90 && s[1] <= 0xBF && kk_utf8_is_cont(s[2]) && kk_utf8_is_cont(s[3]))
    || (b >= 0xF1 &&  b <= 0xF3 && kk_utf8_is_cont(s[1]) && kk_utf8_is_cont(s[2]) && kk_utf8_is_cont(s[3]))
    || (b == 0xF4 && s[1] >= 0x80 && s[1] <= 0x8F && kk_utf8_is_cont(s[2]) && kk_utf8_is_cont(s[3])))
  {
    *count = 4;
    kk_char_t c = (((b & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F));
    kk_assert_internal(c >= 0x10000 && c <= 0x10FFFF);
    return c;
  }
  // invalid, skip continuation bytes
  // note: we treat a full illegal sequence as 1 invalid codepoint (including its continuation bytes)
  // this is important as it allows later to pre-allocate a buffer of the right size even if some
  // sequences are invalid.
  else {
    *count = (size_t)(kk_utf8_next(s) - s);
    return kk_char_replacement;
  }
}

// Non-validating utf8 decoding of a single code point
static inline kk_char_t kk_utf8_read(const uint8_t* s, size_t* count) {
  kk_char_t b = *s;
  kk_char_t c;
  if (kk_likely(b <= 0x7F)) {
    *count = 1;
    c = b; // fast path ASCII
  }
  else if (b <= 0xBF) { // invalid continuation byte (check is strictly not necessary as we don't validate..)
    *count = (kk_utf8_next(s) - s);  // skip to next
    c = kk_char_replacement;
  }
  else if (b <= 0xDF) { // b >= 0xC0  // 2 bytes
    *count = 2;
    c = (((b & 0x1F) << 6) | (s[1] & 0x3F));
  }
  else if (b <= 0xEF) { // b >= 0xE0  // 3 bytes 
    *count = 3;
    c = (((b & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F));
  }
  else if (b <= 0xF4) { // b >= 0xF0  // 4 bytes 
    *count = 4;
    c = (((b & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F));
  }
  // invalid, skip continuation bytes
  else {  // b >= 0xF5
    *count = (size_t)(kk_utf8_next(s) - s);  // skip to next
    c = kk_char_replacement;
  }
#if (DEBUG!=0)
  size_t dcount;
  kk_assert_internal(c == kk_utf8_read_validate(s, &dcount));
  kk_assert_internal(*count == dcount);
#endif
  return c;
}

// Number of bytes needed to represent a single code point
static inline size_t kk_utf8_len(kk_char_t c) {
  if (kk_unlikely(c == 0)) {
    return 2;
  }
  else if (kk_likely(c <= 0x7F)) {
    return 1;
  }
  else if (c <= 0x07FF) {
    return 2;
  }
  else if (c < 0xD800 || (c > 0xDFFF && c <= 0xFFFF)) {
    return 3;
  }
  else if (c >= 0x10000 && c <= 0x10FFFF) {
    return 4;
  }
  else {
    return 3; // replacement
  }
}

// UTF8 encode a single codepoint
static inline void kk_utf8_write(kk_char_t c, uint8_t* s, size_t* count) {
  if (kk_unlikely(c == 0)) {
    // modified UTF8 zero
    *count = 2;
    s[0] = 0xC0;
    s[1] = 0x80;
  }
  else if (kk_likely(c <= 0x7F)) {
    *count = 1;
    s[0] = (uint8_t)c;
  }
  else if (c <= 0x07FF) {
    *count = 2;
    s[0] = (0xC0 | ((uint8_t)(c >> 6)));
    s[1] = (0x80 | (((uint8_t)c) & 0x3F));
  }
  else if (c < 0xD800 || (c > 0xDFFF && c <= 0xFFFF)) {
    *count = 3;
    s[0] = (0xE0 |  ((uint8_t)(c >> 12)));
    s[1] = (0x80 | (((uint8_t)(c >>  6)) & 0x3F));
    s[2] = (0x80 | (((uint8_t)c) & 0x3F));
  }
  else if (c >= 0x10000 && c <= 0x10FFFF) {
    *count = 4;
    s[0] = (0xF0 |  ((uint8_t)(c >> 18)));
    s[1] = (0x80 | (((uint8_t)(c >> 12)) & 0x3F));
    s[2] = (0x80 | (((uint8_t)(c >>  6)) & 0x3F));
    s[3] = (0x80 | (((uint8_t)c) & 0x3F));
  }
  else {
    // invalid: encode 0xFFFD
    *count = 3;
    s[0] = 0xEF;
    s[1] = 0xBF;
    s[2] = 0xBD;
  }
}


/*--------------------------------------------------------------------------------------------------
  
--------------------------------------------------------------------------------------------------*/
static inline size_t kk_decl_pure kk_string_len(kk_string_t str, kk_context_t* ctx) {    // bytes in UTF8
  size_t len = kk_string_len_borrow(str);
  kk_string_drop(str,ctx);
  return len;
}

kk_decl_export size_t kk_decl_pure kk_string_count(kk_string_t str);  // number of code points

kk_decl_export int kk_string_cmp_borrow(kk_string_t str1, kk_string_t str2);
kk_decl_export int kk_string_cmp(kk_string_t str1, kk_string_t str2, kk_context_t* ctx);
kk_decl_export int kk_string_icmp_borrow(kk_string_t str1, kk_string_t str2);             // ascii case insensitive
kk_decl_export int kk_string_icmp(kk_string_t str1, kk_string_t str2, kk_context_t* ctx);    // ascii case insensitive

static inline bool kk_string_is_eq(kk_string_t s1, kk_string_t s2, kk_context_t* ctx) {
  return (kk_string_cmp(s1, s2, ctx) == 0);
}
static inline bool kk_string_is_neq(kk_string_t s1, kk_string_t s2, kk_context_t* ctx) {
  return (kk_string_cmp(s1, s2, ctx) != 0);
}

kk_decl_export kk_string_t kk_string_cat(kk_string_t s1, kk_string_t s2, kk_context_t* ctx);

kk_decl_export kk_string_t kk_string_from_char(kk_char_t c, kk_context_t* ctx);
kk_decl_export kk_string_t kk_string_from_chars(kk_vector_t v, kk_context_t* ctx);
kk_decl_export kk_vector_t kk_string_to_chars(kk_string_t s, kk_context_t* ctx);

kk_decl_export kk_string_t kk_integer_to_string(kk_integer_t x, kk_context_t* ctx);
kk_decl_export kk_string_t kk_integer_to_hex_string(kk_integer_t x, bool use_capitals, kk_context_t* ctx);

kk_decl_export kk_vector_t kk_string_splitv(kk_string_t s, kk_string_t sep, kk_context_t* ctx);
kk_decl_export kk_vector_t kk_string_splitv_atmost(kk_string_t s, kk_string_t sep, size_t n, kk_context_t* ctx);

kk_decl_export kk_string_t kk_string_repeat(kk_string_t s, size_t n, kk_context_t* ctx);

kk_decl_export size_t kk_string_index_of1(kk_string_t str, kk_string_t sub, kk_context_t* ctx);     // returns 0 for not found, or index + 1
kk_decl_export size_t kk_string_last_index_of1(kk_string_t str, kk_string_t sub, kk_context_t* ctx);
kk_decl_export bool   kk_string_starts_with(kk_string_t str, kk_string_t pre, kk_context_t* ctx);
kk_decl_export bool   kk_string_ends_with(kk_string_t str, kk_string_t post, kk_context_t* ctx);
kk_decl_export bool   kk_string_contains(kk_string_t str, kk_string_t sub, kk_context_t* ctx);

kk_decl_export kk_string_t  kk_string_to_upper(kk_string_t str, kk_context_t* ctx);
kk_decl_export kk_string_t  kk_string_to_lower(kk_string_t strs, kk_context_t* ctx);
kk_decl_export kk_string_t  kk_string_trim_left(kk_string_t strs, kk_context_t* ctx);
kk_decl_export kk_string_t  kk_string_trim_right(kk_string_t strs, kk_context_t* ctx);

kk_decl_export kk_unit_t kk_println(kk_string_t s, kk_context_t* ctx);
kk_decl_export kk_unit_t kk_print(kk_string_t s, kk_context_t* ctx);
kk_decl_export kk_unit_t kk_trace(kk_string_t s, kk_context_t* ctx);
kk_decl_export kk_unit_t kk_trace_any(kk_string_t s, kk_box_t x, kk_context_t* ctx);
kk_decl_export kk_string_t kk_show_any(kk_box_t x, kk_context_t* ctx);

kk_decl_export kk_string_t kk_double_show_fixed(double d, int32_t prec, kk_context_t* ctx);
kk_decl_export kk_string_t kk_double_show_exp(double d, int32_t prec, kk_context_t* ctx);


#endif // include guard
