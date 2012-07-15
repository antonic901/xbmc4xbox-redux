/* jconfig.vc --- jconfig.h for Microsoft Visual C++ on Windows 95 or NT. */
/* see jconfig.txt for explanations */

#define JPEG_LIB_VERSION 80
#define LIBJPEG_TURBO_VERSION 1.2.0
#define C_ARITH_CODING_SUPPORTED
#define D_ARITH_CODING_SUPPORTED

#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
/* #define void char */
/* #define const */
#undef CHAR_IS_UNSIGNED
#define HAVE_STDDEF_H
#define HAVE_STDLIB_H
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS	/* we presume a 32-bit flat memory model */
#undef NEED_SHORT_EXTERNAL_NAMES
#undef INCOMPLETE_TYPES_BROKEN

/* Define "boolean" as unsigned char, not int, per Windows custom */
#ifndef __RPCNDR_H__		/* don't conflict if rpcndr.h already read */
typedef unsigned char boolean;
#endif
#define HAVE_BOOLEAN		/* prevent jmorecfg.h from redefining it */

/* Define "INT32" as int, not long, per Windows custom */
#if !(defined(_BASETSD_H_) || defined(_BASETSD_H))   /* don't conflict if basetsd.h already read */
typedef short INT16;
typedef signed int INT32;
#endif
#define XMD_H                   /* prevent jmorecfg.h from redefining it */

#ifdef JPEG_INTERNALS

#undef RIGHT_SHIFT_IS_UNSIGNED

#endif /* JPEG_INTERNALS */

