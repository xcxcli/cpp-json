#include<stdio.h>
#include"json.h"

static int ret=0,cnt=0,pass=0;

#define EXPECT_EQ_BASE(equality,expect,actual,format)\
	do{\
		++cnt;\
		if(equality)++pass;\
		else{\
			fprintf(stderr,"%s:%d: expect: " format " actual: " format "\n",__FILE__,__LINE__,expect,actual);\
			ret=1;\
		}\
	}while(0)
#define EXPECT_EQ_INT(expect,actual) EXPECT_EQ_BASE(expect==actual,expect,actual,"%d")

void parse_null(){
	json_value v;
	v.type=JSON_FALSE;EXPECT_EQ_INT(JSON_PARSE_OK,json_parse(&v,"null"));
	EXPECT_EQ_INT(JSON_NULL,json_get_type(&v));
}
void parse_true(){
	json_value v;
	v.type=JSON_FALSE;EXPECT_EQ_INT(JSON_PARSE_OK,json_parse(&v,"true"));
	EXPECT_EQ_INT(JSON_TRUE,json_get_type(&v));
}
void parse_false(){
	json_value v;
	v.type=JSON_TRUE;EXPECT_EQ_INT(JSON_PARSE_OK,json_parse(&v,"false"));
	EXPECT_EQ_INT(JSON_FALSE,json_get_type(&v));
}
void parse_expect_value(){
	json_value v;
	v.type=JSON_FALSE;EXPECT_EQ_INT(JSON_PARSE_EXPECT_VALUE,json_parse(&v,""));
	EXPECT_EQ_INT(JSON_NULL,json_get_type(&v));
	v.type=JSON_FALSE;EXPECT_EQ_INT(JSON_PARSE_EXPECT_VALUE,json_parse(&v," "));
	EXPECT_EQ_INT(JSON_NULL,json_get_type(&v));
}
void parse_invalid_value(){
	json_value v;
	v.type=JSON_FALSE;EXPECT_EQ_INT(JSON_PARSE_INVALID_VALUE,json_parse(&v,"nul"));
	EXPECT_EQ_INT(JSON_NULL,json_get_type(&v));
	v.type=JSON_FALSE;EXPECT_EQ_INT(JSON_PARSE_INVALID_VALUE,json_parse(&v,"?"));
	EXPECT_EQ_INT(JSON_NULL,json_get_type(&v));
}
void parse_root_not_singular(){
	json_value v;
	v.type=JSON_FALSE;EXPECT_EQ_INT(JSON_PARSE_ROOT_NOT_SINGULAR,json_parse(&v,"null null"));
	EXPECT_EQ_INT(JSON_NULL,json_get_type(&v));
}
void test_parse(){
	parse_null();
	parse_true();
	parse_false();
	parse_expect_value();
	parse_invalid_value();
	parse_root_not_singular();
}

int main(){
	test_parse();
	printf("test:%d passed:%d\n",cnt,pass);
	return ret;
}
