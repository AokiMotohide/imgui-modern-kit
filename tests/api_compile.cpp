#include <imkit/imkit.h>
namespace reference {
ImGuiStyle &overload_6();
const char *overload_20();
bool overload_24(const char *name, bool *p_open = NULL, ImGuiWindowFlags flags = 0);
void overload_25();
bool overload_26(const char *str_id, const ImVec2 &size = ImVec2(0, 0), ImGuiChildFlags child_flags = 0,
                 ImGuiWindowFlags window_flags = 0);
bool overload_27(ImGuiID id, const ImVec2 &size = ImVec2(0, 0), ImGuiChildFlags child_flags = 0,
                 ImGuiWindowFlags window_flags = 0);
void overload_28();
bool overload_29();
bool overload_30();
bool overload_31(ImGuiFocusedFlags flags = 0);
bool overload_32(ImGuiHoveredFlags flags = 0);
ImDrawList *overload_33();
float overload_34();
ImVec2 overload_35();
ImVec2 overload_36();
float overload_37();
float overload_38();
ImGuiViewport *overload_39();
void overload_40(const ImVec2 &pos, ImGuiCond cond = 0, const ImVec2 &pivot = ImVec2(0, 0));
void overload_41(const ImVec2 &size, ImGuiCond cond = 0);
void overload_42(const ImVec2 &size_min, const ImVec2 &size_max, ImGuiSizeCallback custom_callback = NULL,
                 void *custom_callback_data = NULL);
void overload_43(const ImVec2 &size);
void overload_44(bool collapsed, ImGuiCond cond = 0);
void overload_45();
void overload_46(const ImVec2 &scroll);
void overload_47(float alpha);
void overload_48(ImGuiID viewport_id);
void overload_49(const ImVec2 &pos, ImGuiCond cond = 0);
void overload_50(const ImVec2 &size, ImGuiCond cond = 0);
void overload_51(bool collapsed, ImGuiCond cond = 0);
void overload_52();
void overload_53(const char *name, const ImVec2 &pos, ImGuiCond cond = 0);
void overload_54(const char *name, const ImVec2 &size, ImGuiCond cond = 0);
void overload_55(const char *name, bool collapsed, ImGuiCond cond = 0);
void overload_56(const char *name);
float overload_57();
float overload_58();
void overload_59(float scroll_x);
void overload_60(float scroll_y);
float overload_61();
float overload_62();
void overload_63(float center_x_ratio = 0.5f);
void overload_64(float center_y_ratio = 0.5f);
void overload_65(float local_x, float center_x_ratio = 0.5f);
void overload_66(float local_y, float center_y_ratio = 0.5f);
void overload_67(ImFont *font, float font_size_base_unscaled);
void overload_68();
ImFont *overload_69();
float overload_70();
ImFontBaked *overload_71();
void overload_72(ImGuiCol idx, ImU32 col);
void overload_73(ImGuiCol idx, const ImVec4 &col);
void overload_74(int count = 1);
void overload_75(ImGuiStyleVar idx, float val);
void overload_76(ImGuiStyleVar idx, const ImVec2 &val);
void overload_77(ImGuiStyleVar idx, float val_x);
void overload_78(ImGuiStyleVar idx, float val_y);
void overload_79(int count = 1);
void overload_80(ImGuiItemFlags option, bool enabled);
void overload_81();
void overload_82(float item_width);
void overload_83();
void overload_84(float item_width);
float overload_85();
void overload_86(float wrap_local_pos_x = 0.0f);
void overload_87();
ImVec2 overload_88();
ImU32 overload_89(ImGuiCol idx, float alpha_mul = 1.0f);
ImU32 overload_90(const ImVec4 &col);
ImU32 overload_91(ImU32 col, float alpha_mul = 1.0f);
const ImVec4 &overload_92(ImGuiCol idx);
ImVec2 overload_93();
void overload_94(const ImVec2 &pos);
ImVec2 overload_95();
ImVec2 overload_96();
float overload_97();
float overload_98();
void overload_99(const ImVec2 &local_pos);
void overload_100(float local_x);
void overload_101(float local_y);
ImVec2 overload_102();
void overload_103();
void overload_104(float offset_from_start_x = 0.0f, float spacing = -1.0f);
void overload_105();
void overload_106();
void overload_107(const ImVec2 &size);
void overload_108(float indent_w = 0.0f);
void overload_109(float indent_w = 0.0f);
void overload_110();
void overload_111();
void overload_112();
float overload_113();
float overload_114();
float overload_115();
float overload_116();
void overload_117(const char *str_id);
void overload_118(const char *str_id_begin, const char *str_id_end);
void overload_119(const void *ptr_id);
void overload_120(int int_id);
void overload_121();
ImGuiID overload_122(const char *str_id);
ImGuiID overload_123(const char *str_id_begin, const char *str_id_end);
ImGuiID overload_124(const void *ptr_id);
ImGuiID overload_125(int int_id);
void overload_126(const char *text, const char *text_end = NULL);
void overload_127(const char *fmt, ...);
void overload_128(const char *fmt, va_list args);
void overload_129(const ImVec4 &col, const char *fmt, ...);
void overload_130(const ImVec4 &col, const char *fmt, va_list args);
void overload_131(const char *fmt, ...);
void overload_132(const char *fmt, va_list args);
void overload_133(const char *fmt, ...);
void overload_134(const char *fmt, va_list args);
void overload_135(const char *label, const char *fmt, ...);
void overload_136(const char *label, const char *fmt, va_list args);
void overload_137(const char *fmt, ...);
void overload_138(const char *fmt, va_list args);
void overload_139(const char *label);
bool overload_140(const char *label, const ImVec2 &size = ImVec2(0, 0));
bool overload_141(const char *label);
bool overload_142(const char *str_id, const ImVec2 &size, ImGuiButtonFlags flags = 0);
bool overload_143(const char *str_id, ImGuiDir dir);
bool overload_144(const char *label, bool *v);
bool overload_145(const char *label, int *flags, int flags_value);
bool overload_146(const char *label, unsigned int *flags, unsigned int flags_value);
bool overload_147(const char *label, bool active);
bool overload_148(const char *label, int *v, int v_button);
void overload_149(float fraction, const ImVec2 &size_arg = ImVec2(-FLT_MIN, 0), const char *overlay = NULL);
void overload_150();
bool overload_151(const char *label);
bool overload_152(const char *label, const char *url = NULL);
void overload_153(ImTextureRef tex_ref, const ImVec2 &image_size, const ImVec2 &uv0 = ImVec2(0, 0),
                  const ImVec2 &uv1 = ImVec2(1, 1));
void overload_154(ImTextureRef tex_ref, const ImVec2 &image_size, const ImVec2 &uv0 = ImVec2(0, 0),
                  const ImVec2 &uv1 = ImVec2(1, 1), const ImVec4 &bg_col = ImVec4(0, 0, 0, 0),
                  const ImVec4 &tint_col = ImVec4(1, 1, 1, 1));
bool overload_155(const char *str_id, ImTextureRef tex_ref, const ImVec2 &image_size,
                  const ImVec2 &uv0 = ImVec2(0, 0), const ImVec2 &uv1 = ImVec2(1, 1),
                  const ImVec4 &bg_col = ImVec4(0, 0, 0, 0), const ImVec4 &tint_col = ImVec4(1, 1, 1, 1));
bool overload_156(const char *label, const char *preview_value, ImGuiComboFlags flags = 0);
void overload_157();
bool overload_158(const char *label, int *current_item, const char *const items[], int items_count,
                  int popup_max_height_in_items = -1);
bool overload_159(const char *label, int *current_item, const char *items_separated_by_zeros,
                  int popup_max_height_in_items = -1);
bool overload_160(const char *label, int *current_item, const char *(*getter)(void *user_data, int idx),
                  void *user_data, int items_count, int popup_max_height_in_items = -1);
bool overload_161(const char *label, float *v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f,
                  const char *format = "%.3f", ImGuiSliderFlags flags = 0);
bool overload_162(const char *label, float v[2], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f,
                  const char *format = "%.3f", ImGuiSliderFlags flags = 0);
bool overload_163(const char *label, float v[3], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f,
                  const char *format = "%.3f", ImGuiSliderFlags flags = 0);
bool overload_164(const char *label, float v[4], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f,
                  const char *format = "%.3f", ImGuiSliderFlags flags = 0);
bool overload_165(const char *label, float *v_current_min, float *v_current_max, float v_speed = 1.0f,
                  float v_min = 0.0f, float v_max = 0.0f, const char *format = "%.3f",
                  const char *format_max = NULL, ImGuiSliderFlags flags = 0);
bool overload_166(const char *label, int *v, float v_speed = 1.0f, int v_min = 0, int v_max = 0,
                  const char *format = "%d", ImGuiSliderFlags flags = 0);
bool overload_167(const char *label, int v[2], float v_speed = 1.0f, int v_min = 0, int v_max = 0,
                  const char *format = "%d", ImGuiSliderFlags flags = 0);
bool overload_168(const char *label, int v[3], float v_speed = 1.0f, int v_min = 0, int v_max = 0,
                  const char *format = "%d", ImGuiSliderFlags flags = 0);
bool overload_169(const char *label, int v[4], float v_speed = 1.0f, int v_min = 0, int v_max = 0,
                  const char *format = "%d", ImGuiSliderFlags flags = 0);
bool overload_170(const char *label, int *v_current_min, int *v_current_max, float v_speed = 1.0f,
                  int v_min = 0, int v_max = 0, const char *format = "%d", const char *format_max = NULL,
                  ImGuiSliderFlags flags = 0);
bool overload_171(const char *label, ImGuiDataType data_type, void *p_data, float v_speed = 1.0f,
                  const void *p_min = NULL, const void *p_max = NULL, const char *format = NULL,
                  ImGuiSliderFlags flags = 0);
bool overload_172(const char *label, ImGuiDataType data_type, void *p_data, int components,
                  float v_speed = 1.0f, const void *p_min = NULL, const void *p_max = NULL,
                  const char *format = NULL, ImGuiSliderFlags flags = 0);
bool overload_173(const char *label, float *v, float v_min, float v_max, const char *format = "%.3f",
                  ImGuiSliderFlags flags = 0);
bool overload_174(const char *label, float v[2], float v_min, float v_max, const char *format = "%.3f",
                  ImGuiSliderFlags flags = 0);
bool overload_175(const char *label, float v[3], float v_min, float v_max, const char *format = "%.3f",
                  ImGuiSliderFlags flags = 0);
bool overload_176(const char *label, float v[4], float v_min, float v_max, const char *format = "%.3f",
                  ImGuiSliderFlags flags = 0);
bool overload_177(const char *label, float *v_rad, float v_degrees_min = -360.0f,
                  float v_degrees_max = +360.0f, const char *format = "%.0f deg", ImGuiSliderFlags flags = 0);
bool overload_178(const char *label, int *v, int v_min, int v_max, const char *format = "%d",
                  ImGuiSliderFlags flags = 0);
bool overload_179(const char *label, int v[2], int v_min, int v_max, const char *format = "%d",
                  ImGuiSliderFlags flags = 0);
bool overload_180(const char *label, int v[3], int v_min, int v_max, const char *format = "%d",
                  ImGuiSliderFlags flags = 0);
bool overload_181(const char *label, int v[4], int v_min, int v_max, const char *format = "%d",
                  ImGuiSliderFlags flags = 0);
bool overload_182(const char *label, ImGuiDataType data_type, void *p_data, const void *p_min,
                  const void *p_max, const char *format = NULL, ImGuiSliderFlags flags = 0);
bool overload_183(const char *label, ImGuiDataType data_type, void *p_data, int components, const void *p_min,
                  const void *p_max, const char *format = NULL, ImGuiSliderFlags flags = 0);
bool overload_184(const char *label, const ImVec2 &size, float *v, float v_min, float v_max,
                  const char *format = "%.3f", ImGuiSliderFlags flags = 0);
bool overload_185(const char *label, const ImVec2 &size, int *v, int v_min, int v_max,
                  const char *format = "%d", ImGuiSliderFlags flags = 0);
bool overload_186(const char *label, const ImVec2 &size, ImGuiDataType data_type, void *p_data,
                  const void *p_min, const void *p_max, const char *format = NULL,
                  ImGuiSliderFlags flags = 0);
bool overload_187(const char *label, char *buf, size_t buf_size, ImGuiInputTextFlags flags = 0,
                  ImGuiInputTextCallback callback = NULL, void *user_data = NULL);
bool overload_188(const char *label, char *buf, size_t buf_size, const ImVec2 &size = ImVec2(0, 0),
                  ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL,
                  void *user_data = NULL);
bool overload_189(const char *label, const char *hint, char *buf, size_t buf_size,
                  ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL,
                  void *user_data = NULL);
bool overload_190(const char *label, float *v, float step = 0.0f, float step_fast = 0.0f,
                  const char *format = "%.3f", ImGuiInputTextFlags flags = 0);
bool overload_191(const char *label, float v[2], const char *format = "%.3f", ImGuiInputTextFlags flags = 0);
bool overload_192(const char *label, float v[3], const char *format = "%.3f", ImGuiInputTextFlags flags = 0);
bool overload_193(const char *label, float v[4], const char *format = "%.3f", ImGuiInputTextFlags flags = 0);
bool overload_194(const char *label, int *v, int step = 1, int step_fast = 100,
                  ImGuiInputTextFlags flags = 0);
bool overload_195(const char *label, int v[2], ImGuiInputTextFlags flags = 0);
bool overload_196(const char *label, int v[3], ImGuiInputTextFlags flags = 0);
bool overload_197(const char *label, int v[4], ImGuiInputTextFlags flags = 0);
bool overload_198(const char *label, double *v, double step = 0.0, double step_fast = 0.0,
                  const char *format = "%.6f", ImGuiInputTextFlags flags = 0);
bool overload_199(const char *label, ImGuiDataType data_type, void *p_data, const void *p_step = NULL,
                  const void *p_step_fast = NULL, const char *format = NULL, ImGuiInputTextFlags flags = 0);
bool overload_200(const char *label, ImGuiDataType data_type, void *p_data, int components,
                  const void *p_step = NULL, const void *p_step_fast = NULL, const char *format = NULL,
                  ImGuiInputTextFlags flags = 0);
bool overload_201(const char *label, float col[3], ImGuiColorEditFlags flags = 0);
bool overload_202(const char *label, float col[4], ImGuiColorEditFlags flags = 0);
bool overload_203(const char *label, float col[3], ImGuiColorEditFlags flags = 0);
bool overload_204(const char *label, float col[4], ImGuiColorEditFlags flags = 0,
                  const float *ref_col = NULL);
bool overload_205(const char *desc_id, const ImVec4 &col, ImGuiColorEditFlags flags = 0,
                  const ImVec2 &size = ImVec2(0, 0));
bool overload_206(const char *label);
bool overload_207(const char *str_id, const char *fmt, ...);
bool overload_208(const void *ptr_id, const char *fmt, ...);
bool overload_209(const char *str_id, const char *fmt, va_list args);
bool overload_210(const void *ptr_id, const char *fmt, va_list args);
bool overload_211(const char *label, ImGuiTreeNodeFlags flags = 0);
bool overload_212(const char *str_id, ImGuiTreeNodeFlags flags, const char *fmt, ...);
bool overload_213(const void *ptr_id, ImGuiTreeNodeFlags flags, const char *fmt, ...);
bool overload_214(const char *str_id, ImGuiTreeNodeFlags flags, const char *fmt, va_list args);
bool overload_215(const void *ptr_id, ImGuiTreeNodeFlags flags, const char *fmt, va_list args);
void overload_216(const char *str_id);
void overload_217(const void *ptr_id);
void overload_218();
float overload_219();
bool overload_220(const char *label, ImGuiTreeNodeFlags flags = 0);
bool overload_221(const char *label, bool *p_visible, ImGuiTreeNodeFlags flags = 0);
void overload_222(bool is_open, ImGuiCond cond = 0);
void overload_223(ImGuiID storage_id);
bool overload_224(ImGuiID storage_id);
bool overload_225(const char *label, bool selected = false, ImGuiSelectableFlags flags = 0,
                  const ImVec2 &size = ImVec2(0, 0));
bool overload_226(const char *label, bool *p_selected, ImGuiSelectableFlags flags = 0,
                  const ImVec2 &size = ImVec2(0, 0));
ImGuiMultiSelectIO *overload_227(ImGuiMultiSelectFlags flags, int selection_size = -1, int items_count = -1);
ImGuiMultiSelectIO *overload_228();
void overload_229(ImGuiSelectionUserData selection_user_data);
bool overload_230();
bool overload_231(const char *label, const ImVec2 &size = ImVec2(0, 0));
void overload_232();
bool overload_233(const char *label, int *current_item, const char *const items[], int items_count,
                  int height_in_items = -1);
bool overload_234(const char *label, int *current_item, const char *(*getter)(void *user_data, int idx),
                  void *user_data, int items_count, int height_in_items = -1);
void overload_235(const char *label, const float *values, int values_count, int values_offset = 0,
                  const char *overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX,
                  ImVec2 graph_size = ImVec2(0, 0), int stride = sizeof(float));
void overload_236(const char *label, float (*values_getter)(void *data, int idx), void *data,
                  int values_count, int values_offset = 0, const char *overlay_text = NULL,
                  float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0, 0));
