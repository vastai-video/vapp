 
#ifndef VAST_VAPPI_H
#define VAST_VAPPI_H
 
#ifdef _WIN32
#ifdef LIBVAPP_EXPORTS
#define VAPP_API __declspec(dllexport)
#else
#define VAPP_API __declspec(dllimport)
#endif
#else
#define VAPP_API
#endif
/*
 * \file vappi.h
 * VAPP Image Processing Functionality.
 */
 
#include "vappdefs.h"
#include "vast_runtime.h"
#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup vappi VAPP Image Processing
 * The set of image processing functions available in the library.
 * @{
 */


/**
 * 3 channel 8-bit unsigned planar image resize.
 *
 * For common parameter descriptions, see \ref CommonResizeSqrPlanarPixelParameters.
 *
 */
VAPP_API VappStatus
vappiYUV420Resize_8u_P3(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, int eInterpolation);


VAPP_API VappStatus 
vappiNV12Resize_8u_P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u **pDst, VappiShape2D *oDstShape, int eInterpolation, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiYUV420Resize_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u **pDst, VappiShape2D *oDstShape, int eInterpolation, vastStream_t vastStreamCtx);

VappStatus 
vappiYUV420_1_In_N_out_Resize_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u **pDst, VappiShape2D *oDstShape,int oOutnumber,int eInterpolation, vastStream_t vastStreamCtx);

VappStatus 
vappiNV12_1_In_N_out_Resize_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u **pDst, VappiShape2D *oDstShape,int oOutnumber,int eInterpolation, vastStream_t vastStreamCtx);
/**
 * 3 channel 8-bit unsigned image resize.
 *
 * For common parameter descriptions, see \ref CommonResizeSqrPackedPixelParameters.
 *
 */
VAPP_API VappStatus
vappiNV12Resize_8u_P2(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, int eInterpolation);

VAPP_API VappStatus
vappiResize_8u_C3(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, int eInterpolation);

VAPP_API VappStatus 
vappiResizePlus_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, int nCropwidth, int nCropHeight, int eInterpolation, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiResizePlus_8u_P1_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, VappiRect oCropSize, int eInterpolation, vastStream_t vastStreamCtx);                            

VAPP_API VappStatus
vappiResize_8u_C3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, int eInterpolation, vastStream_t vastStreamCtx);                         

VAPP_API VappStatus 
vappiResize_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, int eInterpolation, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiResize_8u_P1_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, int eInterpolation, vastStream_t vastStreamCtx);

VAPP_API VappStatus
vappiCrop_8u_C3(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiRect oCropSize, int nDstStep);

VAPP_API VappStatus
vappiCrop_8u_C3R_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiRect oCropSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx);

VAPP_API VappStatus
vappiCrop_8u_P3R_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiRect oCropSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiCrop_8u_P1R_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiRect oCropSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiRGBAPLANARRotate_8u_P4(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, int nAngle);
VAPP_API VappStatus 
vappiRGBAPLANARRotate_8u_P4_Ctx_single(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, int nAngle, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiRGBAPLANARRotate_8u_P4_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, int nAngle, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx);

/**
 * 2 channel 8-bit unsigned image mirror.
 *
 * For common parameter descriptions, see \ref CommonMirrorParameters.
 *
 */

VAPP_API VappStatus
vappiYUV420Mirror_8u_P3(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, VappiAxis eAxis);  

VAPP_API VappStatus
vappiNV12Mirror_8u_P2(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, VappiAxis eAxis);  

VAPP_API VappStatus
vappiMirror_8u_P3R_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, VappiAxis eAxis, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx);    
VAPP_API VappStatus 
vappiMirror_8u_P1R_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, VappiAxis eAxis, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx);                                                    


