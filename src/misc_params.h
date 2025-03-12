#ifndef VAPP_SRC_MISC_PARAMS_H
#define VAPP_SRC_MISC_PARAMS_H

#include <stdint.h> 
#include <string.h>
#include "vapp.h"
#include "vdsp_op.h"
#define MAX_CHAN_ROTATE 4
#define RUN_CUSTOM  30000

//typedef enum {
//     PILLOW_FILTER_BOX = 0x1000,  ///< pillow box filter method
//     PILLOW_FILTER_BILINEAR,      ///< pillow bilinear filter method
//     PILLOW_FILTER_HAMMING,       ///< pillow hamming filter method, unsupported now
//     PILLOW_FILTER_BICUBIC,       ///< pillow bicubic filter method
//     PILLOW_FILTER_LANCZOS,       ///< pillow lanczos filter method

//     CV_FILTER_BILINEAR = 0x2000,  ///< OpenCV INTER_LINEAR method
//     CV_FILTER_LANCZOS,            ///< OpenCV INTER_LANCZOS method, unsupported now
//     CV_FILTER_NEAREST,            ///< OpenCV INTER_NEAREST method
//     CV_FILTER_AREA                ///< OpenCV INTER_AREA method

// } img_interpolate_method_t;


// typedef struct {
//     customized_shape_t iimage_shape;
//     int32_t            oimage_cnt;
//     uint32_t           resize_type; // BILINEAR or BICUBIC or LANCOZ
//     int32_t            image_format;      //ImageFormat_enum: support YUV_NV12 and YUV_I420
//     customized_shape_t oimage_shape[16];
// } customized_example_t;
typedef struct {
    int height;
    int width;
    int h_pitch;
    int w_pitch;
} shape_with_pitch_t, yuv_nv12_shape_t, image_shape_layout_t,customized_example_t;

typedef struct {
    shape_with_pitch_t iimage_shape;      //!< input image shape
    int32_t            oimage_cnt;        //!< output image count, maximum 16
    uint32_t           resize_type;       //!< RESIZE_TYPE:BILINEAR, BICUBIC, LANCZOS, BILINEAR_CV
    int32_t            image_format;      //!< ImageFormat_enum: support YUV_NV12 and YUV_I420
    shape_with_pitch_t oimage_shape[16];  //!< output image shape list
} scale_u8_t;

typedef enum {
    _XI_TILE_YUV_NV12_TYPE_ = 0,  ///< YUV_NV12
    _XI_TILE_YUV_I420_TYPE_,      ///< YUV_I420
    _XI_TILE_RGB888_TYPE_,        ///< RGB888
    _XI_TILE_RGB_PLANAR_TYPE_,    ///< RGB planar

    _XI_TILE_BAYER_BG_TYPE_,  ///< Bayer BG
    _XI_TILE_BAYER_GB_TYPE_,  ///< Bayer GB
    _XI_TILE_BAYER_RG_TYPE_,  ///< Bayer RG
    _XI_TILE_BAYER_GR_TYPE_,  ///< Bayer GR

    _XI_TILE_GRAY_TYPE_,          ///< gray
    _XI_TILE_BGR888_TYPE_,        ///< BGR888
    _XI_TILE_RGBA_TYPE_,        ///< RGBARGBA...
} img_format_t;

typedef struct {
    img_format_t         in_coding;      ///< image format
    customized_shape_t in_img;  ///< source image shape
    customized_shape_t out_img;  ///< dest image shape
    int                  x;             ///< crop left-top x coord
    int                  y;             ///< crop left-top x coord
} crop_para_t;

typedef struct {
    img_format_t       in_coding; ///< image format
    customized_shape_t in_img;    ///< source image shape
    customized_shape_t out_img;   ///< dest image shape
    img_2d_roi_t       roi;       ///< Only process the ROI area of the target image.
    int                x;         ///< crop left-top x coord
    int                y;         ///< crop left-top x coord
} crop_roi_para_t;

typedef struct {
    customized_shape_t in_img;
    customized_shape_t out_img;
    int32_t            in_coding;   //ImageFormat_enum
    int32_t            out_coding;  //ImageFormat_enum
    VappiInterpolationMode  method;
    int32_t            color_space;  //ColorSpac_enum
    int32_t            block_num;
    int32_t            block_id;
} resize_multi_core_op_t;