void overload_237(const char *label, const float *values, int values_count, int values_offset = 0,
                  const char *overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX,
                  ImVec2 graph_size = ImVec2(0, 0), int stride = sizeof(float));
void overload_238(const char *label, float (*values_getter)(void *data, int idx), void *data,
                  int values_count, int values_offset = 0, const char *overlay_text = NULL,
                  float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0, 0));
void overload_239(const char *prefix, bool b);
void overload_240(const char *prefix, int v);
void overload_241(const char *prefix, unsigned int v);
void overload_242(const char *prefix, float v, const char *float_format = NULL);
bool overload_243();
void overload_244();
bool overload_245();
void overload_246();
bool overload_247(const char *label, bool enabled = true);
void overload_248();
bool overload_249(const char *label, const char *shortcut = NULL, bool selected = false, bool enabled = true);
bool overload_250(const char *label, const char *shortcut, bool *p_selected, bool enabled = true);
bool overload_251();
void overload_252();
void overload_253(const char *fmt, ...);
void overload_254(const char *fmt, va_list args);
bool overload_255();
void overload_256(const char *fmt, ...);
void overload_257(const char *fmt, va_list args);
bool overload_258(const char *str_id, ImGuiWindowFlags flags = 0);
bool overload_259(const char *name, bool *p_open = NULL, ImGuiWindowFlags flags = 0);
void overload_260();
bool overload_261(const char *str_id, ImGuiPopupFlags popup_flags = 0);
bool overload_262(ImGuiID id, ImGuiPopupFlags popup_flags = 0);
bool overload_263(const char *str_id = NULL, ImGuiPopupFlags popup_flags = 0);
void overload_264();
bool overload_265(const char *str_id = NULL, ImGuiPopupFlags popup_flags = 0);
bool overload_266(const char *str_id = NULL, ImGuiPopupFlags popup_flags = 0);
bool overload_267(const char *str_id = NULL, ImGuiPopupFlags popup_flags = 0);
bool overload_268(const char *str_id, ImGuiPopupFlags flags = 0);
bool overload_269(const char *str_id, int columns, ImGuiTableFlags flags = 0,
                  const ImVec2 &outer_size = ImVec2(0.0f, 0.0f), float inner_width = 0.0f);
