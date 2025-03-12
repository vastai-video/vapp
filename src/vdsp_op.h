#ifndef _VAPP_VDSP_OP_H_
#define _VAPP_VDSP_OP_H_

#include "log.h"
#include "md5.h"
#include "rt_util.h"
#ifdef __linux__
#include <pthread.h>
int vapp_usleep(unsigned usec);
#endif
#include <inttypes.h>
#define PAD_ALIGN(x, a) (((x)+(a)-1)&~((a)-1))

#ifdef ANDROID
#define DEFAULT_OP_PATH "/vendor/etc/op/"
#elif  __linux__
#define DEFAULT_OP_PATH "/opt/vastai/vaststream/lib/op/ext_op/video/"
#elif _WIN32
#define DEFAULT_OP_PATH "C\:\\vastai\\"
#endif


//ELF_FILE
#define ELF_FILE_NAME "vapp_op" //contains all ops

//geometry
#define RESIZE_ELF_NAME "scale_ext_op"
#define RESIZE_OP_FUNC "opf_scale_u8"
#define RESIZE_ELF_NAME_MULTI "resize_roi_ext_op"
#define RESIZE_OP_FUNC_MULTI "resize_multi_core_op"
#define CROP_ELF_NAME "crop_ext_op"
#define CROP_OP_FUNC "img_crop_u8_op"
#define ROTATE_ELF_NAME "simple_rotate_ext_op"
#define ROTATE_OP_FUNC "simple_rotate_op"
#define ROTATE_RGBA_INTERLEAVED_ELF_NAME "rotate_rgba_interleaved_ext_op"
#define ROTATE_RGBA_INTERLEAVED_OP_FUNC "rotate_rgba_interleaved_op"
#define ROTATE_RGBA_INTERLEAVED_ROI_ELF_NAME "rotate_rgba_interleaved_roi_ext_op"
#define ROTATE_RGBA_INTERLEAVED_ROI_OP_FUNC "rotate_rgba_interleaved_roi_op"
#define FLIP_ELF_NAME "flip_ext_op"
#define FLIP_OP_FUNC "flip_op"
#define ROIFLIP_ELF_NAME "flip_roi_ext_op"
#define ROIFLIP_OP_FUNC "flip_roi_op"
#define REMAP_ELF_NAME "remap_ext_op"
#define REMAP_OP_FUNC "remap_op"
#define ROICROP_ELF_NAME "img_crop_roi_u8_op"
#define ROICROP_OP_FUNC "img_crop_roi_u8_op"
#define WARPPERSPECTIVE_ELF_NAME "warp_perspective_u8_op"
#define WARPPERSPECTIVE_OP_FUNC "warp_perspective_u8_op"
#define TRANSLATE_ELF_NAME  "translateTransform_op"
#define TRANSLATE_OP_FUNC   "translateTransform"
#define PERMUTE_ELF_NAME    "permute_roi_op"
#define PERMUTE_OP_FUNC     "permute_roi_op"
#define TRANSPOSE_ELF_NAME    "ffmpeg_transpose_ext_op"
#define TRANSPOSE_OP_FUNC     "ffmpeg_transpose_op"
#define CROPSCALE_ELF_NAME    "crop_scales_ext_op"
#define CROPSCALE_OP_FUNC     "crop_scales_op"

//csc
#define CVTCOLOR_ELF_NAME "cvtcolor_op"
#define CVTCOLOR_OP_FUNC "opf_cvtcolor_u8"
#define CVTCOLOR_ELF_NAME_ASYNC "cvtcolor_roi_ext_op"
#define CVTCOLOR_OP_FUNC_ASYNC "cvtcolor_roi_ext_op"
#define NV12CSC_ELF_NAME "color_space_conversion_ext_op"
#define NV12CSC_OP_FUNC "color_space_op"
#define BAYERTONV12_ELF_NAME "bayer_convert_ext_op"
#define BAYERTONV12_OP_FUNC "bayer_to_nv12_op"

//arithmetic and logical
#define NV12OVERLAY_ELF_NAME "ffmpeg_overlay_ext_op"
#define NV12OVERLAY_OP_FUNC "ffmpeg_overlay_op"
#define BITDEPTHCVT_ELF_NAME "bit10_bit8_op"
#define BITDEPTHCVT_OP_FUNC "bit10_bit8_op"
#define ADAPTIVETHRESHOLD_ELF_NAME "adaptivethreshold_ext_op"
#define ADAPTIVETHRESHOLD_OP_FUNC "adaptiveThreshold"

//filtering functions 
#define UNSHARP_ELF_NAME "unsharp_op"
#define UNSHARP_OP_FUNC "unsharp_u8_op"
#define EQ_ELF_NAME "eq_op"
#define EQ_OP_FUNC "eq_u8_op"

#define HQDN3D_ELF_NAME "hqdn3d_op"
#define HQDN3D_OP_FUNC "hqdn3d_op"

#define CAS_ELF_NAME "ffmpeg_cas_ext_op"
#define CAS_OP_FUNC "opf_ffmpeg_cas"

#define SAD_ELF_NAME "sad_ext_op"
#define SAD_OP_FUNC "sad_u8_op"

#define DETECTION_ELF_NAME "detection_op"
#define DETECTION_OP_FUNC "detection_u8_op"

#define CVTCOLOR_U8_ELF_NAME "cvtcolor_ext_op"
#define CVTCOLOR_U8_OP_FUNC "opf_cvtcolor_u8"

typedef struct {
    const char * elf;
    const char * func_name;
    int reg;
    uint32_t pfun;
}VappOpEntrys;

extern VappOpEntrys  vapp_op_entrys[OP_RESERVED];

#define GET_OUTPUT_FLAG 16
#define ASYNC_CASE

#ifdef __cplusplus
extern "C"// C++
{
#endif

typedef struct {
    int height;
    int width;
    int h_pitch;
    int w_pitch;
} customized_shape_t;

typedef struct {
    int32_t y;  ///< roi start point in height direction
    int32_t x;  ///< roi start point in width direction
    int32_t h;    ///< roi length in height direction
    int32_t w;    ///< roi length in width direction
} img_2d_roi_t;


enum ImageFormat_enum {
    YUV_NV12    = 0,
    YUV_I420    = 1,
    RGB_PLANAR  = 5000,
    BGR_PLANAR  = 5001,
    RGB888      = 5002,
    BGR888      = 5003,
    GRAY        = 5004,
    RGBA_PLANAR = 5005,
    RGBA_INTER  = 5006,
    IMAGE_FORMAT_BUTT
};

VappStatus vapp_run_op(unsigned int devID, op_params *op_par);
VappStatus vapp_run_op_multi(unsigned int devID, op_params *op_par);
VappStatus vapp_run_op_multi_async(unsigned int devID, op_params *op_par, VastStream * stream);
int64_t time_usec(void);
VappStatus vapp_register_op(unsigned int devID, OpRegister *op_reg, uint32_t * entry, char * name, char * func);
VappStatus vapp_register_op_stream(unsigned int devID, VastStream * stream,void *reserved);
VappStatus vapp_find_op_entry(VappOpFunc op, VastStream * stream);
void print_op_params(RTStream* rt_stream);
#ifdef __cplusplus
}
#endif

#endif