#define jcopy_block_row tb_jcopy_block_row
#define jcopy_sample_rows tb_jcopy_sample_rows
#define jdiv_round_up tb_jdiv_round_up
#define jinit_1pass_quantizer tb_jinit_1pass_quantizer
#define jinit_2pass_quantizer tb_jinit_2pass_quantizer
#define jinit_c_coef_controller tb_jinit_c_coef_controller
#define jinit_c_main_controller tb_jinit_c_main_controller
#define jinit_c_master_control tb_jinit_c_master_control
#define jinit_c_prep_controller tb_jinit_c_prep_controller
#define jinit_color_converter tb_jinit_color_converter
#define jinit_color_deconverter tb_jinit_color_deconverter
#define jinit_compress_master tb_jinit_compress_master
#define jinit_d_coef_controller tb_jinit_d_coef_controller
#define jinit_d_main_controller tb_jinit_d_main_controller
#define jinit_d_post_controller tb_jinit_d_post_controller
#define jinit_downsampler tb_jinit_downsampler
#define jinit_forward_dct tb_jinit_forward_dct
#define jinit_huff_decoder tb_jinit_huff_decoder
#define jinit_huff_encoder tb_jinit_huff_encoder
#define jinit_input_controller tb_jinit_input_controller
#define jinit_inverse_dct tb_jinit_inverse_dct
#define jinit_marker_reader tb_jinit_marker_reader
#define jinit_marker_writer tb_jinit_marker_writer
#define jinit_master_decompress tb_jinit_master_decompress
#define jinit_memory_mgr tb_jinit_memory_mgr
#define jinit_merged_upsampler tb_jinit_merged_upsampler
#define jinit_phuff_decoder tb_jinit_phuff_decoder
#define jinit_phuff_encoder tb_jinit_phuff_encoder
#define jinit_upsampler tb_jinit_upsampler
#define jpeg_CreateCompress tb_jpeg_CreateCompress
#define jpeg_CreateDecompress tb_jpeg_CreateDecompress
#define jpeg_abort tb_jpeg_abort
#define jpeg_abort_compress tb_jpeg_abort_compress
#define jpeg_abort_decompress tb_jpeg_abort_decompress
#define jpeg_add_quant_table tb_jpeg_add_quant_table
#define jpeg_alloc_huff_table tb_jpeg_alloc_huff_table
#define jpeg_alloc_quant_table tb_jpeg_alloc_quant_table
#define jpeg_calc_jpeg_dimensions tb_jpeg_calc_jpeg_dimensions
#define jpeg_calc_output_dimensions tb_jpeg_calc_output_dimensions
#define jpeg_consume_input tb_jpeg_consume_input
#define jpeg_copy_critical_parameters tb_jpeg_copy_critical_parameters
#define jpeg_core_output_dimensions tb_jpeg_core_output_dimensions
#define jpeg_default_colorspace tb_jpeg_default_colorspace
#define jpeg_default_qtables tb_jpeg_default_qtables
#define jpeg_destroy tb_jpeg_destroy
#define jpeg_destroy_compress tb_jpeg_destroy_compress
#define jpeg_destroy_decompress tb_jpeg_destroy_decompress
#define jpeg_fdct_float tb_jpeg_fdct_float
#define jpeg_fdct_ifast tb_jpeg_fdct_ifast
#define jpeg_fdct_islow tb_jpeg_fdct_islow
#define jpeg_fill_bit_buffer tb_jpeg_fill_bit_buffer
#define jpeg_finish_compress tb_jpeg_finish_compress
#define jpeg_finish_decompress tb_jpeg_finish_decompress
#define jpeg_finish_output tb_jpeg_finish_output
#define jpeg_free_large tb_jpeg_free_large
#define jpeg_free_small tb_jpeg_free_small
#define jpeg_gen_optimal_table tb_jpeg_gen_optimal_table
#define jpeg_get_large tb_jpeg_get_large
#define jpeg_get_small tb_jpeg_get_small
#define jpeg_has_multiple_scans tb_jpeg_has_multiple_scans
#define jpeg_huff_decode tb_jpeg_huff_decode
#define jpeg_idct_1x1 tb_jpeg_idct_1x1
#define jpeg_idct_2x2 tb_jpeg_idct_2x2
#define jpeg_idct_4x4 tb_jpeg_idct_4x4
#define jpeg_idct_float tb_jpeg_idct_float
#define jpeg_idct_ifast tb_jpeg_idct_ifast
#define jpeg_idct_islow tb_jpeg_idct_islow
#define jpeg_input_complete tb_jpeg_input_complete
#define jpeg_make_c_derived_tbl tb_jpeg_make_c_derived_tbl
#define jpeg_make_d_derived_tbl tb_jpeg_make_d_derived_tbl
#define jpeg_mem_available tb_jpeg_mem_available
#define jpeg_mem_dest tb_jpeg_mem_dest
#define jpeg_mem_init tb_jpeg_mem_init
#define jpeg_mem_src tb_jpeg_mem_src
#define jpeg_mem_term tb_jpeg_mem_term
#define jpeg_new_colormap tb_jpeg_new_colormap
#define jpeg_open_backing_store tb_jpeg_open_backing_store
#define jpeg_quality_scaling tb_jpeg_quality_scaling
#define jpeg_read_coefficients tb_jpeg_read_coefficients
#define jpeg_read_header tb_jpeg_read_header
#define jpeg_read_raw_data tb_jpeg_read_raw_data
#define jpeg_read_scanlines tb_jpeg_read_scanlines
#define jpeg_resync_to_restart tb_jpeg_resync_to_restart
#define jpeg_save_markers tb_jpeg_save_markers
#define jpeg_set_colorspace tb_jpeg_set_colorspace
#define jpeg_set_defaults tb_jpeg_set_defaults
#define jpeg_set_linear_quality tb_jpeg_set_linear_quality
#define jpeg_set_marker_processor tb_jpeg_set_marker_processor
#define jpeg_set_quality tb_jpeg_set_quality
#define jpeg_simple_progression tb_jpeg_simple_progression
#define jpeg_start_compress tb_jpeg_start_compress
#define jpeg_start_decompress tb_jpeg_start_decompress
#define jpeg_start_output tb_jpeg_start_output
#define jpeg_std_error tb_jpeg_std_error
#define jpeg_stdio_dest tb_jpeg_stdio_dest
#define jpeg_stdio_src tb_jpeg_stdio_src
#define jpeg_suppress_tables tb_jpeg_suppress_tables
#define jpeg_write_coefficients tb_jpeg_write_coefficients
#define jpeg_write_m_byte tb_jpeg_write_m_byte
#define jpeg_write_m_header tb_jpeg_write_m_header
#define jpeg_write_marker tb_jpeg_write_marker
#define jpeg_write_raw_data tb_jpeg_write_raw_data
#define jpeg_write_scanlines tb_jpeg_write_scanlines
#define jpeg_write_tables tb_jpeg_write_tables
#define jround_up tb_jround_up
#define jzero_far tb_jzero_far