void overload_270();
void overload_271(ImGuiTableRowFlags row_flags = 0, float min_row_height = 0.0f);
bool overload_272();
bool overload_273(int column_n);
void overload_274(const char *label, ImGuiTableColumnFlags flags = 0, float init_width_or_weight = 0.0f,
                  ImGuiID user_data = 0);
void overload_275(int cols, int rows);
void overload_276(const char *label);
void overload_277();
void overload_278();
ImGuiTableSortSpecs *overload_279();
int overload_280();
int overload_281();
int overload_282();
const char *overload_283(int column_n = -1);
ImGuiTableColumnFlags overload_284(int column_n = -1);
void overload_285(int column_n, bool v);
int overload_286();
void overload_287(ImGuiTableBgTarget target, ImU32 color, int column_n = -1);
void overload_288(int count = 1, const char *id = NULL, bool borders = true);
void overload_289();
int overload_290();
float overload_291(int column_index = -1);
void overload_292(int column_index, float width);
float overload_293(int column_index = -1);
void overload_294(int column_index, float offset_x);
int overload_295();
bool overload_296(const char *str_id, ImGuiTabBarFlags flags = 0);
void overload_297();
bool overload_298(const char *label, bool *p_open = NULL, ImGuiTabItemFlags flags = 0);
void overload_299();
bool overload_300(const char *label, ImGuiTabItemFlags flags = 0);
void overload_301(const char *tab_or_docked_window_label);
ImGuiID overload_302(ImGuiID dockspace_id, const ImVec2 &size = ImVec2(0, 0), ImGuiDockNodeFlags flags = 0,
                     const ImGuiWindowClass *window_class = NULL);
ImGuiID overload_303(ImGuiID dockspace_id = 0, const ImGuiViewport *viewport = NULL,
                     ImGuiDockNodeFlags flags = 0, const ImGuiWindowClass *window_class = NULL);
void overload_304(ImGuiID dock_id, ImGuiCond cond = 0);
void overload_305(const ImGuiWindowClass *window_class);
ImGuiID overload_306();
bool overload_307();
bool overload_315(ImGuiDragDropFlags flags = 0);
bool overload_316(const char *type, const void *data, size_t sz, ImGuiCond cond = 0);
void overload_317();
bool overload_318();
const ImGuiPayload *overload_319(const char *type, ImGuiDragDropFlags flags = 0);
void overload_320();
const ImGuiPayload *overload_321();
void overload_322(bool disabled = true);
void overload_323();
void overload_324(const ImVec2 &clip_rect_min, const ImVec2 &clip_rect_max,
                  bool intersect_with_current_clip_rect);
void overload_325();
void overload_326();
void overload_327(int offset = 0);
void overload_328(bool visible);
void overload_329();
bool overload_330(ImGuiHoveredFlags flags = 0);
bool overload_331();
bool overload_332();
bool overload_333(ImGuiMouseButton mouse_button = 0);
bool overload_334();
bool overload_335();
bool overload_336();
bool overload_337();
bool overload_338();
bool overload_339();
bool overload_340();
bool overload_341();
bool overload_342();
ImGuiID overload_343();
ImVec2 overload_344();
ImVec2 overload_345();
ImVec2 overload_346();
ImGuiItemFlags overload_347();
int overload_348(ImGuiMouseButton mouse_button = 0, float delay = -1.0f);
ImGuiViewport *overload_349();
ImDrawList *overload_350(ImGuiViewport *viewport = NULL);
ImDrawList *overload_351(ImGuiViewport *viewport = NULL);
bool overload_352(const ImVec2 &size);
bool overload_353(const ImVec2 &rect_min, const ImVec2 &rect_max);
double overload_354();
int overload_355();
ImDrawListSharedData *overload_356();
const char *overload_357(ImGuiCol idx);
void overload_358(ImGuiStorage *storage);
ImGuiStorage *overload_359();
ImVec2 overload_360(const char *text, const char *text_end = NULL, bool hide_text_after_double_hash = false,
                    float wrap_width = -1.0f);
ImVec4 overload_361(ImU32 in);
ImU32 overload_362(const ImVec4 &in);
void overload_363(float r, float g, float b, float &out_h, float &out_s, float &out_v);
void overload_364(float h, float s, float v, float &out_r, float &out_g, float &out_b);
bool overload_365(ImGuiKey key);
bool overload_366(ImGuiKey key, bool repeat = true);
bool overload_367(ImGuiKey key);
bool overload_368(ImGuiKeyChord key_chord);
int overload_369(ImGuiKey key, float repeat_delay, float rate);
const char *overload_370(ImGuiKey key);
void overload_371(bool want_capture_keyboard);
bool overload_372(ImGuiKeyChord key_chord, ImGuiInputFlags flags = 0);
void overload_373(ImGuiKeyChord key_chord, ImGuiInputFlags flags = 0);
bool overload_374(ImGuiKey key);
bool overload_375(ImGuiMouseButton button);
bool overload_376(ImGuiMouseButton button, bool repeat = false);
bool overload_377(ImGuiMouseButton button);
bool overload_378(ImGuiMouseButton button);
bool overload_379(ImGuiMouseButton button, float delay = -1.f);
int overload_380(ImGuiMouseButton button);
bool overload_381(const ImVec2 &r_min, const ImVec2 &r_max, bool clip = true);
bool overload_382(const ImVec2 *mouse_pos = NULL);
bool overload_383();
ImVec2 overload_384();
ImVec2 overload_385();
bool overload_386(ImGuiMouseButton button, float lock_threshold = -1.0f);
ImVec2 overload_387(ImGuiMouseButton button = 0, float lock_threshold = -1.0f);
void overload_388(ImGuiMouseButton button = 0);
ImGuiMouseCursor overload_389();
void overload_390(ImGuiMouseCursor cursor_type);
void overload_391(bool want_capture_mouse);
const char *overload_392();
void overload_393(const char *text);
} // namespace reference
decltype(&reference::overload_6) volatile api_6 =
    static_cast<decltype(&reference::overload_6)>(&imkit::GetStyle);
decltype(&reference::overload_20) volatile api_20 =
    static_cast<decltype(&reference::overload_20)>(&imkit::GetVersion);
decltype(&reference::overload_24) volatile api_24 =
    static_cast<decltype(&reference::overload_24)>(&imkit::Begin);
decltype(&reference::overload_25) volatile api_25 =
    static_cast<decltype(&reference::overload_25)>(&imkit::End);
decltype(&reference::overload_26) volatile api_26 =
    static_cast<decltype(&reference::overload_26)>(&imkit::BeginChild);
decltype(&reference::overload_27) volatile api_27 =
    static_cast<decltype(&reference::overload_27)>(&imkit::BeginChild);
decltype(&reference::overload_28) volatile api_28 =
    static_cast<decltype(&reference::overload_28)>(&imkit::EndChild);
decltype(&reference::overload_29) volatile api_29 =
    static_cast<decltype(&reference::overload_29)>(&imkit::IsWindowAppearing);
decltype(&reference::overload_30) volatile api_30 =
    static_cast<decltype(&reference::overload_30)>(&imkit::IsWindowCollapsed);
decltype(&reference::overload_31) volatile api_31 =
    static_cast<decltype(&reference::overload_31)>(&imkit::IsWindowFocused);
decltype(&reference::overload_32) volatile api_32 =
    static_cast<decltype(&reference::overload_32)>(&imkit::IsWindowHovered);
decltype(&reference::overload_33) volatile api_33 =
    static_cast<decltype(&reference::overload_33)>(&imkit::GetWindowDrawList);
decltype(&reference::overload_34) volatile api_34 =
    static_cast<decltype(&reference::overload_34)>(&imkit::GetWindowDpiScale);
decltype(&reference::overload_35) volatile api_35 =
    static_cast<decltype(&reference::overload_35)>(&imkit::GetWindowPos);
decltype(&reference::overload_36) volatile api_36 =
    static_cast<decltype(&reference::overload_36)>(&imkit::GetWindowSize);
decltype(&reference::overload_37) volatile api_37 =
    static_cast<decltype(&reference::overload_37)>(&imkit::GetWindowWidth);
decltype(&reference::overload_38) volatile api_38 =
    static_cast<decltype(&reference::overload_38)>(&imkit::GetWindowHeight);
decltype(&reference::overload_39) volatile api_39 =
    static_cast<decltype(&reference::overload_39)>(&imkit::GetWindowViewport);
decltype(&reference::overload_40) volatile api_40 =
    static_cast<decltype(&reference::overload_40)>(&imkit::SetNextWindowPos);
decltype(&reference::overload_41) volatile api_41 =
    static_cast<decltype(&reference::overload_41)>(&imkit::SetNextWindowSize);
decltype(&reference::overload_42) volatile api_42 =
    static_cast<decltype(&reference::overload_42)>(&imkit::SetNextWindowSizeConstraints);
decltype(&reference::overload_43) volatile api_43 =
    static_cast<decltype(&reference::overload_43)>(&imkit::SetNextWindowContentSize);
decltype(&reference::overload_44) volatile api_44 =
    static_cast<decltype(&reference::overload_44)>(&imkit::SetNextWindowCollapsed);
decltype(&reference::overload_45) volatile api_45 =
    static_cast<decltype(&reference::overload_45)>(&imkit::SetNextWindowFocus);
decltype(&reference::overload_46) volatile api_46 =
    static_cast<decltype(&reference::overload_46)>(&imkit::SetNextWindowScroll);
