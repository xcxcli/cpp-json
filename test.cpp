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
	TEST_STRING("Hello\0World","\"Hello\\u0000World\"");
	TEST_STRING("\x24","\"\\u0024\"");//Dollar sign U+0024
	TEST_STRING("\xC2\xA2","\"\\u00A2\"");//Cents sign U+00A2
	TEST_STRING("\xE2\x82\xAC","\"\\u20AC\"");//Euro sign U+20AC
	TEST_STRING("\xF0\x9D\x84\x9E","\"\\uD834\\uDD1E\"");//G clef sign U+1D11E
	TEST_STRING("\xF0\x9D\x84\x9E","\"\\ud834\\udd1e\"");
}

int json_error;
#define TEST_ERROR(json)\
	do{\
		json_value v;json_init(&v);\
		EXPECT_INT(json_error,json_parse(&v,json));\
		EXPECT_INT(JSON_NULL,json_get_type(&v));\
		json_free(&v);\
	}while(0)
void parse_expect_value(){
	json_error=JSON_PARSE_EXPECT_VALUE;
	TEST_ERROR("");
	TEST_ERROR(" ");
}
void parse_invalid_value(){
	json_error=JSON_PARSE_INVALID_VALUE;
	TEST_ERROR("nul");
	TEST_ERROR("?");
	TEST_ERROR("+0");
	TEST_ERROR("+1");
	TEST_ERROR(".123");//at least 1 digit before '.'
	TEST_ERROR("1.");//at least 1 digit after '.'
	TEST_ERROR("INF");
	TEST_ERROR("inf");
	TEST_ERROR("NAN");
	TEST_ERROR("nan");
}
void parse_root_not_singular(){
	json_error=JSON_PARSE_ROOT_NOT_SINGULAR;
	TEST_ERROR("null null");
	TEST_ERROR("0123");//after 0 should be '.','E','e' or nothing
	TEST_ERROR("0x0");
	TEST_ERROR("0x123");
}
void parse_number_too_big(){
	json_error=JSON_PARSE_NUMBER_TOO_BIG;
	TEST_ERROR("1e309");
	TEST_ERROR("-1e309");
}
void parse_miss_quotation_mark(){
	json_error=JSON_PARSE_MISS_QUOTATION_MARK;
	TEST_ERROR("\"");
	TEST_ERROR("\"abc");
}
void parse_invalid_string_char(){
	json_error=JSON_PARSE_INVALID_STRING_ESCAPE;
	TEST_ERROR("\"\\v\"");
	TEST_ERROR("\"\\'\"");
	TEST_ERROR("\"\\0\"");
	TEST_ERROR("\"\\x12\"");
}
void parse_invalid_string_escape(){
	json_error=JSON_PARSE_INVALID_STRING_CHAR;
	TEST_ERROR("\"\x01\"");
	TEST_ERROR("\"\x1F\"");
}
void parse_invalid_unicode_hex(){
	json_error=JSON_PARSE_INVALID_UNICODE_HEX;
	TEST_ERROR("\"\\u\"");
	TEST_ERROR("\"\\u0\"");
	TEST_ERROR("\"\\u01\"");
	TEST_ERROR("\"\\u012\"");
	TEST_ERROR("\"\\u/000\"");
	TEST_ERROR("\"\\uG000\"");
	TEST_ERROR("\"\\u0/00\"");
	TEST_ERROR("\"\\u0G00\"");
	TEST_ERROR("\"\\u00/0\"");
	TEST_ERROR("\"\\u00G0\"");
	TEST_ERROR("\"\\u000/\"");
	TEST_ERROR("\"\\u000G\"");
	TEST_ERROR("\"\\u 123\"");
}
void parse_invalid_unicode_surrogate(){
	json_error=JSON_PARSE_INVALID_UNICODE_SURROGATE;
	TEST_ERROR("\"\\uD800\"");
	TEST_ERROR("\"\\uDBFF\"");
	TEST_ERROR("\"\\uD800\\\\\"");
	TEST_ERROR("\"\\uD800\\uDBFF\"");
	TEST_ERROR("\"\\uD800\\uE000\"");
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
	parse_invalid_unicode_hex();
	parse_invalid_unicode_surrogate();
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
