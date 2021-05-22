#include"json.h"
#include<math.h>
#include<string.h>
#include<errno.h>
#include<assert.h>
#include<stdlib.h>
#include<stdio.h>
#define EXPECT(c,ch) do{assert(*c->json==ch),++c->json;}while(0)
typedef struct json_text{
	#ifndef JSON_STACK_INIT_SIZE
	#define JSON_STACK_INIT_SIZE 16
	#endif
	const char*json;char*stack;size_t size,top;
	void*push(size_t sz){
		assert(sz>0);void*ret;
		if(top+sz>=size){
			if(size==0)size=JSON_STACK_INIT_SIZE;
			while(top+sz>=size)size+=size>>1;
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

void json_parse_ws(json_text*c){
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
	#define ISDIGIT(ch) ((ch)>='0'&&(ch)<='9')
	#define ISDIGIT_(ch) ((ch)>'0'&&(ch)<='9')
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
#define PUTC(c,ch) do{*(char*)(c)->push(1)=(char)(ch);}while(0)
void json_utf8(json_text*c,unsigned u){
	if(u<=0x7F)PUTC(c,u);
	else if(u<=0x7FF){PUTC(c,u>>6|0xC0);PUTC(c,(u&0x3F)|0x80);}
	else if(u<=0xFFFF){PUTC(c,u>>12|0xE0);PUTC(c,((u>>6)&0x3f)|0x80);PUTC(c,(u&0x3f)|0x80);}
	else{assert(u<=0x10FFFF);PUTC(c,u>>18|0xF0);PUTC(c,((u>>12)&0x3f)|0x80);PUTC(c,((u>>6)&0x3f)|0x80);PUTC(c,(u&0x3f)|0x80);}
}
int json_parse_string_raw(json_text*c,char**s,size_t&len){
	#define STRING_ERROR(ret) do{c->top=pre;return ret;}while(0)
	EXPECT(c,'"');len=0;
	const char*p=c->json;char ch;size_t pre=c->top;unsigned u,u2;
	while(1)switch(ch=*p++){
		case'"':
			len=c->top-pre,memcpy(*s=new char[len],c->pop(len),len);
			c->json=p;return JSON_PARSE_OK;
		case'\0':STRING_ERROR(JSON_PARSE_MISS_QUOTATION_MARK);
		case'\\':
			switch(*p++){
				#define ESCAPE(expect,escape) case expect:PUTC(c,escape);break;
				ESCAPE('"','"')ESCAPE('\\','\\')ESCAPE('/','/')ESCAPE('b','\b')ESCAPE('f','\f')ESCAPE('n','\n')ESCAPE('r','\r')ESCAPE('t','\t')
				#undef ESCAPE
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
int json_parse_string(json_text*c,json_value*v){
	char*s;size_t len;int ret=json_parse_string_raw(c,&s,len);
	if(ret==JSON_PARSE_OK){json_set_string(v,s,len);delete s;}
	return ret;
}
int json_parse_array(json_text*c,json_value*v){
	EXPECT(c,'[');json_parse_ws(c);
	if(*c->json==']'){
		++c->json,v->type=JSON_ARRAY,v->u.a={NULL,0};
		return JSON_PARSE_OK;
	}
	int ret;json_value e;size_t sz=0;
	while(1){
		if((ret=json_parse_value(c,&e))!=JSON_PARSE_OK)break;
		memcpy(c->push(JSON_VALUE_SIZE),&e,JSON_VALUE_SIZE),++sz,json_parse_ws(c);
		if(*c->json==','){++c->json,json_parse_ws(c);continue;}
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
int json_parse_object(json_text*c,json_value*v){
	EXPECT(c,'{');json_parse_ws(c);
	if(*c->json=='}'){
		++c->json,v->type=JSON_OBJECT,v->u.o={NULL,0};
		return JSON_PARSE_OK;
	}
	int ret;json_member m;size_t sz=0;
	while(1){
		m.k=NULL;json_init(&m.v);
		if(*c->json!='"'){ret=JSON_PARSE_MISS_KEY;break;}
		if((ret=json_parse_string_raw(c,&m.k,m.len))!=JSON_PARSE_OK)break;
		json_parse_ws(c);
		if(*c->json!=':'){delete m.k;ret=JSON_PARSE_MISS_COLON;break;}
		++c->json,json_parse_ws(c);
		if((ret=json_parse_value(c,&m.v))!=JSON_PARSE_OK){delete m.k;break;}
		memcpy((char*)c->push(JSON_MEMBER_SIZE),&m,JSON_MEMBER_SIZE),++sz,json_parse_ws(c);
		if(*c->json==','){++c->json,json_parse_ws(c);continue;}
		else if(*c->json=='}'){
			++c->json,v->type=JSON_OBJECT,v->u.o.size=sz;
			memcpy(v->u.o.m=new json_member[sz],c->pop(sz*JSON_MEMBER_SIZE),sz*JSON_MEMBER_SIZE);
			return JSON_PARSE_OK;
		}
		ret=JSON_PARSE_MISS_COMMA_OR_CURLY_BRACKET;break;
	}
	while(sz--){
		json_member*_m=(json_member*)c->pop(JSON_MEMBER_SIZE);
		json_free(&_m->v);delete _m->k;
	}
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
		case'{':return json_parse_object(c,v);
		case'\0':return JSON_PARSE_EXPECT_VALUE;
	}
}
int json_parse(json_value*v,const char*json){
	assert(json!=NULL);json_init(v);
	json_text c;int ret;
	c.json=json,c.stack=NULL,c.size=c.top=0,v->type=JSON_NULL;
	json_parse_ws(&c);
	if((ret=json_parse_value(&c,v))==JSON_PARSE_OK){
		json_parse_ws(&c);
		if(*c.json!='\0')ret=JSON_PARSE_ROOT_NOT_SINGULAR,v->type=JSON_NULL;
	}
	assert(c.top==0),delete c.stack;
	return ret;
}

#define PUTS(c,s,len) memcpy((char*)c->push(len),s,len)
int json_stringify_value(json_text*c,const json_value*v);
void json_stringify_string(json_text*c,const char*s,size_t len){
	assert(s!=NULL);PUTC(c,'"');unsigned char ch;
	for(size_t i=0;i<len;++i)
		switch(ch=(unsigned char)s[i]){
			#define ESCAPE(expect,escape) case expect:PUTS(c,escape,2);break;
			ESCAPE('"',"\\\"")ESCAPE('\\',"\\\\")ESCAPE('\b',"\\b")ESCAPE('\f',"\\f")ESCAPE('\n',"\\n")ESCAPE('\r',"\\r")ESCAPE('\t',"\\t")
			#undef ESCAPE
			default:
				if(ch<0x20){
					char buffer[7];
					sprintf(buffer,"\\u%04X",ch);
					PUTS(c,buffer,6);
				}
				else PUTC(c,ch);
		}
	PUTC(c,'"');
}
int json_stringify_value(json_text*c,const json_value*v){
	switch(v->type){
		case JSON_NULL:PUTS(c,"null",4);break;
		case JSON_TRUE:PUTS(c,"true",4);break;
		case JSON_FALSE:PUTS(c,"false",5);break;
		case JSON_NUMBER:c->top-=32-sprintf((char*)c->push(32),"%.17g",v->u.n);break;
		case JSON_STRING:json_stringify_string(c,v->u.s.s,v->u.s.len);break;
		case JSON_ARRAY:{
			PUTC(c,'[');
			size_t i=0,sz=v->u.a.size;const json_value*t=v->u.a.e;
			for(;i<sz;++i,++t){
				json_stringify_value(c,t);
				if(i+1!=sz)PUTC(c,',');
			}
			PUTC(c,']');break;
		}
		case JSON_OBJECT:{
			PUTC(c,'{');
			size_t i=0,sz=v->u.o.size;const json_member*t=v->u.o.m;
			for(;i<sz;++i,++t){
				json_stringify_string(c,t->k,t->len);PUTC(c,':');json_stringify_value(c,&t->v);
				if(i+1!=sz)PUTC(c,',');
			}
			PUTC(c,'}');break;
		}
	}
	return JSON_STRINGIFY_OK;
}
int json_stringify(const json_value*v,char**json,size_t*len){
	assert(v!=NULL&&json!=NULL);
	json_text c;int ret;
	c.stack=new char[c.size=JSON_STACK_INIT_SIZE],c.top=0;
	if((ret=json_stringify_value(&c,v))!=JSON_STRINGIFY_OK)delete c.stack;
	if(len)*len=c.top;
	PUTC(&c,'\0');*json=c.stack;
	return JSON_STRINGIFY_OK;
}

void json_free(json_value*v){
	assert(v!=NULL);
	if(v->type==JSON_STRING)delete v->u.s.s;
	else if(v->type==JSON_ARRAY){
		size_t t=v->u.a.size;
		while(t--)json_free(v->u.a.e+t);
		delete v->u.a.e;
	}
	else if(v->type==JSON_OBJECT){
		size_t t=v->u.o.size;
		while(t--){
			json_member*m=v->u.o.m+t;
			delete m->k;json_free(&m->v);
		}
		delete v->u.o.m;
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

json_type json_get_type(const json_value*v){assert(v!=NULL);return v->type;}
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
size_t json_get_array_size(const json_value*v){
	assert(v!=NULL&&v->type==JSON_ARRAY);
	return v->u.a.size;
}
json_value*json_get_array_element(const json_value*v,size_t index){
	assert(v!=NULL&&v->type==JSON_ARRAY&&index<v->u.a.size);
	return v->u.a.e+index;
}
size_t json_get_object_size(const json_value*v){
	assert(v!=NULL&&v->type==JSON_OBJECT);
	return v->u.o.size;
}
const char*json_get_object_key(const json_value*v,size_t index){
	assert(v!=NULL&&v->type==JSON_OBJECT&&index<v->u.o.size);
	return v->u.o.m[index].k;
}
size_t json_get_object_key_length(const json_value*v,size_t index){
	assert(v!=NULL&&v->type==JSON_OBJECT&&index<v->u.o.size);
	return v->u.o.m[index].len;
};
json_value*json_get_object_value(const json_value*v,size_t index){
	assert(v!=NULL&&v->type==JSON_OBJECT&&index<v->u.o.size);
	return &v->u.o.m[index].v;
};