decltype(&reference::overload_47) volatile api_47 =
    static_cast<decltype(&reference::overload_47)>(&imkit::SetNextWindowBgAlpha);
decltype(&reference::overload_48) volatile api_48 =
    static_cast<decltype(&reference::overload_48)>(&imkit::SetNextWindowViewport);
decltype(&reference::overload_49) volatile api_49 =
    static_cast<decltype(&reference::overload_49)>(&imkit::SetWindowPos);
decltype(&reference::overload_50) volatile api_50 =
    static_cast<decltype(&reference::overload_50)>(&imkit::SetWindowSize);
decltype(&reference::overload_51) volatile api_51 =
    static_cast<decltype(&reference::overload_51)>(&imkit::SetWindowCollapsed);
decltype(&reference::overload_52) volatile api_52 =
    static_cast<decltype(&reference::overload_52)>(&imkit::SetWindowFocus);
decltype(&reference::overload_53) volatile api_53 =
    static_cast<decltype(&reference::overload_53)>(&imkit::SetWindowPos);
decltype(&reference::overload_54) volatile api_54 =
    static_cast<decltype(&reference::overload_54)>(&imkit::SetWindowSize);
decltype(&reference::overload_55) volatile api_55 =
    static_cast<decltype(&reference::overload_55)>(&imkit::SetWindowCollapsed);
decltype(&reference::overload_56) volatile api_56 =
    static_cast<decltype(&reference::overload_56)>(&imkit::SetWindowFocus);
decltype(&reference::overload_57) volatile api_57 =
    static_cast<decltype(&reference::overload_57)>(&imkit::GetScrollX);
decltype(&reference::overload_58) volatile api_58 =
    static_cast<decltype(&reference::overload_58)>(&imkit::GetScrollY);
decltype(&reference::overload_59) volatile api_59 =
    static_cast<decltype(&reference::overload_59)>(&imkit::SetScrollX);
decltype(&reference::overload_60) volatile api_60 =
    static_cast<decltype(&reference::overload_60)>(&imkit::SetScrollY);
decltype(&reference::overload_61) volatile api_61 =
    static_cast<decltype(&reference::overload_61)>(&imkit::GetScrollMaxX);
decltype(&reference::overload_62) volatile api_62 =
    static_cast<decltype(&reference::overload_62)>(&imkit::GetScrollMaxY);
decltype(&reference::overload_63) volatile api_63 =
    static_cast<decltype(&reference::overload_63)>(&imkit::SetScrollHereX);
decltype(&reference::overload_64) volatile api_64 =
    static_cast<decltype(&reference::overload_64)>(&imkit::SetScrollHereY);
decltype(&reference::overload_65) volatile api_65 =
    static_cast<decltype(&reference::overload_65)>(&imkit::SetScrollFromPosX);
decltype(&reference::overload_66) volatile api_66 =
    static_cast<decltype(&reference::overload_66)>(&imkit::SetScrollFromPosY);
decltype(&reference::overload_67) volatile api_67 =
    static_cast<decltype(&reference::overload_67)>(&imkit::PushFont);
decltype(&reference::overload_68) volatile api_68 =
    static_cast<decltype(&reference::overload_68)>(&imkit::PopFont);
decltype(&reference::overload_69) volatile api_69 =
    static_cast<decltype(&reference::overload_69)>(&imkit::GetFont);
decltype(&reference::overload_70) volatile api_70 =
    static_cast<decltype(&reference::overload_70)>(&imkit::GetFontSize);
decltype(&reference::overload_71) volatile api_71 =
    static_cast<decltype(&reference::overload_71)>(&imkit::GetFontBaked);
decltype(&reference::overload_72) volatile api_72 =
    static_cast<decltype(&reference::overload_72)>(&imkit::PushStyleColor);
decltype(&reference::overload_73) volatile api_73 =
    static_cast<decltype(&reference::overload_73)>(&imkit::PushStyleColor);
decltype(&reference::overload_74) volatile api_74 =
    static_cast<decltype(&reference::overload_74)>(&imkit::PopStyleColor);
decltype(&reference::overload_75) volatile api_75 =
    static_cast<decltype(&reference::overload_75)>(&imkit::PushStyleVar);
decltype(&reference::overload_76) volatile api_76 =
    static_cast<decltype(&reference::overload_76)>(&imkit::PushStyleVar);
decltype(&reference::overload_77) volatile api_77 =
    static_cast<decltype(&reference::overload_77)>(&imkit::PushStyleVarX);
decltype(&reference::overload_78) volatile api_78 =
    static_cast<decltype(&reference::overload_78)>(&imkit::PushStyleVarY);
decltype(&reference::overload_79) volatile api_79 =
    static_cast<decltype(&reference::overload_79)>(&imkit::PopStyleVar);
decltype(&reference::overload_80) volatile api_80 =
    static_cast<decltype(&reference::overload_80)>(&imkit::PushItemFlag);
decltype(&reference::overload_81) volatile api_81 =
    static_cast<decltype(&reference::overload_81)>(&imkit::PopItemFlag);
decltype(&reference::overload_82) volatile api_82 =
    static_cast<decltype(&reference::overload_82)>(&imkit::PushItemWidth);
decltype(&reference::overload_83) volatile api_83 =
    static_cast<decltype(&reference::overload_83)>(&imkit::PopItemWidth);
decltype(&reference::overload_84) volatile api_84 =
    static_cast<decltype(&reference::overload_84)>(&imkit::SetNextItemWidth);
decltype(&reference::overload_85) volatile api_85 =
    static_cast<decltype(&reference::overload_85)>(&imkit::CalcItemWidth);
decltype(&reference::overload_86) volatile api_86 =
    static_cast<decltype(&reference::overload_86)>(&imkit::PushTextWrapPos);
decltype(&reference::overload_87) volatile api_87 =
    static_cast<decltype(&reference::overload_87)>(&imkit::PopTextWrapPos);
decltype(&reference::overload_88) volatile api_88 =
    static_cast<decltype(&reference::overload_88)>(&imkit::GetFontTexUvWhitePixel);
decltype(&reference::overload_89) volatile api_89 =
    static_cast<decltype(&reference::overload_89)>(&imkit::GetColorU32);
decltype(&reference::overload_90) volatile api_90 =
    static_cast<decltype(&reference::overload_90)>(&imkit::GetColorU32);
decltype(&reference::overload_91) volatile api_91 =
    static_cast<decltype(&reference::overload_91)>(&imkit::GetColorU32);
decltype(&reference::overload_92) volatile api_92 =
    static_cast<decltype(&reference::overload_92)>(&imkit::GetStyleColorVec4);
decltype(&reference::overload_93) volatile api_93 =
    static_cast<decltype(&reference::overload_93)>(&imkit::GetCursorScreenPos);
decltype(&reference::overload_94) volatile api_94 =
    static_cast<decltype(&reference::overload_94)>(&imkit::SetCursorScreenPos);
decltype(&reference::overload_95) volatile api_95 =
    static_cast<decltype(&reference::overload_95)>(&imkit::GetContentRegionAvail);
decltype(&reference::overload_96) volatile api_96 =
    static_cast<decltype(&reference::overload_96)>(&imkit::GetCursorPos);
decltype(&reference::overload_97) volatile api_97 =
    static_cast<decltype(&reference::overload_97)>(&imkit::GetCursorPosX);
decltype(&reference::overload_98) volatile api_98 =
    static_cast<decltype(&reference::overload_98)>(&imkit::GetCursorPosY);
decltype(&reference::overload_99) volatile api_99 =
    static_cast<decltype(&reference::overload_99)>(&imkit::SetCursorPos);
decltype(&reference::overload_100) volatile api_100 =
    static_cast<decltype(&reference::overload_100)>(&imkit::SetCursorPosX);
decltype(&reference::overload_101) volatile api_101 =
    static_cast<decltype(&reference::overload_101)>(&imkit::SetCursorPosY);
decltype(&reference::overload_102) volatile api_102 =
    static_cast<decltype(&reference::overload_102)>(&imkit::GetCursorStartPos);
decltype(&reference::overload_103) volatile api_103 =
    static_cast<decltype(&reference::overload_103)>(&imkit::Separator);
decltype(&reference::overload_104) volatile api_104 =
    static_cast<decltype(&reference::overload_104)>(&imkit::SameLine);
decltype(&reference::overload_105) volatile api_105 =
    static_cast<decltype(&reference::overload_105)>(&imkit::NewLine);
decltype(&reference::overload_106) volatile api_106 =
    static_cast<decltype(&reference::overload_106)>(&imkit::Spacing);
decltype(&reference::overload_107) volatile api_107 =
    static_cast<decltype(&reference::overload_107)>(&imkit::Dummy);
decltype(&reference::overload_108) volatile api_108 =
    static_cast<decltype(&reference::overload_108)>(&imkit::Indent);
decltype(&reference::overload_109) volatile api_109 =
    static_cast<decltype(&reference::overload_109)>(&imkit::Unindent);
decltype(&reference::overload_110) volatile api_110 =
    static_cast<decltype(&reference::overload_110)>(&imkit::BeginGroup);
decltype(&reference::overload_111) volatile api_111 =
    static_cast<decltype(&reference::overload_111)>(&imkit::EndGroup);
decltype(&reference::overload_112) volatile api_112 =
    static_cast<decltype(&reference::overload_112)>(&imkit::AlignTextToFramePadding);
decltype(&reference::overload_113) volatile api_113 =
    static_cast<decltype(&reference::overload_113)>(&imkit::GetTextLineHeight);
decltype(&reference::overload_114) volatile api_114 =
    static_cast<decltype(&reference::overload_114)>(&imkit::GetTextLineHeightWithSpacing);
decltype(&reference::overload_115) volatile api_115 =
    static_cast<decltype(&reference::overload_115)>(&imkit::GetFrameHeight);
