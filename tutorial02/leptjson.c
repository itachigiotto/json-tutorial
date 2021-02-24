#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <math.h> /* HUGE_VAL */
#include <errno.h> /* errno */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* type_str, lept_type vtype) {
    size_t pos;
    EXPECT(c, type_str[0]);
    for (pos = 0; type_str[pos + 1] != '\0'; ++pos) {
        if (c->json[pos] != type_str[pos + 1]) {
            return LEPT_PARSE_INVALID_VALUE;
        }
    }
    c->json += pos;
    v->type = vtype;
    return LEPT_PARSE_OK;
}

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

static int lept_parse_number(lept_context* c, lept_value* v) {
    char* end;
    const char* pos = c->json;
    /* process '-' */
    if (*pos == '-') ++pos;

    /* process int */
    if (*pos == '0') {
        ++pos;
        /* if (*pos != '\0' && *pos != '.' && *pos != 'e' && *pos != 'E')
            return LEPT_PARSE_ROOT_NOT_SINGULAR; */
    }
    else if (!ISDIGIT1TO9(*pos)) {
        return LEPT_PARSE_INVALID_VALUE;
    }
    else {
        ++pos;
        while (ISDIGIT1TO9(*pos)) ++pos;
    }

    /* process frac */
    if (*pos == '.') {
        ++pos;
        if (!ISDIGIT(*pos)) return LEPT_PARSE_INVALID_VALUE;
        while (ISDIGIT(*pos)) ++pos;
    }

    /* process exp */
    if (*pos == 'e' || *pos == 'E') {
        ++pos;
        if (*pos == '-' || *pos == '+') ++pos;
        if (!ISDIGIT(*pos)) return LEPT_PARSE_INVALID_VALUE;
        while (ISDIGIT(*pos)) ++pos;
    }

    errno = 0;
    v->n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL)) {
        return LEPT_PARSE_NUMBER_TOO_BIG;
    }
    c->json = pos;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