VAPP_API VappStatus
vappiRemap_8u_P3R_Ctx(unsigned int devID, const Vapp8u * const pSrc, const Vapp32f * const pMap1, const Vapp32f * const pMap2, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, VappiSize oDstSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiRemap_8u_P1R_Ctx(unsigned int devID, const Vapp8u * const pSrc, const Vapp32f * const pMap1, const Vapp32f * const pMap2, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, VappiSize oDstSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiWrapPerspective_8u_P3R_Ctx(unsigned int devID, const Vapp8u * const pSrc, const Vapp64f * const pM,  VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, VappiSize oDstSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx);


VAPP_API VappStatus 
vappiWrapPerspective_8u_P1R_Ctx(unsigned int devID, const Vapp8u * const pSrc, const Vapp64f * const pM,  VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, VappiSize oDstSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx);
VAPP_API VappStatus  
vappiRemapFixedMap_8u_P1R_Ctx(unsigned int devID, const Vapp8u * const pSrc, const Vapp16s * const pMap1, const Vapp16u * const pMap2, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, VappiSize oDstSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx);                           

//color space conversion 
/** @name RGBToGray 
 *  RGB to CCIR601 Gray conversion.
 *
 *  Here is how VAPP converts gamma corrected RGB to CCIR601 Gray.
 *
 *  \code   
 *   nGray =  0.299F * R + 0.587F * G + 0.114F * B; 
 *  \endcode
 *
 * @{
 *
 */

/**
 * 3 channel 8-bit unsigned packed RGB to 1 channel 8-bit unsigned packed Gray conversion.
 *
 * \param pSrc \ref source_image_pointer.
 * \param nSrcStep \ref source_image_line_step.
 * \param pDst \ref destination_image_pointer.
 * \param nDstStep \ref destination_image_line_step.
 * \param oSizeROI \ref roi_specification.
 * \return \ref image_data_error_codes, \ref roi_error_codes
 */
VAPP_API VappStatus vappiColorToGray_8u_C3C1(unsigned int devID, const Vapp8u * pSrc, VappiSize oSrcSize, int nSrcStep, Vapp8u * pDst, int nDstStep);

VAPP_API VappStatus vappiColorToGray_8u_C3C1R_Ctx(unsigned int devID, const Vapp8u * pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx);

VAPP_API VappStatus vappiColorP2C_8u_P3C3R_Ctx(unsigned int devID, const Vapp8u * pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx);

VAPP_API VappStatus vappiColorC2P_8u_C3P3R_Ctx(unsigned int devID, const Vapp8u * pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, int nDstStep, int nRoiNumber, VappiRect *pSizeROI,  vastStream_t vastStreamCtx);

/** @defgroup rgbtoyuv RGBToYUV 
 *  RGB to YUV color conversion.
 *
 *  Here is how VAPP converts gamma corrected RGB or BGR to YUV. For digital RGB values in the range [0..255], 
 *  Y has the range [0..255], U varies in the range [-112..+112], 
 *  and V in the range [-157..+157]. To fit in the range of [0..255], a constant value
 *  of 128 is added to computed U and V values, and V is then saturated.
 *
 *  \code   
 *  Vapp32f nY =  0.299F * R + 0.587F * G + 0.114F * B; 
 *  Vapp32f nU = (0.492F * ((Vapp32f)nB - nY)) + 128.0F;
 *  Vapp32f nV = (0.877F * ((Vapp32f)nR - nY)) + 128.0F;
 *  if (nV > 255.0F) 
 *      nV = 255.0F;
 *  \endcode
 *
 * @{
 *
 */

/**
 * 3 channel 8-bit unsigned packed RGB to 3 channel 8-bit unsigned planar YUV420 color conversion.
 *
 * \param pSrc \ref source_image_pointer.
 * \param nSrcStep \ref source_image_line_step.
 * \param pDst \ref destination_planar_image_pointer_array.
 * \param rDstStep \ref destination_planar_image_line_step_array.
 * \param oSizeROI \ref roi_specification.
 * \return \ref image_data_error_codes, \ref roi_error_codes
 */
VAPP_API VappStatus vappiRGBToYUV420_8u_C3P3R(const Vapp8u * pSrc, int nSrcStep, Vapp8u * pDst[3], int rDstStep[3], VappiSize oSizeROI);
                                                                                                                                        

VAPP_API VappStatus 
vappiAdaptiveThreshold_8u_P1R_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, 
                            Vapp64f nThreshold, int nBlockSize, int nMaxValue, vastStream_t vastStreamCtx);                                                                          
/** @} end of Image Processing module */



VAPP_API VappStatus 
vappiTranslateTransform_8u_P1R_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, 
                            Vapp64f nOffsetX, Vapp64f nOffsetY, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiTranspose_8u_P1R_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                        Vapp8u *pDst, int nDstStep, int nRoiNumber, VappiRect *pSizeROI,vastStream_t vastStreamCtx);                            

VAPP_API VappStatus
vappiYUV420SAD_8u_P3(unsigned int devID,const Vapp8u * const pSrc, const Vapp8u * const pSrc2,
                        VappiSize oSrcSize, int nSrcStep, const Vapp8u * const pDst);

VAPP_API VappStatus
vappiStaticTextDetection_8u_P3_ctx(unsigned int devID, VappiTextDetectionBuffers *buffers, VappiSize oSrcSize, Vapp32u nSrcStep,
                        VappiTextDetectionParam *param, Vapp8u first_frame, vastStream_t vastStreamCtx);

