#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}


static int lept_parse_literal(lept_context* c, lept_value* v, lept_type type, const char * str) {
    EXPECT(c, *str);
    ++str;
    while (*str != '\0') {
        if (*str != *c->json) {
            return LEPT_PARSE_INVALID_VALUE;
        }
        ++str;
        ++(c->json);
    }

    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_true(lept_context* c, lept_value* v) {
    EXPECT(c, 't');
    if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_TRUE;
    return LEPT_PARSE_OK;
}

static int lept_parse_false(lept_context* c, lept_value* v) {
    EXPECT(c, 'f');
    if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 4;
    v->type = LEPT_FALSE;
    return LEPT_PARSE_OK;
}

static int lept_parse_null(lept_context* c, lept_value* v) {
    EXPECT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    char* end;
    printf("before strtod:c->json: %s, end: %s, v->n: %lf\n", c->json, end, v->n);

    /* \TODO validate number */
    const char * tmp = c->json;
    if (*tmp == '-') {
        ++tmp;
    }
    // 整数部分如果是 0 开始，只能是单个 0；而由 1-9 开始的话，可以加任意数量的数字（0-9）
    if (*tmp == '0') {
        ++tmp;
    } else if (ISDIGIT1TO9(*tmp)) {  // 1 to 9
        ++tmp;
        while (ISDIGIT(*tmp)) {
            ++tmp;
        }
    } else {  // 整数部分是必须选择的, 所以有个else
        return LEPT_PARSE_INVALID_VALUE;
    }

    // 小数部分比较直观，就是小数点后是一或多个数字（0-9, 可选
    if (*tmp == '.') {
        ++tmp;
        if (ISDIGIT(*tmp)) {
            ++tmp;
            while (ISDIGIT(*tmp)) {
                ++tmp;
            }
        } else {
            return LEPT_PARSE_INVALID_VALUE;
        }
    }

    // 指数部分由大写 E 或小写 e 开始，然后可有正负号，之后是一或多个数字（0-9, 可选
    if (*tmp == 'e' || *tmp == 'E') {
        ++tmp;
        if (*tmp == '+' || *tmp == '-') {
            ++tmp;
        }
        // 一或多个数字（0-9）
        if (ISDIGIT(*tmp)) {
            ++tmp;
            while (ISDIGIT(*tmp)) {
                ++tmp;
            }
        }
    }

    // 防止出现 0123, 0x0这种情况
    if (*tmp != '\0') {
        v->type = LEPT_NULL;
        return LEPT_PARSE_ROOT_NOT_SINGULAR;
    }
    errno = 0;
    v->n = strtod(c->json, &end);
    printf("after strtod:c->json: %s, end: %s, v->n: %lf\n", c->json, end, v->n);
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL)){
        errno = 0;
        printf("return LEPT_PARSE_NUMBER_TOO_BIG\n");
        return LEPT_PARSE_NUMBER_TOO_BIG;
    }

    if (c->json == end)   {
        // 表明c->json目前是空的.
        printf("return LEPT_PARSE_INVALID_VALUE\n");
        return LEPT_PARSE_INVALID_VALUE;
    }
    c->json = end;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't': return lept_parse_literal(c, v, LEPT_TRUE, "true");
        case 'f': return lept_parse_literal(c, v, LEPT_FALSE, "false");
        case 'n': return lept_parse_literal(c, v, LEPT_NULL, "null");
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    printf("Parsing: json: %s\n", json);
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
