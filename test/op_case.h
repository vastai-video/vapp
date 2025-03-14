#ifndef VAST_OP_CASE_H
#define VAST_OP_CASE_H

#include <vapp.h>
#include <vappi.h>
#include <stdio.h>
#include <stdlib.h>
#if __linux__
#include <getopt.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#elif _WIN32
#include "getopt.h"
#include "windows.h"
#endif
#include <string.h>
#define MAX_THREAD_NUM 32
typedef struct {
    float                  contrast;     //luma
    float                  brightness;   //luma
    float                  saturation;   //chroma
} Vapp_eq;


#define vappSafeCall(expr, status)  {if(status == 0){ VappStatus code = expr ; if(code < 0) { fprintf (stderr, "Error code : %d (%d)\n", code, __LINE__); status = code;} } else{return -1;}} 
int64_t time_usec(void);
typedef struct {
    char **input_file;
    char *output_file;
    int input_count;
    VappiShape2D input_size;
    int layer_width;
    int layer_height;
    VappiShape2D output_size;
    int plane;
    int test_case;
    int frame_count;
    char * config_file;
    float contrast;     //luma
    float brightness;   //luma
    float saturation;   //chroma
    int device_id;
    int degree;
    char *elf_file;
} CommandLineArgs;


typedef struct{
    int plane_size;
    int out_plane_size;
    int image_size;
    int device_id; 
    int out_image_size;
    int out_image_size2;
    int out_roimap_size;
    int nSrcStep;
    int nDstStep;
    VappiSize oSrcSize;
    VappiShape2D inSrcSize;
    union{
        VappiSize oDstSize;
        VappiShape2D outSize;
        VappiRect oCropSize;
        Vapp_eq   EqParam;
    };
    Vapp8u* src_img;
    Vapp8u* src_img2;
    Vapp8u* dst_img;
    Vapp8u* dst_img2;
}OperatorCfg;
int run_test_case(CommandLineArgs *args);
int test_remap_crop(CommandLineArgs *args, OperatorCfg* op_cfg, FILE *fp_input, FILE *fp_out, Vapp8u * pSrcImg, Vapp8u * pDstImg);
int test_cycle_remap(CommandLineArgs *args, OperatorCfg* op_cfg, FILE *fp_input, FILE *fp_out, Vapp8u * pSrcImg, Vapp8u * pDstImg);
int test_cycle_crop_planar(CommandLineArgs *args, OperatorCfg* op_cfg, FILE *fp_input, FILE *fp_out, Vapp8u * pSrcImg, Vapp8u * pDstImg);
int test_fixed_remap(CommandLineArgs *args, OperatorCfg* op_cfg, FILE *fp_input, FILE *fp_out);
int test_c2p_remap(CommandLineArgs *args, OperatorCfg* iop_cfg, FILE *fp_input, FILE *fp_out, Vapp8u * pSrcImg, Vapp8u * pDstImg);
int test_adaptive_thresh(CommandLineArgs *args, OperatorCfg* op_cfg, FILE *fp_input, FILE *fp_out);
int test_gray_warpperspective(CommandLineArgs *args, OperatorCfg* op_cfg, FILE *fp_input);
int test_translate_transform(CommandLineArgs *args, OperatorCfg* op_cfg, FILE *fp_input);
int test_transpose(CommandLineArgs *args, OperatorCfg* op_cfg, FILE *fp_input);
int test_multi_stream();
int test_multi_stream2();
int process_one_stream(void *arg);
int process_one_stream2(void *arg);
int process_one_stream3(void *arg);
int test_remap_threshold(void *arg);


int test_multi_stream_remap_thresh(CommandLineArgs *args, OperatorCfg* cfg, FILE *fp_input, FILE *fp_out);
int test_static_text_detection(CommandLineArgs *args, OperatorCfg* op_cfg, FILE *fp_input, FILE *fp_out);

#endif