decltype(&reference::overload_116) volatile api_116 =
    static_cast<decltype(&reference::overload_116)>(&imkit::GetFrameHeightWithSpacing);
decltype(&reference::overload_117) volatile api_117 =
    static_cast<decltype(&reference::overload_117)>(&imkit::PushID);
decltype(&reference::overload_118) volatile api_118 =
    static_cast<decltype(&reference::overload_118)>(&imkit::PushID);
decltype(&reference::overload_119) volatile api_119 =
    static_cast<decltype(&reference::overload_119)>(&imkit::PushID);
decltype(&reference::overload_120) volatile api_120 =
    static_cast<decltype(&reference::overload_120)>(&imkit::PushID);
decltype(&reference::overload_121) volatile api_121 =
    static_cast<decltype(&reference::overload_121)>(&imkit::PopID);
decltype(&reference::overload_122) volatile api_122 =
    static_cast<decltype(&reference::overload_122)>(&imkit::GetID);
decltype(&reference::overload_123) volatile api_123 =
    static_cast<decltype(&reference::overload_123)>(&imkit::GetID);
decltype(&reference::overload_124) volatile api_124 =
    static_cast<decltype(&reference::overload_124)>(&imkit::GetID);
decltype(&reference::overload_125) volatile api_125 =
    static_cast<decltype(&reference::overload_125)>(&imkit::GetID);
decltype(&reference::overload_126) volatile api_126 =
    static_cast<decltype(&reference::overload_126)>(&imkit::TextUnformatted);
decltype(&reference::overload_127) volatile api_127 =
    static_cast<decltype(&reference::overload_127)>(&imkit::Text);
decltype(&reference::overload_128) volatile api_128 =
    static_cast<decltype(&reference::overload_128)>(&imkit::TextV);
decltype(&reference::overload_129) volatile api_129 =
    static_cast<decltype(&reference::overload_129)>(&imkit::TextColored);
decltype(&reference::overload_130) volatile api_130 =
    static_cast<decltype(&reference::overload_130)>(&imkit::TextColoredV);
decltype(&reference::overload_131) volatile api_131 =
    static_cast<decltype(&reference::overload_131)>(&imkit::TextDisabled);
decltype(&reference::overload_132) volatile api_132 =
    static_cast<decltype(&reference::overload_132)>(&imkit::TextDisabledV);
decltype(&reference::overload_133) volatile api_133 =
    static_cast<decltype(&reference::overload_133)>(&imkit::TextWrapped);
decltype(&reference::overload_134) volatile api_134 =
    static_cast<decltype(&reference::overload_134)>(&imkit::TextWrappedV);
decltype(&reference::overload_135) volatile api_135 =
    static_cast<decltype(&reference::overload_135)>(&imkit::LabelText);
decltype(&reference::overload_136) volatile api_136 =
    static_cast<decltype(&reference::overload_136)>(&imkit::LabelTextV);
decltype(&reference::overload_137) volatile api_137 =
    static_cast<decltype(&reference::overload_137)>(&imkit::BulletText);
decltype(&reference::overload_138) volatile api_138 =
    static_cast<decltype(&reference::overload_138)>(&imkit::BulletTextV);
decltype(&reference::overload_139) volatile api_139 =
    static_cast<decltype(&reference::overload_139)>(&imkit::SeparatorText);
decltype(&reference::overload_140) volatile api_140 =
    static_cast<decltype(&reference::overload_140)>(&imkit::Button);
decltype(&reference::overload_141) volatile api_141 =
    static_cast<decltype(&reference::overload_141)>(&imkit::SmallButton);
decltype(&reference::overload_142) volatile api_142 =
    static_cast<decltype(&reference::overload_142)>(&imkit::InvisibleButton);
decltype(&reference::overload_143) volatile api_143 =
    static_cast<decltype(&reference::overload_143)>(&imkit::ArrowButton);
decltype(&reference::overload_144) volatile api_144 =
    static_cast<decltype(&reference::overload_144)>(&imkit::Checkbox);
decltype(&reference::overload_145) volatile api_145 =
    static_cast<decltype(&reference::overload_145)>(&imkit::CheckboxFlags);
decltype(&reference::overload_146) volatile api_146 =
    static_cast<decltype(&reference::overload_146)>(&imkit::CheckboxFlags);
decltype(&reference::overload_147) volatile api_147 =
    static_cast<decltype(&reference::overload_147)>(&imkit::RadioButton);
decltype(&reference::overload_148) volatile api_148 =
    static_cast<decltype(&reference::overload_148)>(&imkit::RadioButton);
decltype(&reference::overload_149) volatile api_149 =
    static_cast<decltype(&reference::overload_149)>(&imkit::ProgressBar);
decltype(&reference::overload_150) volatile api_150 =
    static_cast<decltype(&reference::overload_150)>(&imkit::Bullet);
decltype(&reference::overload_151) volatile api_151 =
    static_cast<decltype(&reference::overload_151)>(&imkit::TextLink);
decltype(&reference::overload_152) volatile api_152 =
    static_cast<decltype(&reference::overload_152)>(&imkit::TextLinkOpenURL);
decltype(&reference::overload_153) volatile api_153 =
    static_cast<decltype(&reference::overload_153)>(&imkit::Image);
decltype(&reference::overload_154) volatile api_154 =
    static_cast<decltype(&reference::overload_154)>(&imkit::ImageWithBg);
decltype(&reference::overload_155) volatile api_155 =
    static_cast<decltype(&reference::overload_155)>(&imkit::ImageButton);
decltype(&reference::overload_156) volatile api_156 =
    static_cast<decltype(&reference::overload_156)>(&imkit::BeginCombo);
decltype(&reference::overload_157) volatile api_157 =
    static_cast<decltype(&reference::overload_157)>(&imkit::EndCombo);
decltype(&reference::overload_158) volatile api_158 =
    static_cast<decltype(&reference::overload_158)>(&imkit::Combo);
decltype(&reference::overload_159) volatile api_159 =
    static_cast<decltype(&reference::overload_159)>(&imkit::Combo);
decltype(&reference::overload_160) volatile api_160 =
    static_cast<decltype(&reference::overload_160)>(&imkit::Combo);
decltype(&reference::overload_161) volatile api_161 =
    static_cast<decltype(&reference::overload_161)>(&imkit::DragFloat);
decltype(&reference::overload_162) volatile api_162 =
    static_cast<decltype(&reference::overload_162)>(&imkit::DragFloat2);
decltype(&reference::overload_163) volatile api_163 =
    static_cast<decltype(&reference::overload_163)>(&imkit::DragFloat3);
decltype(&reference::overload_164) volatile api_164 =
    static_cast<decltype(&reference::overload_164)>(&imkit::DragFloat4);
decltype(&reference::overload_165) volatile api_165 =
    static_cast<decltype(&reference::overload_165)>(&imkit::DragFloatRange2);
decltype(&reference::overload_166) volatile api_166 =
    static_cast<decltype(&reference::overload_166)>(&imkit::DragInt);
decltype(&reference::overload_167) volatile api_167 =
    static_cast<decltype(&reference::overload_167)>(&imkit::DragInt2);
decltype(&reference::overload_168) volatile api_168 =
    static_cast<decltype(&reference::overload_168)>(&imkit::DragInt3);
decltype(&reference::overload_169) volatile api_169 =
    static_cast<decltype(&reference::overload_169)>(&imkit::DragInt4);
decltype(&reference::overload_170) volatile api_170 =
    static_cast<decltype(&reference::overload_170)>(&imkit::DragIntRange2);
decltype(&reference::overload_171) volatile api_171 =
    static_cast<decltype(&reference::overload_171)>(&imkit::DragScalar);
decltype(&reference::overload_172) volatile api_172 =
    static_cast<decltype(&reference::overload_172)>(&imkit::DragScalarN);
decltype(&reference::overload_173) volatile api_173 =
    static_cast<decltype(&reference::overload_173)>(&imkit::SliderFloat);
decltype(&reference::overload_174) volatile api_174 =
    static_cast<decltype(&reference::overload_174)>(&imkit::SliderFloat2);
decltype(&reference::overload_175) volatile api_175 =
    static_cast<decltype(&reference::overload_175)>(&imkit::SliderFloat3);
decltype(&reference::overload_176) volatile api_176 =
    static_cast<decltype(&reference::overload_176)>(&imkit::SliderFloat4);
decltype(&reference::overload_177) volatile api_177 =
    static_cast<decltype(&reference::overload_177)>(&imkit::SliderAngle);
decltype(&reference::overload_178) volatile api_178 =
    static_cast<decltype(&reference::overload_178)>(&imkit::SliderInt);
decltype(&reference::overload_179) volatile api_179 =
    static_cast<decltype(&reference::overload_179)>(&imkit::SliderInt2);
decltype(&reference::overload_180) volatile api_180 =
    static_cast<decltype(&reference::overload_180)>(&imkit::SliderInt3);
decltype(&reference::overload_181) volatile api_181 =
    static_cast<decltype(&reference::overload_181)>(&imkit::SliderInt4);
decltype(&reference::overload_182) volatile api_182 =
    static_cast<decltype(&reference::overload_182)>(&imkit::SliderScalar);
decltype(&reference::overload_183) volatile api_183 =
    static_cast<decltype(&reference::overload_183)>(&imkit::SliderScalarN);
decltype(&reference::overload_184) volatile api_184 =
    static_cast<decltype(&reference::overload_184)>(&imkit::VSliderFloat);
decltype(&reference::overload_185) volatile api_185 =
    static_cast<decltype(&reference::overload_185)>(&imkit::VSliderInt);
decltype(&reference::overload_186) volatile api_186 =
    static_cast<decltype(&reference::overload_186)>(&imkit::VSliderScalar);
