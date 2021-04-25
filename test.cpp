#include<stdio.h>
#include<time.h>
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
#define EXPECT_EQ_INT(expect,actual) EXPECT_EQ_BASE((expect)==(actual),expect,actual,"%d")
#define EXPECT_EQ_DOUBLE(expect,actual) EXPECT_EQ_BASE((expect)==(actual),expect,actual,"%.17f")

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
#define TEST_NUMBER(expect,json)\
	do{\
		json_value v;v.type=JSON_NULL;\
		EXPECT_EQ_INT(JSON_PARSE_OK,json_parse(&v,json));\
		EXPECT_EQ_INT(JSON_NUMBER,json_get_type(&v));\
		EXPECT_EQ_DOUBLE(expect,json_get_number(&v));\
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

#define TEST_ERROR(error,json)\
	do{\
		json_value v;v.type=JSON_NULL;\
		EXPECT_EQ_INT(error,json_parse(&v,json));\
		EXPECT_EQ_INT(JSON_NULL,json_get_type(&v));\
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
void parse(){
	parse_null();
	parse_true();
	parse_false();
	parse_number();
	parse_expect_value();
	parse_invalid_value();
	parse_root_not_singular();
	parse_number_too_big();
}

int main(){
	clock_t t=clock();
	parse();
	printf("test:%d passed:%d time:%dms\n",cnt,pass,int(clock()-t));
	return ret;
}
