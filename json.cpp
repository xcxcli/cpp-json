#include"json.h"
#include<math.h>
#include<string.h>
#include<errno.h>
#include<assert.h>
#include<stdlib.h>
#define EXPECT(c,ch) do{assert(*c->json==ch),++c->json;}while(0)
#define ISDIGIT(ch) ((ch)>='0'&&(ch)<='9')
#define ISDIGIT_(ch) ((ch)>'0'&&(ch)<='9')

#ifndef JSON_STACK_INIT_SIZE
#define JSON_STACK_INIT_SIZE 256
#endif
typedef struct json_text{
	const char*json;char*stack;size_t size,top;
	void*push(size_t sz){
		assert(sz>0);void*ret;
		if(top+sz>=size){
			if(size==0)size=JSON_STACK_INIT_SIZE;
			while(top+sz>=size)size+=size>>1;
			//stack=(char*)realloc(stack,size);
			if(stack==NULL)stack=new char[size];
			else{
				char*t=new char[size];
				memcpy(t,stack,top);delete stack;stack=t;
			}
		}
		ret=stack+top,top+=sz;
		return ret;
	}
	void*pop(size_t sz){
		assert(size>=sz);
		return stack+(top-=sz);
	}
}json_text;

void json_parse_whitespace(json_text*c){
	const char*p=c->json;
	while(*p==' '||*p=='\n'||*p=='\t'||*p=='\r')++p;
	c->json=p;
}
int json_parse_literal(json_text*c,json_value*v,const char*literal,const json_type type){
	EXPECT(c,literal[0]);size_t i=1;
	for(;literal[i];++i)if(literal[i]!=c->json[i-1])return JSON_PARSE_INVALID_VALUE;
	c->json+=i-1,v->type=type;
	return JSON_PARSE_OK;
}
const char*json_parse_hex(const char*p,unsigned&u){
	u=0;
	for(int i=0;i<4;++i){
		char ch=*p++;u<<=4;
		if(ch>='0'&&ch<='9')u|=ch-'0';
		else if(ch>='A'&&ch<='F')u|=ch-'A'+10;
		else if(ch>='a'&&ch<='f')u|=ch-'a'+10;
		else return NULL;
	}
	return p;
}