decltype(&reference::overload_187) volatile api_187 =
    static_cast<decltype(&reference::overload_187)>(&imkit::InputText);
decltype(&reference::overload_188) volatile api_188 =
    static_cast<decltype(&reference::overload_188)>(&imkit::InputTextMultiline);
decltype(&reference::overload_189) volatile api_189 =
    static_cast<decltype(&reference::overload_189)>(&imkit::InputTextWithHint);
decltype(&reference::overload_190) volatile api_190 =
    static_cast<decltype(&reference::overload_190)>(&imkit::InputFloat);
decltype(&reference::overload_191) volatile api_191 =
    static_cast<decltype(&reference::overload_191)>(&imkit::InputFloat2);
decltype(&reference::overload_192) volatile api_192 =
    static_cast<decltype(&reference::overload_192)>(&imkit::InputFloat3);
decltype(&reference::overload_193) volatile api_193 =
    static_cast<decltype(&reference::overload_193)>(&imkit::InputFloat4);
decltype(&reference::overload_194) volatile api_194 =
    static_cast<decltype(&reference::overload_194)>(&imkit::InputInt);
decltype(&reference::overload_195) volatile api_195 =
    static_cast<decltype(&reference::overload_195)>(&imkit::InputInt2);
decltype(&reference::overload_196) volatile api_196 =
    static_cast<decltype(&reference::overload_196)>(&imkit::InputInt3);
decltype(&reference::overload_197) volatile api_197 =
    static_cast<decltype(&reference::overload_197)>(&imkit::InputInt4);
decltype(&reference::overload_198) volatile api_198 =
    static_cast<decltype(&reference::overload_198)>(&imkit::InputDouble);
decltype(&reference::overload_199) volatile api_199 =
    static_cast<decltype(&reference::overload_199)>(&imkit::InputScalar);
decltype(&reference::overload_200) volatile api_200 =
    static_cast<decltype(&reference::overload_200)>(&imkit::InputScalarN);
decltype(&reference::overload_201) volatile api_201 =
    static_cast<decltype(&reference::overload_201)>(&imkit::ColorEdit3);
decltype(&reference::overload_202) volatile api_202 =
    static_cast<decltype(&reference::overload_202)>(&imkit::ColorEdit4);
decltype(&reference::overload_203) volatile api_203 =
    static_cast<decltype(&reference::overload_203)>(&imkit::ColorPicker3);
decltype(&reference::overload_204) volatile api_204 =
    static_cast<decltype(&reference::overload_204)>(&imkit::ColorPicker4);
decltype(&reference::overload_205) volatile api_205 =
    static_cast<decltype(&reference::overload_205)>(&imkit::ColorButton);
decltype(&reference::overload_206) volatile api_206 =
    static_cast<decltype(&reference::overload_206)>(&imkit::TreeNode);
decltype(&reference::overload_207) volatile api_207 =
    static_cast<decltype(&reference::overload_207)>(&imkit::TreeNode);
decltype(&reference::overload_208) volatile api_208 =
    static_cast<decltype(&reference::overload_208)>(&imkit::TreeNode);
decltype(&reference::overload_209) volatile api_209 =
    static_cast<decltype(&reference::overload_209)>(&imkit::TreeNodeV);
decltype(&reference::overload_210) volatile api_210 =
    static_cast<decltype(&reference::overload_210)>(&imkit::TreeNodeV);
decltype(&reference::overload_211) volatile api_211 =
    static_cast<decltype(&reference::overload_211)>(&imkit::TreeNodeEx);
decltype(&reference::overload_212) volatile api_212 =
    static_cast<decltype(&reference::overload_212)>(&imkit::TreeNodeEx);
decltype(&reference::overload_213) volatile api_213 =
    static_cast<decltype(&reference::overload_213)>(&imkit::TreeNodeEx);
decltype(&reference::overload_214) volatile api_214 =
    static_cast<decltype(&reference::overload_214)>(&imkit::TreeNodeExV);
decltype(&reference::overload_215) volatile api_215 =
    static_cast<decltype(&reference::overload_215)>(&imkit::TreeNodeExV);
decltype(&reference::overload_216) volatile api_216 =
    static_cast<decltype(&reference::overload_216)>(&imkit::TreePush);
decltype(&reference::overload_217) volatile api_217 =
    static_cast<decltype(&reference::overload_217)>(&imkit::TreePush);
decltype(&reference::overload_218) volatile api_218 =
    static_cast<decltype(&reference::overload_218)>(&imkit::TreePop);
decltype(&reference::overload_219) volatile api_219 =
    static_cast<decltype(&reference::overload_219)>(&imkit::GetTreeNodeToLabelSpacing);
decltype(&reference::overload_220) volatile api_220 =
    static_cast<decltype(&reference::overload_220)>(&imkit::CollapsingHeader);
decltype(&reference::overload_221) volatile api_221 =
    static_cast<decltype(&reference::overload_221)>(&imkit::CollapsingHeader);
decltype(&reference::overload_222) volatile api_222 =
    static_cast<decltype(&reference::overload_222)>(&imkit::SetNextItemOpen);
decltype(&reference::overload_223) volatile api_223 =
    static_cast<decltype(&reference::overload_223)>(&imkit::SetNextItemStorageID);
decltype(&reference::overload_224) volatile api_224 =
    static_cast<decltype(&reference::overload_224)>(&imkit::TreeNodeGetOpen);
decltype(&reference::overload_225) volatile api_225 =
    static_cast<decltype(&reference::overload_225)>(&imkit::Selectable);
decltype(&reference::overload_226) volatile api_226 =
    static_cast<decltype(&reference::overload_226)>(&imkit::Selectable);
decltype(&reference::overload_227) volatile api_227 =
    static_cast<decltype(&reference::overload_227)>(&imkit::BeginMultiSelect);
decltype(&reference::overload_228) volatile api_228 =
    static_cast<decltype(&reference::overload_228)>(&imkit::EndMultiSelect);
decltype(&reference::overload_229) volatile api_229 =
    static_cast<decltype(&reference::overload_229)>(&imkit::SetNextItemSelectionUserData);
decltype(&reference::overload_230) volatile api_230 =
    static_cast<decltype(&reference::overload_230)>(&imkit::IsItemToggledSelection);
decltype(&reference::overload_231) volatile api_231 =
    static_cast<decltype(&reference::overload_231)>(&imkit::BeginListBox);
decltype(&reference::overload_232) volatile api_232 =
    static_cast<decltype(&reference::overload_232)>(&imkit::EndListBox);
decltype(&reference::overload_233) volatile api_233 =
    static_cast<decltype(&reference::overload_233)>(&imkit::ListBox);
decltype(&reference::overload_234) volatile api_234 =
    static_cast<decltype(&reference::overload_234)>(&imkit::ListBox);
decltype(&reference::overload_235) volatile api_235 =
    static_cast<decltype(&reference::overload_235)>(&imkit::PlotLines);
decltype(&reference::overload_236) volatile api_236 =
    static_cast<decltype(&reference::overload_236)>(&imkit::PlotLines);
decltype(&reference::overload_237) volatile api_237 =
    static_cast<decltype(&reference::overload_237)>(&imkit::PlotHistogram);
decltype(&reference::overload_238) volatile api_238 =
    static_cast<decltype(&reference::overload_238)>(&imkit::PlotHistogram);
decltype(&reference::overload_239) volatile api_239 =
    static_cast<decltype(&reference::overload_239)>(&imkit::Value);
decltype(&reference::overload_240) volatile api_240 =
    static_cast<decltype(&reference::overload_240)>(&imkit::Value);
decltype(&reference::overload_241) volatile api_241 =
    static_cast<decltype(&reference::overload_241)>(&imkit::Value);
decltype(&reference::overload_242) volatile api_242 =
    static_cast<decltype(&reference::overload_242)>(&imkit::Value);
decltype(&reference::overload_243) volatile api_243 =
    static_cast<decltype(&reference::overload_243)>(&imkit::BeginMenuBar);
decltype(&reference::overload_244) volatile api_244 =
    static_cast<decltype(&reference::overload_244)>(&imkit::EndMenuBar);
decltype(&reference::overload_245) volatile api_245 =
    static_cast<decltype(&reference::overload_245)>(&imkit::BeginMainMenuBar);
decltype(&reference::overload_246) volatile api_246 =
    static_cast<decltype(&reference::overload_246)>(&imkit::EndMainMenuBar);
decltype(&reference::overload_247) volatile api_247 =
    static_cast<decltype(&reference::overload_247)>(&imkit::BeginMenu);
decltype(&reference::overload_248) volatile api_248 =
    static_cast<decltype(&reference::overload_248)>(&imkit::EndMenu);
decltype(&reference::overload_249) volatile api_249 =
    static_cast<decltype(&reference::overload_249)>(&imkit::MenuItem);
decltype(&reference::overload_250) volatile api_250 =
    static_cast<decltype(&reference::overload_250)>(&imkit::MenuItem);
decltype(&reference::overload_251) volatile api_251 =
    static_cast<decltype(&reference::overload_251)>(&imkit::BeginTooltip);
decltype(&reference::overload_252) volatile api_252 =
    static_cast<decltype(&reference::overload_252)>(&imkit::EndTooltip);
decltype(&reference::overload_253) volatile api_253 =
    static_cast<decltype(&reference::overload_253)>(&imkit::SetTooltip);
decltype(&reference::overload_254) volatile api_254 =
    static_cast<decltype(&reference::overload_254)>(&imkit::SetTooltipV);
decltype(&reference::overload_255) volatile api_255 =
    static_cast<decltype(&reference::overload_255)>(&imkit::BeginItemTooltip);