typedef struct{
    customized_shape_t in_img;
    customized_shape_t out_img;
}rotate_param_chan_t;

typedef enum
{
    DATA_TYPE_U8 = 0,
    DATA_TYPE_F16 = 1,
    DATA_TYPE_F32 = 2,
    DATA_TYPE_END
}input_data_type_e;

typedef enum
{
    IMAGE_DEFAULT = 0,                  //single plane, no need do any de-interleave or interleave
    IMAGE_NV12 = 1,
}image_type_e;


typedef enum
{
    FLIP_IMAGE_NVI2 = 0,                  //single plane, no need do any de-interleave or interleave
    FLIP_IMAGE_YUV420P = 1,
}flip_format_e;

typedef struct
{
    int32_t channel;
    VappiRotateDegree rotate_degree;
    input_data_type_e data_type;
    image_type_e image_type;
    rotate_param_chan_t rotate_param[MAX_CHAN_ROTATE];
}rotate_param_t;

typedef enum {
    ROTATE_ANGLE_0,
    ROTATE_ANGLE_90,
    ROTATE_ANGLE_180,
    ROTATE_ANGLE_270,
} rotate_angle_t;
typedef struct {
    uint32_t       in_width;
    uint32_t       in_height;
    uint32_t       in_pitch;
    uint32_t       out_width;
    uint32_t       out_height;
    uint32_t       out_pitch;
    rotate_angle_t angle;
    int32_t        block_num;
    int32_t        block_id;
} rotate_rgba_interleaved_para_t;

typedef struct {
    uint32_t       in_width;
    uint32_t       in_height;
    uint32_t       in_pitch;
    uint32_t       out_width;
    uint32_t       out_height;
    uint32_t       out_pitch;
    rotate_angle_t angle;
    img_2d_roi_t   roi;
} rotate_rgba_interleaved_roi_para_t;

// enum flip_direction {
//     VERTICAL   = 0,
//     HORIZONTAL = 1,
//     XY_FLIP    = -1,
// };

typedef struct {
    customized_shape_t    in_img;
    customized_shape_t    out_img;
    VappiAxis direction;
    int32_t             ImageFormat;  //0: YUV_NV12  other: planar
} yuv_nv12_flip_t;

typedef struct {
    customized_shape_t    in_img;
    customized_shape_t    out_img;
    VappiAxis direction;
    img_2d_roi_t        roi;
    int32_t             ImageFormat;  //5000: RGB_PLANAR 5001: BGR_PLANAR 5004:GRAY
} roi_flip_t;



//remap cfg
typedef enum {
    REMAP_BORDER_CONSTANT = 0,  //!< `iiiiii|abcdefgh|iiiiiii`  with some specified `i`
} RemapBorderType;

typedef enum {
    REMAP_INTER_LINEAR  = 1,
} RemapInterpolationType;

typedef enum {
    REMAP_PLANAR = 0,
} RemapImageType;

typedef enum {
	REMAP_MAP_32FC1 = 0,
	REMAP_MAP_32FC2 = 1,
	REMAP_MAP_16SC2 = 2,
	REMAP_MAP_16UC1 = 3,
} RemapMapType;                 //current supported map1&map2 type is both REMAP_MAP_32FC1, or map1 type is REMAP_MAP_16SC2 and map2 type is REMAP_MAP_16UC1.

typedef struct {
    int32_t height;
    int32_t width;
    int32_t h_pitch;
    int32_t w_pitch;
} remap_map_shape_t;

typedef struct {
    uint64_t            addr[1];
    remap_map_shape_t   shape;
    RemapMapType        type;
} remap_map_t;

typedef struct {
    int32_t channel;
    int32_t height;
    int32_t width;
    int32_t c_pitch;
    int32_t h_pitch;
    int32_t w_pitch;
} remap_image_shape_t;

typedef struct {
    uint64_t            addr[1];
    remap_image_shape_t shape;
    RemapImageType      type;
} remap_image_t;

typedef struct {
    int32_t c_start;
    int32_t h_start;
    int32_t w_start;
    int32_t c_len;
    int32_t h_len;
    int32_t w_len;
} remap_roi_t;            //region of interest of output, for multi core processing

