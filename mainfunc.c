#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <gtk/gtk.h>
#include "cJSON.h"

//  服装销售基本信息
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
/**************必要的全局变量***************/

//首先是用来标记需要改变信息类型 其中 1表示更改服饰类型信息 2表示修改衣服基本信息 3表示修改衣服销售基本信息 0表示不能进行修改操作
int clothInfoChangeType = 0;
CLO_TYPE * mainchain;
GtkTreeView * treeview;
GtkWidget * window;
/***************系统的主要功能***************/

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
CLO_TYPE * chain_init(char * file_path) {

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
	return chainhead;
}
//这里是将链表输出为一个文件
int chain_save(CLO_TYPE * chain) {

	CLO_TYPE * tmptype = chain;
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

	file = fopen("db.json", "wb");

	int a = fputs(out, file);

	fclose(file);
	cJSON_Delete(root);

	return a;
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

//这里的pattern t表示类型信息 b表示服装基本信息 s表示销售基本信息
void treeview_change (char pattern) {
	GtkListStore * list;
	GtkTreeViewColumn * column;
	GtkCellRenderer * cellrenderer = gtk_cell_renderer_text_new();
	int n, i;

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
	else if (pattern == '4') {
		column = gtk_tree_view_column_new_with_attributes("季节", cellrenderer, "text", 0, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("销售总件数", cellrenderer, "text", 1, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("销售总额", cellrenderer, "text", 2, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("评价>=3件数", cellrenderer, "text", 3, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("评价<3件数", cellrenderer, "text", 4, NULL);
		gtk_tree_view_append_column(treeview, column);
		column = gtk_tree_view_column_new_with_attributes("评价<3件数", cellrenderer, "text", 5, NULL);
		gtk_tree_view_append_column(treeview, column);
	}
	else if (pattern == '5') {

	}
}

//下来是 liststore 的数据的填充
GtkListStore * liststore_change(char  pattern) {
	//先创建liststore	
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


//GtkTreeView * treeview, CLO_TYPE * chain, char pattern
/*下来是显示所有的信息*/
   /*先是用于使用gtk_signal_connect的结构*/
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
//下来是开始数据统计
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

// typedef struct data4 {	

// } DATA_COUNT4;

// typedef struct data5 {

// } DATA_COUNT5;

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
	else if (data[0] == '4') {

	}
	else if (data[0] == '5') {

	}
}




int main(int argc, char * argv[]) {
	mainchain = chain_init("db.json"); //JSON文件的编写格式见db.json
	CLO_TYPE * tempchain = mainchain;


















	/*gtk widget declare*/
	GtkBuilder *builder;
	GtkTreeIter  iter;
	GtkTreeSelection * select;
	/*gkt init*/
	gtk_init(&argc, &argv);

	/*gtk get widget from builder*/
	builder = gtk_builder_new_from_file("interface.glade");
	window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
	treeview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview"));
 	select = gtk_tree_view_get_selection(treeview);
 	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
	//For menu
	GtkWidget * menuquit = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem5"));
	GtkWidget * menushowtypeall = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem4"));
	GtkWidget * menushowbaseall = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem11"));
	GtkWidget * menushowsellall = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem12"));
	GtkWidget * menudelete = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem6"));
	GtkWidget * menuchange = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem7"));
	GtkWidget * menuinput1 = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem16"));
	GtkWidget * menuinput2 = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem17"));
	GtkWidget * menuinput3 = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem18"));
	GtkWidget * menusearch1 = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem9"));
	GtkWidget * menusearch2 = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem14"));
	GtkWidget * menusearch3 = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem15"));
	GtkWidget * menudata1 = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem19"));
	GtkWidget * menudata2 = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem20"));
	GtkWidget * menudata3 = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem21"));


	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 500, 280);


	


	/*connect signal*/
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(menuquit), "activate", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(menushowtypeall), "activate", G_CALLBACK(show_info_all), "t");
	g_signal_connect(G_OBJECT(menushowbaseall), "activate", G_CALLBACK(show_info_all), "b");
 	g_signal_connect(G_OBJECT(menushowsellall), "activate", G_CALLBACK(show_info_all), "s");
 	g_signal_connect(G_OBJECT(menudelete), "activate", G_CALLBACK(cloth_info_delete), select);
 	g_signal_connect(G_OBJECT(menuchange), "activate", G_CALLBACK(cloth_info_change), select);
 	g_signal_connect(G_OBJECT(menuinput1), "activate", G_CALLBACK(cloth_info_input), "1");
 	g_signal_connect(G_OBJECT(menuinput2), "activate", G_CALLBACK(cloth_info_input), "2");
 	g_signal_connect(G_OBJECT(menuinput3), "activate", G_CALLBACK(cloth_info_input), "3");
 	g_signal_connect(G_OBJECT(menusearch1), "activate", G_CALLBACK(cloth_info_search), "1");
 	g_signal_connect(G_OBJECT(menusearch2), "activate", G_CALLBACK(cloth_info_search), "2");
 	g_signal_connect(G_OBJECT(menusearch3), "activate", G_CALLBACK(cloth_info_search), "3");
 	g_signal_connect(G_OBJECT(menudata1), "activate", G_CALLBACK(data_count), "1");
 	g_signal_connect(G_OBJECT(menudata2), "activate", G_CALLBACK(data_count), "2");
 	g_signal_connect(G_OBJECT(menudata3), "activate", G_CALLBACK(data_count), "3");

	g_object_unref(G_OBJECT(builder));

	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}