decltype(&reference::overload_256) volatile api_256 =
    static_cast<decltype(&reference::overload_256)>(&imkit::SetItemTooltip);
decltype(&reference::overload_257) volatile api_257 =
    static_cast<decltype(&reference::overload_257)>(&imkit::SetItemTooltipV);
decltype(&reference::overload_258) volatile api_258 =
    static_cast<decltype(&reference::overload_258)>(&imkit::BeginPopup);
decltype(&reference::overload_259) volatile api_259 =
    static_cast<decltype(&reference::overload_259)>(&imkit::BeginPopupModal);
decltype(&reference::overload_260) volatile api_260 =
    static_cast<decltype(&reference::overload_260)>(&imkit::EndPopup);
decltype(&reference::overload_261) volatile api_261 =
    static_cast<decltype(&reference::overload_261)>(&imkit::OpenPopup);
decltype(&reference::overload_262) volatile api_262 =
    static_cast<decltype(&reference::overload_262)>(&imkit::OpenPopup);
decltype(&reference::overload_263) volatile api_263 =
    static_cast<decltype(&reference::overload_263)>(&imkit::OpenPopupOnItemClick);
decltype(&reference::overload_264) volatile api_264 =
    static_cast<decltype(&reference::overload_264)>(&imkit::CloseCurrentPopup);
decltype(&reference::overload_265) volatile api_265 =
    static_cast<decltype(&reference::overload_265)>(&imkit::BeginPopupContextItem);
decltype(&reference::overload_266) volatile api_266 =
    static_cast<decltype(&reference::overload_266)>(&imkit::BeginPopupContextWindow);
decltype(&reference::overload_267) volatile api_267 =
    static_cast<decltype(&reference::overload_267)>(&imkit::BeginPopupContextVoid);
decltype(&reference::overload_268) volatile api_268 =
    static_cast<decltype(&reference::overload_268)>(&imkit::IsPopupOpen);
decltype(&reference::overload_269) volatile api_269 =
    static_cast<decltype(&reference::overload_269)>(&imkit::BeginTable);
decltype(&reference::overload_270) volatile api_270 =
    static_cast<decltype(&reference::overload_270)>(&imkit::EndTable);
decltype(&reference::overload_271) volatile api_271 =
    static_cast<decltype(&reference::overload_271)>(&imkit::TableNextRow);
decltype(&reference::overload_272) volatile api_272 =
    static_cast<decltype(&reference::overload_272)>(&imkit::TableNextColumn);
decltype(&reference::overload_273) volatile api_273 =
    static_cast<decltype(&reference::overload_273)>(&imkit::TableSetColumnIndex);
decltype(&reference::overload_274) volatile api_274 =
    static_cast<decltype(&reference::overload_274)>(&imkit::TableSetupColumn);
decltype(&reference::overload_275) volatile api_275 =
    static_cast<decltype(&reference::overload_275)>(&imkit::TableSetupScrollFreeze);
decltype(&reference::overload_276) volatile api_276 =
    static_cast<decltype(&reference::overload_276)>(&imkit::TableHeader);
decltype(&reference::overload_277) volatile api_277 =
    static_cast<decltype(&reference::overload_277)>(&imkit::TableHeadersRow);
decltype(&reference::overload_278) volatile api_278 =
    static_cast<decltype(&reference::overload_278)>(&imkit::TableAngledHeadersRow);
decltype(&reference::overload_279) volatile api_279 =
    static_cast<decltype(&reference::overload_279)>(&imkit::TableGetSortSpecs);
decltype(&reference::overload_280) volatile api_280 =
    static_cast<decltype(&reference::overload_280)>(&imkit::TableGetColumnCount);
decltype(&reference::overload_281) volatile api_281 =
    static_cast<decltype(&reference::overload_281)>(&imkit::TableGetColumnIndex);
decltype(&reference::overload_282) volatile api_282 =
    static_cast<decltype(&reference::overload_282)>(&imkit::TableGetRowIndex);
decltype(&reference::overload_283) volatile api_283 =
    static_cast<decltype(&reference::overload_283)>(&imkit::TableGetColumnName);
decltype(&reference::overload_284) volatile api_284 =
    static_cast<decltype(&reference::overload_284)>(&imkit::TableGetColumnFlags);
decltype(&reference::overload_285) volatile api_285 =
    static_cast<decltype(&reference::overload_285)>(&imkit::TableSetColumnEnabled);
decltype(&reference::overload_286) volatile api_286 =
    static_cast<decltype(&reference::overload_286)>(&imkit::TableGetHoveredColumn);
decltype(&reference::overload_287) volatile api_287 =
    static_cast<decltype(&reference::overload_287)>(&imkit::TableSetBgColor);
decltype(&reference::overload_288) volatile api_288 =
    static_cast<decltype(&reference::overload_288)>(&imkit::Columns);
decltype(&reference::overload_289) volatile api_289 =
    static_cast<decltype(&reference::overload_289)>(&imkit::NextColumn);
decltype(&reference::overload_290) volatile api_290 =
    static_cast<decltype(&reference::overload_290)>(&imkit::GetColumnIndex);
decltype(&reference::overload_291) volatile api_291 =
    static_cast<decltype(&reference::overload_291)>(&imkit::GetColumnWidth);
decltype(&reference::overload_292) volatile api_292 =
    static_cast<decltype(&reference::overload_292)>(&imkit::SetColumnWidth);
decltype(&reference::overload_293) volatile api_293 =
    static_cast<decltype(&reference::overload_293)>(&imkit::GetColumnOffset);
decltype(&reference::overload_294) volatile api_294 =
    static_cast<decltype(&reference::overload_294)>(&imkit::SetColumnOffset);
decltype(&reference::overload_295) volatile api_295 =
    static_cast<decltype(&reference::overload_295)>(&imkit::GetColumnsCount);
decltype(&reference::overload_296) volatile api_296 =
    static_cast<decltype(&reference::overload_296)>(&imkit::BeginTabBar);
decltype(&reference::overload_297) volatile api_297 =
    static_cast<decltype(&reference::overload_297)>(&imkit::EndTabBar);
decltype(&reference::overload_298) volatile api_298 =
    static_cast<decltype(&reference::overload_298)>(&imkit::BeginTabItem);
decltype(&reference::overload_299) volatile api_299 =
    static_cast<decltype(&reference::overload_299)>(&imkit::EndTabItem);
decltype(&reference::overload_300) volatile api_300 =
    static_cast<decltype(&reference::overload_300)>(&imkit::TabItemButton);
decltype(&reference::overload_301) volatile api_301 =
    static_cast<decltype(&reference::overload_301)>(&imkit::SetTabItemClosed);
decltype(&reference::overload_302) volatile api_302 =
    static_cast<decltype(&reference::overload_302)>(&imkit::DockSpace);
decltype(&reference::overload_303) volatile api_303 =
    static_cast<decltype(&reference::overload_303)>(&imkit::DockSpaceOverViewport);
decltype(&reference::overload_304) volatile api_304 =
    static_cast<decltype(&reference::overload_304)>(&imkit::SetNextWindowDockID);
decltype(&reference::overload_305) volatile api_305 =
    static_cast<decltype(&reference::overload_305)>(&imkit::SetNextWindowClass);
decltype(&reference::overload_306) volatile api_306 =
    static_cast<decltype(&reference::overload_306)>(&imkit::GetWindowDockID);
decltype(&reference::overload_307) volatile api_307 =
    static_cast<decltype(&reference::overload_307)>(&imkit::IsWindowDocked);
decltype(&reference::overload_315) volatile api_315 =
    static_cast<decltype(&reference::overload_315)>(&imkit::BeginDragDropSource);
decltype(&reference::overload_316) volatile api_316 =
    static_cast<decltype(&reference::overload_316)>(&imkit::SetDragDropPayload);
decltype(&reference::overload_317) volatile api_317 =
    static_cast<decltype(&reference::overload_317)>(&imkit::EndDragDropSource);
decltype(&reference::overload_318) volatile api_318 =
    static_cast<decltype(&reference::overload_318)>(&imkit::BeginDragDropTarget);
decltype(&reference::overload_319) volatile api_319 =
    static_cast<decltype(&reference::overload_319)>(&imkit::AcceptDragDropPayload);
decltype(&reference::overload_320) volatile api_320 =
    static_cast<decltype(&reference::overload_320)>(&imkit::EndDragDropTarget);
decltype(&reference::overload_321) volatile api_321 =
    static_cast<decltype(&reference::overload_321)>(&imkit::GetDragDropPayload);
decltype(&reference::overload_322) volatile api_322 =
    static_cast<decltype(&reference::overload_322)>(&imkit::BeginDisabled);
decltype(&reference::overload_323) volatile api_323 =
    static_cast<decltype(&reference::overload_323)>(&imkit::EndDisabled);
decltype(&reference::overload_324) volatile api_324 =
    static_cast<decltype(&reference::overload_324)>(&imkit::PushClipRect);
decltype(&reference::overload_325) volatile api_325 =
    static_cast<decltype(&reference::overload_325)>(&imkit::PopClipRect);
decltype(&reference::overload_326) volatile api_326 =
    static_cast<decltype(&reference::overload_326)>(&imkit::SetItemDefaultFocus);
decltype(&reference::overload_327) volatile api_327 =
    static_cast<decltype(&reference::overload_327)>(&imkit::SetKeyboardFocusHere);
decltype(&reference::overload_328) volatile api_328 =
    static_cast<decltype(&reference::overload_328)>(&imkit::SetNavCursorVisible);
decltype(&reference::overload_329) volatile api_329 =
    static_cast<decltype(&reference::overload_329)>(&imkit::SetNextItemAllowOverlap);
decltype(&reference::overload_330) volatile api_330 =
    static_cast<decltype(&reference::overload_330)>(&imkit::IsItemHovered);
