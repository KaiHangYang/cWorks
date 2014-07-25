#include <gtk/gtk.h>
enum {
	COLUMN_STRING,
	COLUMN_INT,
	COLUMN_INT2,
	COLUMN_N
};



int main(int argc, char *argv[]) {

	/*gtk widget declare*/
	GtkWidget *window;
	GtkBuilder *builder;
	GtkTreeView * treeview;
	GtkListStore *list, *list1;
	GtkTreeIter  iter;
	/*gkt init*/
	gtk_init(&argc, &argv);
	/*gtk get widget from builder*/
	builder = gtk_builder_new_from_file("interface.glade");
	window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
	treeview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview"));
	list = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore"));
	list1 = gtk_list_store_new(COLUMN_N, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);


	// gtk_list_store_append(list1, &iter);
	// gtk_list_store_set(list1, &iter, COLUMN_STRING, "asdasd", COLUMN_INT, -1);
	// gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(list1));

	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 500, 280);
	/*connect signal*/
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_object_unref(G_OBJECT(builder));

	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}