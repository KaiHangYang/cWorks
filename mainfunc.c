#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <gtk/gtk.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "main.h"
/**************必要的全局变量***************/

//首先是用来标记需要改变信息类型 其中 1表示更改服饰类型信息 2表示修改衣服基本信息 3表示修改衣服销售基本信息 0表示不能进行修改操作
int clothInfoChangeType = 0;
CLO_TYPE * mainchain = NULL;
GtkTreeView * treeview;
GtkWidget * window;

/*********首先是使用的cJSON的函数*********/
/*
  Copyright (c) 2009 Dave Gamble

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/* cJSON */
/* JSON parser in C. */

static const char *ep;

const char *cJSON_GetErrorPtr(void) {return ep;}

static int cJSON_strcasecmp(const char *s1,const char *s2)
{
	if (!s1) return (s1==s2)?0:1;if (!s2) return 1;
	for(; tolower(*s1) == tolower(*s2); ++s1, ++s2)	if(*s1 == 0)	return 0;
	return tolower(*(const unsigned char *)s1) - tolower(*(const unsigned char *)s2);
}

static void *(*cJSON_malloc)(size_t sz) = malloc;
static void (*cJSON_free)(void *ptr) = free;

static char* cJSON_strdup(const char* str)
{
      size_t len;
      char* copy;

      len = strlen(str) + 1;
      if (!(copy = (char*)cJSON_malloc(len))) return 0;
      memcpy(copy,str,len);
      return copy;
}

void cJSON_InitHooks(cJSON_Hooks* hooks)
{
    if (!hooks) { /* Reset hooks */
        cJSON_malloc = malloc;
        cJSON_free = free;
        return;
    }

	cJSON_malloc = (hooks->malloc_fn)?hooks->malloc_fn:malloc;
	cJSON_free	 = (hooks->free_fn)?hooks->free_fn:free;
}

/* Internal constructor. */
static cJSON *cJSON_New_Item(void)
{
	cJSON* node = (cJSON*)cJSON_malloc(sizeof(cJSON));
	if (node) memset(node,0,sizeof(cJSON));
	return node;
}

/* Delete a cJSON structure. */
void cJSON_Delete(cJSON *c)
{
	cJSON *next;
	while (c)
	{
		next=c->next;
		if (!(c->type&cJSON_IsReference) && c->child) cJSON_Delete(c->child);
		if (!(c->type&cJSON_IsReference) && c->valuestring) cJSON_free(c->valuestring);
		if (c->string) cJSON_free(c->string);
		cJSON_free(c);
		c=next;
	}
}

/* Parse the input text to generate a number, and populate the result into item. */
static const char *parse_number(cJSON *item,const char *num)
{
	double n=0,sign=1,scale=0;int subscale=0,signsubscale=1;

	if (*num=='-') sign=-1,num++;	/* Has sign? */
	if (*num=='0') num++;			/* is zero */
	if (*num>='1' && *num<='9')	do	n=(n*10.0)+(*num++ -'0');	while (*num>='0' && *num<='9');	/* Number? */
	if (*num=='.' && num[1]>='0' && num[1]<='9') {num++;		do	n=(n*10.0)+(*num++ -'0'),scale--; while (*num>='0' && *num<='9');}	/* Fractional part? */
	if (*num=='e' || *num=='E')		/* Exponent? */
	{	num++;if (*num=='+') num++;	else if (*num=='-') signsubscale=-1,num++;		/* With sign? */
		while (*num>='0' && *num<='9') subscale=(subscale*10)+(*num++ - '0');	/* Number? */
	}

	n=sign*n*pow(10.0,(scale+subscale*signsubscale));	/* number = +/- number.fraction * 10^+/- exponent */
	
	item->valuedouble=n;
	item->valueint=(int)n;
	item->type=cJSON_Number;
	return num;
}

/* Render the number nicely from the given item into a string. */
static char *print_number(cJSON *item)
{
	char *str;
	double d=item->valuedouble;
	if (fabs(((double)item->valueint)-d)<=DBL_EPSILON && d<=INT_MAX && d>=INT_MIN)
	{
		str=(char*)cJSON_malloc(21);	/* 2^64+1 can be represented in 21 chars. */
		if (str) sprintf(str,"%d",item->valueint);
	}
	else
	{
		str=(char*)cJSON_malloc(64);	/* This is a nice tradeoff. */
		if (str)
		{
			if (fabs(floor(d)-d)<=DBL_EPSILON && fabs(d)<1.0e60)sprintf(str,"%.0f",d);
			else if (fabs(d)<1.0e-6 || fabs(d)>1.0e9)			sprintf(str,"%e",d);
			else												sprintf(str,"%f",d);
		}
	}
	return str;
}

static unsigned parse_hex4(const char *str)
{
	unsigned h=0;
	if (*str>='0' && *str<='9') h+=(*str)-'0'; else if (*str>='A' && *str<='F') h+=10+(*str)-'A'; else if (*str>='a' && *str<='f') h+=10+(*str)-'a'; else return 0;
	h=h<<4;str++;
	if (*str>='0' && *str<='9') h+=(*str)-'0'; else if (*str>='A' && *str<='F') h+=10+(*str)-'A'; else if (*str>='a' && *str<='f') h+=10+(*str)-'a'; else return 0;
	h=h<<4;str++;
	if (*str>='0' && *str<='9') h+=(*str)-'0'; else if (*str>='A' && *str<='F') h+=10+(*str)-'A'; else if (*str>='a' && *str<='f') h+=10+(*str)-'a'; else return 0;
	h=h<<4;str++;
	if (*str>='0' && *str<='9') h+=(*str)-'0'; else if (*str>='A' && *str<='F') h+=10+(*str)-'A'; else if (*str>='a' && *str<='f') h+=10+(*str)-'a'; else return 0;
	return h;
}

/* Parse the input text into an unescaped cstring, and populate item. */
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
static const char *parse_string(cJSON *item,const char *str)
{
	const char *ptr=str+1;char *ptr2;char *out;int len=0;unsigned uc,uc2;
	if (*str!='\"') {ep=str;return 0;}	/* not a string! */
	
	while (*ptr!='\"' && *ptr && ++len) if (*ptr++ == '\\') ptr++;	/* Skip escaped quotes. */
	
	out=(char*)cJSON_malloc(len+1);	/* This is how long we need for the string, roughly. */
	if (!out) return 0;
	
	ptr=str+1;ptr2=out;
	while (*ptr!='\"' && *ptr)
	{
		if (*ptr!='\\') *ptr2++=*ptr++;
		else
		{
			ptr++;
			switch (*ptr)
			{
				case 'b': *ptr2++='\b';	break;
				case 'f': *ptr2++='\f';	break;
				case 'n': *ptr2++='\n';	break;
				case 'r': *ptr2++='\r';	break;
				case 't': *ptr2++='\t';	break;
				case 'u':	 /* transcode utf16 to utf8. */
					uc=parse_hex4(ptr+1);ptr+=4;	/* get the unicode char. */

					if ((uc>=0xDC00 && uc<=0xDFFF) || uc==0)	break;	/* check for invalid.	*/

					if (uc>=0xD800 && uc<=0xDBFF)	/* UTF16 surrogate pairs.	*/
					{
						if (ptr[1]!='\\' || ptr[2]!='u')	break;	/* missing second-half of surrogate.	*/
						uc2=parse_hex4(ptr+3);ptr+=6;
						if (uc2<0xDC00 || uc2>0xDFFF)		break;	/* invalid second-half of surrogate.	*/
						uc=0x10000 + (((uc&0x3FF)<<10) | (uc2&0x3FF));
					}

					len=4;if (uc<0x80) len=1;else if (uc<0x800) len=2;else if (uc<0x10000) len=3; ptr2+=len;
					
					switch (len) {
						case 4: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 3: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 2: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 1: *--ptr2 =(uc | firstByteMark[len]);
					}
					ptr2+=len;
					break;
				default:  *ptr2++=*ptr; break;
			}
			ptr++;
		}
	}
	*ptr2=0;
	if (*ptr=='\"') ptr++;
	item->valuestring=out;
	item->type=cJSON_String;
	return ptr;
}

/* Render the cstring provided to an escaped version that can be printed. */
static char *print_string_ptr(const char *str)
{
	const char *ptr;char *ptr2,*out;int len=0;unsigned char token;
	
	if (!str) return cJSON_strdup("");
	ptr=str;while ((token=*ptr) && ++len) {if (strchr("\"\\\b\f\n\r\t",token)) len++; else if (token<32) len+=5;ptr++;}
	
	out=(char*)cJSON_malloc(len+3);
	if (!out) return 0;

	ptr2=out;ptr=str;
	*ptr2++='\"';
	while (*ptr)
	{
		if ((unsigned char)*ptr>31 && *ptr!='\"' && *ptr!='\\') *ptr2++=*ptr++;
		else
		{
			*ptr2++='\\';
			switch (token=*ptr++)
			{
				case '\\':	*ptr2++='\\';	break;
				case '\"':	*ptr2++='\"';	break;
				case '\b':	*ptr2++='b';	break;
				case '\f':	*ptr2++='f';	break;
				case '\n':	*ptr2++='n';	break;
				case '\r':	*ptr2++='r';	break;
				case '\t':	*ptr2++='t';	break;
				default: sprintf(ptr2,"u%04x",token);ptr2+=5;	break;	/* escape and print */
			}
		}
	}
	*ptr2++='\"';*ptr2++=0;
	return out;
}
/* Invote print_string_ptr (which is useful) on an item. */
static char *print_string(cJSON *item)	{return print_string_ptr(item->valuestring);}

/* Predeclare these prototypes. */
static const char *parse_value(cJSON *item,const char *value);
static char *print_value(cJSON *item,int depth,int fmt);
static const char *parse_array(cJSON *item,const char *value);
static char *print_array(cJSON *item,int depth,int fmt);
static const char *parse_object(cJSON *item,const char *value);
static char *print_object(cJSON *item,int depth,int fmt);

/* Utility to jump whitespace and cr/lf */
static const char *skip(const char *in) {while (in && *in && (unsigned char)*in<=32) in++; return in;}

/* Parse an object - create a new root, and populate. */
cJSON *cJSON_ParseWithOpts(const char *value,const char **return_parse_end,int require_null_terminated)
{
	const char *end=0;
	cJSON *c=cJSON_New_Item();
	ep=0;
	if (!c) return 0;       /* memory fail */

	end=parse_value(c,skip(value));
	if (!end)	{cJSON_Delete(c);return 0;}	/* parse failure. ep is set. */

	/* if we require null-terminated JSON without appended garbage, skip and then check for a null terminator */
	if (require_null_terminated) {end=skip(end);if (*end) {cJSON_Delete(c);ep=end;return 0;}}
	if (return_parse_end) *return_parse_end=end;
	return c;
}
/* Default options for cJSON_Parse */
cJSON *cJSON_Parse(const char *value) {return cJSON_ParseWithOpts(value,0,0);}

/* Render a cJSON item/entity/structure to text. */
char *cJSON_Print(cJSON *item)				{return print_value(item,0,1);}
char *cJSON_PrintUnformatted(cJSON *item)	{return print_value(item,0,0);}

/* Parser core - when encountering text, process appropriately. */
static const char *parse_value(cJSON *item,const char *value)
{
	if (!value)						return 0;	/* Fail on null. */
	if (!strncmp(value,"null",4))	{ item->type=cJSON_NULL;  return value+4; }
	if (!strncmp(value,"false",5))	{ item->type=cJSON_False; return value+5; }
	if (!strncmp(value,"true",4))	{ item->type=cJSON_True; item->valueint=1;	return value+4; }
	if (*value=='\"')				{ return parse_string(item,value); }
	if (*value=='-' || (*value>='0' && *value<='9'))	{ return parse_number(item,value); }
	if (*value=='[')				{ return parse_array(item,value); }
	if (*value=='{')				{ return parse_object(item,value); }

	ep=value;return 0;	/* failure. */
}

/* Render a value to text. */
static char *print_value(cJSON *item,int depth,int fmt)
{
	char *out=0;
	if (!item) return 0;
	switch ((item->type)&255)
	{
		case cJSON_NULL:	out=cJSON_strdup("null");	break;
		case cJSON_False:	out=cJSON_strdup("false");break;
		case cJSON_True:	out=cJSON_strdup("true"); break;
		case cJSON_Number:	out=print_number(item);break;
		case cJSON_String:	out=print_string(item);break;
		case cJSON_Array:	out=print_array(item,depth,fmt);break;
		case cJSON_Object:	out=print_object(item,depth,fmt);break;
	}
	return out;
}

/* Build an array from input text. */
static const char *parse_array(cJSON *item,const char *value)
{
	cJSON *child;
	if (*value!='[')	{ep=value;return 0;}	/* not an array! */

	item->type=cJSON_Array;
	value=skip(value+1);
	if (*value==']') return value+1;	/* empty array. */

	item->child=child=cJSON_New_Item();
	if (!item->child) return 0;		 /* memory fail */
	value=skip(parse_value(child,skip(value)));	/* skip any spacing, get the value. */
	if (!value) return 0;

	while (*value==',')
	{
		cJSON *new_item;
		if (!(new_item=cJSON_New_Item())) return 0; 	/* memory fail */
		child->next=new_item;new_item->prev=child;child=new_item;
		value=skip(parse_value(child,skip(value+1)));
		if (!value) return 0;	/* memory fail */
	}

	if (*value==']') return value+1;	/* end of array */
	ep=value;return 0;	/* malformed. */
}

