#ifndef JSON_H__
#define JSON_H__ "0.1.0"
#include<stddef.h>
typedef enum{JSON_NULL,JSON_TRUE,JSON_FALSE,JSON_NUMBER,JSON_STRING,JSON_ARRAY,JSON_OBJECT}json_type;
typedef struct json_value json_value;
struct json_value{
	union{
		double n;
		struct{char*s;size_t len;}s;
		struct{json_value*e;size_t size;}a;
	}u;
	json_type type;
};
#define JSON_VALUE_SIZE (sizeof(json_value))
enum{
	JSON_PARSE_OK=0,
	JSON_PARSE_EXPECT_VALUE,
	JSON_PARSE_INVALID_VALUE,
	JSON_PARSE_ROOT_NOT_SINGULAR,
	JSON_PARSE_NUMBER_TOO_BIG,
	JSON_PARSE_MISS_QUOTATION_MARK,
	JSON_PARSE_INVALID_STRING_CHAR,
	JSON_PARSE_INVALID_STRING_ESCAPE,
	JSON_PARSE_INVALID_UNICODE_HEX,
	JSON_PARSE_INVALID_UNICODE_SURROGATE,
	JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET
};

#define json_init(v) do{(v)->type=JSON_NULL;}while(0)
void json_free(json_value*v);
int json_parse(json_value*v,const char*json);

#define json_set_null(v) json_free(v)
void json_set_boolean(json_value*v,bool b);
void json_set_number(json_value*v,double n);
void json_set_string(json_value*v,const char*s,size_t len);

json_type json_get_type(const json_value*v);
double json_get_number(const json_value*v);
const char*json_get_string(const json_value*v);
size_t json_get_string_length(const json_value*v);
json_value*json_get_array_element(const json_value*v,size_t index);
size_t json_get_array_size(const json_value*v);
#endif
