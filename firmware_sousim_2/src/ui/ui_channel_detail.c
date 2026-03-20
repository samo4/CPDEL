#include "ui.h"
#include <stdio.h>

void ui_create_channel_detail_screen(void) {
    ui_ChannelDetailScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_ChannelDetailScreen, LV_OBJ_FLAG_SCROLLABLE);

    // Title Bar
    lv_obj_t * title = lv_label_create(ui_ChannelDetailScreen);
    lv_label_set_text(title, "Channel Detail");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t * back_btn = lv_btn_create(ui_ChannelDetailScreen);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_event_cb(back_btn, ui_event_navigate_back, LV_EVENT_CLICKED, NULL);
    lv_obj_t * back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_lbl);

    // Graph Button
    lv_obj_t * graph_btn = lv_btn_create(ui_ChannelDetailScreen);
    lv_obj_set_size(graph_btn, 80, 40);
    lv_obj_align(graph_btn, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_add_event_cb(graph_btn, ui_event_navigate_graph, LV_EVENT_CLICKED, NULL);
    lv_obj_t * graph_lbl = lv_label_create(graph_btn);
    lv_label_set_text(graph_lbl, "Graph");
    lv_obj_center(graph_lbl);

    // Main Content Area (Grid layout)
    lv_obj_t * cont = lv_obj_create(ui_ChannelDetailScreen);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(75));
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Layout configuration:
    // Row 1: Voltage Setpoint (Spinbox or Arc)
    // Row 2: Current Setpoint
    // Row 3: LV Cutoff enable + threshold
    // Row 4: Power reading (large)

    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);

    // Voltage Setpoint
    lv_obj_t * v_cont = lv_obj_create(cont);
    lv_obj_set_grid_cell(v_cont, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_t * v_label = lv_label_create(v_cont);
    lv_label_set_text(v_label, "Set Voltage (V)");
    lv_obj_align(v_label, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_t * v_spinbox = lv_spinbox_create(v_cont);
    lv_spinbox_set_digit_format(v_spinbox, 5, 2);
    lv_spinbox_set_range(v_spinbox, 0, 3000); // 0-30.00V
    lv_obj_set_width(v_spinbox, 80);
    lv_obj_center(v_spinbox);
    lv_spinbox_set_value(v_spinbox, 1200); // 12.00V

    // Current Setpoint
    lv_obj_t * c_cont = lv_obj_create(cont);
    lv_obj_set_grid_cell(c_cont, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_t * c_label = lv_label_create(c_cont);
    lv_label_set_text(c_label, "Set Current (A)");
    lv_obj_align(c_label, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_t * c_spinbox = lv_spinbox_create(c_cont);
    lv_spinbox_set_digit_format(c_spinbox, 5, 3);
    lv_spinbox_set_range(c_spinbox, 0, 5000); // 0-5.000A
    lv_obj_set_width(c_spinbox, 80);
    lv_obj_center(c_spinbox);
    lv_spinbox_set_value(c_spinbox, 1500); // 1.500A

    // LV Cutoff
    lv_obj_t * lv_cont = lv_obj_create(cont);
    lv_obj_set_grid_cell(lv_cont, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_t * lv_label_t = lv_label_create(lv_cont);
    lv_label_set_text(lv_label_t, "LV Cutoff");
    lv_obj_align(lv_label_t, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lv_obj_t * lv_sw = lv_switch_create(lv_cont);
    lv_obj_set_size(lv_sw, 40, 20);
    lv_obj_align(lv_sw, LV_ALIGN_TOP_RIGHT, 0, 0);

    lv_obj_t * lv_spinbox = lv_spinbox_create(lv_cont);
    lv_spinbox_set_digit_format(lv_spinbox, 4, 2);
    lv_obj_set_width(lv_spinbox, 70);
    lv_obj_align(lv_spinbox, LV_ALIGN_BOTTOM_MID, 0, -5);

    // Readings Area
    lv_obj_t * read_cont = lv_obj_create(cont);
    lv_obj_set_grid_cell(read_cont, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 2); // Span 2 rows
    
    lv_obj_t * read_label = lv_label_create(read_cont);
    lv_label_set_text(read_label, "Live Readings");
    lv_obj_align(read_label, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t * v_read = lv_label_create(read_cont);
    lv_label_set_text(v_read, "12.00 V");
    lv_obj_set_style_text_font(v_read, &lv_font_montserrat_24, 0);
    lv_obj_align(v_read, LV_ALIGN_CENTER, 0, -20);
    
    lv_obj_t * c_read = lv_label_create(read_cont);
    lv_label_set_text(c_read, "1.500 A");
    lv_obj_set_style_text_font(c_read, &lv_font_montserrat_24, 0);
    lv_obj_align(c_read, LV_ALIGN_CENTER, 0, 20);
    
    // Output Toggle (Big)
    lv_obj_t * out_btn = lv_btn_create(cont);
    lv_obj_set_grid_cell(out_btn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
    lv_obj_t * out_lbl = lv_label_create(out_btn);
    lv_label_set_text(out_lbl, "OUTPUT ON");
    lv_obj_center(out_lbl);
    lv_obj_set_style_bg_color(out_btn, lv_palette_main(LV_PALETTE_GREEN), LV_STATE_CHECKED);
    lv_obj_add_flag(out_btn, LV_OBJ_FLAG_CHECKABLE);

}