/* Render an array to text */
static char *print_array(cJSON *item,int depth,int fmt)
{
	char **entries;
	char *out=0,*ptr,*ret;int len=5;
	cJSON *child=item->child;
	int numentries=0,i=0,fail=0;
	
	/* How many entries in the array? */
	while (child) numentries++,child=child->next;
	/* Explicitly handle numentries==0 */
	if (!numentries)
	{
		out=(char*)cJSON_malloc(3);
		if (out) strcpy(out,"[]");
		return out;
	}
	/* Allocate an array to hold the values for each */
	entries=(char**)cJSON_malloc(numentries*sizeof(char*));
	if (!entries) return 0;
	memset(entries,0,numentries*sizeof(char*));
	/* Retrieve all the results: */
	child=item->child;
	while (child && !fail)
	{
		ret=print_value(child,depth+1,fmt);
		entries[i++]=ret;
		if (ret) len+=strlen(ret)+2+(fmt?1:0); else fail=1;
		child=child->next;
	}
	
	/* If we didn't fail, try to malloc the output string */
	if (!fail) out=(char*)cJSON_malloc(len);
	/* If that fails, we fail. */
	if (!out) fail=1;

	/* Handle failure. */
	if (fail)
	{
		for (i=0;i<numentries;i++) if (entries[i]) cJSON_free(entries[i]);
		cJSON_free(entries);
		return 0;
	}
	
	/* Compose the output array. */
	*out='[';
	ptr=out+1;*ptr=0;
	for (i=0;i<numentries;i++)
	{
		strcpy(ptr,entries[i]);ptr+=strlen(entries[i]);
		if (i!=numentries-1) {*ptr++=',';if(fmt)*ptr++=' ';*ptr=0;}
		cJSON_free(entries[i]);
	}
	cJSON_free(entries);
	*ptr++=']';*ptr++=0;
	return out;	
}

/* Build an object from the text. */
static const char *parse_object(cJSON *item,const char *value)
{
	cJSON *child;
	if (*value!='{')	{ep=value;return 0;}	/* not an object! */
	
	item->type=cJSON_Object;
	value=skip(value+1);
	if (*value=='}') return value+1;	/* empty array. */
	
	item->child=child=cJSON_New_Item();
	if (!item->child) return 0;
	value=skip(parse_string(child,skip(value)));
	if (!value) return 0;
	child->string=child->valuestring;child->valuestring=0;
	if (*value!=':') {ep=value;return 0;}	/* fail! */
	value=skip(parse_value(child,skip(value+1)));	/* skip any spacing, get the value. */
	if (!value) return 0;
	
	while (*value==',')
	{
		cJSON *new_item;
		if (!(new_item=cJSON_New_Item()))	return 0; /* memory fail */
		child->next=new_item;new_item->prev=child;child=new_item;
		value=skip(parse_string(child,skip(value+1)));
		if (!value) return 0;
		child->string=child->valuestring;child->valuestring=0;
		if (*value!=':') {ep=value;return 0;}	/* fail! */
		value=skip(parse_value(child,skip(value+1)));	/* skip any spacing, get the value. */
		if (!value) return 0;
	}
	
	if (*value=='}') return value+1;	/* end of array */
	ep=value;return 0;	/* malformed. */
}

/* Render an object to text. */
static char *print_object(cJSON *item,int depth,int fmt)
{
	char **entries=0,**names=0;
	char *out=0,*ptr,*ret,*str;int len=7,i=0,j;
	cJSON *child=item->child;
	int numentries=0,fail=0;
	/* Count the number of entries. */
	while (child) numentries++,child=child->next;
	/* Explicitly handle empty object case */
	if (!numentries)
	{
		out=(char*)cJSON_malloc(fmt?depth+4:3);
		if (!out)	return 0;
		ptr=out;*ptr++='{';
		if (fmt) {*ptr++='\n';for (i=0;i<depth-1;i++) *ptr++='\t';}
		*ptr++='}';*ptr++=0;
		return out;
	}
	/* Allocate space for the names and the objects */
	entries=(char**)cJSON_malloc(numentries*sizeof(char*));
	if (!entries) return 0;
	names=(char**)cJSON_malloc(numentries*sizeof(char*));
	if (!names) {cJSON_free(entries);return 0;}
	memset(entries,0,sizeof(char*)*numentries);
	memset(names,0,sizeof(char*)*numentries);

	/* Collect all the results into our arrays: */
	child=item->child;depth++;if (fmt) len+=depth;
	while (child)
	{
		names[i]=str=print_string_ptr(child->string);
		entries[i++]=ret=print_value(child,depth,fmt);
		if (str && ret) len+=strlen(ret)+strlen(str)+2+(fmt?2+depth:0); else fail=1;
		child=child->next;
	}
	
	/* Try to allocate the output string */
	if (!fail) out=(char*)cJSON_malloc(len);
	if (!out) fail=1;

	/* Handle failure */
	if (fail)
	{
		for (i=0;i<numentries;i++) {if (names[i]) cJSON_free(names[i]);if (entries[i]) cJSON_free(entries[i]);}
		cJSON_free(names);cJSON_free(entries);
		return 0;
	}
	
	/* Compose the output: */
	*out='{';ptr=out+1;if (fmt)*ptr++='\n';*ptr=0;
	for (i=0;i<numentries;i++)
	{
		if (fmt) for (j=0;j<depth;j++) *ptr++='\t';
		strcpy(ptr,names[i]);ptr+=strlen(names[i]);
		*ptr++=':';if (fmt) *ptr++='\t';
		strcpy(ptr,entries[i]);ptr+=strlen(entries[i]);
		if (i!=numentries-1) *ptr++=',';
		if (fmt) *ptr++='\n';*ptr=0;
		cJSON_free(names[i]);cJSON_free(entries[i]);
	}
	
	cJSON_free(names);cJSON_free(entries);
	if (fmt) for (i=0;i<depth-1;i++) *ptr++='\t';
	*ptr++='}';*ptr++=0;
	return out;	
}

/* Get Array size/item / object item. */
int    cJSON_GetArraySize(cJSON *array)							{cJSON *c=array->child;int i=0;while(c)i++,c=c->next;return i;}
cJSON *cJSON_GetArrayItem(cJSON *array,int item)				{cJSON *c=array->child;  while (c && item>0) item--,c=c->next; return c;}
cJSON *cJSON_GetObjectItem(cJSON *object,char *string)	{cJSON *c=object->child; while (c && cJSON_strcasecmp(c->string,string)) c=c->next; return c;}

/* Utility for array list handling. */
static void suffix_object(cJSON *prev,cJSON *item) {prev->next=item;item->prev=prev;}
/* Utility for handling references. */
static cJSON *create_reference(cJSON *item) {cJSON *ref=cJSON_New_Item();if (!ref) return 0;memcpy(ref,item,sizeof(cJSON));ref->string=0;ref->type|=cJSON_IsReference;ref->next=ref->prev=0;return ref;}

/* Add item to array/object. */
void   cJSON_AddItemToArray(cJSON *array, cJSON *item)						{cJSON *c=array->child;if (!item) return; if (!c) {array->child=item;} else {while (c && c->next) c=c->next; suffix_object(c,item);}}
void   cJSON_AddItemToObject(cJSON *object,const char *string,cJSON *item)	{if (!item) return; if (item->string) cJSON_free(item->string);item->string=cJSON_strdup(string);cJSON_AddItemToArray(object,item);}
void	cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item)						{cJSON_AddItemToArray(array,create_reference(item));}
void	cJSON_AddItemReferenceToObject(cJSON *object,const char *string,cJSON *item)	{cJSON_AddItemToObject(object,string,create_reference(item));}

cJSON *cJSON_DetachItemFromArray(cJSON *array,int which)			{cJSON *c=array->child;while (c && which>0) c=c->next,which--;if (!c) return 0;
	if (c->prev) c->prev->next=c->next;if (c->next) c->next->prev=c->prev;if (c==array->child) array->child=c->next;c->prev=c->next=0;return c;}
void   cJSON_DeleteItemFromArray(cJSON *array,int which)			{cJSON_Delete(cJSON_DetachItemFromArray(array,which));}
cJSON *cJSON_DetachItemFromObject(cJSON *object,const char *string) {int i=0;cJSON *c=object->child;while (c && cJSON_strcasecmp(c->string,string)) i++,c=c->next;if (c) return cJSON_DetachItemFromArray(object,i);return 0;}
void   cJSON_DeleteItemFromObject(cJSON *object,const char *string) {cJSON_Delete(cJSON_DetachItemFromObject(object,string));}

/* Replace array/object items with new ones. */
void   cJSON_ReplaceItemInArray(cJSON *array,int which,cJSON *newitem)		{cJSON *c=array->child;while (c && which>0) c=c->next,which--;if (!c) return;
	newitem->next=c->next;newitem->prev=c->prev;if (newitem->next) newitem->next->prev=newitem;
	if (c==array->child) array->child=newitem; else newitem->prev->next=newitem;c->next=c->prev=0;cJSON_Delete(c);}
void   cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem){int i=0;cJSON *c=object->child;while(c && cJSON_strcasecmp(c->string,string))i++,c=c->next;if(c){newitem->string=cJSON_strdup(string);cJSON_ReplaceItemInArray(object,i,newitem);}}

/* Create basic types: */
cJSON *cJSON_CreateNull(void)					{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_NULL;return item;}
cJSON *cJSON_CreateTrue(void)					{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_True;return item;}
cJSON *cJSON_CreateFalse(void)					{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_False;return item;}
cJSON *cJSON_CreateBool(int b)					{cJSON *item=cJSON_New_Item();if(item)item->type=b?cJSON_True:cJSON_False;return item;}
cJSON *cJSON_CreateNumber(double num)			{cJSON *item=cJSON_New_Item();if(item){item->type=cJSON_Number;item->valuedouble=num;item->valueint=(int)num;}return item;}
cJSON *cJSON_CreateString(char *string)	{cJSON *item=cJSON_New_Item();if(item){item->type=cJSON_String;item->valuestring=cJSON_strdup(string);}return item;}
cJSON *cJSON_CreateArray(void)					{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_Array;return item;}
cJSON *cJSON_CreateObject(void)					{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_Object;return item;}