typedef struct {
	remap_image_shape_t        in_shape;   //
	remap_image_shape_t        out_shape;  //
	RemapImageType             image_type;
	remap_map_shape_t          map1;
	remap_map_shape_t          map2;
	RemapMapType               map1_type;
	RemapMapType               map2_type;
	RemapInterpolationType     inter_type;
	RemapBorderType            border_type;
	int32_t                    border_value;
	remap_roi_t                roi;
} remap_cfg_t;

typedef enum {
    WARP_PERSPECTIVE_BORDER_CONSTANT = 0,  //!< `iiiiii|abcdefgh|iiiiiii`  with some specified `i`
} WarpPerspectiveBorderType;

typedef enum {
    //    WARP_PERSPECTIVE_INTER_NEAREST = 0,
    WARP_PERSPECTIVE_INTER_LINEAR = 1,
    //    WARP_PERSPECTIVE_INTER_CUBIC   = 2,
} WarpPerspectiveInterpolationType;

typedef enum {
    WARP_PERSPECTIVE_FORWARD_MAP = 0,
    WARP_PERSPECTIVE_INVERSE_MAP = 16,
} WarpPerspectiveFlags;

typedef enum {
    WARP_PERSPECTIVE_PLANAR = 0,
} WarpPerspectiveImageType;

typedef struct {
    int32_t channel;
    int32_t height;
    int32_t width;
    int32_t c_pitch;
    int32_t h_pitch;
    int32_t w_pitch;
} warp_perspective_image_shape_t;

typedef struct {
    uint64_t                       addr[1];
    warp_perspective_image_shape_t shape;
    WarpPerspectiveImageType       type;
} warp_perspective_image_t;

typedef struct {
    int32_t c_start;
    int32_t h_start;
    int32_t w_start;
    int32_t c_len;
    int32_t h_len;
    int32_t w_len;
} warp_perspective_roi_t;  //region of interest of output, for multi core processing

typedef struct {
    warp_perspective_image_shape_t   in_image_shape;   //
    warp_perspective_image_shape_t   out_image_shape;  //
    WarpPerspectiveImageType         image_type;
    double                           M[9];  //transform matrix
    WarpPerspectiveInterpolationType inter_type;
    WarpPerspectiveBorderType        border_type;
    int32_t                          border_value;//0
    WarpPerspectiveFlags             flag;//0
    warp_perspective_roi_t           roi;
} warp_perspective_cfg_t;


//-------- Struct definition --------
//Picture information for translateTransform
typedef struct {
    int32_t channel;  //Channel of actual part of one picture
    int32_t height;   //Height of actual part of one picture
    int32_t width;    //Width of actual part of one picture
    int32_t c_pitch;  //channel of whole part of one picture
    int32_t h_pitch;  //Height of whole part of one picture
    int32_t w_pitch;  //width of whole part of one picture
} translateTransform_picture_shape_t;

//Image information for translateTransform
typedef struct {
    int32_t pos_x;    //X position of one image in entire picture
    int32_t pos_y;    //Y position of one image in entire picture
    int32_t pos_z;    //Z position of one image in entire picture
    int32_t channel;  //Channel of actual part of one image
    int32_t height;   //Height of actual part of one image
    int32_t width;    //Width of actual part of one image
} translateTransform_image_shape_t;

//Information of offset
typedef struct {
    double offset_x;  //Offset of one image in x direction
    double offset_y;  //Offset of one image in y direction
} translateTransform_image_offset_t;

//Configuration for translateTransform
typedef struct {
    translateTransform_picture_shape_t pic;           //Parameters for entire picture
    translateTransform_image_shape_t   in_image;      //Parameters for input image
    translateTransform_image_offset_t  image_offset;  //Parameters for image offset
} translateTransform_cfg_t;


// transpose
typedef struct {
    uint32_t iimage_width;
    uint32_t iimage_height;
} image_shape_t;

typedef struct {
    image_shape_t    iimage_shape;
    image_shape_t    oimage_shape;
    int32_t          channel;
    int32_t          ele_bytes;
} permute_op_t;

typedef struct {
    image_shape_t    iimage_shape;
    image_shape_t    oimage_shape;
    img_2d_roi_t     roi;
    int32_t          channel;
    int32_t          ele_bytes;
} permute_roi_op_t;

//eq
typedef struct {
    float                  contrast;     //luma
    float                  brightness;   //luma
    float                  saturation;   //chroma
} eq_image_adjust_para_t;