decltype(&reference::overload_331) volatile api_331 =
    static_cast<decltype(&reference::overload_331)>(&imkit::IsItemActive);
decltype(&reference::overload_332) volatile api_332 =
    static_cast<decltype(&reference::overload_332)>(&imkit::IsItemFocused);
decltype(&reference::overload_333) volatile api_333 =
    static_cast<decltype(&reference::overload_333)>(&imkit::IsItemClicked);
decltype(&reference::overload_334) volatile api_334 =
    static_cast<decltype(&reference::overload_334)>(&imkit::IsItemVisible);
decltype(&reference::overload_335) volatile api_335 =
    static_cast<decltype(&reference::overload_335)>(&imkit::IsItemEdited);
decltype(&reference::overload_336) volatile api_336 =
    static_cast<decltype(&reference::overload_336)>(&imkit::IsItemActivated);
decltype(&reference::overload_337) volatile api_337 =
    static_cast<decltype(&reference::overload_337)>(&imkit::IsItemDeactivated);
decltype(&reference::overload_338) volatile api_338 =
    static_cast<decltype(&reference::overload_338)>(&imkit::IsItemDeactivatedAfterEdit);
decltype(&reference::overload_339) volatile api_339 =
    static_cast<decltype(&reference::overload_339)>(&imkit::IsItemToggledOpen);
decltype(&reference::overload_340) volatile api_340 =
    static_cast<decltype(&reference::overload_340)>(&imkit::IsAnyItemHovered);
decltype(&reference::overload_341) volatile api_341 =
    static_cast<decltype(&reference::overload_341)>(&imkit::IsAnyItemActive);
decltype(&reference::overload_342) volatile api_342 =
    static_cast<decltype(&reference::overload_342)>(&imkit::IsAnyItemFocused);
decltype(&reference::overload_343) volatile api_343 =
    static_cast<decltype(&reference::overload_343)>(&imkit::GetItemID);
decltype(&reference::overload_344) volatile api_344 =
    static_cast<decltype(&reference::overload_344)>(&imkit::GetItemRectMin);
decltype(&reference::overload_345) volatile api_345 =
    static_cast<decltype(&reference::overload_345)>(&imkit::GetItemRectMax);
decltype(&reference::overload_346) volatile api_346 =
    static_cast<decltype(&reference::overload_346)>(&imkit::GetItemRectSize);
decltype(&reference::overload_347) volatile api_347 =
    static_cast<decltype(&reference::overload_347)>(&imkit::GetItemFlags);
decltype(&reference::overload_348) volatile api_348 =
    static_cast<decltype(&reference::overload_348)>(&imkit::GetItemClickedCountWithSingleClickDelay);
decltype(&reference::overload_349) volatile api_349 =
    static_cast<decltype(&reference::overload_349)>(&imkit::GetMainViewport);
decltype(&reference::overload_350) volatile api_350 =
    static_cast<decltype(&reference::overload_350)>(&imkit::GetBackgroundDrawList);
decltype(&reference::overload_351) volatile api_351 =
    static_cast<decltype(&reference::overload_351)>(&imkit::GetForegroundDrawList);
decltype(&reference::overload_352) volatile api_352 =
    static_cast<decltype(&reference::overload_352)>(&imkit::IsRectVisible);
decltype(&reference::overload_353) volatile api_353 =
    static_cast<decltype(&reference::overload_353)>(&imkit::IsRectVisible);
decltype(&reference::overload_354) volatile api_354 =
    static_cast<decltype(&reference::overload_354)>(&imkit::GetTime);
decltype(&reference::overload_355) volatile api_355 =
    static_cast<decltype(&reference::overload_355)>(&imkit::GetFrameCount);
decltype(&reference::overload_356) volatile api_356 =
    static_cast<decltype(&reference::overload_356)>(&imkit::GetDrawListSharedData);
decltype(&reference::overload_357) volatile api_357 =
    static_cast<decltype(&reference::overload_357)>(&imkit::GetStyleColorName);
decltype(&reference::overload_358) volatile api_358 =
    static_cast<decltype(&reference::overload_358)>(&imkit::SetStateStorage);
decltype(&reference::overload_359) volatile api_359 =
    static_cast<decltype(&reference::overload_359)>(&imkit::GetStateStorage);
decltype(&reference::overload_360) volatile api_360 =
    static_cast<decltype(&reference::overload_360)>(&imkit::CalcTextSize);
decltype(&reference::overload_361) volatile api_361 =
    static_cast<decltype(&reference::overload_361)>(&imkit::ColorConvertU32ToFloat4);
decltype(&reference::overload_362) volatile api_362 =
    static_cast<decltype(&reference::overload_362)>(&imkit::ColorConvertFloat4ToU32);
decltype(&reference::overload_363) volatile api_363 =
    static_cast<decltype(&reference::overload_363)>(&imkit::ColorConvertRGBtoHSV);
decltype(&reference::overload_364) volatile api_364 =
    static_cast<decltype(&reference::overload_364)>(&imkit::ColorConvertHSVtoRGB);
decltype(&reference::overload_365) volatile api_365 =
    static_cast<decltype(&reference::overload_365)>(&imkit::IsKeyDown);
decltype(&reference::overload_366) volatile api_366 =
    static_cast<decltype(&reference::overload_366)>(&imkit::IsKeyPressed);
decltype(&reference::overload_367) volatile api_367 =
    static_cast<decltype(&reference::overload_367)>(&imkit::IsKeyReleased);
decltype(&reference::overload_368) volatile api_368 =
    static_cast<decltype(&reference::overload_368)>(&imkit::IsKeyChordPressed);
decltype(&reference::overload_369) volatile api_369 =
    static_cast<decltype(&reference::overload_369)>(&imkit::GetKeyPressedAmount);
decltype(&reference::overload_370) volatile api_370 =
    static_cast<decltype(&reference::overload_370)>(&imkit::GetKeyName);
decltype(&reference::overload_371) volatile api_371 =
    static_cast<decltype(&reference::overload_371)>(&imkit::SetNextFrameWantCaptureKeyboard);
decltype(&reference::overload_372) volatile api_372 =
    static_cast<decltype(&reference::overload_372)>(&imkit::Shortcut);
decltype(&reference::overload_373) volatile api_373 =
    static_cast<decltype(&reference::overload_373)>(&imkit::SetNextItemShortcut);
decltype(&reference::overload_374) volatile api_374 =
    static_cast<decltype(&reference::overload_374)>(&imkit::SetItemKeyOwner);
decltype(&reference::overload_375) volatile api_375 =
    static_cast<decltype(&reference::overload_375)>(&imkit::IsMouseDown);
decltype(&reference::overload_376) volatile api_376 =
    static_cast<decltype(&reference::overload_376)>(&imkit::IsMouseClicked);
decltype(&reference::overload_377) volatile api_377 =
    static_cast<decltype(&reference::overload_377)>(&imkit::IsMouseReleased);
decltype(&reference::overload_378) volatile api_378 =
    static_cast<decltype(&reference::overload_378)>(&imkit::IsMouseDoubleClicked);
decltype(&reference::overload_379) volatile api_379 =
    static_cast<decltype(&reference::overload_379)>(&imkit::IsMouseReleasedWithDelay);
decltype(&reference::overload_380) volatile api_380 =
    static_cast<decltype(&reference::overload_380)>(&imkit::GetMouseClickedCount);
decltype(&reference::overload_381) volatile api_381 =
    static_cast<decltype(&reference::overload_381)>(&imkit::IsMouseHoveringRect);
decltype(&reference::overload_382) volatile api_382 =
    static_cast<decltype(&reference::overload_382)>(&imkit::IsMousePosValid);
decltype(&reference::overload_383) volatile api_383 =
    static_cast<decltype(&reference::overload_383)>(&imkit::IsAnyMouseDown);
decltype(&reference::overload_384) volatile api_384 =
    static_cast<decltype(&reference::overload_384)>(&imkit::GetMousePos);
decltype(&reference::overload_385) volatile api_385 =
    static_cast<decltype(&reference::overload_385)>(&imkit::GetMousePosOnOpeningCurrentPopup);
decltype(&reference::overload_386) volatile api_386 =
    static_cast<decltype(&reference::overload_386)>(&imkit::IsMouseDragging);
decltype(&reference::overload_387) volatile api_387 =
    static_cast<decltype(&reference::overload_387)>(&imkit::GetMouseDragDelta);
decltype(&reference::overload_388) volatile api_388 =
    static_cast<decltype(&reference::overload_388)>(&imkit::ResetMouseDragDelta);
decltype(&reference::overload_389) volatile api_389 =
    static_cast<decltype(&reference::overload_389)>(&imkit::GetMouseCursor);
decltype(&reference::overload_390) volatile api_390 =
    static_cast<decltype(&reference::overload_390)>(&imkit::SetMouseCursor);
decltype(&reference::overload_391) volatile api_391 =
    static_cast<decltype(&reference::overload_391)>(&imkit::SetNextFrameWantCaptureMouse);
decltype(&reference::overload_392) volatile api_392 =
    static_cast<decltype(&reference::overload_392)>(&imkit::GetClipboardText);
decltype(&reference::overload_393) volatile api_393 =
    static_cast<decltype(&reference::overload_393)>(&imkit::SetClipboardText);
int main() {
    const auto presets = imkit::ThemePresets();
    const auto theme = imkit::MakeTheme(imkit::ThemePreset::Ocean);
    return presets.size() == 12 && theme.scheme == imkit::ColorScheme::Dark &&
                   imkit::ThemeScaleMinimum == .5f &&
                   imkit::ThemeScaleDefault == 1.25f &&
                   imkit::ThemeScaleMaximum == 2.5f
               ? 0
               : 1;
}