/* Create Arrays: */
cJSON *cJSON_CreateIntArray(const int *numbers,int count)		{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
cJSON *cJSON_CreateFloatArray(const float *numbers,int count)	{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
cJSON *cJSON_CreateDoubleArray(const double *numbers,int count)	{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
cJSON *cJSON_CreateStringArray(char **strings,int count)	{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateString(strings[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}

/* Duplication */
cJSON *cJSON_Duplicate(cJSON *item,int recurse)
{
	cJSON *newitem,*cptr,*nptr=0,*newchild;
	/* Bail on bad ptr */
	if (!item) return 0;
	/* Create new item */
	newitem=cJSON_New_Item();
	if (!newitem) return 0;
	/* Copy over all vars */
	newitem->type=item->type&(~cJSON_IsReference),newitem->valueint=item->valueint,newitem->valuedouble=item->valuedouble;
	if (item->valuestring)	{newitem->valuestring=cJSON_strdup(item->valuestring);	if (!newitem->valuestring)	{cJSON_Delete(newitem);return 0;}}
	if (item->string)		{newitem->string=cJSON_strdup(item->string);			if (!newitem->string)		{cJSON_Delete(newitem);return 0;}}
	/* If non-recursive, then we're done! */
	if (!recurse) return newitem;
	/* Walk the ->next chain for the child. */
	cptr=item->child;
	while (cptr)
	{
		newchild=cJSON_Duplicate(cptr,1);		/* Duplicate (with recurse) each item in the ->next chain */
		if (!newchild) {cJSON_Delete(newitem);return 0;}
		if (nptr)	{nptr->next=newchild,newchild->prev=nptr;nptr=newchild;}	/* If newitem->child already set, then crosswire ->prev and ->next and move on */
		else		{newitem->child=newchild;nptr=newchild;}					/* Set newitem->child and move to it */
		cptr=cptr->next;
	}
	return newitem;
}

void cJSON_Minify(char *json)
{
	char *into=json;
	while (*json)
	{
		if (*json==' ') json++;
		else if (*json=='\t') json++;	// Whitespace characters.
		else if (*json=='\r') json++;
		else if (*json=='\n') json++;
		else if (*json=='/' && json[1]=='/')  while (*json && *json!='\n') json++;	// double-slash comments, to end of line.
		else if (*json=='/' && json[1]=='*') {while (*json && !(*json=='*' && json[1]=='/')) json++;json+=2;}	// multiline comments.
		else if (*json=='\"'){*into++=*json++;while (*json && *json!='\"'){if (*json=='\\') *into++=*json++;*into++=*json++;}*into++=*json++;} // string literals, which are \" sensitive.
		else *into++=*json++;			// All other characters.
	}
	*into=0;	// and null-terminate.
}

/***************系统的主要功能 下来是我的函数***************/

//辅助函数1 json 解析
cJSON * json_dofile(char * filename) {
	FILE * file = fopen(filename, "rb");
	fseek(file, 0, SEEK_END);
	long len = ftell(file);
	fseek(file, 0, SEEK_SET);
	char * data = (char *)malloc(sizeof(char)*(len+1));
	fread(data, 1, len, file);
	fclose(file);

	cJSON * json;

	json = cJSON_Parse(data);

	free(data);

	if (!json) {
		printf("Error before: [%s]\n",cJSON_GetErrorPtr());
	}
	else {
		return json;
	}

}
//辅助函数2 ITOA把数字转化为字符串
char * ITOA(int a) {
	int i=0, j, temp = a;
	char * str;

	while (temp) {
		temp /= 10;
		i++;
	}

	if (a == 0){
		str = (char *)malloc(sizeof(char)*2);
		strcpy(str, "0");
	}
	else {
		str = (char *)malloc(sizeof(char)*(i+1));
		temp =i;
	
		for (j=0; j < temp; j++, i--) {
			str[j] = a/(pow(10, i-1)) + '0';
			a %= (int)pow(10, i-1);
		}
	
		str[j] = '\0';
	}

	return str;
}
//把字符转化为字符串
char * CTOS(char ch) {
	char * a;
	a = (char *)malloc(sizeof(char)*2);
	a[0] = ch;
	a[1] = '\0';
	return a;
}
//把数字浮点数字符串转化为浮点数
float STOF(char * str) {
	float tmp = 0;
	int len = strlen(str);
	int i, j, k;

	if(len == 0 || str[0] == '\0') {
		return 0;
	}

	for (i=0; i < len-1; i++) {
		if (str[i] == '.') {
			break;
		}

		if (!(str[i]>= '0' && str[i] <= '9')) {
			return 0;
		}
	}
	if (i == len-1) {
		i += 1;
	}
	for (j=0, k=0; k < len; j++, k++) {
		if (str[k] == '.') {
			j--;
			continue;
		}
		else {
			tmp += (str[k] - '0')*pow(10, i - 1 - j);
		}
	}
	return tmp;
}
//辅助函数3 输出全部的内容 (在还没做图形界面的时候测试用的)
void outPutSome(char type) {

	CLO_TYPE * tmp = mainchain;
	CLO_BASE * tmp_base;
	CLO_SELL * tmp_sell;

	while (tmp != NULL) {
		if (tmp->type_num == type) {
			break;
		}
		else {
			tmp = tmp->type_next;
		}
	}
	printf("%c\t%s\n\n", tmp->type_num, tmp->type_name);
	tmp_base = tmp->base_info;

	while (tmp_base != NULL) {
		printf("\t%c\n\t%s\n\t%c\n\t%f\n\t%d\n\t%f\n", tmp_base->cloth_type, tmp_base->cloth_name, tmp_base->cloth_sex, tmp_base->cloth_price, tmp_base->cloth_sold_num, tmp_base->cloth_comment);

		tmp_sell = tmp_base->sell_info;

		while(tmp_sell != NULL) {
			printf("\t\t%s\n\t\t%s\n\t\t%s\n\t\t%d\n", tmp_sell->cloth_name, tmp_sell->sold_time, tmp_sell->consumer_name, tmp_sell->consumer_comment);

			tmp_sell = tmp_sell->sell_next;
		}

		tmp_base = tmp_base->base_next;
	}
}


//"链表的初始化 是需要从文件中读取数据的 因为写过网页所以我打算使用json格式的数据格式"
void chain_init(char * file_path) {

	cJSON * root = json_dofile(file_path);
	cJSON * type, * base, * baseroot, * sell, * sellroot;

	int typeNum = cJSON_GetArraySize(root);
	int baseNum, sellNum;
	int i, j, k;

	CLO_TYPE * chainhead;
	CLO_TYPE * tmp_type;
	CLO_BASE * tmp_base, * tmp_base_head;
	CLO_SELL * tmp_sell, * tmp_sell_head;

	tmp_type = chainhead = (CLO_TYPE *)malloc(sizeof(CLO_TYPE));
	tmp_type->type_next = NULL;

	if (cJSON_GetArraySize(root) == 0) {
		mainchain = NULL;
		return;
	}

	// 对root里面的元素进行遍历
	for (i=0; i < typeNum; i++) {
		type = cJSON_GetObjectItem(root, ITOA(i));
		baseroot = cJSON_GetObjectItem(type, "baseInfo");
		baseNum = cJSON_GetArraySize(baseroot);

		tmp_type->type_num = cJSON_GetObjectItem(type, "typeNum")->valuestring[0];
		strcpy(tmp_type->type_name, cJSON_GetObjectItem(type, "typeName")->valuestring);

		tmp_base_head = tmp_base = (CLO_BASE *)malloc(sizeof(CLO_BASE));
		tmp_base->base_next	= NULL;

		for (j=0; j < baseNum; j++) {
			base = cJSON_GetObjectItem(baseroot, ITOA(j));
			sellroot = cJSON_GetObjectItem(base, "sellInfo");
			sellNum = cJSON_GetArraySize(sellroot);

			tmp_base->cloth_type = cJSON_GetObjectItem(base, "clothType")->valuestring[0];
			strcpy(tmp_base->cloth_name, cJSON_GetObjectItem(base, "clothName")->valuestring);
			tmp_base->cloth_sex = cJSON_GetObjectItem(base, "clothSex")->valuestring[0];
			tmp_base->cloth_price = (float)cJSON_GetObjectItem(base, "clothPrice")->valuedouble;
			tmp_base->cloth_sold_num = cJSON_GetObjectItem(base, "clothSoldNum")->valueint;
			tmp_base->cloth_comment = (float)cJSON_GetObjectItem(base, "clothComment")->valuedouble;

			tmp_sell_head = tmp_sell = (CLO_SELL *)malloc(sizeof(CLO_SELL));
			tmp_sell->sell_next = NULL;

			for (k=0; k < sellNum; k++) {
				sell = cJSON_GetObjectItem(sellroot, ITOA(k));

				strcpy(tmp_sell->cloth_name, cJSON_GetObjectItem(sell, "clothName")->valuestring);
				strcpy(tmp_sell->sold_time, cJSON_GetObjectItem(sell, "soldTime")->valuestring);
				strcpy(tmp_sell->consumer_name, cJSON_GetObjectItem(sell, "consumerName")->valuestring);
				tmp_sell->consumer_comment = cJSON_GetObjectItem(sell, "consumerComment")->valueint;

				if (k != sellNum - 1) {
					tmp_sell = tmp_sell->sell_next = (CLO_SELL *)malloc(sizeof(CLO_SELL));
					tmp_sell->sell_next = NULL;
				}
			}

			tmp_base->sell_info = tmp_sell_head;

			if (j != baseNum - 1) {
				tmp_base = tmp_base->base_next = (CLO_BASE *)malloc(sizeof(CLO_BASE));
				tmp_base->base_next = NULL;
			}

		}

		tmp_type->base_info = tmp_base_head;

		if (i != typeNum - 1) {
			tmp_type = tmp_type->type_next = (CLO_TYPE *)malloc(sizeof(CLO_TYPE));
			tmp_type->type_next = NULL;
		}
	}
	
	/*这样链表就建好了*/
	mainchain = chainhead;
}
//这里是将链表输出为一个文件
void chain_save(char * filename) {

	CLO_TYPE * tmptype = mainchain;
	CLO_BASE * tmpbase;
	CLO_SELL * tmpsell;
	FILE * file;

	int i, j, k;

	cJSON * root, * type, * base, *baseroot, * sell, * sellroot;

	root = cJSON_CreateObject();

	for (i=0; tmptype != NULL; i++) {
		cJSON_AddItemToObject(root, ITOA(i), type=cJSON_CreateObject());
		cJSON_AddStringToObject(type, "typeNum", CTOS(tmptype->type_num));
		cJSON_AddStringToObject(type, "typeName", tmptype->type_name);
		cJSON_AddItemToObject(type, "baseInfo", baseroot=cJSON_CreateObject());

		tmpbase = tmptype->base_info;

		for (j=0; tmpbase != NULL; j++) {
			cJSON_AddItemToObject(baseroot, ITOA(j), base=cJSON_CreateObject());
			cJSON_AddStringToObject(base, "clothType", CTOS(tmpbase->cloth_type));
			cJSON_AddStringToObject(base, "clothName", tmpbase->cloth_name);
			cJSON_AddStringToObject(base , "clothSex", CTOS(tmpbase->cloth_sex));
			cJSON_AddNumberToObject(base, "clothPrice", tmpbase->cloth_price);
			cJSON_AddNumberToObject(base, "clothSoldNum", tmpbase->cloth_sold_num);
			cJSON_AddNumberToObject(base, "clothComment", tmpbase->cloth_comment);
			cJSON_AddItemToObject(base, "sellInfo", sellroot=cJSON_CreateObject());

			tmpsell = tmpbase->sell_info;

			for (k=0; tmpsell != NULL; k++) {
				cJSON_AddItemToObject(sellroot, ITOA(k), sell=cJSON_CreateObject());
				cJSON_AddStringToObject(sell, "clothName", tmpsell->cloth_name);
				cJSON_AddStringToObject(sell, "soldTime", tmpsell->sold_time);
				cJSON_AddStringToObject(sell, "consumerName", tmpsell->consumer_name);
				cJSON_AddNumberToObject(sell, "consumerComment", tmpsell->consumer_comment);

				tmpsell = tmpsell->sell_next;
			}

			tmpbase = tmpbase->base_next;
		}

		tmptype = tmptype->type_next;
	}

	char * out = cJSON_Print(root);

	file = fopen(filename, "wb");

	int a = fputs(out, file);

	fclose(file);
	cJSON_Delete(root);
}
/*****数据的基本维护功能*****/

/*首先是clothtype的数据操作*/

//由于更改type信息的时候type下面的衣服基本信息也得改所以就添加了这个
void cloth_type_info_check(CLO_TYPE * tmp) {
	CLO_BASE * tmpbase = tmp->base_info;

	while (tmpbase != NULL) {

		tmpbase->cloth_type = tmp->type_num;

		tmpbase = tmpbase->base_next;
	}
}

//type的重复性检测 n表示num m表示name
int cloth_type_check(char pattern, char * content) {
	CLO_TYPE * tmp = mainchain;

	if (pattern == 'n') {
		while (tmp != NULL) {
			if (tmp->type_num == content[0]) {
				return 0;
			}
			tmp = tmp->type_next;
		}
		return 1;
	}
	else if (pattern == 'm') {
		while (tmp != NULL) {
			if (strcmp(tmp->type_name, content) == 0) {
				return 0;
			}
			tmp = tmp->type_next;
		}
		return 1;
	}
}
//查找目标节点pattern是n或m,n的话是以编号查找,m的话是以名字查找
CLO_TYPE * cloth_type_search(char pattern, char * content) {
	CLO_TYPE * tmpChain = mainchain;
	CLO_TYPE * prevChain = mainchain;
	if (pattern == 'n') {
		if (tmpChain->type_num == content[0]) {
			return NULL;
		}
		while (tmpChain != NULL) {
			if (tmpChain->type_num == content[0]) {
				return prevChain;
			}
			else {
				prevChain = tmpChain;
				tmpChain = tmpChain->type_next;
			}
		}
	}
	else if (pattern == 'm') {
		if (strcmp(mainchain->type_name, content) == 0) {
			return NULL;
		}
		while (tmpChain != NULL) {
			if (strcmp(tmpChain->type_name, content) == 0) {
				return prevChain;
			}
			else {
				prevChain = tmpChain;
				tmpChain = tmpChain->type_next;
			}
		}
	}
}

//服装类型  删除功能
void cloth_type_info_delete(char * str) {

	CLO_TYPE * chain = mainchain;
	CLO_TYPE * tmpChain;

	if (strlen(str) == 1) {
		tmpChain = cloth_type_search('n', str);

		if (tmpChain == NULL) {
			chain = chain->type_next;
		}
		else {
			tmpChain->type_next = tmpChain->type_next->type_next;
		}

	}
	else if (strlen(str) > 1) {
		tmpChain = cloth_type_search('m', str);

		if (tmpChain == NULL) {
			chain = chain->type_next;
		}
		else {
			tmpChain->type_next	= tmpChain->type_next->type_next;
		}
	}

	mainchain = chain;
}
//服装分类数据更改 pattern 表示更改的数据类型 n表示标号 m表示名字 content, 这里的node是由上面的search函数获得的
void cloth_type_info_change(CLO_TYPE * node, char pattern, char * content) {

	CLO_TYPE * changeNode;
	if (node == NULL) {
		changeNode = mainchain;
	}
	else {
		changeNode = node->type_next;
	}

	if (pattern == 'n') {

		if (cloth_type_check(pattern, content)) {
			changeNode->type_num = content[0];

			cloth_type_info_check(changeNode);
		}
	}
	else if (pattern == 'm') {
		if (cloth_type_check(pattern, content)) {
			strcpy(changeNode->type_name, content);
		}
	}
}
//服装分类信息的录入
int cloth_type_info_input(char * typeNum, char * typeName) {

	CLO_TYPE * chainEnd;
	CLO_TYPE * tmp = mainchain;


	if (mainchain == NULL) {
		mainchain = (CLO_TYPE *)malloc(sizeof(CLO_TYPE));
		mainchain->type_next = NULL;
		mainchain->base_info = NULL;
		mainchain->type_num = typeNum[0];
		strcpy(mainchain->type_name, typeName);

	}
	else {
		if(cloth_type_check('n' , typeNum)) {

			if (cloth_type_check('m', typeName)) {

				while (tmp->type_next != NULL) {
					tmp = tmp->type_next;
				}

				chainEnd = tmp;
				chainEnd->type_next = (CLO_TYPE *)malloc(sizeof(CLO_TYPE));
				chainEnd = chainEnd->type_next;

				chainEnd->type_num = typeNum[0];
				strcpy(chainEnd->type_name, typeName);

				chainEnd->type_next = NULL;
			}
			else {
				return 0;
			}

		}
		else {
			return 0;
		}
	}

	return 1;
}

/*clothbase 数据处理*/

//由于更改衣服信息的时候会出现更改衣服信息的情况所以下面的销售信息也是要更改的
void cloth_base_info_check(CLO_BASE * tmp) {
	CLO_SELL * tmpsell = tmp->sell_info;

	while (tmpsell != NULL) {

		strcpy(tmpsell->cloth_name, tmp->cloth_name);
		tmpsell = tmpsell->sell_next;
	}
}

//搜索目标和前面一样  content为搜索衣服的名字
CLO_BASE * cloth_base_search(char type, char * clothname) {
	CLO_TYPE * tmp = mainchain;
	CLO_BASE * tmpbase;
	CLO_BASE * prevbase;
	int i;

	while (tmp != NULL) {
		if (tmp->type_num == type) {
			tmpbase = tmp->base_info;

			if (strcmp(tmpbase->cloth_name, clothname) == 0) {
				return NULL;
			}

			while (tmpbase != NULL) {

				if (strcmp(clothname, tmpbase->cloth_name) == 0) {
					break;
				}
				else {
					prevbase = tmpbase;
					tmpbase = tmpbase->base_next;
				}
			}
			break;
		}
		else {
			tmp = tmp->type_next;
		}
	}

	return prevbase;
}
//数据删除
void cloth_base_info_delete(char type, char * clothname) {
	CLO_TYPE * tmpChain = mainchain;
	CLO_BASE * tmpNode = cloth_base_search(type, clothname);

	if (tmpNode == NULL) {
		while(tmpChain != NULL) {
			if (tmpChain->type_num == type) {
				break;
			}
			else {
				tmpChain = tmpChain->type_next;
			}
		}

		tmpNode = tmpChain->base_info;

		tmpChain->base_info = tmpNode->base_next;
	}
	else {
		tmpNode->base_next = tmpNode->base_next->base_next;
	}
}
//数据更改
void cloth_base_info_change(CLO_BASE * node, char prevtype, char nowtype, char * clothname, char clothsex, float clothprice) {
	CLO_BASE * changeNode, * tmpbase=NULL;
	CLO_TYPE * tmp = mainchain;

	if(node == NULL) {
		while(tmp != NULL) {
			if (tmp->type_num == prevtype) {
				break;
			}
			else {
				tmp = tmp->type_next;
			}
		}
		changeNode = tmp->base_info;
	}
	else {
		changeNode = node->base_next;
	}

	if (prevtype == nowtype) {
		strcpy(changeNode->cloth_name, clothname);
		changeNode->cloth_sex = clothsex;
		changeNode->cloth_price = clothprice;
		//这里的东西显然不够
	}
	else {
		tmp = mainchain;

		while(tmp != NULL) {
			if(tmp->type_num == nowtype) {
				tmpbase = tmp->base_info;
				break;
			}
			else {
				tmp = tmp->type_next;
			}
		}

		if (tmpbase != NULL) {
			while(tmpbase->base_next != NULL) {
				tmpbase = tmpbase->base_next;
			}
			tmpbase->base_next = changeNode;

			cloth_base_info_delete(prevtype, changeNode->cloth_name);

			tmpbase->base_next->base_next = NULL;
			changeNode->cloth_type = nowtype;
			strcpy(changeNode->cloth_name, clothname);
			changeNode->cloth_sex = clothsex;
			changeNode->cloth_price = clothprice;
		}
		else {
			strcpy(changeNode->cloth_name, clothname);
			changeNode->cloth_sex = clothsex;
			changeNode->cloth_price = clothprice;
		}
	}

	cloth_base_info_check(changeNode);
}
//数据录入
void cloth_base_info_input(char type, char * clothname, char clothsex, float clothprice) {
	CLO_TYPE * tmp = mainchain;
	CLO_BASE * tmpbase = NULL;

	while (tmp != NULL) {
		if (tmp->type_num == type) {
			tmpbase = tmp->base_info;
			break;
		}
		else {
			tmp = tmp->type_next;
		}
	}

	if (tmp == NULL) {
		return;
	}

	if (tmpbase == NULL) {

		tmp->base_info = (CLO_BASE *)malloc(sizeof(CLO_BASE));
		tmp->base_info->base_next = NULL;
		tmpbase = tmp->base_info;
		tmpbase->cloth_type = type;
		strcpy(tmpbase->cloth_name, clothname);
		tmpbase->cloth_sex = clothsex;
		tmpbase->cloth_price = clothprice;
		tmpbase->sell_info = NULL;
		tmpbase->cloth_sold_num = 0;
		tmpbase->cloth_comment = 0;

	}
	else {
		while (tmpbase->base_next != NULL) {
			tmpbase = tmpbase->base_next;
		}

		tmpbase->base_next = (CLO_BASE *)malloc(sizeof(CLO_BASE));
		tmpbase = tmpbase->base_next;

		tmpbase->base_next = NULL;
		tmpbase->cloth_type = type;
		strcpy(tmpbase->cloth_name, clothname);
		tmpbase->cloth_sex = clothsex;
		tmpbase->cloth_price = clothprice;
		tmpbase->sell_info = NULL;
		tmpbase->cloth_sold_num = 0;
		tmpbase->cloth_comment = 0;
	}
}

/****sellinfo 数据的处理****/

void cloth_sell_info_check(CLO_BASE * tmpbase) {
	CLO_SELL * tmpsell = tmpbase->sell_info;

	tmpbase->cloth_sold_num = 0;
	tmpbase->cloth_comment = 0;

	while (tmpsell != NULL) {
		tmpbase->cloth_sold_num++;
		tmpbase->cloth_comment += tmpsell->consumer_comment;

		tmpsell = tmpsell->sell_next;
	}
	if (tmpbase->cloth_sold_num != 0) {
		tmpbase->cloth_comment = (float)tmpbase->cloth_comment/(float)tmpbase->cloth_sold_num;
	}
	else {
		tmpbase->cloth_comment = 0;
	}
}

//辅助查找
CLO_BASE * cloth_sell_search(char * clothname) {
	CLO_TYPE * tmp = mainchain;
	CLO_BASE * tmpbase;

	while(tmp != NULL) {

		tmpbase = tmp->base_info;

		while (tmpbase != NULL) {
			if (strcmp(tmpbase->cloth_name, clothname) == 0) {
				return tmpbase;
			}
			else {
				tmpbase = tmpbase->base_next;
			}
		}

		tmp = tmp->type_next;
	}

	return NULL;
}
//数据的删除 这里因为购买数据可能重复，所以删除的话需要很多数据
void cloth_sell_info_delete(char * clothname, char * soldtime, char * consumername, int consumercomment) {
	CLO_BASE * tmpbase = cloth_sell_search(clothname);
	CLO_SELL * nownode, * prevnode;

	nownode = tmpbase->sell_info;
	prevnode = NULL;

	while (nownode != NULL) {
		if (strcmp(nownode->sold_time, soldtime) == 0 && strcmp(nownode->consumer_name, consumername) == 0 && nownode->consumer_comment == consumercomment) {
			if (prevnode == NULL) {
				tmpbase->sell_info = nownode->sell_next;
				break;
			}
			else {
				prevnode->sell_next = nownode->sell_next;
				break;
			}
		}
		else {
			prevnode = nownode;
			nownode = nownode->sell_next;
		}
	}

	cloth_sell_info_check(tmpbase);
}
//信息更改 按照逻辑来说是不能改名字的 除了原来的衣服更改名字
void cloth_sell_info_change(char * name, char * prevtime, char * nowtime, char * prevcname, char * nowcname, int prevcom, int nowcom) {

	CLO_TYPE * tmp = mainchain;
	CLO_BASE * tmpbase = cloth_sell_search(name);
	CLO_SELL * tmpsell = tmpbase->sell_info;

	while (tmpsell != NULL) {
		if (strcmp(tmpsell->sold_time, prevtime) == 0 && strcmp(tmpsell->consumer_name, prevcname) == 0 && tmpsell->consumer_comment == prevcom) {
			strcpy(tmpsell->sold_time, nowtime);
			strcpy(tmpsell->consumer_name, nowcname);
			tmpsell->consumer_comment = nowcom;
		}
		tmpsell = tmpsell->sell_next;
	}

	cloth_sell_info_check(tmpbase);
}
//销售信息的录入
void cloth_sell_info_input(char * clothname, char * soldtime, char *cname, int ccomment) {
	CLO_BASE * tmpbase = cloth_sell_search(clothname);
	if (tmpbase == NULL) {
		return;
	}

	CLO_SELL * tmpsell = tmpbase->sell_info;
	if (tmpsell == NULL) {
		tmpbase->sell_info = (CLO_SELL *)malloc(sizeof(CLO_SELL));
		tmpbase->sell_info->sell_next = NULL;
		tmpsell = tmpbase->sell_info;

		strcpy(tmpsell->cloth_name, clothname);
		strcpy(tmpsell->sold_time, soldtime);
		strcpy(tmpsell->consumer_name, cname);
		tmpsell->consumer_comment = ccomment;
	}
	else {		
		while (tmpsell->sell_next != NULL) {
			tmpsell = tmpsell->sell_next;
		}

		tmpsell = tmpsell->sell_next = (CLO_SELL *)malloc(sizeof(CLO_SELL));
		tmpsell->sell_next = NULL;

		strcpy(tmpsell->cloth_name, clothname);
		strcpy(tmpsell->sold_time, soldtime);
		strcpy(tmpsell->consumer_name, cname);
		tmpsell->consumer_comment = ccomment;
	}


	cloth_sell_info_check(tmpbase);
}

/*********下面是gtk liststore的变换**********/

//这里的pattern t表示类型信息 b表示服装基本信息 s表示销售基本信息 1~6分别是数据统计相关
void treeview_change (char pattern) {
	GtkListStore * list;
	GtkTreeViewColumn * column;
	GtkCellRenderer * cellrenderer = gtk_cell_renderer_text_new();
	int n, i;
	CLO_TYPE * tmp = mainchain;


//先清除所有的列
	n = gtk_tree_view_get_n_columns(treeview);

	for (i=0; i < n; i++) {
		column = gtk_tree_view_get_column(treeview, 0);
		gtk_tree_view_remove_column(treeview, column);
	}
//下来添加
	if (pattern == 't') {

		column = gtk_tree_view_column_new_with_attributes("分类编码",cellrenderer, "text", 0, NULL);
		gtk_tree_view_append_column(treeview, column);

		column = gtk_tree_view_column_new_with_attributes("分类名称",cellrenderer, "text", 1, NULL);
		gtk_tree_view_append_column(treeview, column);

	}
	else if (pattern == 'b') {
		column = gtk_tree_view_column_new_with_attributes("分类码",cellrenderer, "text", 0, NULL);
		gtk_tree_view_append_column(treeview, column);

		column = gtk_tree_view_column_new_with_attributes("服饰名称",cellrenderer, "text", 1, NULL);
		gtk_tree_view_append_column(treeview, column);

		column = gtk_tree_view_column_new_with_attributes("样式",cellrenderer, "text", 2, NULL);
		gtk_tree_view_append_column(treeview, column);

		column = gtk_tree_view_column_new_with_attributes("单价", cellrenderer, "text", 3, NULL);
		gtk_tree_view_append_column(treeview, column);

		column = gtk_tree_view_column_new_with_attributes("售出件数",cellrenderer, "text", 4, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("评价指数",cellrenderer, "text", 5, NULL);
		gtk_tree_view_append_column(treeview, column);
	}
	else if (pattern == 's') {
		column = gtk_tree_view_column_new_with_attributes("服饰名称",cellrenderer, "text", 0, NULL);
		gtk_tree_view_append_column(treeview, column);

		column = gtk_tree_view_column_new_with_attributes("销售日期",cellrenderer, "text", 1, NULL);
		gtk_tree_view_append_column(treeview, column);

		column = gtk_tree_view_column_new_with_attributes("客户名称",cellrenderer, "text", 2, NULL);
		gtk_tree_view_append_column(treeview, column);

		column = gtk_tree_view_column_new_with_attributes("客户评价",cellrenderer, "text", 3, NULL);
		gtk_tree_view_append_column(treeview, column);
	}
	else if (pattern == '1') {
		column = gtk_tree_view_column_new_with_attributes("分类名称", cellrenderer, "text", 0, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("销售总件数", cellrenderer, "text", 1, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("销售总额", cellrenderer, "text", 2, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("评价>=3件数", cellrenderer, "text", 3, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("评价<3件数", cellrenderer, "text", 4, NULL);
		gtk_tree_view_append_column(treeview, column);
	}
	else if (pattern == '2') {
		column = gtk_tree_view_column_new_with_attributes("服饰名称", cellrenderer, "text", 0, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("分类名称", cellrenderer, "text", 1, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("售出件数", cellrenderer, "text", 2, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("销售金额", cellrenderer, "text", 3, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("评价指数", cellrenderer, "text", 4, NULL);
		gtk_tree_view_append_column(treeview, column);
	}
	else if (pattern == '3') {
		column = gtk_tree_view_column_new_with_attributes("客户名称", cellrenderer, "text", 0, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("所购服装总件数", cellrenderer, "text", 1, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("消费总金额", cellrenderer, "text", 2, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("总体评价度", cellrenderer, "text", 3, NULL);
		gtk_tree_view_append_column(treeview, column);
	}
	else if (pattern == '4' || pattern == '5') {
		column = gtk_tree_view_column_new_with_attributes("季节", cellrenderer, "text", 0, NULL);
		gtk_tree_view_append_column(treeview, column);
		i = 1;
		while(tmp != NULL) {
			column = gtk_tree_view_column_new_with_attributes(tmp->type_name, cellrenderer, "text", i, NULL);
			gtk_tree_view_append_column(treeview, column);
			i++;
			tmp = tmp->type_next;
		}

	}
	else if (pattern == '6') {
		column = gtk_tree_view_column_new_with_attributes("销售总收入", cellrenderer, "text", 0, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("销售总量", cellrenderer, "text", 1, NULL);
		gtk_tree_view_append_column(treeview, column);
	}
}

//下来是 liststore 的数据的填充
GtkListStore * liststore_change(char  pattern) {
	//先创建liststore	
	CLO_TYPE * tmp = mainchain;
	int i = 0, j;

	if (pattern == 't') {
		return gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	}
	else if (pattern == 'b') {
		return gtk_list_store_new(6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_INT, G_TYPE_FLOAT);
	}
	else if (pattern == 's') {
		return gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
	}
	else if (pattern == '1') {
		return gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_INT, G_TYPE_FLOAT, G_TYPE_INT, G_TYPE_INT);
	}
	else if (pattern == '2') {
		return gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_FLOAT, G_TYPE_FLOAT);
	}
	else if (pattern == '3') {
		return gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_INT, G_TYPE_FLOAT, G_TYPE_FLOAT);
	}
	else if (pattern == '4') {
		GType * types;

		while (tmp != NULL) {
			i++;
			tmp = tmp->type_next;
		}

		types = (GType *)malloc(sizeof(GType)*(i+1));

		types[0] = G_TYPE_STRING;

		for (j=1; j <= i; j++) {
			types[j] = G_TYPE_INT;
		}

		return gtk_list_store_newv(i+1, types);;
	}
	else if (pattern == '5') {
		GType * types;

		while (tmp != NULL) {
			i++;
			tmp = tmp->type_next;
		}

		types = (GType *)malloc(sizeof(GType)*(i+1));
		types[0] = G_TYPE_STRING;

		for (j=1; j <= i; j++) {
			types[j] = G_TYPE_FLOAT;
		}

		return gtk_list_store_newv(i+1, types);
	}
	else if (pattern == '6') {
		return gtk_list_store_new(2, G_TYPE_INT, G_TYPE_FLOAT);
	}
}
	//下来是要处理显示数据的函数了 fresh表示是否刷新liststore, >0 刷新 , <0不
void show_type_info(CLO_TYPE * chain, int fresh) {

	CLO_TYPE * tmp = chain;
	GtkListStore * list;
	if (fresh>0) {
		list = liststore_change('t');
		gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(list));
		treeview_change('t');
		clothInfoChangeType = 1;
	}
	else {
		list = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	}

	GtkTreeIter iter;


	//下来开始向liststore上面插入数据
	
	while (tmp != NULL) {
		gtk_list_store_append(list, &iter);

		gtk_list_store_set(list, &iter, 0, CTOS(tmp->type_num), 1, tmp->type_name, -1);

		tmp = tmp->type_next;
	}
}
void show_base_info(CLO_BASE * chain, int fresh) {

	CLO_BASE * tmp = chain;
	GtkListStore * list;
	if (fresh > 0) {
		list = liststore_change('b');
		treeview_change('b');
		gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(list));
		clothInfoChangeType = 2;
	}
	else {
		list = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	}
	GtkTreeIter iter;


	while (tmp != NULL) {
		gtk_list_store_append(list, &iter);

		gtk_list_store_set(list, &iter, 0, CTOS(tmp->cloth_type), 1, tmp->cloth_name, 2, CTOS(tmp->cloth_sex), 3, tmp->cloth_price, 4, tmp->cloth_sold_num, 5, tmp->cloth_comment, -1);

		tmp = tmp->base_next;
	}
}
void show_sell_info(CLO_SELL * chain, int fresh) {

	CLO_SELL * tmp = chain;
	GtkListStore * list;
	GtkTreeIter iter;

	if (fresh > 0) {
		list = liststore_change('s');
		treeview_change('s');
		gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(list));
		clothInfoChangeType = 3;
	}
	else {
		list = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));	
	}


	while (tmp != NULL) {
		gtk_list_store_append(list, &iter);

		gtk_list_store_set(list, &iter, 0, tmp->cloth_name, 1, tmp->sold_time, 2, tmp->consumer_name, 3, tmp->consumer_comment, -1);

		tmp = tmp->sell_next;
	}
}


/*下来是显示所有的信息*/
void show_info_all(GtkWidget * widget, char * pattern) {
	CLO_TYPE * tmptype = mainchain;
	CLO_BASE * tmpbase;

	int flag = 1;
	if (pattern[0] == 't') {
		show_type_info(tmptype, 1);
	}
	else if (pattern[0] == 'b') {

		if (tmptype != NULL) {
			show_base_info(tmptype->base_info, 1);
			tmptype = tmptype->type_next;
		}
		else {
			show_base_info(NULL, 1);
			return;
		}

		while (tmptype != NULL) {
			show_base_info(tmptype->base_info, -1);
			tmptype = tmptype->type_next;
		}
	}
	else if (pattern[0] == 's') {

		if (tmptype == NULL) {
			show_sell_info(NULL, 1);
			return;
		}

		while (tmptype != NULL) {
			tmpbase = tmptype->base_info;

			if (flag) {
				if (tmpbase != NULL) {
					show_sell_info(tmpbase->sell_info, 1);
					tmpbase = tmpbase->base_next;
				}
				else {
					show_sell_info(NULL, 1);
					return;
				}
				flag = 0;
			}

			while (tmpbase != NULL) {
				show_sell_info(tmpbase->sell_info, -1);
				tmpbase = tmpbase->base_next;
			}
			tmptype = tmptype->type_next;
		}

	}
}
//首先是信息的删除功能
void cloth_info_delete(GtkWidget * widget, GtkTreeSelection * selection) {

	GtkTreeIter iter;
	GtkTreeModel * model;

	CLO_TYPE * tmp = mainchain;

	if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
		return;
	}

	if (clothInfoChangeType == 0) {
		return;
	}
	else if (clothInfoChangeType == 1) {
		char * typeNum, * typeName;
		gtk_tree_model_get(model, &iter, 0, &typeNum, 1, &typeName, -1);

		cloth_type_info_delete(typeName);

		free(typeNum);
		free(typeName);
		show_info_all(widget, "t");
	}
	else if (clothInfoChangeType == 2) {
		char * clothName, * clothType;
		gtk_tree_model_get(model, &iter, 0, &clothType, 1, &clothName, -1);

		cloth_base_info_delete(clothType[0], clothName);

		free(clothName);
		free(clothType);
		show_info_all(widget, "b");
	}
	else if (clothInfoChangeType == 3) {
		char * clothName, * clothSoldTime, * clothCName;
		int clothCComment;
		gtk_tree_model_get(model, &iter, 0, &clothName, 1, &clothSoldTime, 2, &clothCName, 3, &clothCComment, -1);

		cloth_sell_info_delete(clothName, clothSoldTime, clothCName, clothCComment);

		free(clothName);
		free(clothSoldTime);
		free(clothCName);

		show_info_all(widget, "s");
	}
}
//再者是信息的更改功能
void cloth_info_change(GtkWidget * widget, GtkTreeSelection * selection) {

	CLO_TYPE * tmp = mainchain;
	GtkWidget * dialog;
	GtkWidget * box = gtk_grid_new();
	GtkWidget * entry1, * entry2, * entry3, * entry4;
	GtkWidget * label1, * label2, * label3, * label4;
	GtkTreeIter iter;
	GtkTreeModel * model;
	GtkEntryBuffer * buffer;

	if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
		return;
	}

	if (clothInfoChangeType == 0) {
		return;
	}
	else if (clothInfoChangeType == 1) {
		char * typeNum, * typeName;
		char * tmpnum, * tmpname;

		tmpnum = (char *)malloc(2);
		tmpname = (char *)malloc(21);

		gtk_tree_model_get(model, &iter, 0, &typeNum, 1, &typeName, -1);
		entry1 = gtk_entry_new();
		label1 = gtk_label_new("分类编码");
		entry2 = gtk_entry_new();
		label2 = gtk_label_new("分类名称");

		gtk_grid_set_row_spacing (GTK_GRID (box), 5);
		gtk_grid_attach(GTK_GRID(box), label1, 0, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), label2, 0, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry1, 1, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry2, 1, 1, 1, 1);

		dialog = gtk_dialog_new_with_buttons("更改分类信息", GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_确定", GTK_RESPONSE_ACCEPT, "_取消", GTK_RESPONSE_REJECT, NULL);
		gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
		GtkWidget * content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

		gtk_container_add(GTK_CONTAINER(content), box);

		CLO_TYPE * node = cloth_type_search('m', typeName);

		gtk_widget_show_all(dialog);

		int response = gtk_dialog_run(GTK_DIALOG(dialog));

		if (response == GTK_RESPONSE_ACCEPT) {
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry1));
			strcpy(tmpnum, gtk_entry_buffer_get_text(buffer));
			tmpnum[1] = '\0';
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry2));
			strcpy(tmpname, gtk_entry_buffer_get_text(buffer));
			tmpname[20] = '\0';

			if (tmpnum[0] >= '1' && tmpnum[0] <= '9') {
				cloth_type_info_change(node, 'n', tmpnum);
			}

			if (tmpname[0] != '\0') {
				cloth_type_info_change(node, 'm', tmpname);
			}

			show_info_all(widget, "t");
		}

		gtk_widget_destroy(dialog);
	}
	else if (clothInfoChangeType == 2) {
		char * clothname, * clothsex, * clothtype;
		char *tmpname, * tmpsex, * tmptype, * tmpstr;
		float  clothprice, tmpprice;

		tmpname = (char *)malloc(31);
		tmpsex = (char *)malloc(2);
		tmptype = (char *)malloc(2);
		tmpstr = (char *)malloc(21);

		gtk_tree_model_get(model, &iter, 0, &clothtype, 1, &clothname, 2, &clothsex, 3, &clothprice, -1);

		label1 = gtk_label_new("分类编码");
		label2 = gtk_label_new("服饰名称");
		label3 = gtk_label_new("服饰样式");
		label4 = gtk_label_new("服饰单价");
		entry1 = gtk_entry_new();
		entry2 = gtk_entry_new();
		entry3 = gtk_entry_new();
		entry4 = gtk_entry_new();

		gtk_grid_set_row_spacing (GTK_GRID (box), 5);
		gtk_grid_attach(GTK_GRID(box), label1, 0, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), label2, 0, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(box), label3, 0, 2, 1, 1);
		gtk_grid_attach(GTK_GRID(box), label4, 0, 3, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry1, 1, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry2, 1, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry3, 1, 2, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry4, 1, 3, 1, 1);

		dialog = gtk_dialog_new_with_buttons("更改服饰基本信息", GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_确定", GTK_RESPONSE_ACCEPT, "_取消", GTK_RESPONSE_REJECT, NULL);
		gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
		GtkWidget * content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

		gtk_container_add(GTK_CONTAINER(content), box);

		CLO_BASE * node = cloth_base_search(clothtype[0], clothname);

		gtk_widget_show_all(dialog);

		int response = gtk_dialog_run(GTK_DIALOG(dialog));

		if (response == GTK_RESPONSE_ACCEPT) {
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry1));
			strcpy(tmptype, gtk_entry_buffer_get_text(buffer));
			tmptype[1] = '\0';
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry2));
			strcpy(tmpname, gtk_entry_buffer_get_text(buffer));
			tmpname[30] = '\0';
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry3));
			strcpy(tmpsex, gtk_entry_buffer_get_text(buffer));
			tmpsex[1] = '\0';
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry4));
			strcpy(tmpstr, gtk_entry_buffer_get_text(buffer));
			tmpprice = STOF(tmpstr);


			if (!(tmptype[0] >= '1' && tmptype[0] <= '9')) {
				tmptype[0] = clothtype[0];
			}

			if (tmpname[0] == '\0') {
				strcpy(tmpname, clothname);
			}
			if ((tmpsex[0] != '0' && tmpsex[0] != '1' && tmpsex[0] != '9')||tmpsex[0] == '\0') {
				tmpsex[0] = clothsex[0];
			}

			if (!(tmpprice > 0)) {
				tmpprice = clothprice;
			}

			cloth_base_info_change(node, clothtype[0], tmptype[0], tmpname, tmpsex[0], tmpprice);

			show_info_all(widget, "b");

			free(clothname);
			free(clothsex);
			free(clothtype);
			free(tmpname);
			free(tmpsex);
			free(tmptype);
			free(tmpstr);
		}

		gtk_widget_destroy(dialog);
	}
	else if (clothInfoChangeType == 3) {
		char * clothname, * soldtime, * cname;
		char * tmptime, * tmpcname, *tmpstr;
		int ccom, tmpccom;

		tmptime = (char *)malloc(11);
		tmpcname = (char *)malloc(21);
		tmpstr = (char *)malloc(21);

		gtk_tree_model_get(model, &iter, 0, &clothname, 1, &soldtime, 2, &cname, 3, &ccom, -1);

		entry1 = gtk_entry_new();
		entry2 = gtk_entry_new();
		entry3 = gtk_entry_new();
		label1 = gtk_label_new("销售日期 ");
		label2 = gtk_label_new("客户名称 ");
		label3 = gtk_label_new("客户评价 ");

		gtk_grid_set_row_spacing(GTK_GRID(box), 5);
		gtk_grid_attach(GTK_GRID(box), label1, 0, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), label2, 0, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(box), label3, 0, 2, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry1, 1, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry2, 1, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry3, 1, 2, 1, 1);

		dialog = gtk_dialog_new_with_buttons("更改销售基本信息", GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_确定", GTK_RESPONSE_ACCEPT, "_取消", GTK_RESPONSE_REJECT, NULL);
		gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
		GtkWidget * content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

		gtk_container_add(GTK_CONTAINER(content), box);

		gtk_widget_show_all(dialog);

		int response = gtk_dialog_run(GTK_DIALOG(dialog));

		if (response == GTK_RESPONSE_ACCEPT) {
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry1));
			strcpy(tmptime, gtk_entry_buffer_get_text(buffer));
			tmptime[10] = '\0';
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry2));
			strcpy(tmpcname, gtk_entry_buffer_get_text(buffer));
			tmpcname[20] = '\0';
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry3));
			strcpy(tmpstr, gtk_entry_buffer_get_text(buffer));
			tmpccom = tmpstr[0] - '0';

			if (tmptime[0] == '\0') {
				strcpy(tmptime, soldtime);
			}

			if (tmpcname[0] == '\0') {
				strcpy(tmpcname, cname);
			}

			if (!(tmpccom>=1&&tmpccom<=5)) {
				tmpccom = ccom;
			}

			//g_print("%s\n%s\n%s\n%d\n", clothname,tmptime, tmpcname, tmpccom);
			cloth_sell_info_change(clothname, soldtime, tmptime, cname, tmpcname, ccom, tmpccom);

			show_info_all(widget, "s");

			free(clothname);
			free(soldtime);
			free(cname);
			free(tmptime);
			free(tmpcname);
			free(tmpstr);
		}

		gtk_widget_destroy(dialog);
	}
}
//下来是信息的填充功能
void cloth_info_input(GtkWidget * widget, char * data) {

	GtkWidget * dialog;
	GtkWidget * content;
	GtkWidget *entry1, *entry2, *entry3, *entry4;
	GtkWidget *label1, *label2, *label3, *label4;
	GtkWidget *box = gtk_grid_new();
	GtkEntryBuffer *buffer;
	int response;

	if (data[0] == '1') {
		char * typeNum, * typeName;

		typeNum = (char *)malloc(11);
		typeName = (char *)malloc(11);

		dialog = gtk_dialog_new_with_buttons("添加服饰分类信息", GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_确定", GTK_RESPONSE_ACCEPT, "_取消", GTK_RESPONSE_REJECT, NULL);

		content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

		entry1 = gtk_entry_new();
		entry2 = gtk_entry_new();
		label1 = gtk_label_new("分类编码");
		label2 = gtk_label_new("分类名称");

		gtk_grid_set_row_spacing(GTK_GRID(box), 5);
		gtk_grid_attach(GTK_GRID(box), label1, 0, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), label2, 0, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry1, 1, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry2, 1, 1, 1, 1);

		gtk_container_add(GTK_CONTAINER(content), box);

		gtk_widget_show_all(dialog);

		response = gtk_dialog_run(GTK_DIALOG(dialog));

		if (response == GTK_RESPONSE_ACCEPT) {
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry1));
			strcpy(typeNum, gtk_entry_buffer_get_text(buffer));
			typeNum[1] = '\0';
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry2));
			strcpy(typeName, gtk_entry_buffer_get_text(buffer));
			typeName[10] = '\0';

			if(!(typeNum[0] >= '0' && typeNum[0] <= '9') || typeName[0] == '\0') {
			}
			else {
				cloth_type_info_input(typeNum, typeName);
			}

			show_info_all(widget, "t");

			free(typeName);
			free(typeNum);
		}

		gtk_widget_destroy(dialog);
	}
	else if (data[0] == '2') {
		char * clothtype, * clothname, * clothsex, * str;
		float clothprice;

		clothtype = (char *)malloc(11);
		clothname = (char *)malloc(31);
		clothsex = (char *)malloc(11);
		str = (char *)malloc(21);

		dialog = gtk_dialog_new_with_buttons("添加服饰基本信息", GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_确定", GTK_RESPONSE_ACCEPT, "_取消", GTK_RESPONSE_REJECT, NULL);

		content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

		entry1 = gtk_entry_new();
		entry2 = gtk_entry_new();
		entry3 = gtk_entry_new();
		entry4 = gtk_entry_new();
		label1 = gtk_label_new("分类编码");
		label2 = gtk_label_new("服饰名称");
		label3 = gtk_label_new("样式");
		label4 = gtk_label_new("单价");

		gtk_grid_set_row_spacing(GTK_GRID(box), 5);
		gtk_grid_attach(GTK_GRID(box), label1, 0, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), label2, 0, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(box), label3, 0, 2, 1, 1);
		gtk_grid_attach(GTK_GRID(box), label4, 0, 3, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry1, 1, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry2, 1, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry3, 1, 2, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry4, 1, 3, 1, 1);

		gtk_container_add(GTK_CONTAINER(content), box);

		gtk_widget_show_all(dialog);

		response = gtk_dialog_run(GTK_DIALOG(dialog));

		if(response == GTK_RESPONSE_ACCEPT) {
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry1));
			strcpy(clothtype, gtk_entry_buffer_get_text(buffer));
			clothtype[1] = '\0';
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry2));
			strcpy(clothname, gtk_entry_buffer_get_text(buffer));
			clothname[30] = '\0';
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry3));
			strcpy(clothsex, gtk_entry_buffer_get_text(buffer));
			clothsex[1] = '\0';
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry4));
			strcpy(str, gtk_entry_buffer_get_text(buffer));
			clothprice = STOF(str);

			if(!(clothtype[0] >= '0' && clothtype[0] <= '9') || clothname[0] == '\0' || (clothsex[0] != '0' && clothsex[0] != '1' && clothsex[0] != '9') || !(clothprice >= 0)) {
			}
			else {
				cloth_base_info_input(clothtype[0], clothname, clothsex[0], clothprice);
			}

			show_info_all(widget, "b");
			free(str);
			free(clothtype);
			free(clothsex);
			free(clothname);
		}

		gtk_widget_destroy(dialog);
	}
	else if (data[0] == '3') {
		char * clothname, * soldtime, * cname, * str;
		int ccom;

		clothname = (char *)malloc(31);
		soldtime = (char *)malloc(11);
		cname = (char *)malloc(21);
		str = (char *)malloc(21);

		dialog  = gtk_dialog_new_with_buttons("添加销售信息", GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_确定", GTK_RESPONSE_ACCEPT, "_取消", GTK_RESPONSE_REJECT, NULL);

		content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

		entry1 = gtk_entry_new();
		entry2 = gtk_entry_new();
		entry3 = gtk_entry_new();
		entry4 = gtk_entry_new();
		label1 = gtk_label_new("服饰名称");
		label2 = gtk_label_new("销售日期");
		label3 = gtk_label_new("客户名称");
		label4 = gtk_label_new("客户评价");

		gtk_grid_set_row_spacing(GTK_GRID(box), 5);
		gtk_grid_attach(GTK_GRID(box), label1, 0, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), label2, 0, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(box), label3, 0, 2, 1, 1);
		gtk_grid_attach(GTK_GRID(box), label4, 0, 3, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry1, 1, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry2, 1, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry3, 1, 2, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry4, 1, 3, 1, 1);

		gtk_container_add(GTK_CONTAINER(content), box);

		gtk_widget_show_all(dialog);

		response = gtk_dialog_run(GTK_DIALOG(dialog));

		if (response == GTK_RESPONSE_ACCEPT) {
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry1));
			strcpy(clothname, gtk_entry_buffer_get_text(buffer));
			clothname[30] = '\0';
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry2));
			strcpy(soldtime, gtk_entry_buffer_get_text(buffer));
			soldtime[10] = '\0';
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry3));
			strcpy(cname, gtk_entry_buffer_get_text(buffer));
			cname[20] = '\0';
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry4));
			strcpy(str, gtk_entry_buffer_get_text(buffer));
			ccom = str[0] - '0';

			if(clothname[0] == '\0' || soldtime[0] == '\0' || cname[0] == '\0' || !(ccom >= 1 && ccom <= 5)) {
			}
			else {
				cloth_sell_info_input(clothname, soldtime, cname, ccom);
			}

			free(clothname);
			free(soldtime);
			free(cname);
			free(str);

			show_info_all(widget, "s");
		}

		gtk_widget_destroy(dialog);
	}
}
//下来是信息的查询功能
CLO_TYPE * type_search(char typenum) {
	CLO_TYPE * tmp = mainchain;
	CLO_TYPE * typeass = NULL;
	CLO_TYPE * typeasshead = NULL;
	while (tmp != NULL) {
		if (tmp->type_num == typenum) {
			if (typeass == NULL) {
				typeasshead = typeass = (CLO_TYPE *)malloc(sizeof(CLO_TYPE));
				typeass->type_num = tmp->type_num;
				strcpy(typeass->type_name,tmp->type_name);
				typeass->type_next = NULL;
			}
			else {
				typeass->type_next = (CLO_TYPE *)malloc(sizeof(CLO_TYPE));
				typeass = typeass->type_next;
				typeass->type_num = tmp->type_num;
				strcpy(typeass->type_name,tmp->type_name);
				typeass->type_next = NULL;
			}
		}
		tmp = tmp->type_next;
	}
	return typeasshead;
}
CLO_BASE * base_search_byname(char * name) {
	CLO_TYPE * tmp = mainchain;
	CLO_BASE * tmpbase;
	CLO_BASE * baseass = NULL;
	CLO_BASE * baseasshead = NULL;

	while (tmp != NULL) {
		tmpbase = tmp->base_info;

		while (tmpbase != NULL) {
			if (strstr(tmpbase->cloth_name, name) != NULL) {
				if (baseass == NULL) {
					baseasshead = baseass = (CLO_BASE *)malloc(sizeof(CLO_BASE));
				}
				else {
					baseass->base_next = (CLO_BASE *)malloc(sizeof(CLO_BASE));
					baseass = baseass->base_next;
				}
				baseass->cloth_type = tmpbase->cloth_type;
				strcpy(baseass->cloth_name, tmpbase->cloth_name);
				baseass->cloth_sex = tmpbase->cloth_sex;
				baseass->cloth_price = tmpbase->cloth_price;
				baseass->cloth_sold_num = tmpbase->cloth_sold_num;
				baseass->cloth_comment = tmpbase->cloth_comment;
				baseass->sell_info = tmpbase->sell_info;
				baseass->base_next = NULL;
			}
			tmpbase = tmpbase->base_next;
		}

		tmp = tmp->type_next;
	}

	return baseasshead;
}
CLO_BASE * base_search_bytype(char type, char * bottom, char * top) {

	float bprice, tprice;

	bprice = STOF(bottom);
	tprice = STOF(top);

	if (bottom[0] != '\0' && top[0] != '\0' && bprice > tprice) {
		return NULL;
	}

	CLO_TYPE * tmp = mainchain;
	CLO_BASE * tmpbase;
	CLO_BASE * baseass = NULL, * baseasshead = NULL;
	int i;

	while (tmp != NULL) {
		if (tmp->type_num == type) {
			break;
		}
		else {
			tmp = tmp->type_next;
		}
	}

	if (tmp == NULL) {
		return NULL;
	}
	else {
		tmpbase = tmp->base_info;
	}

	while (tmpbase != NULL) {

		if (bottom[0] != '\0' && top[0] != '\0') {
			i = (tmpbase->cloth_price >= bprice);
		}
		else if (bottom != '\0') {
			i = (tmpbase->cloth_price >= bprice);
		}
		else if (top != '\0') {
			i = (tmpbase->cloth_price <= tprice);
		}
		else {
			i = 1;
		}

		if (i) {
			if (baseass == NULL) {
				baseasshead = baseass = (CLO_BASE *)malloc(sizeof(CLO_BASE));
			}
			else {
				baseass->base_next = (CLO_BASE *)malloc(sizeof(CLO_BASE));
				baseass = baseass->base_next;
			}
			baseass->cloth_type = tmpbase->cloth_type;
			strcpy(baseass->cloth_name, tmpbase->cloth_name);
			baseass->cloth_sex = tmpbase->cloth_sex;
			baseass->cloth_price = tmpbase->cloth_price;
			baseass->cloth_sold_num = tmpbase->cloth_sold_num;
			baseass->cloth_comment = tmpbase->cloth_comment;
			baseass->sell_info = tmpbase->sell_info;
			baseass->base_next = NULL;
		}

		tmpbase = tmpbase->base_next;
	}

	return baseasshead;

}
CLO_SELL * sell_search(char * clothname, char * soldtime, char * cname, char ccom) {
	CLO_TYPE * tmp = mainchain;
	CLO_BASE * tmpbase;
	CLO_SELL * sellass = NULL, * sellasshead = NULL, * tmpsell;
	int i, j , k, x;
	int flag;

	while (tmp != NULL) {
		tmpbase = tmp->base_info;

		while (tmpbase != NULL) {
			tmpsell = tmpbase->sell_info;

			while (tmpsell != NULL) {
				i = strcmp(clothname, tmpsell->cloth_name);
				j = strcmp(soldtime, tmpsell->sold_time);
				k = strcmp(cname, tmpsell->consumer_name);
				x = ((ccom-'0') != tmpsell->consumer_comment);

				if (clothname[0] != '\0' && soldtime[0] != '\0' && cname[0] != '\0' && ccom != '\0') {
					if (i == 0 && j == 0 && k == 0 && x == 0) {
						flag = 1;
					}
					else {
						flag = 0;
					}
				}
				else if (clothname[0] == '\0' && soldtime[0] != '\0' && cname[0] != '\0' && ccom != '\0') {
					if (j == 0 && k == 0 && x == 0) {
						flag = 1;
					}
					else {
						flag = 0;
					}
				}
				else if (clothname[0] != '\0' && soldtime[0] == '\0' && cname[0] != '\0' && ccom != '\0') {
					if (i == 0 && k == 0 && x == 0) {
						flag = 1;
					}
					else {
						flag = 0;
					}
				}
				else if (clothname[0] != '\0' && soldtime[0] != '\0' && cname[0] == '\0' && ccom != '\0') {
					if (i == 0 && j == 0 && x == 0) {
						flag = 1;
					}
					else {
						flag = 0;
					}
				}
				else if (clothname[0] != '\0' && soldtime[0] != '\0' && cname[0] != '\0' && ccom == '\0') {
					if (i == 0 && j == 0 && k == 0) {
						flag = 1;
					}
					else {
						flag = 0;
					}
				}
				else if (clothname[0] == '\0' && soldtime[0] == '\0' && cname[0] != '\0' && ccom != '\0') {
					if (x == 0 && k == 0) {
						flag = 1;
					}
					else {
						flag = 0;
					}
				}
				else if (clothname[0] == '\0' && soldtime[0] != '\0' && cname[0] == '\0' && ccom != '\0') {
					if (j == 0 && x == 0) {
						flag = 1;
					}
					else {
						flag = 0;
					}
				}
				else if (clothname[0] == '\0' && soldtime[0] != '\0' && cname[0] != '\0' && ccom == '\0') {
					if (j == 0 && k == 0) {
						flag = 1;
					}
					else {
						flag = 0;
					}
				}
				else if (clothname[0] != '\0' && soldtime[0] == '\0' && cname[0] == '\0' && ccom != '\0') {
					if (i == 0 && x == 0) {
						flag = 1;
					}
					else {
						flag = 0;
					}
				}
				else if (clothname[0] != '\0' && soldtime[0] == '\0' && cname[0] != '\0' && ccom == '\0') {
					if (i == 0 && k == 0) {
						flag = 1;
					}
					else {
						flag = 0;
					}
				}
				else if (clothname[0] != '\0' && soldtime[0] != '\0' && cname[0] == '\0' && ccom == '\0') {
					if (i == 0 && j == 0) {
						flag = 1;
					}
					else {
						flag = 0;
					}
				}
				else if (clothname[0] == '\0' && soldtime[0] == '\0' && cname[0] == '\0' && ccom != '\0') {
					if (x == 0) {
						flag = 1;
					}
					else {
						flag = 0;
					}
				}
				else if (clothname[0] == '\0' && soldtime[0] == '\0' && cname[0] != '\0' && ccom == '\0') {
					if (k == 0) {
						flag = 1;
					}
					else {
						flag = 0;
					}
				}
				else if (clothname[0] == '\0' && soldtime[0] != '\0' && cname[0] == '\0' && ccom == '\0') {
					if (j == 0) {
						flag = 1;
					}
					else {
						flag = 0;
					}
				}
				else if (clothname[0] != '\0' && soldtime[0] == '\0' && cname[0] == '\0' && ccom == '\0') {
					if (i == 0) {
						flag = 1;
					}
					else {
						flag = 0;
					}
				}

				if (flag) {
					if (sellass == NULL) {
						sellass = sellasshead = (CLO_SELL *)malloc(sizeof(CLO_SELL));
					}
					else {
						sellass->sell_next = (CLO_SELL *)malloc(sizeof(CLO_SELL));
						sellass = sellass->sell_next;
					}

					strcpy(sellass->cloth_name, tmpsell->cloth_name);
					strcpy(sellass->sold_time, tmpsell->sold_time);
					strcpy(sellass->consumer_name, tmpsell->consumer_name);
					sellass->consumer_comment = tmpsell->consumer_comment;
					sellass->sell_next = NULL;
				}

				tmpsell = tmpsell->sell_next;
			}

			tmpbase = tmpbase->base_next;
		}

		tmp = tmp->type_next;
	}

	return sellasshead;
}
void cloth_info_search(GtkWidget * widget, char * data) {

	GtkWidget * dialog, * content;
	GtkWidget * entry1, *entry2, *entry3, *entry4;
	GtkWidget * label1, *label2, *label3, *label4;
	GtkWidget * box = gtk_grid_new();
	GtkWidget * box1 = gtk_grid_new();
	GtkWidget * box2 = gtk_grid_new();
	GtkWidget * radio1;
	GtkWidget * radio2;
	GtkWidget * notice;

	GtkEntryBuffer * buffer;
	int response;

	if (data[0] == '1') {
		char * typenum = (char *)malloc(11);
		CLO_TYPE * typeass;

		dialog = gtk_dialog_new_with_buttons("查询服饰分类信息", GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_确定", GTK_RESPONSE_ACCEPT, "_取消", GTK_RESPONSE_REJECT, NULL);

		content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

		entry1 = gtk_entry_new();
		label1 = gtk_label_new("服饰分类编码");
		notice = gtk_label_new("提示: 输入搜寻分类码。");

		gtk_grid_set_row_spacing(GTK_GRID(box), 5);
		gtk_grid_attach(GTK_GRID(box), label1, 0, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry1, 1, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), notice, 0, 1, 2, 1);

		gtk_container_add(GTK_CONTAINER(content), box);

		gtk_widget_show_all(dialog);

		response = gtk_dialog_run(GTK_DIALOG(dialog));

		if (response == GTK_RESPONSE_ACCEPT) {
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry1));
			strcpy(typenum, gtk_entry_buffer_get_text(buffer));
			typenum[1] = '\0';

			if (typenum[0] != '\0') {
				typeass = type_search(typenum[0]);
				show_type_info(typeass, 1);
				clothInfoChangeType = 1;
			}
			else {
				show_type_info(NULL, 1);
				clothInfoChangeType = 1;
			}
		}
		free(typenum);
		gtk_widget_destroy(dialog);
	}
	else if (data[0] == '2') {
		char * namepart = (char *)malloc(31);
		char * bottom = (char *)malloc(11);
		char * top = (char *)malloc(11);
		CLO_BASE * baseass;

		dialog = gtk_dialog_new_with_buttons("查询服饰基本信息", GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_确定", GTK_RESPONSE_ACCEPT, "_取消", GTK_RESPONSE_REJECT, NULL);

		content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

		entry1 = gtk_entry_new();
		label1 = gtk_label_new("   服饰名称关键字:\t");
		entry2 = gtk_entry_new();
		label2 = gtk_label_new("   服饰分类码:\t");
		entry3 = gtk_entry_new();
		label3 = gtk_label_new(" 价格区间:");
		entry4 = gtk_entry_new();
		label4 = gtk_label_new("到");
		notice = gtk_label_new("\n提示: 有两种可选的搜寻方式，第二种的价格区间可以只输入一边。");


		gtk_grid_set_row_spacing(GTK_GRID(box), 5);
		gtk_grid_set_row_spacing(GTK_GRID(box1), 3);
		gtk_grid_set_row_spacing(GTK_GRID(box2), 3);

		gtk_grid_attach(GTK_GRID(box1), label1, 0, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box1), entry1, 1, 0, 2, 1);

		gtk_grid_attach(GTK_GRID(box2), label2, 0, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(box2), entry2, 1, 1, 2, 1);

		gtk_grid_attach(GTK_GRID(box2), label3, 0, 3, 1, 1);
		gtk_grid_attach(GTK_GRID(box2), entry3, 1, 3, 1, 1);
		gtk_grid_attach(GTK_GRID(box2), label4, 2, 3, 1, 1);
		gtk_grid_attach(GTK_GRID(box2), entry4, 3, 3, 1, 1);

		radio1 = gtk_radio_button_new_with_label(NULL, "根据服饰名称字段查找");
		radio2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio1), "根据分类码和价格区间查找");

		gtk_grid_attach(GTK_GRID(box), radio1, 0, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), box1, 0, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(box), radio2, 0, 2, 1, 1);
		gtk_grid_attach(GTK_GRID(box), box2, 0, 3, 1, 1);
		gtk_grid_attach(GTK_GRID(box), notice, 0, 4, 1, 1);

		gtk_container_add(GTK_CONTAINER(content), box);

		gtk_widget_show_all(dialog);

		response = gtk_dialog_run(GTK_DIALOG(dialog));

		if (response == GTK_RESPONSE_ACCEPT) {
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio1))) {
				buffer = gtk_entry_get_buffer(GTK_ENTRY(entry1));
				strcpy(namepart, gtk_entry_buffer_get_text(buffer));

				if (namepart[0] != '\0') {
					baseass = base_search_byname(namepart);
				}
				else {
					baseass = NULL;
				}
			}
			else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio2))) {
				buffer = gtk_entry_get_buffer(GTK_ENTRY(entry2));
				strcpy(namepart, gtk_entry_buffer_get_text(buffer));
				buffer = gtk_entry_get_buffer(GTK_ENTRY(entry3));
				strcpy(bottom, gtk_entry_buffer_get_text(buffer));
				buffer = gtk_entry_get_buffer(GTK_ENTRY(entry4));
				strcpy(top, gtk_entry_buffer_get_text(buffer));

				if (namepart[0] != '\0') {
					baseass = base_search_bytype(namepart[0], bottom, top);
				}
				else {
					baseass = NULL;
				}
			}
			show_base_info(baseass, 1);
		}

		free(namepart);
		free(bottom);
		free(top);

		gtk_widget_destroy(dialog);
	}
	else if (data[0] == '3') {
		char * clothname, * soldtime, * cname, * ccom;
		CLO_SELL * sellass;

		clothname = (char *)malloc(31);
		soldtime = (char *)malloc(11);
		cname = (char *)malloc(21);
		ccom = (char *)malloc(11);

		dialog = gtk_dialog_new_with_buttons("查询服饰销售信息", GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_确定", GTK_RESPONSE_ACCEPT, "_取消", GTK_RESPONSE_REJECT, NULL);

		content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

		entry1 = gtk_entry_new();
		entry2 = gtk_entry_new();
		entry3 = gtk_entry_new();
		entry4 = gtk_entry_new();
		label1 = gtk_label_new("服饰名称");
		label2 = gtk_label_new("销售日期");
		label3 = gtk_label_new("客户名称");
		label4 = gtk_label_new("客户评价");
		notice = gtk_label_new("提示: 每个信息不是必须\n填,仅根据您的输入搜索。");

		gtk_grid_set_row_spacing(GTK_GRID(box), 5);

		gtk_grid_attach(GTK_GRID(box), label1, 0, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), label2, 0, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(box), label3, 0, 2, 1, 1);
		gtk_grid_attach(GTK_GRID(box), label4, 0, 3, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry1, 1, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry2, 1, 1, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry3, 1, 2, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry4, 1, 3, 1, 1);
		gtk_grid_attach(GTK_GRID(box), notice, 0, 4, 2, 1);

		gtk_container_add(GTK_CONTAINER(content), box);

		gtk_widget_show_all(dialog);

		response = gtk_dialog_run(GTK_DIALOG(dialog));

		if (response == GTK_RESPONSE_ACCEPT) {
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry1));
			strcpy(clothname, gtk_entry_buffer_get_text(buffer));
			clothname[30] = '\0';
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry2));
			strcpy(soldtime, gtk_entry_buffer_get_text(buffer));
			soldtime[10] = '\0';
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry3));
			strcpy(cname, gtk_entry_buffer_get_text(buffer));
			cname[20] = '\0';
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry4));
			strcpy(ccom, gtk_entry_buffer_get_text(buffer));

			if (clothname[0] == '\0' && soldtime[0] == '\0' && cname[0] == '\0' && ccom[0] == '\0') {
				sellass = NULL;
			}
			else {
				sellass = sell_search(clothname, soldtime, cname, ccom[0]);
			}

			show_sell_info(sellass, 1);
		}

		free(clothname);
		free(soldtime);
		free(cname);
		free(ccom);

		gtk_widget_destroy(dialog);
	}
	else {
		return;
	}
}

