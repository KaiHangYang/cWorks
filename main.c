#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <gtk/gtk.h>
#include "main.h"


int main(int argc, char * argv[]) {

	/*gtk widget declare*/
	GtkBuilder *builder;
	GtkTreeIter  iter;
	GtkTreeSelection * select;
	/*gkt init*/
	gtk_init(&argc, &argv);

	/*gtk get widget from builder*/
	builder = gtk_builder_new_from_file("interface.xml");
	window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
	treeview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview"));
 	select = gtk_tree_view_get_selection(treeview);
 	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);

 	//add icon
 	gtk_window_set_icon(GTK_WINDOW(window), create_pixbuf("wolf.png"));
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
	GtkWidget * menudata4_1 = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem24"));
	GtkWidget * menudata4_2 = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem25"));
	GtkWidget * menudata6 = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem23"));
	GtkWidget * menuabout = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem13"));
	GtkWidget * menusave = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem3"));
	GtkWidget * menuopen = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem2"));
	GtkWidget * menunew = GTK_WIDGET(gtk_builder_get_object(builder, "imagemenuitem1"));

	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 500, 280);


	


	/*菜单的信号绑定*/
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(main_quit), NULL);
	g_signal_connect(G_OBJECT(menuquit), "activate", G_CALLBACK(main_quit), NULL);
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
 	g_signal_connect(G_OBJECT(menudata4_1), "activate", G_CALLBACK(data_count), "4");
 	g_signal_connect(G_OBJECT(menudata4_2), "activate", G_CALLBACK(data_count), "5");
 	g_signal_connect(G_OBJECT(menudata6), "activate", G_CALLBACK(data_count), "6");
 	g_signal_connect(G_OBJECT(menuabout), "activate", G_CALLBACK(about), NULL);
 	g_signal_connect(G_OBJECT(menusave), "activate", G_CALLBACK(main_save), NULL);
 	g_signal_connect(G_OBJECT(menuopen), "activate", G_CALLBACK(main_open), NULL);
 	g_signal_connect(G_OBJECT(menunew), "activate", G_CALLBACK(main_new), NULL);

	g_object_unref(G_OBJECT(builder));

	gtk_widget_show_all(window);

	init();

	gtk_main();

	return 0;
}