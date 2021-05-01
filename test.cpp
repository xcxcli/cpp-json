#include<stdio.h>
#include<string.h>
#include<time.h>
#include"json.h"

static int ret=0,cnt=0,pass=0;

#define EXPECT_BASE(equality,expect,actual,format)\
	do{\
		++cnt;\
		if(equality)++pass;\
		else{\
			fprintf(stderr,"%s:%d: expect: " format " actual: " format "\n",__FILE__,__LINE__,expect,actual);\
			ret=1;\
		}\
	}while(0)
#define EXPECT_TRUE(expect) EXPECT_BASE((expect)==JSON_TRUE,"true","false","%s")
#define EXPECT_FALSE(expect) EXPECT_BASE((expect)==JSON_FALSE,"false","true","%s")
#define EXPECT_INT(expect,actual) EXPECT_BASE((expect)==(actual),expect,actual,"%d")
#define EXPECT_DOUBLE(expect,actual) EXPECT_BASE((expect)==(actual),expect,actual,"%.17f")
#define EXPECT_STRING(expect,actual,len)\
	EXPECT_BASE(sizeof expect==len+1&&memcmp(expect,actual,len)==0,expect,actual,"%s")

void parse_null(){
	json_value v;json_init(&v);
	EXPECT_INT(JSON_PARSE_OK,json_parse(&v,"null"));
	EXPECT_INT(JSON_NULL,json_get_type(&v));
	json_free(&v);
}
void parse_true(){
	json_value v;json_init(&v);
	EXPECT_INT(JSON_PARSE_OK,json_parse(&v,"true"));
	EXPECT_INT(JSON_TRUE,json_get_type(&v));
	json_free(&v);
}
void parse_false(){
	json_value v;json_init(&v);
	EXPECT_INT(JSON_PARSE_OK,json_parse(&v,"false"));
	EXPECT_INT(JSON_FALSE,json_get_type(&v));
	json_free(&v);
}
#define TEST_NUMBER(expect,json)\
	do{\
		json_value v;json_init(&v);\
		EXPECT_INT(JSON_PARSE_OK,json_parse(&v,json));\
		EXPECT_INT(JSON_NUMBER,json_get_type(&v));\
		EXPECT_DOUBLE(expect,json_get_number(&v));\
		json_free(&v);\
	}while(0)
void parse_number(){
	TEST_NUMBER(0.0,"0");
	TEST_NUMBER(0.0,"-0");
	TEST_NUMBER(0.0,"-0.0");
	TEST_NUMBER(1.0,"1");
	TEST_NUMBER(-1.0,"-1");
	TEST_NUMBER(1.5,"1.5");
	TEST_NUMBER(-1.5,"-1.5");
	TEST_NUMBER(3.1416,"3.1416");
	TEST_NUMBER(1E10,"1E10");
	TEST_NUMBER(1e10,"1e10");
	TEST_NUMBER(1E+10,"1E+10");
	TEST_NUMBER(1E-10,"1E-10");
	TEST_NUMBER(-1E10,"-1E10");
	TEST_NUMBER(-1e10,"-1e10");
	TEST_NUMBER(-1E+10,"-1E+10");
	TEST_NUMBER(-1E-10,"-1E-10");
	TEST_NUMBER(1.234E+10,"1.234E+10");
	TEST_NUMBER(1.234E-10,"1.234E-10");
	TEST_NUMBER(0.0,"1e-10000");//overflow
	TEST_NUMBER(1.0000000000000002,"1.0000000000000002");//the smallest number >1
	TEST_NUMBER( 4.9406564584124654e-324,"4.9406564584124654e-324");//minimum denormal
	TEST_NUMBER(-4.9406564584124654e-324,"-4.9406564584124654e-324");
	TEST_NUMBER( 2.2250738585072009e-308,"2.2250738585072009e-308");//Max subnormal double
	TEST_NUMBER(-2.2250738585072009e-308,"-2.2250738585072009e-308");
	TEST_NUMBER( 2.2250738585072014e-308,"2.2250738585072014e-308");//Min normal positive double
	TEST_NUMBER(-2.2250738585072014e-308,"-2.2250738585072014e-308");
	TEST_NUMBER( 1.7976931348623157e+308,"1.7976931348623157e+308");//Max double
	TEST_NUMBER(-1.7976931348623157e+308,"-1.7976931348623157e+308");
}
#define TEST_STRING(expect,json)\
	do{\
		json_value v;json_init(&v);\
		EXPECT_INT(JSON_PARSE_OK,json_parse(&v,json));\
		EXPECT_INT(JSON_STRING,json_get_type(&v));\
		EXPECT_STRING(expect,json_get_string(&v),json_get_string_length(&v));\
		json_free(&v);\
	}while(0)
