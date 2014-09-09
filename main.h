/********我的三向十字链表***********/
typedef struct cloth_sell_info {
	//服饰名称
	char cloth_name[30];
	//销售日期
	char sold_time[10];
	//客户名称
	char consumer_name[20];
	//客户评价
	int consumer_comment;

	struct cloth_sell_info * sell_next;
} CLO_SELL;

//  服装基本信息的结构类型
typedef struct cloth_base_info {
	//分类码
	char cloth_type; //是和type number相关的
	//服饰名称
	char cloth_name[30];
	//式样
	char cloth_sex; // 0表示男 1表示女 9表示中性
	//单价
	float cloth_price; // 单价
	//售出件数
	int cloth_sold_num; // 不用手动输入
	//评价指数
	float cloth_comment; // 不用手动输入

	struct cloth_base_info * base_next;

	struct cloth_sell_info * sell_info;
} CLO_BASE;

// 服装种类的结构类型 1~5
typedef struct cloth_type {
	//分类编码
	char type_num; 
	//分类名称
	char type_name[10]; //分类名称

	struct cloth_type * type_next;
	struct cloth_base_info * base_info;

} CLO_TYPE;

//数据统计辅助数据结构
typedef struct data1 {
	char type_name[10];
	int sold_total_num;
	float total_income;
	int comment_high_num;
	int comment_low_num;

	struct data1 * next;
} DATA_COUNT1;

typedef struct data2 {
	char cloth_name[30];
	char type_name[10];
	int sold_num;
	float total_income;
	float total_comment;

	struct data2 * next;
} DATA_COUNT2;

typedef struct data3 {
	char cname[20];
	int total_num;
	float total_expense;
	float total_comment;

	struct data3 * next;
} DATA_COUNT3;

typedef struct detail {
	char type_num;
	int total_num;
	float total_income;

	struct detail * next;
} DETAILS;

typedef struct data4 {	
	char season[10];
	DETAILS * detail;
	struct data4 * next;
} DATA_COUNT4;
typedef struct data6 {
	int total_num;
	float total_income;
} DATA_COUNT6;


/*cJSON 的头文件*/
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

#ifndef cJSON__h
#define cJSON__h

