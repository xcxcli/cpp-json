#include"json.h"
#include<math.h>
#include<errno.h>
#include<assert.h>
#include<stdlib.h>
#define EXPECT(c,ch) do{assert(*c->json==ch),++c->json;}while(0)
#define ISDIGIT(ch) ((ch)>='0'&&(ch)<='9')
#define ISDIGIT_(ch) ((ch)>'0'&&(ch)<='9')

typedef struct{
	const char*json;
}json_text;

void json_parse_whitespace(json_text*c){
	const char*p=c->json;
	while(*p==' '||*p=='\n'||*p=='\t'||*p=='\r')++p;
	c->json=p;
}
int json_parse_literal(json_text*c,json_value*v,const char*literal,const json_type type){
	size_t i=1;
	EXPECT(c,literal[0]);
	for(;literal[i];++i)if(literal[i]!=c->json[i-1])return JSON_PARSE_INVALID_VALUE;
	c->json+=i-1,v->type=type;
	return JSON_PARSE_OK;
}

int json_parse_number(json_text*c,json_value*v){
	const char*p=c->json;
	if(*p=='-')++p;
	if(*p=='0')++p;
	else{
		if(!ISDIGIT_(*p))return JSON_PARSE_INVALID_VALUE;
		++p;while(ISDIGIT(*p))++p;
	}
	if(*p=='.'){
		++p;
		if(!ISDIGIT(*p))return JSON_PARSE_INVALID_VALUE;
		++p;while(ISDIGIT(*p))++p;
	}
	if(*p=='E'||*p=='e'){
		++p;
		if(*p=='+'||*p=='-')++p;
		if(!ISDIGIT(*p))return JSON_PARSE_INVALID_VALUE;
		++p;while(ISDIGIT(*p))++p;
	}
	errno=0,v->n=strtod(c->json,NULL);
	if(errno==ERANGE&&(v->n==HUGE_VAL||v->n==-HUGE_VAL))return JSON_PARSE_NUMBER_TOO_BIG;
	c->json=p,v->type=JSON_NUMBER;
	return JSON_PARSE_OK;
}
int json_parse_value(json_text*c,json_value*v){
	switch(*c->json){
		case'n':return json_parse_literal(c,v,"null",JSON_NULL);
		case't':return json_parse_literal(c,v,"true",JSON_TRUE);
		case'f':return json_parse_literal(c,v,"false",JSON_FALSE);
		case'\0':return JSON_PARSE_EXPECT_VALUE;
		default:return json_parse_number(c,v);
	}
}
int json_parse(json_value*v,const char*json){
	assert(json!=NULL);
	json_text c;int ret;
	c.json=json,v->type=JSON_NULL;
	json_parse_whitespace(&c);
	if((ret=json_parse_value(&c,v))==JSON_PARSE_OK){
		json_parse_whitespace(&c);
		if(*c.json!='\0')ret=JSON_PARSE_ROOT_NOT_SINGULAR,v->type=JSON_NULL;
	}
	return ret;
}
json_type json_get_type(const json_value*v){
	assert(v!=NULL);
	return v->type;
}
double json_get_number(const json_value*v){
	assert(v!=NULL&&v->type==JSON_NUMBER);
	return v->n;
}
