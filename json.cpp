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
#define JSON_STACK_INIT_SIZE 16
#endif
typedef struct{const char*json;char*stack;size_t sz,top;}json_text;
void*json_text_push(json_text*c,size_t sz){
	assert(sz>0);void*ret;
	if(c->top+sz>=c->sz){
		if(c->sz==0)c->sz=JSON_STACK_INIT_SIZE;
		while(c->top+sz>=c->sz)c->sz+=c->sz>>1;
		#if 0
		c->stack=(char*)realloc(c->stack,c->sz);
		#else
		if(c->stack==NULL)c->stack=new char[c->sz];
		else{
			char*t=new char[c->sz];
			memcpy(t,c->stack,c->top);
			delete c->stack;
			c->stack=t;
		}
		#endif
	}
	ret=c->stack+c->top,c->top+=sz;
	return ret;
}
void*json_text_pop(json_text*c,size_t sz){
	assert(c->sz>=sz);
	return c->stack+(c->top-=sz);
}

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
#define PUTC(c,ch) do{*(char*)json_text_push(c,1)=(ch);}while(0)
int json_parse_string(json_text*c,json_value*v){
	EXPECT(c,'"');
	const char*p=c->json;size_t pre=c->top,len;
	while(1){
		char ch=*p++;
		switch(ch){
			case'"':
				len=c->top-pre,json_set_string(v,(char*)json_text_pop(c,len),len);
				c->json=p;
				return JSON_PARSE_OK;
			case'\0':c->top=pre;return JSON_PARSE_MISS_QUOTATION_MARK;
			case'\\':
				switch(*p++){
					case '"':PUTC(c,'"');break;//"\/bfnrt u
					case '\\':PUTC(c,'\\');break;
					case '/':PUTC(c,'/');break;
					case 'b':PUTC(c,'\b');break;
					case 'f':PUTC(c,'\f');break;
					case 'n':PUTC(c,'\n');break;
					case 'r':PUTC(c,'\r');break;
					case 't':PUTC(c,'\t');break;
					default:c->top=pre;return JSON_PARSE_INVALID_STRING_ESCAPE;
				}
				break;
			default:
				if((unsigned char)ch<0x20){c->top=pre;return JSON_PARSE_INVALID_STRING_CHAR;}
				PUTC(c,ch);
		}
	}
}
int json_parse_value(json_text*c,json_value*v){
	switch(*c->json){
		case'n':return json_parse_literal(c,v,"null",JSON_NULL);
		case't':return json_parse_literal(c,v,"true",JSON_TRUE);
		case'f':return json_parse_literal(c,v,"false",JSON_FALSE);
		default:return json_parse_number(c,v);
		case'"':return json_parse_string(c,v);
		case'\0':return JSON_PARSE_EXPECT_VALUE;
	}
}
int json_parse(json_value*v,const char*json){
	assert(json!=NULL);json_init(v);
	json_text c;int ret;
	c.json=json,c.stack=NULL,c.sz=c.top=0,v->type=JSON_NULL;
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