typedef struct {
    img_format_t           img_type;      ///< image format
    customized_shape_t     in_image_shape;   ///< image shape
    customized_shape_t     out_image_shape;   ///< image shape
    eq_image_adjust_para_t image_eq;      ///
} eq_para_t;

typedef struct {
    img_format_t           img_type;      ///< image format
    customized_shape_t     in_image_shape;   ///< image shape
} sad_para_t;

typedef struct {
    img_format_t            img_type;         ///< image format
    customized_shape_t      in_image_shape;   ///< in image shape
    customized_shape_t      out_image_shape;  ///< out image shape
    VappiTextDetectionParam params;
    int                     first_frame;
} dection_para_t;

typedef struct {
    float lamount;  ///< -2--5,luma effect strength 0时保持原样，>0 锐化，<0 模糊
    float camount;  ///< -2--5,chroma effect strength 0时保持原样，>0 锐化，<0 模糊
} unsharp_image_para_t;

typedef struct {
    int                    img_type;              ///< image format  0--nv12,  1--yuv420p
    shape_with_pitch_t     in_image_shape;           ///< image shape
    shape_with_pitch_t     out_image_shape;           ///< image shape
    unsharp_image_para_t   image_unsharp_param;   ///
} unsharp_para_t;

//hqdn3d
enum hqdn3d_enum {
    denoise_temporal = 0,
};

typedef struct {
    shape_with_pitch_t     in_image_shape;
    shape_with_pitch_t     out_image_shape;
    float          luma_temporal;
    float          chroma_temporal;
    int32_t        first_flag;         ///< 1: first frame of the video
    enum hqdn3d_enum    hqdn3d_enum;
    int32_t        image_type;      ///< image format
} hqdn3d_t;

//transpose same with ffmpeg
typedef struct {
    int                     in_width;
    int                     in_height;
    int                     in_w_pitch;
    int                     in_h_pitch;
    int                     out_width;
    int                     out_height;
    int                     out_w_pitch;
    int                     out_h_pitch;
    img_format_t            img_type;
    VappiTransposeDirection direction;  ///< transpose direction
} transpose_op_t;


typedef enum {
    BIT_10_TO_8,// P010(10bit) to NV12
    BIT_8_TO_10,// NV12 to 10LE planar
    BIT_MAX,
}ConvertType;

typedef struct {
    customized_shape_t       in_img;
    customized_shape_t       out_img;
    ConvertType       type;
} BitDepthCvt_t;

//overlay cfg
typedef struct {
    customized_shape_t img;
    uint16_t    x;
    uint16_t    y;
} overlay_layer;

typedef struct {
    customized_shape_t back_img;
    overlay_layer layer_img;
} overlay_cfg;

typedef enum
{
  VAPPI_BAYER_RGGB, //CV_BayerBG,
  VAPPI_BAYER_GRBG, //CV_BayerGB,
  VAPPI_BAYER_BGGR, //CV_BayerRG,
  VAPPI_BAYER_GBRG  //CV_BayerGR
}VappiBayerType;

typedef struct
{
  VappiBayerType bType;           //输入bayer图像格式
  int32_t iimage_width;         //输入bayer图像宽度
  int32_t iimage_height;        //输入bayer图像高度
  int32_t oimage_width;         //输出nv12图像宽度
  int32_t oimage_height;        //输出nv12图像高度
  uint32_t iimage_width_pitch; 	//输入bayer图像宽度pitch
  uint32_t iimage_height_pitch;	//输入bayer图像高度pitch
  uint32_t oimage_width_pitch;	//输出nv12图像宽度pitch
  uint32_t oimage_height_pitch;	//输出nv12图像高度pitch
}bayer_cvt_color_t;

typedef struct {
    customized_shape_t       in_img;         ///< input image shape
    customized_shape_t       out_img;        ///< output image shape
    VappiColorSpace in_color_space;       ///< input image color space
    VappiColorSpace out_color_space;      ///< output image color space
    char              fullrange_in;         ///< input is PC range
    char              fullrange_out;        ///< output is PC range
    double            peak_luminance;       ///< nominal peak luminance for SDR (cd/m^2)
    /**
    * If allow_approximate_gamma is set, out-of-range pixels may be clipped,
    * which could interfere with further processing of image highlights.
    */
    char approximate_gamma;                 ///< use LUT to evaluate transfer functions
    /**
    * When speaking of images captured by a camera, scene-referred means that the
    * intensities in the image RGB channels are proportional to the intensities in
    * the scene that was photographed.
    */
    char scene_referred;                    ///< use scene-referred transfer functions
} color_space_conversion_op_t;

