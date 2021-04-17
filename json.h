#ifndef JSON_H__
#define JSON_H__ "0.1.0"
typedef enum{JSON_NULL,JSON_TRUE,JSON_FALSE,JSON_NUMBER,JSON_STRING,JSON_ARRAY,JSON_OBJECT}json_type;
typedef struct{
	json_type type;
}json_value;
enum{
	JSON_PARSE_OK=0,
	JSON_PARSE_EXPECT_VALUE,
	JSON_PARSE_INVALID_VALUE,
	JSON_PARSE_ROOT_NOT_SINGULAR
};
json_type json_get_type(const json_value*v);
int json_parse(json_value*v,const char*json);
#endif