VAPP_API VappStatus
vappiStaticTextDetection_8u_P3_Host(unsigned int devID, VappiTextDetectionBuffers *buffers, VappiSize oSrcSize, Vapp32u nSrcStep,
                        VappiTextDetectionParam *param, Vapp8u first_frame);

VAPP_API VappStatus 
vappiNV12Unsharp_8u_Px_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, float nLumaAmount, float nChromAmount, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiYUV420Unsharp_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, float nLumaAmount, float nChromAmount, vastStream_t vastStreamCtx);

VAPP_API VappStatus
vappiNV12Hqdn3d_8u_Px_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, float luma_temporal  , float chroma_temporal   , vastStream_t vastStreamCtx);

VAPP_API VappStatus
vappiYUV420Hqdn3d_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, float luma_temporal  , float chroma_temporal   , vastStream_t vastStreamCtx);

VAPP_API VappStatus
vappiYUV420Mirror_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx, VappiAxis eAxis);
VAPP_API VappStatus
vappiNV12Mirror_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx, VappiAxis eAxis);

VAPP_API VappStatus
vappiYUV420EQ_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, float brightness, float contrast, float saturation, vastStream_t vastStreamCtx);

VAPP_API VappStatus
vappiNV12EQ_8u_Px_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, float brightness, float contrast, float saturation, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiBitDepthConvert_Bit8ToBit10_8u_P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiBitDepthConvert_Bit10ToBit8_8u_P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiBayerGRGBToNV12_8u_C1P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiBayerRGGBToNV12_8u_C1P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiBayerBGGRToNV12_8u_C1P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiBayerGBGRToNV12_8u_C1P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx);

                            
VAPP_API VappStatus 
vappiNV12Rotate_8u_P3_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u *pDst, VappiShape2D oDstShape, int nAngle, vastStream_t vastStreamCtx);

VAPP_API VappStatus  
vappiYUV420Rotate_8u_P3_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u *pDst, VappiShape2D oDstShape, int nAngle, vastStream_t vastStreamCtx);


VAPP_API VappStatus 
vappiNV12CSC_8u_P2_Ctx(unsigned int devID,const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u *pDst, VappiShape2D oDstShape,VappiColorSpace oSrcColorSpace, Vapp8u nSrcFullRange,
                            VappiColorSpace oDstColorSpace, Vapp8u nDstFullRange,Vapp64f nPeakLuminance,
                            Vapp8u nApproximateGamma, Vapp8u nSceneReferred,vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiYUV420CSC_8u_P2_Ctx(unsigned int devID,const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u *pDst, VappiShape2D oDstShape,VappiColorSpace oSrcColorSpace, Vapp8u nSrcFullRange,
                            VappiColorSpace oDstColorSpace, Vapp8u nDstFullRange,Vapp64f nPeakLuminance,
                            Vapp8u nApproximateGamma, Vapp8u nSceneReferred,vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiYUV420Transpose_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx, VappiTransposeDirection eDirection);

VAPP_API VappStatus 
vappiNV12Transpose_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx, VappiTransposeDirection eDirection);


VAPP_API VappStatus 
vappiNV12Overlay_8u_P2_Ctx(unsigned int devID, const Vapp8u * const pSrc1, VappiShape2D oSrc1Shape,
                            Vapp8u *pSrc2, VappiShape2D  oSrc2Shape, int nX, int nY, vastStream_t vastStreamCtx);

VAPP_API VappStatus
vappiYUV420_1_In_N_out_Cropscale_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u **pDst, VappiShape2D *oDstShape, int offsetWidth, int offsetHeight, int cropWidth, int cropHeight,
                            int oOutnumber, int eInterpolation, vastStream_t vastStreamCtx);

VAPP_API VappStatus
vappiNV12_1_In_N_out_Cropscale_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u **pDst, VappiShape2D *oDstShape, int offsetWidth, int offsetHeight, int cropWidth, int cropHeight,
                            int oOutnumber, int eInterpolation, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiRGBA2NV12_8u_C4P2R_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u **pDst, VappiShape2D *oDstShape,int eCvtSpace, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiRGBPLANARCas_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, Vapp8u * pDst, VappiShape2D oDstShape, 
                             float nStrength, int nPlanes, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiYUV420Cas_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, Vapp8u * pDst, VappiShape2D oDstShape, 
                             float nStrength, int nPlanes, vastStream_t vastStreamCtx);

VAPP_API VappStatus 
vappiNV12Cas_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, Vapp8u * pDst, VappiShape2D oDstShape, 
                             float nStrength, int nPlanes, vastStream_t vastStreamCtx);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NV_VAPPI_H */