typedef enum {
    SCALE_LANCZOS,
    SCALE_BILINEAR,
    SCALE_BICUBIC,
    SCALE_BUTT
} ScaleMethodType;

typedef struct {
    shape_with_pitch_t input_shape;
    uint16_t offset_w;
    uint16_t offset_h;
    uint16_t crop_width;
    uint16_t crop_height;
    ScaleMethodType scale_method;
    uint8_t oimage_cnt;
    shape_with_pitch_t output_shape[16];
} cropscale_op_t;

enum ColorCvtCode_enum {
    //YUV 4:2:0 family to RGB
    COLOR_YUV2RGB_NV12 = 0,              //!< RGB_PLANAR
    COLOR_YUV2BGR_NV12 = 1,              //!< BGR_PLANAR
    COLOR_NO_CHANGE    = 2,              //!< unsupported in cvtcolor
    COLOR_BGR2RGB      = 3,              //!< BGR to RGB PLANAR
    COLOR_RGB2BGR      = COLOR_BGR2RGB,  //!< RGB to BGR PLANAR

    COLOR_BGR2RGB_INTERLEAVE2PLANAR,  //!< BGR888 to RGB planar
    COLOR_RGB2BGR_INTERLEAVE2PLANAR,  //!< RGB888 to BGR planar
    COLOR_BGR2BGR_INTERLEAVE2PLANAR,  //!< BGR888 to BGR planar
    COLOR_RGB2RGB_INTERLEAVE2PLANAR,  //!< RGB888 to RGB planar
    COLOR_YUV2GRAY_NV12,              //!< YUV_NV12 to GRAY
    COLOR_BGR2GRAY_INTERLEAVE,        //!< BGR888 to GRAY
    COLOR_BGR2GRAY_PLANAR,            //!< BGR planar to GRAY
    COLOR_RGB2GRAY_INTERLEAVE,        //!< RGB888 to GRAY
    COLOR_RGB2GRAY_PLANAR,            //!< RGB planar to GRAY
    COLOR_RGB2YUV_NV12_PLANAR,        //!< RGB_PLANAR to YUV NV12
    COLOR_BGR2YUV_NV12_PLANAR,        //!< BGR_PLANAR to YUV NV12

    COLOR_3CH_PLANAR2INTERLEAVE,  //!<
    COLOR_RGB2YUV_NV12_888,       //!< RGB888 to YUV NV12
    COLOR_YUV2RGB_NV12_888,       //!< YUV NV12 to RGB888

    COLOR_YUV_NV12_TO_I420,  //!< YUV_NV12 to YUV_I420

    COLOR_BGR2YUV_NV12_888,       //!< BGR888 to YUV NV12
    COLOR_YUV2BGR_NV12_888,       //!< YUV NV12 to BGR888
    COLOR_YUV_I420_TO_NV12,
    COLOR_ARGB2YUV_NV12_8888,
    COLOR_BGRA2YUV_NV12_8888,
    COLOR_CVT_CODE_BUTT  //!< only means end of the enum
};
typedef uint16_t ColorSpaceUint16_t;
typedef struct {
    int                    iimage_width;
    int                    iimage_height;
    int                    oimage_width;
    int                    oimage_height;
    uint32_t               iimage_width_pitch;  ///< pitch is in pixel unit
    uint32_t               iimage_height_pitch;
    uint32_t               oimage_width_pitch;
    uint32_t               oimage_height_pitch;
    enum ColorCvtCode_enum cvt_type;

    //! only valid when cvt_type == COLOR_RGB2YUV_NV12_PLANAR or COLOR_BGR2YUV_NV12_PLANAR or COLOR_RGB2YUV_NV12_888 or COLOR_YUV2RGB_NV12_888
    ColorSpaceUint16_t color_space;
    uint16_t           rsvd;
} cvt_color_t;

typedef struct {
    uint32_t     width;
    uint32_t     height;
    uint32_t     in_w_pitch;
    uint32_t     in_h_pitch;
    uint32_t     out_w_pitch;
    uint32_t     out_h_pitch;
    img_format_t img_type;
    float        strength;  ///< sharpening strength
    int32_t      planes;    ///< what planes to filter
} cas_param_t;