#ifdef __cplusplus
extern "C"
{
#endif

/* cJSON Types: */
#define cJSON_False 0
#define cJSON_True 1
#define cJSON_NULL 2
#define cJSON_Number 3
#define cJSON_String 4
#define cJSON_Array 5
#define cJSON_Object 6
	
#define cJSON_IsReference 256

/* The cJSON structure: */
typedef struct cJSON {
	struct cJSON *next,*prev;	/* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
	struct cJSON *child;		/* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */

	int type;					/* The type of the item, as above. */

	char *valuestring;			/* The item's string, if type==cJSON_String */
	int valueint;				/* The item's number, if type==cJSON_Number */
	double valuedouble;			/* The item's number, if type==cJSON_Number */

	char *string;				/* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
} cJSON;

typedef struct cJSON_Hooks {
      void *(*malloc_fn)(size_t sz);
      void (*free_fn)(void *ptr);
} cJSON_Hooks;

/* Supply malloc, realloc and free functions to cJSON */
extern void cJSON_InitHooks(cJSON_Hooks* hooks);


/* Supply a block of JSON, and this returns a cJSON object you can interrogate. Call cJSON_Delete when finished. */
extern cJSON *cJSON_Parse(const char *value);
/* Render a cJSON entity to text for transfer/storage. Free the char* when finished. */
extern char  *cJSON_Print(cJSON *item);
/* Render a cJSON entity to text for transfer/storage without any formatting. Free the char* when finished. */
extern char  *cJSON_PrintUnformatted(cJSON *item);
/* Delete a cJSON entity and all subentities. */
extern void   cJSON_Delete(cJSON *c);

/* Returns the number of items in an array (or object). */
extern int	  cJSON_GetArraySize(cJSON *array);
/* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
extern cJSON *cJSON_GetArrayItem(cJSON *array,int item);
/* Get item "string" from object. Case insensitive. */
extern cJSON *cJSON_GetObjectItem(cJSON *object,char *string);

/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when cJSON_Parse() returns 0. 0 when cJSON_Parse() succeeds. */
extern const char *cJSON_GetErrorPtr(void);
	
/* These calls create a cJSON item of the appropriate type. */
extern cJSON *cJSON_CreateNull(void);
extern cJSON *cJSON_CreateTrue(void);
extern cJSON *cJSON_CreateFalse(void);
extern cJSON *cJSON_CreateBool(int b);
extern cJSON *cJSON_CreateNumber(double num);
extern cJSON *cJSON_CreateString(char *string);
extern cJSON *cJSON_CreateArray(void);
extern cJSON *cJSON_CreateObject(void);

/* These utilities create an Array of count items. */
extern cJSON *cJSON_CreateIntArray(const int *numbers,int count);
extern cJSON *cJSON_CreateFloatArray(const float *numbers,int count);
extern cJSON *cJSON_CreateDoubleArray(const double *numbers,int count);
extern cJSON *cJSON_CreateStringArray(char **strings,int count);

/* Append item to the specified array/object. */
extern void cJSON_AddItemToArray(cJSON *array, cJSON *item);
extern void	cJSON_AddItemToObject(cJSON *object,const char *string,cJSON *item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing cJSON to a new cJSON, but don't want to corrupt your existing cJSON. */
extern void cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item);
extern void	cJSON_AddItemReferenceToObject(cJSON *object,const char *string,cJSON *item);

/* Remove/Detatch items from Arrays/Objects. */
extern cJSON *cJSON_DetachItemFromArray(cJSON *array,int which);
extern void   cJSON_DeleteItemFromArray(cJSON *array,int which);
extern cJSON *cJSON_DetachItemFromObject(cJSON *object,const char *string);
extern void   cJSON_DeleteItemFromObject(cJSON *object,const char *string);
	
/* Update array items. */
extern void cJSON_ReplaceItemInArray(cJSON *array,int which,cJSON *newitem);
extern void cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem);

/* Duplicate a cJSON item */
extern cJSON *cJSON_Duplicate(cJSON *item,int recurse);
/* Duplicate will create a new, identical cJSON item to the one you pass, in new memory that will
need to be released. With recurse!=0, it will duplicate any children connected to the item.
The item->next and ->prev pointers are always zero on return from Duplicate. */

/* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
extern cJSON *cJSON_ParseWithOpts(const char *value,const char **return_parse_end,int require_null_terminated);

extern void cJSON_Minify(char *json);

/* Macros for creating things quickly. */
#define cJSON_AddNullToObject(object,name)		cJSON_AddItemToObject(object, name, cJSON_CreateNull())
#define cJSON_AddTrueToObject(object,name)		cJSON_AddItemToObject(object, name, cJSON_CreateTrue())
#define cJSON_AddFalseToObject(object,name)		cJSON_AddItemToObject(object, name, cJSON_CreateFalse())
#define cJSON_AddBoolToObject(object,name,b)	cJSON_AddItemToObject(object, name, cJSON_CreateBool(b))
#define cJSON_AddNumberToObject(object,name,n)	cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n))
#define cJSON_AddStringToObject(object,name,s)	cJSON_AddItemToObject(object, name, cJSON_CreateString(s))

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define cJSON_SetIntValue(object,val)			((object)?(object)->valueint=(object)->valuedouble=(val):(val))

#ifdef __cplusplus
}
#endif

#endif
/*********我的函数头文件*********/
extern int clothInfoChangeType;
extern CLO_TYPE * mainchain;
extern GtkTreeView * treeview;
extern GtkWidget * window;
/*******************下来是我的函数***************/