void parse_string(){
	TEST_STRING("","\"\"");
	TEST_STRING("Hello","\"Hello\"");
	TEST_STRING("Hello\nWorld","\"Hello\\nWorld\"");
	TEST_STRING("\" \\ / \b \f \n \r \t","\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
}

#define TEST_ERROR(error,json)\
	do{\
		json_value v;json_init(&v);\
		EXPECT_INT(error,json_parse(&v,json));\
		EXPECT_INT(JSON_NULL,json_get_type(&v));\
		json_free(&v);\
	}while(0)
void parse_expect_value(){
	TEST_ERROR(JSON_PARSE_EXPECT_VALUE,"");
	TEST_ERROR(JSON_PARSE_EXPECT_VALUE," ");
}
void parse_invalid_value(){
	TEST_ERROR(JSON_PARSE_INVALID_VALUE,"nul");
	TEST_ERROR(JSON_PARSE_INVALID_VALUE,"?");
	TEST_ERROR(JSON_PARSE_INVALID_VALUE,"+0");
	TEST_ERROR(JSON_PARSE_INVALID_VALUE,"+1");
	TEST_ERROR(JSON_PARSE_INVALID_VALUE,".123");//at least 1 digit before '.'
	TEST_ERROR(JSON_PARSE_INVALID_VALUE,"1.");//at least 1 digit after '.'
	TEST_ERROR(JSON_PARSE_INVALID_VALUE,"INF");
	TEST_ERROR(JSON_PARSE_INVALID_VALUE,"inf");
	TEST_ERROR(JSON_PARSE_INVALID_VALUE,"NAN");
	TEST_ERROR(JSON_PARSE_INVALID_VALUE,"nan");
}
void parse_root_not_singular(){
	TEST_ERROR(JSON_PARSE_ROOT_NOT_SINGULAR,"null null");
	TEST_ERROR(JSON_PARSE_ROOT_NOT_SINGULAR,"0123");//after 0 should be '.','E','e' or nothing
	TEST_ERROR(JSON_PARSE_ROOT_NOT_SINGULAR,"0x0");
	TEST_ERROR(JSON_PARSE_ROOT_NOT_SINGULAR,"0x123");
}
void parse_number_too_big(){
	TEST_ERROR(JSON_PARSE_NUMBER_TOO_BIG,"1e309");
	TEST_ERROR(JSON_PARSE_NUMBER_TOO_BIG,"-1e309");
}
void parse_miss_quotation_mark(){
	TEST_ERROR(JSON_PARSE_MISS_QUOTATION_MARK,"\"");
	TEST_ERROR(JSON_PARSE_MISS_QUOTATION_MARK,"\"abc");
}
void parse_invalid_string_char(){
	TEST_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE,"\"\\v\"");
	TEST_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE,"\"\\'\"");
	TEST_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE,"\"\\0\"");
	TEST_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE,"\"\\x12\"");
}
void parse_invalid_string_escape(){
	TEST_ERROR(JSON_PARSE_INVALID_STRING_CHAR,"\"\x01\"");
	TEST_ERROR(JSON_PARSE_INVALID_STRING_CHAR,"\"\x1F\"");
}
void parse(){
	parse_null();
	parse_true();
	parse_false();
	parse_number();
	parse_string();

	parse_expect_value();
	parse_invalid_value();
	parse_root_not_singular();
	parse_number_too_big();
	parse_miss_quotation_mark();
	parse_invalid_string_char();
	parse_invalid_string_escape();
}

void access_null(){
	json_value v;json_init(&v);
	json_set_string(&v,"a",1);
	json_set_null(&v);
	EXPECT_INT(JSON_NULL,json_get_type(&v));
	json_free(&v);
}
void access_boolean(){
	json_value v;json_init(&v);
	json_set_string(&v,"a",1);
	json_set_boolean(&v,true);
	EXPECT_TRUE(json_get_type(&v));
	json_set_boolean(&v,false);
	EXPECT_FALSE(json_get_type(&v));
	json_free(&v);
}
void access_number(){
	json_value v;json_init(&v);
	json_set_string(&v,"a",1);
	json_set_number(&v,2.718281828);
	EXPECT_DOUBLE(2.718281828,json_get_number(&v));
	json_free(&v);
}
void access_string(){
	json_value v;json_init(&v);
	json_set_string(&v,"a",1);
	json_set_string(&v,"string",6);
	EXPECT_STRING("string",json_get_string(&v),json_get_string_length(&v));
	json_free(&v);
}
void access(){
	access_null();
	access_boolean();
	access_number();
	access_string();
}

int main(){
	clock_t t=clock();
	parse(),access();
	printf("test:%d passed:%d time:%dms\n",cnt,pass,int(clock()-t));
	return ret;
}