int json_parse_value(json_text*c,json_value*v);
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
	errno=0,v->u.n=strtod(c->json,NULL);
	if(errno==ERANGE&&(v->u.n==HUGE_VAL||v->u.n==-HUGE_VAL))return JSON_PARSE_NUMBER_TOO_BIG;
	c->json=p,v->type=JSON_NUMBER;
	return JSON_PARSE_OK;
}
#define PUTC(c,ch) do{*(char*)c->push(1)=(char)(ch);}while(0)
#define STRING_ERROR(ret) do{c->top=pre;return ret;}while(0)
void json_utf8(json_text*c,unsigned u){
	if(u<=0x7F)PUTC(c,u);
	else if(u<=0x7FF){PUTC(c,u>>6|0xC0);PUTC(c,(u&0x3F)|0x80);}
	else if(u<=0xFFFF){PUTC(c,u>>12|0xE0);PUTC(c,((u>>6)&0x3f)|0x80);PUTC(c,(u&0x3f)|0x80);}
	else{assert(u<=0x10FFFF);PUTC(c,u>>18|0xF0);PUTC(c,((u>>12)&0x3f)|0x80);PUTC(c,((u>>6)&0x3f)|0x80);PUTC(c,(u&0x3f)|0x80);}
}
int json_parse_string(json_text*c,json_value*v){
	EXPECT(c,'"');
	const char*p=c->json;char ch;size_t pre=c->top,len;unsigned u,u2;
	while(1)switch(ch=*p++){
		case'"':
			len=c->top-pre,json_set_string(v,(char*)c->pop(len),len);
			c->json=p;return JSON_PARSE_OK;
		case'\0':STRING_ERROR(JSON_PARSE_MISS_QUOTATION_MARK);
		case'\\':
			switch(*p++){
				#define ESCAPE(expect,escape) case expect:PUTC(c,escape);break;
				ESCAPE('"','"')ESCAPE('\\','\\')ESCAPE('/','/')ESCAPE('b','\b')ESCAPE('f','\f')ESCAPE('n','\n')ESCAPE('r','\r')ESCAPE('t','\t')
				case 'u':
					if(!(p=json_parse_hex(p,u)))STRING_ERROR(JSON_PARSE_INVALID_UNICODE_HEX);
					if(u>=0xD800&&u<=0xDBFF){
						if(*p++!='\\')STRING_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE);
						if(*p++!='u')STRING_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE);
						if(!(p=json_parse_hex(p,u2)))STRING_ERROR(JSON_PARSE_INVALID_UNICODE_HEX);
						if(u2>0xDFFF||u2<0xDC00)STRING_ERROR(JSON_PARSE_INVALID_UNICODE_SURROGATE);
						u=0x10000+(((u-0xD800)<<10)|(u2-0xDC00));
					}
					json_utf8(c,u);break;
				default:STRING_ERROR(JSON_PARSE_INVALID_STRING_ESCAPE);
			}
			break;
		default:
			if((unsigned char)ch<0x20)STRING_ERROR(JSON_PARSE_INVALID_STRING_CHAR);
			PUTC(c,ch);
	}
}
int json_parse_array(json_text*c,json_value*v){
	EXPECT(c,'[');json_parse_whitespace(c);
	if(*c->json==']'){
		++c->json,v->type=JSON_ARRAY,v->u.a={NULL,0};
		return JSON_PARSE_OK;
	}
	int ret;json_value e;size_t sz=0;
	while(1){
		if((ret=json_parse_value(c,&e))!=JSON_PARSE_OK)break;
		memcpy(c->push(JSON_VALUE_SIZE),&e,JSON_VALUE_SIZE);
		++sz,json_parse_whitespace(c);
		if(*c->json==','){++c->json,json_parse_whitespace(c);continue;}
		else if(*c->json==']'){
			++c->json,v->type=JSON_ARRAY,v->u.a.size=sz;
			memcpy(v->u.a.e=new json_value[sz],c->pop(sz*JSON_VALUE_SIZE),sz*JSON_VALUE_SIZE);
			return JSON_PARSE_OK;
		}
		ret=JSON_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;break;
	}
	while(sz--)json_free((json_value*)c->pop(JSON_VALUE_SIZE));
	return ret;
}
int json_parse_value(json_text*c,json_value*v){
	switch(*c->json){
		case'n':return json_parse_literal(c,v,"null",JSON_NULL);
		case't':return json_parse_literal(c,v,"true",JSON_TRUE);
		case'f':return json_parse_literal(c,v,"false",JSON_FALSE);
		default:return json_parse_number(c,v);
		case'"':return json_parse_string(c,v);
		case'[':return json_parse_array(c,v);
		case'\0':return JSON_PARSE_EXPECT_VALUE;
	}
}
int json_parse(json_value*v,const char*json){
	assert(json!=NULL);json_init(v);
	json_text c;int ret;
	c.json=json,c.stack=NULL,c.size=c.top=0,v->type=JSON_NULL;
	json_parse_whitespace(&c);
	if((ret=json_parse_value(&c,v))==JSON_PARSE_OK){
		json_parse_whitespace(&c);
		if(*c.json!='\0')ret=JSON_PARSE_ROOT_NOT_SINGULAR,v->type=JSON_NULL;
	}
	assert(c.top==0),delete c.stack;
	return ret;
}

void json_free(json_value*v){
	assert(v!=NULL);
	if(v->type==JSON_STRING)delete v->u.s.s;
	if(v->type==JSON_ARRAY){
		size_t t=json_get_array_size(v);
		while(t--)json_free(json_get_array_element(v,t));
		delete v->u.a.e;
	}
	v->type=JSON_NULL;
}
void json_set_boolean(json_value*v,bool b){
	assert(v!=NULL),json_free(v);
	v->type=b?JSON_TRUE:JSON_FALSE;
}
void json_set_number(json_value*v,double n){
	assert(v!=NULL),json_free(v);
	v->u.n=n,v->type=JSON_NUMBER;
}
void json_set_string(json_value*v,const char*s,size_t len){
	assert(v!=NULL&&(s!=NULL||len==0)),json_free(v);
	v->u.s.s=new char[len+1],memcpy(v->u.s.s,s,len),v->u.s.s[len]='\0';
	v->u.s.len=len,v->type=JSON_STRING;
}

json_type json_get_type(const json_value*v){
	assert(v!=NULL);
	return v->type;
}
double json_get_number(const json_value*v){
	assert(v!=NULL&&v->type==JSON_NUMBER);
	return v->u.n;
}
const char*json_get_string(const json_value*v){
	assert(v!=NULL&&v->type==JSON_STRING);
	return v->u.s.s;
}
size_t json_get_string_length(const json_value*v){
	assert(v!=NULL&&v->type==JSON_STRING);
	return v->u.s.len;
}
json_value*json_get_array_element(const json_value*v,size_t index){
	assert(v!=NULL&&v->type==JSON_ARRAY&&index<v->u.a.size);
	return v->u.a.e+index;
}
size_t json_get_array_size(const json_value*v){
	assert(v!=NULL&&v->type==JSON_ARRAY);
	return v->u.a.size;
}