//使用cJSON解析json文件 filename 填文件路径
extern cJSON * json_dofile(char * filename);
//将整数转化为字符串的函数
extern char * ITOA(int a);
//将字符转化为字符串的函数
extern char * CTOS(char ch);
//将浮点数字符串转化为浮点数的函数
extern float STOF(char * str);
//因为我没有用IDE只用了gcc编译器所以使用这个函数来做前期测试 参数type是cloth_type的num
extern void outPutSome(char type);
//文件中初始化链表的函数 file_path是文件的路径
extern void chain_init(char * file_path);
//链表保存到文件的函数
extern void chain_save(char * filename);
//进行服饰类型码的校对
extern void cloth_type_info_check(CLO_TYPE * tmp);
//用于检测cloth_type是否重复的函数 pattern的值'n'表示num 'm'表示name
extern int cloth_type_check(char pattern, char * content);
//查找目标节点pattern是n或m,n的话是以编号查找,m的话是以名字查找
extern CLO_TYPE * cloth_type_search(char pattern, char * content);
//服装类型  删除功能这里的str可以是一个字符（typenum）或是多个字符（typename）
extern void cloth_type_info_delete(char * str);
//服装分类数据更改 pattern 表示更改的数据类型 n表示标号 m表示名字 content, 这里的node是由上面的search函数获得的
extern void cloth_type_info_change(CLO_TYPE * node, char pattern, char * content);
//服装分类信息的录入
extern int cloth_type_info_input(char * typeNum, char * typeName);
//服饰基本信息校对函数
extern void cloth_base_info_check(CLO_BASE * tmp);
//搜索目标和前面一样  content为搜索衣服的名字
extern CLO_BASE * cloth_base_search(char type, char * clothname);
//数据删除
extern void cloth_base_info_delete(char type, char * clothname);
//数据更改
extern void cloth_base_info_change(CLO_BASE * node, char prevtype, char nowtype, char * clothname, char clothsex, float clothprice);
//数据录入
extern void cloth_base_info_input(char type, char * clothname, char clothsex, float clothprice);
//销售信息校对
extern void cloth_sell_info_check(CLO_BASE * tmpbase);
//辅助查找
extern CLO_BASE * cloth_sell_search(char * clothname);
//数据的删除 这里因为购买数据可能重复，所以删除的话需要很多数据
extern void cloth_sell_info_delete(char * clothname, char * soldtime, char * consumername, int consumercomment);
//信息更改 按照逻辑来说是不能改名字的 除了原来的衣服更改名字
extern void cloth_sell_info_change(char * name, char * prevtime, char * nowtime, char * prevcname, char * nowcname, int prevcom, int nowcom);
//销售信息的录入
extern void cloth_sell_info_input(char * clothname, char * soldtime, char *cname, int ccomment);
//这里的pattern t表示类型信息 b表示服装基本信息 s表示销售基本信息 1~6分别是数据统计相关
extern void treeview_change (char pattern);
// liststore 的数据的填充
extern GtkListStore * liststore_change(char  pattern);
//下来是要处理显示数据的函数了 fresh表示是否刷新liststore, >0 刷新 , <0不
extern void show_type_info(CLO_TYPE * chain, int fresh);
extern void show_base_info(CLO_BASE * chain, int fresh);
extern void show_sell_info(CLO_SELL * chain, int fresh);
/*下来是显示所有的信息*/
extern void show_info_all(GtkWidget * widget, char * pattern);
//信息的删除功能
extern void cloth_info_delete(GtkWidget * widget, GtkTreeSelection * selection);
//信息的更改功能
extern void cloth_info_change(GtkWidget * widget, GtkTreeSelection * selection);
//信息的填充功能
extern void cloth_info_input(GtkWidget * widget, char * data);
//信息的查询功能

//查询cloth_type 根据类型码
extern CLO_TYPE * type_search(char typenum);
//查询服饰基本信息 跟据服饰名字
extern CLO_BASE * base_search_byname(char * name);
//根据服饰类型和价格区间查询
extern CLO_BASE * base_search_bytype(char type, char * bottom, char * top);
//查询服饰销售信息根据下面任意一个或多个信息
extern CLO_SELL * sell_search(char * clothname, char * soldtime, char * cname, char ccom);
//用于gtk信号绑定的查询函数
extern void cloth_info_search(GtkWidget * widget, char * data);
//数据统计方面的函数 data是从1～6分别代表不同的统计
extern void data_count (GtkWidget * widget, char * data);
//下来是一些小的细节

//关于 菜单
extern void about (GtkWidget * widget);
//保存文件 菜单
extern void main_save(GtkWidget * widget);
//打开文件 菜单
extern void main_open(GtkWidget * widget);
//新项目 菜单
extern void main_new (GtkWidget * widget);
//退出缓存功能
extern void main_quit(GtkWidget * widget);
//软件的初始化
extern void init();
//添加图标
extern GdkPixbuf *create_pixbuf(const gchar* filename);
//结束