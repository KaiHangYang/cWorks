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
char * CTOS(char ch) {
	char * a;
	a = (char *)malloc(sizeof(char)*2);
	a[0] = ch;
	a[1] = '\0';
	return a;
}
//辅助函数3 输出全部的内容
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

//type的重复性检测
int cloth_type_check(CLO_TYPE * chain, char typeNum) {
	CLO_TYPE * tmp = chain;
	while (tmp != NULL) {
		if (tmp->type_num == typeNum) {
			return 0;
		}
		tmp = tmp->type_next;
	}
	return 1;
}
//查找目标节点pattern是n或m,n的话是以编号查找,m的话是以名字查找
CLO_TYPE * cloth_type_serach(CLO_TYPE * chain, char pattern, char * content) {
	CLO_TYPE * tmpChain = chain;
	CLO_TYPE * prevChain = chain;
	if (pattern == 'n') {
		if (chain->type_num == content[0]) {
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
		if (strcmp(chain->type_name, content) == 0) {
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
		tmpChain = cloth_type_serach(chain, 'n', str);

		if (tmpChain == NULL) {
			chain = chain->type_next;
		}
		else {
			tmpChain->type_next = tmpChain->type_next->type_next;
		}

	}
	else if (strlen(str) > 1) {
		tmpChain = cloth_type_serach(chain, 'm', str);

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
void cloth_type_info_change(CLO_TYPE * node, CLO_TYPE * mainchain,char pattern, char * content) {

	CLO_TYPE * changeNode;
	if (node == NULL) {
		changeNode = mainchain;
	}
	else {
		changeNode = node->type_next;
	}

	if (pattern == 'n') {
		changeNode->type_num = content[0];

		cloth_type_info_check(changeNode);
	}
	else if (pattern == 'm') {
		strcpy(changeNode->type_name, content);
	}
}
//服装分类信息的录入
void cloth_type_info_input(CLO_TYPE * chain, char typeNum, char * typeName) {

	CLO_TYPE * chainEnd;
	CLO_TYPE * tmp = chain;

	if(cloth_type_check(chain, typeNum)) {
		while (tmp->type_next != NULL) {
			tmp = tmp->type_next;
		}

		chainEnd = tmp;
		chainEnd->type_next = (CLO_TYPE *)malloc(sizeof(CLO_TYPE));
		chainEnd = chainEnd->type_next;

		chainEnd->type_num = typeNum;
		strcpy(chainEnd->type_name, typeName);

		chainEnd->type_next = NULL;
	}
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

	// printf("%d", tmpNode == NULL);

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
void cloth_base_info_change(CLO_BASE * node, CLO_TYPE * chain,char prevtype, char nowtype, char * clothname, char clothsex, float clothprice) {
	CLO_BASE * changeNode, * tmpbase;
	CLO_TYPE * tmp = chain;

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
		tmp = chain;

		while(tmp != NULL) {
			if(tmp->type_num == nowtype) {
				tmpbase = tmp->base_info;
				break;
			}
			else {
				tmp = tmp->type_next;
			}
		}

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

	cloth_base_info_check(changeNode);
}
//数据录入
void cloth_base_info_input(CLO_TYPE * chain, char type, char * clothname, char clothsex, float clothprice) {
	CLO_TYPE * tmp = chain;
	CLO_BASE * tmpbase;

	while (tmp != NULL) {
		if (tmp->type_num == type) {
			tmpbase = tmp->base_info;
			break;
		}
		else {
			tmp = tmp->type_next;
		}
	}

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
void cloth_sell_info_change(CLO_TYPE * chain, char * name, char * prevtime, char * nowtime, char * prevcname, char * nowcname, int prevcom, int nowcom) {

	CLO_TYPE * tmp = chain;
	CLO_BASE * tmpbase = cloth_sell_search(name);
	CLO_SELL * tmpsell = tmpbase->sell_info;

	while (tmpsell != NULL) {
		if (strcmp(tmpsell->cloth_name, name) == 0 && strcmp(tmpsell->sold_time, prevtime) == 0 && strcmp(tmpsell->consumer_name, prevcname) == 0 && tmpsell->consumer_comment == prevcom) {
			strcpy(tmpsell->sold_time, nowtime);
			strcpy(tmpsell->consumer_name, nowcname);
			tmpsell->consumer_comment = nowcom;
		}
		else {
			tmpsell = tmpsell->sell_next;
		}
	}

	cloth_sell_info_check(tmpbase);
}
//销售信息的录入
void cloth_sell_info_input(CLO_TYPE * chain, char * clothname, char * soldtime, char *cname, int ccomment) {
	CLO_BASE * tmpbase = cloth_sell_search(clothname);
	CLO_SELL * tmpsell = tmpbase->sell_info;

	while (tmpsell->sell_next != NULL) {
		tmpsell = tmpsell->sell_next;
	}

	tmpsell = tmpsell->sell_next = (CLO_SELL *)malloc(sizeof(CLO_SELL));
	tmpsell->sell_next = NULL;

	strcpy(tmpsell->cloth_name, clothname);
	strcpy(tmpsell->sold_time, soldtime);
	strcpy(tmpsell->consumer_name, cname);
	tmpsell->consumer_comment = ccomment;

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
	else {
		return gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
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
void cloth_info_change(GtkWidget * widget, ) {

}







int main(int argc, char * argv[]) {
	mainchain = chain_init("db.json"); //JSON文件的编写格式见db.json
	CLO_TYPE * tempchain = mainchain;























	/*gtk widget declare*/
	GtkWidget *window;
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

	
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 500, 280);


	


	/*connect signal*/
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(menuquit), "activate", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(menushowtypeall), "activate", G_CALLBACK(show_info_all), "t");
	g_signal_connect(G_OBJECT(menushowbaseall), "activate", G_CALLBACK(show_info_all), "b");
 	g_signal_connect(G_OBJECT(menushowsellall), "activate", G_CALLBACK(show_info_all), "s");
 	g_signal_connect(G_OBJECT(menudelete), "activate", G_CALLBACK(cloth_info_delete), select);


	g_object_unref(G_OBJECT(builder));

	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}