//数据统计方面的函数
void data_count (GtkWidget * widget, char * data) {
	CLO_TYPE * tmptype = mainchain;
	CLO_BASE * tmpbase;
	CLO_SELL * tmpsell;
	GtkListStore * list;
	GtkTreeIter iter;
	int i = 0, j, k;
	int * tmparr;
	int tmpint;
	float tmpfloat;
	int flag;

	if (data[0] == '1') {
		DATA_COUNT1 * tmpdata = NULL, * tmpdatahead = NULL;
		float * pricearr;

		while (tmptype != NULL) {
			i++;
			if (tmpdata == NULL) {
				tmpdatahead = tmpdata = (DATA_COUNT1 *)malloc(sizeof(DATA_COUNT1));
			}
			else {
				tmpdata->next = (DATA_COUNT1 *)malloc(sizeof(DATA_COUNT1));
				tmpdata = tmpdata->next;
			}
			tmpdata->sold_total_num = 0;
			tmpdata->total_income = 0;
			tmpdata->comment_low_num = 0;
			tmpdata->comment_high_num = 0;
			tmpdata->next = NULL;

			strcpy(tmpdata->type_name, tmptype->type_name);
			tmpbase = tmptype->base_info;
			while (tmpbase != NULL) {

				tmpdata->sold_total_num += tmpbase->cloth_sold_num;
				tmpdata->total_income += tmpbase->cloth_price * tmpbase->cloth_sold_num;

				tmpsell = tmpbase->sell_info;

				while (tmpsell != NULL) {
					if (tmpsell->consumer_comment < 3) {
						tmpdata->comment_low_num ++;
					}
					else {
						tmpdata->comment_high_num++;
					}

					tmpsell = tmpsell->sell_next;
				}

				tmpbase = tmpbase->base_next;
			}

			tmptype = tmptype->type_next;
		}

		list = liststore_change('1');
		treeview_change('1');

		
		if (tmpdata != NULL) {

			tmpdata = tmpdatahead;
			tmparr = (int *)malloc(sizeof(int) * i);
			pricearr = (float *)malloc(sizeof(float) * i);
			i = 0;

			while (tmpdata != NULL) {
				pricearr[i] = tmpdata->total_income;
				tmparr[i] = i;

				gtk_list_store_append(list, &iter);
				gtk_list_store_set(list, &iter, 0, tmpdata->type_name, 1, tmpdata->sold_total_num, 2, tmpdata->total_income, 3, tmpdata->comment_high_num, 4, tmpdata->comment_low_num, -1);
				tmpdata = tmpdata->next;
				i++;
			}

			for (j = 0; j < i; j++) {
	 			for (k = j; k < i; k++) {
	 				if (pricearr[j] <= pricearr[k]) {
	 					tmpfloat = pricearr[j];
	 					pricearr[j] = pricearr[k];
	 					pricearr[k] = tmpfloat;

	 					tmpint = tmparr[j];
	 					tmparr[j] = tmparr[k];
	 					tmparr[k] = tmpint;
	 				}
	 			}
			}
		}
		gtk_list_store_reorder(list, tmparr);
		gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(list));
		clothInfoChangeType = 0;
	}
	else if (data[0] == '2') {
		GtkWidget * dialog = gtk_dialog_new_with_buttons("请输入需要搜索的年份", GTK_WINDOW(window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_确定", GTK_RESPONSE_ACCEPT, "_取消", GTK_RESPONSE_REJECT, NULL);
		GtkWidget * content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
		GtkWidget * label = gtk_label_new("搜寻年份:");
		GtkWidget * entry = gtk_entry_new();
		GtkWidget * box = gtk_grid_new();
		GtkEntryBuffer * buffer;
		DATA_COUNT2 * tmpdata = NULL, * tmpdatahead = NULL;
		int response;
		char * str = (char *)malloc(20);
		char * tmpstr = (char *)malloc(5);
		int * intarr;

		gtk_grid_set_row_spacing(GTK_GRID(box), 5);
		gtk_grid_attach(GTK_GRID(box), label, 0, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(box), entry, 1, 0, 1, 1);

		gtk_container_add(GTK_CONTAINER(content), box);

		gtk_widget_show_all(dialog);

		response = gtk_dialog_run(GTK_DIALOG(dialog));

		if (response == GTK_RESPONSE_ACCEPT) {
			buffer = gtk_entry_get_buffer(GTK_ENTRY(entry));
			strcpy(str, gtk_entry_buffer_get_text(buffer));
			str[4] = '\0';

			if (strlen(str) == 4) {
				while (tmptype != NULL) {

					tmpbase = tmptype->base_info;

					while (tmpbase != NULL) {
						flag = 1;
						tmpsell = tmpbase->sell_info;

						while (tmpsell != NULL) {
							strncpy(tmpstr, tmpsell->sold_time, 4);
							tmpstr[4] = '\0';
							if (strcmp(tmpstr, str) == 0) {
								if (flag) {
									i++;
									if (tmpdata == NULL) {
										tmpdatahead = tmpdata = (DATA_COUNT2 *)malloc(sizeof(DATA_COUNT2));
									}
									else {
										tmpdata->next = (DATA_COUNT2 *)malloc(sizeof(DATA_COUNT2));
										tmpdata = tmpdata->next;
									}
									flag = 0;
									strcpy(tmpdata->cloth_name, tmpsell->cloth_name);
									strcpy(tmpdata->type_name, tmptype->type_name);
									tmpdata->total_comment = tmpbase->cloth_comment;
									tmpdata->sold_num = 0;
									tmpdata->total_income = 0;
									tmpdata->next = NULL;
								}

								tmpdata->total_income += tmpbase->cloth_price;
								tmpdata->sold_num ++;
							}

							tmpsell = tmpsell->sell_next;
						}
						tmpbase = tmpbase->base_next;
					}
					tmptype = tmptype->type_next;
				}
			}

			list = liststore_change('2');
			treeview_change('2');

			tmparr = (int *)malloc(sizeof(int)*i);
			intarr = (int *)malloc(sizeof(int)*i);

			i=0;

			if (tmpdata != NULL) {
				tmpdata = tmpdatahead;

				while (tmpdata != NULL) {

					tmparr[i] = i;
					intarr[i] = tmpdata->sold_num;

					gtk_list_store_append(list, &iter);

					gtk_list_store_set(list, &iter, 0, tmpdata->cloth_name, 1, tmpdata->type_name, 2, tmpdata->sold_num, 3, tmpdata->total_income, 4, tmpdata->total_comment, -1);
					tmpdata = tmpdata->next;
					i++;
				}

				for (j = 0; j < i; j++) {
		 			for (k = j; k < i-1; k++) {
		 				if (intarr[j] < intarr[k]) {
		 					tmpint = intarr[j];
		 					intarr[j] = intarr[k];
		 					intarr[k] = tmpfloat;

		 					tmpint = tmparr[j];
		 					tmparr[j] = tmparr[k];
		 					tmparr[k] = tmpint;
		 				}
		 			}
				}
			}
			gtk_list_store_reorder(list, tmparr);
			gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(list));
			clothInfoChangeType = 0;
		}

		gtk_widget_destroy(dialog);
	}
	else if (data[0] == '3') {
		DATA_COUNT3 * tmpdata = NULL, * tmpdatahead = NULL, * tmpdataprev = NULL;

		while (tmptype != NULL) {
			tmpbase = tmptype->base_info;

			while (tmpbase != NULL) {
				tmpsell = tmpbase->sell_info;

				while (tmpsell != NULL) {

					if (tmpdata == NULL) {
						tmpdatahead = tmpdata = (DATA_COUNT3 *)malloc(sizeof(DATA_COUNT3));
						tmpdata->next = NULL;
						strcpy(tmpdata->cname, tmpsell->consumer_name);
						tmpdata->total_num = 1;
						tmpdata->total_expense = tmpbase->cloth_price;
						tmpdata->total_comment = tmpsell->consumer_comment;
					}
					else {
						tmpdata = tmpdatahead;
						while(tmpdata != NULL) {

							if (strcmp(tmpdata->cname, tmpsell->consumer_name) == 0) {
								tmpdata->total_expense += tmpbase->cloth_price;
								tmpdata->total_comment = (tmpdata->total_comment*tmpdata->total_num + tmpsell->consumer_comment)/(float)(tmpdata->total_num+1);
								tmpdata->total_num++;
								break;
							}
							tmpdataprev = tmpdata;
							tmpdata = tmpdata->next;
						}

						if (tmpdata == NULL) {
							tmpdata = tmpdataprev;
							tmpdata->next = (DATA_COUNT3 *)malloc(sizeof(DATA_COUNT3));
							tmpdata = tmpdata->next;
							tmpdata->next = NULL;
							strcpy(tmpdata->cname, tmpsell->consumer_name);
							tmpdata->total_num = 1;
							tmpdata->total_expense = tmpbase->cloth_price;
							tmpdata->total_comment = tmpsell->consumer_comment;
						}
					}

					tmpsell = tmpsell->sell_next;
				}

				tmpbase = tmpbase->base_next;
			}

			tmptype = tmptype->type_next;
		}

		list = liststore_change('3');
		treeview_change('3');

		tmpdata = tmpdatahead;

		if (tmpdata != NULL) {
			while(tmpdata != NULL) {
				gtk_list_store_append(list, &iter);
				gtk_list_store_set(list, &iter, 0, tmpdata->cname, 1, tmpdata->total_num, 2, tmpdata->total_expense, 3, tmpdata->total_comment, -1);

				tmpdata = tmpdata->next;
			}
		}

		gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(list));

		clothInfoChangeType = 0;
	}
	else if (data[0] == '4' || data[0] == '5') {
		DATA_COUNT4 * tmpdata = NULL, * tmpdatahead = NULL;
		DATA_COUNT4 * spring, * summer, * autumn, * winter;
		DETAILS * detail1, * detail2, * detail3, * detail4;
		int flag = 1;


		while (tmptype != NULL) {

			if (flag == 1) {
				spring = tmpdatahead = tmpdata = (DATA_COUNT4 *)malloc(sizeof(DATA_COUNT4));
				strcpy(tmpdata->season, "春季");
				detail1 = spring->detail = (DETAILS *)malloc(sizeof(DETAILS));
				summer = tmpdata->next = (DATA_COUNT4 *)malloc(sizeof(DATA_COUNT4));
				tmpdata = tmpdata->next;
				strcpy(tmpdata->season, "夏季");
				detail2 = summer->detail = (DETAILS *)malloc(sizeof(DETAILS));
				autumn = tmpdata->next = (DATA_COUNT4 *)malloc(sizeof(DATA_COUNT4));
				tmpdata = tmpdata->next;
				strcpy(tmpdata->season, "秋季");
				detail3 = autumn->detail = (DETAILS *)malloc(sizeof(DETAILS));
				winter = tmpdata->next = (DATA_COUNT4 *)malloc(sizeof(DATA_COUNT4));
				tmpdata = tmpdata->next;
				strcpy(tmpdata->season, "冬季");
				detail4 = winter->detail = (DETAILS *)malloc(sizeof(DETAILS));
				tmpdata->next = NULL;
				flag = 0;
			}
			else {
				detail1 = detail1->next = (DETAILS *)malloc(sizeof(DETAILS));
				detail2 = detail2->next = (DETAILS *)malloc(sizeof(DETAILS));
				detail3 = detail3->next = (DETAILS *)malloc(sizeof(DETAILS));
				detail4 = detail4->next = (DETAILS *)malloc(sizeof(DETAILS));
			}

			detail1->type_num = tmptype->type_num;
			detail1->total_num = 0;
			detail1->total_income = 0;
			detail1->next = NULL;

			detail2->type_num = tmptype->type_num;
			detail2->total_num = 0;
			detail2->total_income = 0;
			detail2->next = NULL;

			detail3->type_num = tmptype->type_num;
			detail3->total_num = 0;
			detail3->total_income = 0;
			detail3->next = NULL;

			detail4->type_num = tmptype->type_num;
			detail4->total_num = 0;
			detail4->total_income = 0;
			detail4->next = NULL;

			tmptype = tmptype->type_next;
		}

		tmptype = mainchain;

		while (tmptype != NULL) {
			tmpbase = tmptype->base_info;

			while (tmpbase != NULL) {
				tmpsell = tmpbase->sell_info;

				while (tmpsell != NULL) {
					tmpdata = tmpdatahead;

					if (tmpsell->sold_time[4] == '0') {
						switch (tmpsell->sold_time[5]) {
							case '1':
							case '2':
							case '3':
								detail1 = spring->detail;
								while (detail1 != NULL) {
									if (detail1->type_num == tmptype->type_num) {
										detail1->total_num ++;
										detail1->total_income += tmpbase->cloth_price;
									}

									detail1 = detail1->next;
								}
								break;
							case '4':
							case '5':
							case '6':
								detail2 = summer->detail;
								while (detail2 != NULL) {
									if (detail2->type_num == tmptype->type_num) {
										detail2->total_num ++;
										detail2->total_income += tmpbase->cloth_price;
									}

									detail2 = detail2->next;
								}
								break;
							case '7':
							case '8':
							case '9':
								detail3 = autumn->detail;
								while (detail3 != NULL) {
									if (detail3->type_num == tmptype->type_num) {
										detail3->total_num ++;
										detail3->total_income += tmpbase->cloth_price;
									}

									detail3 = detail3->next;
								}
								break;
						}
					}
					else {
						detail4 = winter->detail;
						while (detail4 != NULL) {
							if (detail4->type_num == tmptype->type_num) {
								detail4->total_num ++;
								detail4->total_income += tmpbase->cloth_price;
							}

							detail4 = detail4->next;
						}
					}

					tmpsell = tmpsell->sell_next;
				}

				tmpbase = tmpbase->base_next;
			}
			tmptype = tmptype->type_next;
		}

		treeview_change('4');

		tmpdata = tmpdatahead;

		if (tmpdata != NULL) {

			if (data[0] == '4') {
				list = liststore_change('4');

				while (tmpdata != NULL) {

					detail1 = tmpdata->detail;

					gtk_list_store_append(list, &iter);
					gtk_list_store_set(list, &iter, 0, tmpdata->season, -1);
					flag = 1;

					while (detail1 != NULL) {
						gtk_list_store_set(list, &iter, flag, detail1->total_num, -1);
						flag ++;
						detail1 = detail1->next;
					}

					tmpdata = tmpdata->next;
				}
			}
			else {
				list = liststore_change('5');

				while (tmpdata != NULL) {

					detail1 = tmpdata->detail;

					gtk_list_store_append(list, &iter);
					gtk_list_store_set(list, &iter, 0, tmpdata->season, -1);

					flag = 1;
					
					while (detail1 != NULL) {
						gtk_list_store_set(list, &iter, flag, detail1->total_income, -1);

						flag ++;
						detail1 = detail1->next;
					}

					tmpdata = tmpdata->next;
				}	
			}

			gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(list));
		}

		clothInfoChangeType = 0;

	}
	else if (data[0] == '6') {
		int num = 0;
		float sum = 0;

		while (tmptype != NULL) {

			tmpbase = tmptype->base_info;

			while(tmpbase != NULL) {
				num += tmpbase->cloth_sold_num;
				sum += tmpbase->cloth_sold_num * tmpbase->cloth_price;

				tmpbase = tmpbase->base_next;
			}

			tmptype = tmptype->type_next;
		}

		list = liststore_change('6');
		treeview_change('6');

		g_print("%d\t%f\t\n", num, sum);

		gtk_list_store_append(list, &iter);
		gtk_list_store_set(list, &iter, 0, num, 1, sum, -1);

		gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(list));

		clothInfoChangeType = 0;
	}
}
//下来是一些小的细节
void about (GtkWidget * widget) {

	const char ** author = (const char **)malloc(sizeof (char *)*2);
	char str[] = {"杨凯航"};
	author[0] = str;
	author[1] = NULL;
	g_print("%s\n", author[0]);

	GtkWidget * about = gtk_about_dialog_new();
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about), "服饰销售管理系统");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), "Version: 1.0");
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about), "copyright©Kaihang Yang");
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about), "这个软件是我使用gtk+和c语言编写的。");
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about), "https://github.com/KaiHangYang/cWorks");
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(about), "Github");

	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), author);

	gtk_dialog_run(GTK_DIALOG(about));

	gtk_widget_destroy(about);
}
/*******下来是文件方面的功能**********/
//首先是文件保存功能
void main_save(GtkWidget * widget) {
	char * filename;
	GtkWidget * dialog;
	GtkFileChooser * chooser;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
	int res;

	dialog = gtk_file_chooser_dialog_new("保存文件", GTK_WINDOW(window), action, "_取消", GTK_RESPONSE_REJECT, "_确定", GTK_RESPONSE_ACCEPT, NULL);

	chooser = GTK_FILE_CHOOSER(dialog);

	gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);

	gtk_file_chooser_set_current_name (chooser, "未命名");

	res = gtk_dialog_run(GTK_DIALOG(dialog));

	if(res == GTK_RESPONSE_ACCEPT) {
		char * filename = gtk_file_chooser_get_filename(chooser);

		chain_save(filename);

		g_free(filename);
	}

	gtk_widget_destroy(dialog);
}
//下来是文件打开功能
void main_open(GtkWidget * widget) {
	GtkWidget * dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	int res;

	dialog = gtk_file_chooser_dialog_new("打开文件", GTK_WINDOW(window), action, "_取消", GTK_RESPONSE_ACCEPT, "_确定", GTK_RESPONSE_ACCEPT, NULL);

	res = gtk_dialog_run(GTK_DIALOG(dialog));

	if (res == GTK_RESPONSE_ACCEPT) {
		char * filename;

		GtkFileChooser * chooser = GTK_FILE_CHOOSER(dialog);
		filename = gtk_file_chooser_get_filename(chooser);

		chain_init(filename);

		show_info_all(widget, "t");
		clothInfoChangeType = 1;
		g_free(filename);
	}

	gtk_widget_destroy(dialog);

}
//下来是新的项目
void main_new (GtkWidget * widget) {
	treeview_change('0');
	mainchain = NULL;
}
//下来是缓存功能
void main_quit(GtkWidget * widget) {
	chain_save("database/temp.json~");
	gtk_main_quit();
}
//软件初始化
void init() {
	FILE * file;
	char * str[3];
	file = fopen("database/temp.json~", "rb");

	if (file == NULL) {
		mainchain = NULL;
		fclose(file);
	}
	else {
		GtkWidget * widget;
		fclose(file);
		chain_init("database/temp.json~");
		show_info_all(widget, "t");
	}
}