typedef struct{
    op_params op_par;

    int image_format;
    union{
        scale_u8_t scale_cfg;
        overlay_cfg ovl_cfg;
        resize_multi_core_op_t scale_cfg_multi;
        crop_para_t crop_cfg;
        crop_roi_para_t crop_roi_cfg;
        rotate_param_t rotete_cfg;
        rotate_rgba_interleaved_para_t rotete_rgba_interleaved_cfg;
        rotate_rgba_interleaved_roi_para_t rotete_rgba_interleaved_roi_cfg;
        yuv_nv12_flip_t flip_cfg;
        roi_flip_t  roi_flip_cfg;
        remap_cfg_t remap_cfg;
        warp_perspective_cfg_t warp_perspective_cfg;
        translateTransform_cfg_t translateTransform_cfg;
        permute_roi_op_t permute_roi_op_cfg;
        eq_para_t eq_cfg;
        unsharp_para_t unsharp_cfg;
        hqdn3d_t hqdn3d_cfg;
        sad_para_t sad_cfg;
        dection_para_t dect_param;
        BitDepthCvt_t bit_cvt_cfg;
        bayer_cvt_color_t bayer_cfg;
        color_space_conversion_op_t csc_cfg;
        transpose_op_t transpose_cfg;
        cropscale_op_t cropscale_cfg;
        cvt_color_t cvtcolor_cfg;
        cas_param_t cas_cfg;
    };
    //VappiRect crop_rect;
    int nOutputs;
    customized_shape_t  in_img;
    customized_shape_t  out_img;  
    VappiRect * pSizeROI;  
    int eInterpolation;
    VappiShape * p_size_roi; 
    int nChaNum;
    void* ctx;
}geometry_input_params;

//-------- Struct definition --------
//-------- Enum definition --------
//Method
typedef enum {
    ADAPTIVE_THRESH_MEAN_C = 0,  //Replicate in border
} adaptiveThreshold_method_e;

//Type
typedef enum {
    CV_THRESH_BINARY_INV = 0,  //mean - data >= floor(threshold)
} adaptiveThreshold_type_e;
//Picture information for adaptiveThreshold
typedef struct {
    int32_t channel;  //Channel of actual part of one picture
    int32_t height;   //Height of actual part of one picture
    int32_t width;    //Width of actual part of one picture
    int32_t c_pitch;  //channel of whole part of one picture
    int32_t h_pitch;  //Height of whole part of one picture
    int32_t w_pitch;  //width of whole part of one picture
} adaptiveThreshold_picture_shape_t;

//Image information for adaptiveThreshold
typedef struct {
    int32_t x; // int32_t pos_x;    //X position of one image in entire picture
    int32_t y; // int32_t pos_y;    //Y position of one image in entire picture
    int32_t z; // int32_t pos_z;    //Z position of one image in entire picture
    int32_t c; // int32_t channel;  //Channel of actual part of one image
    int32_t h; // int32_t height;   //Height of actual part of one image
    int32_t w; // int32_t width;    //Width of actual part of one image
} adaptiveThreshold_image_shape_t;

//Parameters for max value, threshold, block size
typedef struct {
    double                     threshold;  //Threshold for mean - data >= floor(threshold)
    int32_t                    blockSize;  //Block size for average, must be odd, 3 ~ 27
    int32_t                    maxValue;   //Max value in output, actually uint8, 0 ~ 255
    adaptiveThreshold_method_e method;     //Only support ADAPTIVE_THRESH_MEAN_C
    adaptiveThreshold_type_e   type;       //Only support CV_THRESH_BINARY_INV
} adaptiveThreshold_mean_para_t;

//Configuration for adaptiveThreshold
typedef struct {
    adaptiveThreshold_picture_shape_t in_shape;       //Parameters for entire picture
    adaptiveThreshold_image_shape_t   roi;  //Parameters for input image
    adaptiveThreshold_mean_para_t     meanPara;  //Parameters for max value, threshold, block size
} adaptiveThreshold_cfg_t;

typedef struct{
    op_params op_par;
    //int image_format;
    union{
        BitDepthCvt_t bit_cvt_cfg;
        adaptiveThreshold_cfg_t adaptive_threshold_cfg;
        //
    };
    int nOutputs; 
    VappiShape * p_size_roi; 
    void* ctx;
}al_input_params;

#endif