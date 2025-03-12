 
#ifndef VAST_VAPPIDEFS_H
#define VAST_VAPPIDEFS_H

#include <stdlib.h>
#define __align__(n) __attribute__((aligned(n)))

/**
 * \file nppdefs.h
 * Typedefinitions and macros for VAPP library.
 */
 
#ifdef __cplusplus
extern "C" {
#endif

#define SUB_MODULE_MAX_OPS 100
#define TEST_START 1000
// Workaround for cuda_fp16.h C incompatibility
typedef 
struct
{
   short fp16;
}
Vapp16f;

typedef
struct
{
   short fp16_0;
   short fp16_1;
}
Vapp16f_2;

#define VAPP_HALF_TO_VAPP16F(pHalf) (* reinterpret_cast<Vapp16f *>((void *)(pHalf)))

// If this is a 32-bit Windows compile, don't align to 16-byte at all
        // and use a "union-trick" to create 8-byte alignment.
#if defined(_WIN32) && !defined(_WIN64)

            // On 32-bit Windows platforms, do not force 8-byte alignment.
            //   This is a consequence of a limitation of that platform.
    #define VAPP_ALIGN_8
            // On 32-bit Windows platforms, do not force 16-byte alignment.
            //   This is a consequence of a limitation of that platform.
    #define VAPP_ALIGN_16

#else /* _WIN32 && !_WIN64 */

    #define VAPP_ALIGN_8     __align__(8)
    #define VAPP_ALIGN_16    __align__(16)

#endif /* !__CUDACC__ && _WIN32 && !_WIN64 */


/** \defgroup typedefs_npp VAPP Type Definitions and Constants
 * Definitions of types, structures, enumerations and constants available in the library.
 * @{
 */

/** 
 * Filtering methods.
 */
typedef enum 
{
    VAPPI_RESIZE_NO_RESIZE,       /**< no resize */
    VAPPI_RESIZE_BILINEAR,        /**< bilinear according to Xtensa Imaging(XI) library */
    VAPPI_RESIZE_NEAREST,         /**< nearest (obsolete), use vaceRESIZE_NEAREST_CV instead */
    VAPPI_RESIZE_BICUBIC,         /**< bicubic */
    VAPPI_RESIZE_LANCZOS,         /**< lanczos */
    VAPPI_RESIZE_BILINEAR_PILLOW, /**< bilinear according to PILLOW library */
    VAPPI_RESIZE_BILINEAR_CV,     /**< bilinear according to OpenCV library */
    VAPPI_RESIZE_LANCZOS_PILLOW,  /**< lanczos according to PILLOW library */
    VAPPI_RESIZE_LANCZOS_CV,      /**< lanczos according to OpenCV library */
    VAPPI_RESIZE_BOX_PILLOW,      /**< box_filter according to PILLOW library */
    VAPPI_RESIZE_HAMMING_PILLOW,  /**< hamming according to PILLOW library */
    VAPPI_RESIZE_BICUBIC_PILLOW,  /**< bicubic according to PILLOW library */
    VAPPI_RESIZE_NEAREST_CV,      /**< nearest according to OpenCV library */
    VAPPI_RESIZE_BUTT             /**< reserved */    
} VappiInterpolationMode; 

/**
 * Error Status Codes
 *
 * Almost all VAPP function return error-status information using
 * these return codes.
 * Negative return codes indicate errors, positive return codes indicate
 * warnings, a return code of 0 indicates success.
 */
typedef enum 
{
    /* negative return-codes indicate errors */
    VAPP_NOT_SUPPORTED_MODE_ERROR            = -9999,  
    
    VAPP_REGISTER_ERROR                      = -1036,
    VAPP_GET_ENTRY_ERROR                     = -1035,
    VAPP_DEVICE_STATUS_ERROR                 = -1034,
    VAPP_DEVICE_INIT_ERROR                   = -1033,
    VAPP_INVALID_HOST_POINTER_ERROR          = -1032,
    VAPP_INVALID_DEVICE_POINTER_ERROR        = -1031,
    VAPP_LUT_PALETTE_BITSIZE_ERROR           = -1030,
    VAPP_ZC_MODE_NOT_SUPPORTED_ERROR         = -1028,      /**<  ZeroCrossing mode not supported  */
    VAPP_NOT_SUFFICIENT_COMPUTE_CAPABILITY   = -1027,
    VAPP_TEXTURE_BIND_ERROR                  = -1024,
    VAPP_WRONG_INTERSECTION_ROI_ERROR        = -1020,
    VAPP_HAAR_CLASSIFIER_PIXEL_MATCH_ERROR   = -1006,
    VAPP_MEMFREE_ERROR                       = -1005,
    VAPP_MEMSET_ERROR                        = -1004,
    VAPP_MEMCPY_ERROR                        = -1003,
    VAPP_ALIGNMENT_ERROR                     = -1002,
    VAPP_CUDA_KERNEL_EXECUTION_ERROR         = -1000,

    VAPP_ROUND_MODE_NOT_SUPPORTED_ERROR      = -213,     /**< Unsupported round mode*/
    
    VAPP_QUALITY_INDEX_ERROR                 = -210,     /**< Image pixels are constant for quality index */

    VAPP_RESIZE_NO_OPERATION_ERROR           = -201,     /**< One of the output image dimensions is less than 1 pixel */

    VAPP_OVERFLOW_ERROR                      = -109,     /**< Number overflows the upper or lower limit of the data type */
    VAPP_NOT_EVEN_STEP_ERROR                 = -108,     /**< Step value is not pixel multiple */
    VAPP_HISTOGRAM_NUMBER_OF_LEVELS_ERROR    = -107,     /**< Number of levels for histogram is less than 2 */
    VAPP_LUT_NUMBER_OF_LEVELS_ERROR          = -106,     /**< Number of levels for LUT is less than 2 */

    VAPP_CORRUPTED_DATA_ERROR                = -61,      /**< Processed data is corrupted */
    VAPP_CHANNEL_ORDER_ERROR                 = -60,      /**< Wrong order of the destination channels */
    VAPP_ZERO_MASK_VALUE_ERROR               = -59,      /**< All values of the mask are zero */
    VAPP_QUADRANGLE_ERROR                    = -58,      /**< The quadrangle is nonconvex or degenerates into triangle, line or point */
    VAPP_RECTANGLE_ERROR                     = -57,      /**< Size of the rectangle region is less than or equal to 1 */
    VAPP_COEFFICIENT_ERROR                   = -56,      /**< Unallowable values of the transformation coefficients   */

    VAPP_NUMBER_OF_CHANNELS_ERROR            = -53,      /**< Bad or unsupported number of channels */
    VAPP_COI_ERROR                           = -52,      /**< Channel of interest is not 1, 2, or 3 */
    VAPP_DIVISOR_ERROR                       = -51,      /**< Divisor is equal to zero */

    VAPP_CHANNEL_ERROR                       = -47,      /**< Illegal channel index */
    VAPP_STRIDE_ERROR                        = -37,      /**< Stride is less than the row length */
    
    VAPP_ANCHOR_ERROR                        = -34,      /**< Anchor point is outside mask */
    VAPP_MASK_SIZE_ERROR                     = -33,      /**< Lower bound is larger than upper bound */
    VAPP_NO_ELF_FILE_ERROR                   = -32,
    VAPP_RESIZE_FACTOR_ERROR                 = -23,
    VAPP_INTERPOLATION_ERROR                 = -22,
    VAPP_MIRROR_FLIP_ERROR                   = -21,
    VAPP_MOMENT_00_ZERO_ERROR                = -20,
    VAPP_THRESHOLD_NEGATIVE_LEVEL_ERROR      = -19,
    VAPP_THRESHOLD_ERROR                     = -18,
    VAPP_CONTEXT_MATCH_ERROR                 = -17,
    VAPP_FFT_FLAG_ERROR                      = -16,
    VAPP_FFT_ORDER_ERROR                     = -15,
    VAPP_STEP_ERROR                          = -14,       /**<  Step is less or equal zero */
    VAPP_SCALE_RANGE_ERROR                   = -13,
    VAPP_DATA_TYPE_ERROR                     = -12,
    VAPP_OUT_OFF_RANGE_ERROR                 = -11,
    VAPP_DIVIDE_BY_ZERO_ERROR                = -10,
    VAPP_MEMORY_ALLOCATION_ERR               = -9,
    VAPP_NULL_POINTER_ERROR                  = -8,
    VAPP_RANGE_ERROR                         = -7,
    VAPP_SIZE_ERROR                          = -6,
    VAPP_BAD_ARGUMENT_ERROR                  = -5,
    VAPP_NO_MEMORY_ERROR                     = -4,
    VAPP_NOT_IMPLEMENTED_ERROR               = -3,
    VAPP_ERROR                               = -2,
    VAPP_ERROR_RESERVED                      = -1,
    
    /* success */
    VAPP_NO_ERROR                            = 0,        /**<  Error free operation */
    VAPP_SUCCESS = VAPP_NO_ERROR,                         /**<  Successful operation (same as VAPP_NO_ERROR) */

    /* positive return-codes indicate warnings */
    VAPP_NO_OPERATION_WARNING                = 1,        /**<  Indicates that no operation was performed */
    VAPP_DIVIDE_BY_ZERO_WARNING              = 6,        /**<  Divisor is zero however does not terminate the execution */
    VAPP_AFFINE_QUAD_INCORRECT_WARNING       = 28,       /**<  Indicates that the quadrangle passed to one of affine warping functions doesn't have necessary properties. First 3 vertices are used, the fourth vertex discarded. */
    VAPP_WRONG_INTERSECTION_ROI_WARNING      = 29,       /**<  The given ROI has no interestion with either the source or destination ROI. Thus no operation was performed. */
    VAPP_WRONG_INTERSECTION_QUAD_WARNING     = 30,       /**<  The given quadrangle has no intersection with either the source or destination ROI. Thus no operation was performed. */
    VAPP_DOUBLE_SIZE_WARNING                 = 35,       /**<  Image size isn't multiple of two. Indicates that in case of 422/411/420 sampling the ROI width/height was modified for proper processing. */
    
    VAPP_MISALIGNED_DST_ROI_WARNING          = 10000,    /**<  Speed reduction due to uncoalesced memory accesses warning. */
   
} VappStatus;



typedef struct 
{
    int    major;   /**<  Major version number */
    int    minor;   /**<  Minor version number */
    int    build;   /**<  Build number. This reflects the nightly build this release was made from. */
} VappLibraryVersion;

/** \defgroup npp_basic_types Basic VAPP Data Types
 * Definitions of basic types available in the library.
 * @{
 */


typedef unsigned char       Vapp8u;     /**<  8-bit unsigned chars */
typedef signed char         Vapp8s;     /**<  8-bit signed chars */
typedef unsigned short      Vapp16u;    /**<  16-bit unsigned integers */
typedef short               Vapp16s;    /**<  16-bit signed integers */
typedef unsigned int        Vapp32u;    /**<  32-bit unsigned integers */
typedef int                 Vapp32s;    /**<  32-bit signed integers */
typedef unsigned long long  Vapp64u;    /**<  64-bit unsigned integers */
typedef long long           Vapp64s;    /**<  64-bit signed integers */
typedef float               Vapp32f;    /**<  32-bit (IEEE) floating-point numbers */
typedef double              Vapp64f;    /**<  64-bit floating-point numbers */


/**
 * 2D Size
 * This struct typically represents the size of a a rectangular region in
 * two space.
 */
typedef struct 
{
    int width;  /**<  Rectangle width. */
    int height; /**<  Rectangle height. */
} VappiSize;


typedef struct 
{
    int width;  /**<  Rectangle width. */
    int height; /**<  Rectangle height. */
    int wPitch;
    int hPitch;
} VappiShape2D;

/**
 * 2D Rectangle
 * This struct contains position and size information of a rectangle in 
 * two space.
 * The rectangle's position is usually signified by the coordinate of its
 * upper-left corner.
 */
typedef struct
{
    int x;          /**<  x-coordinate of upper left corner (lowest memory address). */
    int y;          /**<  y-coordinate of upper left corner (lowest memory address). */
    int width;      /**<  Rectangle width. */
    int height;     /**<  Rectangle height. */
} VappiRect;



typedef enum
{
    ROTATE_DEGREE_90 = 0,
    ROTATE_DEGREE_180 = 1,
    ROTATE_DEGREE_270 = 2,
    ROTATE_DEGREE_NEG270 = 3,
    ROTATE_DEGREE_NEG90 = 4,
    ROTATE_DEGREE_END
}VappiRotateDegree;


/**
 * nppiMirror direction controls
 */
typedef enum 
{
    VAPPI_VERTICAL_AXIS,      /**<  Flip around vertical axis in mirror function. */
    VAPPI_HORIZONTAL_AXIS,    /**<  Flip around horizontal axis in mirror function. */
    VAPPI_BOTH_AXIS = -1           /**<  Flip around both axes in mirror function. */
} VappiAxis;

typedef enum {
    VAPPI_UNSPECIFIED_M,
    VAPPI_RGB,
    VAPPI_REC_601,
    VAPPI_REC_709_M,
    VAPPI_FCC,
    VAPPI_SMPTE_240M_M,
    VAPPI_YCGCO,
    VAPPI_REC_2020_NCL,
    VAPPI_REC_2020_CL,
    VAPPI_CHROMATICITY_DERIVED_NCL,
    VAPPI_CHROMATICITY_DERIVED_CL,
    VAPPI_REC_2100_LMS,
    VAPPI_REC_2100_ICTCP,
} VappiMatrixCoefficients;

typedef enum {
    VAPPI_UNSPECIFIED_T,
    VAPPI_LINEAR,
    VAPPI_LOG_100,
    VAPPI_LOG_316,
    VAPPI_REC_709_T,
    VAPPI_REC_470_M_T,
    VAPPI_REC_470_BG_T,
    VAPPI_SMPTE_240M_T,
    VAPPI_XVYCC,
    VAPPI_SRGB,
    VAPPI_ST_2084,
    VAPPI_ARIB_B67,
} VappiTransferCharacteristics;

typedef enum {
    VAPPI_UNSPECIFIED_C,
    VAPPI_REC_470_M_C,
    VAPPI_REC_470_BG_C,
    VAPPI_SMPTE_C,
    VAPPI_REC_709_C,
    VAPPI_FILM,
    VAPPI_REC_2020,
    VAPPI_XYZ,
    VAPPI_DCI_P3,
    VAPPI_DCI_P3_D65,
    VAPPI_JEDEC_P22,
} VappiColorPrimaries;


typedef struct {
    VappiMatrixCoefficients      matrix;         ///< Conversion matrix of YUV/RGB
    VappiTransferCharacteristics transfer;       ///< Conversion function between linear RGB and nonlinear RGB
    VappiColorPrimaries          primaries;      ///< Conversion matrix of RGB/XYZ
} VappiColorSpace;

typedef struct {
    int      plane;
    int      block_size;
    int      text_enable;
    int      extend_enable;
    int      grident_enable;
    int      laplacian_enable;
    int      hist_enable;
    int      morpho_enable;
    int      static_enable;

    int      qp_offset_static;      //-5
    int      qp_offset_static_text; //-7
    int      hist_bin_size;         //8
    int      max_bin_size;          //3
    int      laplacian_th;          //2000
    int      gradient_th;           //10
    float    hist_percentage_th;    //0.65
    float    morphological_th;      //0.5
} VappiTextDetectionParam;

typedef struct {
    Vapp8u *in_img_addr;
    Vapp8s *out_roi_map_final_addr;
    Vapp8u *out_gray_addr;
    Vapp8u *pre_gray_addr;
    Vapp8u *out_sobelx_addr;
    Vapp8u *out_sobely_addr;
    Vapp8u *out_laplacian_addr;
}VappiTextDetectionBuffers;

typedef enum {
    TRANSPOSE_CCLOCK_FLIP,  ///< rotate by 90 degrees counterclockwise and vertically flip
    TRANSPOSE_CLOCK,        ///< rotate by 90 degrees clockwise
    TRANSPOSE_CCLOCK,       ///< rotate by 90 degrees counterclockwise
    TRANSPOSE_CLOCK_FLIP,   ///< rotate by 90 degrees clockwise and vertically flip
} VappiTransposeDirection;

enum ColorSpace_enum {
    COLOR_SPACE_BT709_FULL_RANGE = 0,
    COLOR_SPACE_BT709 = COLOR_SPACE_BT709_FULL_RANGE,
    COLOR_SPACE_BT601_LIMIT_RANGE = 1,
    COLOR_SPACE_BT601 = COLOR_SPACE_BT601_LIMIT_RANGE,
    COLOR_SPACE_BT709_LIMIT_RANGE = 2,
    COLOR_SPACE_BT601_FULL_RANGE = 3,
    COLOR_SPACE_BUTT
};

#ifdef __cplusplus
} /* extern "C" */
#endif

/*@}*/
 
#endif /* NV_VAPPIDEFS_H */
