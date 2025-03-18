#include "vapp.h"
#include "vdsp_op.h"
#include "misc_params.h"

typedef uint16_t ColorSpaceUint16_t;


typedef struct {
    int      image_width;          ///<
    int      image_height;         ///<
    uint32_t iimage_width_pitch;   ///< pitch is in pixel unit
    uint32_t iimage_height_pitch;  ///<
    uint32_t oimage_width_pitch;   ///<
    uint32_t oimage_height_pitch;  ///<

    enum ColorCvtCode_enum cvt_type;     ///<
    ColorSpaceUint16_t     color_space;  ///< only valid for YUV format
    img_2d_roi_t           roi;          ///< roi info
} cvtcolor_roi_extop_t;

typedef struct{
    op_params op_par;
    //int image_format;
    union{
        cvt_color_t cvtcolor_cfg;
        cvtcolor_roi_extop_t cvtcolor_roi_cfg;
    };
    VappiRect * pSizeROI; 
    int nOutputs; 
}csc_input_params;

static int config_color_conversion_op(void * priv_params, uint32_t entry)
{
    int i = 0;
    csc_input_params * in_params = (csc_input_params *)priv_params;
    op_info_t * op_info = &in_params->op_par.op_info;
    int nb_outputs = in_params->nOutputs;
    op_info->op_addr = entry;
    op_info->op_type = GET_OUTPUT_FLAG;//reserved
    /* argument */
    op_info->argument_size = sizeof(op_args_args_t);
    if(!op_info->argument)
    op_info->argument = calloc(1, op_info->argument_size);
    op_args_args_t *p = (op_args_args_t *)op_info->argument;    
    p->seperator = 556082218;
    p->loopCount = 1;
    p->batchSize = 1;
    p->configCount = 1;
    p->inputCount = 1;
    p->outputCount = nb_outputs;
    p->config = 0;//runtime 分配完config内存填写地址
    for(i =0; i < MAX_IN_OUT_ADDR; i++){
        if(in_params->op_par.inout_addr[i]){
            p->inout_addr[i] = in_params->op_par.inout_addr[i];
        }else{
            break;
        }
    }
    
    /* config */
    // in_params->op_par.config.config = (void *)cfg;
    // in_params->op_par.config.size = sizeof(*cfg);
    op_info->config_array = &in_params->op_par.config;
    op_info->config_num = 1;
    return 0;
}


static int update_cvtcolor_roi(void * priv_params)
{
    csc_input_params *in_params = (csc_input_params *)priv_params;
    cvtcolor_roi_extop_t *cfg = &in_params->cvtcolor_roi_cfg;

    cfg->roi.x = in_params->pSizeROI[in_params->op_par.block_id].x;
    cfg->roi.y = in_params->pSizeROI[in_params->op_par.block_id].y;
    cfg->roi.w = in_params->pSizeROI[in_params->op_par.block_id].width;
    cfg->roi.h = in_params->pSizeROI[in_params->op_par.block_id].height;    
    return 0;
}



VappStatus vappiRGBToGray_8u_C3P1(unsigned int devID, const Vapp8u * pSrc, VappiSize oSrcSize, int nSrcStep, Vapp8u * pDst, int nDstStep)
{
    rtError_t vaccRet;
    csc_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }

    if( !pSrc|| !pDst || !nSrcStep || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }    
    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }    
    in_params.op_par.elf_file = malloc(strlen(op_path) + strlen(CVTCOLOR_ELF_NAME) + 2);
    snprintf(in_params.op_par.elf_file, strlen(op_path) + strlen(CVTCOLOR_ELF_NAME) + 2, "%s%s", op_path, CVTCOLOR_ELF_NAME);
    in_params.op_par.custom_op_name = CVTCOLOR_OP_FUNC;     
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t)pDst;

    in_params.cvtcolor_cfg.iimage_width        = oSrcSize.width;    
    in_params.cvtcolor_cfg.iimage_height       = oSrcSize.height;
    in_params.cvtcolor_cfg.iimage_width_pitch  = nSrcStep;
    in_params.cvtcolor_cfg.iimage_height_pitch = oSrcSize.height;  

    in_params.cvtcolor_cfg.oimage_width        = oSrcSize.width;
    in_params.cvtcolor_cfg.oimage_height       = oSrcSize.height;
    in_params.cvtcolor_cfg.oimage_width_pitch  = nDstStep;
    in_params.cvtcolor_cfg.oimage_height_pitch = oSrcSize.height; 
    in_params.cvtcolor_cfg.cvt_type = COLOR_RGB2GRAY_INTERLEAVE;   

    in_params.nOutputs = 1;    
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.config_op_params = config_color_conversion_op;

    vaccRet = vapp_run_op(devID, &in_params.op_par);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        free(in_params.op_par.elf_file);
        return vaccRet;           
    }   

    free(in_params.op_par.elf_file);
    return VAPP_SUCCESS;       
}

VappStatus vappiRGBPToGray_8u_P3P1R_Ctx(unsigned int devID, 
                            const Vapp8u * pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx)
{
    rtError_t vaccRet;
    csc_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }

    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    if( !pSrc|| !pDst || !nSrcStep || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }    
    VastStream * stream = (VastStream *)vastStreamCtx;
    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }   

    vaccRet =  vapp_find_op_entry(OP_CVTCOLOR_ROI_EXTOP, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    }       
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t)pDst;

    in_params.cvtcolor_roi_cfg.image_width        = oSrcSize.width;    
    in_params.cvtcolor_roi_cfg.image_height       = oSrcSize.height;
    in_params.cvtcolor_roi_cfg.iimage_width_pitch  = nSrcStep;
    in_params.cvtcolor_roi_cfg.iimage_height_pitch = oSrcSize.height;  
    in_params.cvtcolor_roi_cfg.oimage_width_pitch  = nDstStep;
    in_params.cvtcolor_roi_cfg.oimage_height_pitch = oSrcSize.height;  
    in_params.cvtcolor_roi_cfg.cvt_type = COLOR_RGB2GRAY_PLANAR;  

    // in_params.cvtcolor_roi_cfg.roi.x = oSizeROI.x;
    // in_params.cvtcolor_roi_cfg.roi.y = oSizeROI.x;
    // in_params.cvtcolor_roi_cfg.roi.w = oSizeROI.width;
    // in_params.cvtcolor_roi_cfg.roi.h = oSizeROI.height;

    in_params.pSizeROI =  calloc(nRoiNumber, sizeof(VappiRect));
    memcpy(in_params.pSizeROI, pSizeROI, nRoiNumber* sizeof(VappiRect));    
    in_params.cvtcolor_roi_cfg.color_space = GRAY;
    in_params.nOutputs = 1;
    in_params.op_par.priv_params = &in_params;
    //in_params.op_par.block_num = BLOCK_NUM;
    in_params.op_par.block_num = nRoiNumber;
    // in_params.op_par.config.config = (void *)&in_params.cvtcolor_roi_cfg;
    // in_params.op_par.config.size = sizeof(in_params.cvtcolor_roi_cfg);    
    in_params.op_par.config_op_params = config_color_conversion_op;
    in_params.op_par.update_cfg = update_cvtcolor_roi;
    in_params.op_par.config.config = (void *)&in_params.cvtcolor_roi_cfg;
    in_params.op_par.config.size = sizeof(in_params.cvtcolor_roi_cfg);
    // stream->op_async->block_num = in_params.op_par.block_num;
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    }   
    free(in_params.pSizeROI); 
    return VAPP_SUCCESS;       
}

VappStatus vappiRGBP2RGB_8u_P3C3R_Ctx(unsigned int devID, 
                            const Vapp8u * pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx)
{
    rtError_t vaccRet;
    csc_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }

    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    if( !pSrc|| !pDst || !nSrcStep || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }    
    VastStream * stream = (VastStream *)vastStreamCtx;
    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }   

    vaccRet =  vapp_find_op_entry(OP_CVTCOLOR_ROI_EXTOP, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    }       
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t)pDst;

    in_params.cvtcolor_roi_cfg.image_width        = oSrcSize.width;    
    in_params.cvtcolor_roi_cfg.image_height       = oSrcSize.height;
    in_params.cvtcolor_roi_cfg.iimage_width_pitch  = nSrcStep;
    in_params.cvtcolor_roi_cfg.iimage_height_pitch = oSrcSize.height;  
    in_params.cvtcolor_roi_cfg.oimage_width_pitch  = nDstStep;
    in_params.cvtcolor_roi_cfg.oimage_height_pitch = oSrcSize.height;  
    in_params.cvtcolor_roi_cfg.cvt_type = COLOR_3CH_PLANAR2INTERLEAVE;  

    // in_params.cvtcolor_roi_cfg.roi.x = oSizeROI.x;
    // in_params.cvtcolor_roi_cfg.roi.y = oSizeROI.x;
    // in_params.cvtcolor_roi_cfg.roi.w = oSizeROI.width;
    // in_params.cvtcolor_roi_cfg.roi.h = oSizeROI.height;
    
    in_params.pSizeROI =  calloc(nRoiNumber, sizeof(VappiRect));
    memcpy(in_params.pSizeROI, pSizeROI, nRoiNumber* sizeof(VappiRect));   
    in_params.cvtcolor_roi_cfg.color_space = RGB888;
    in_params.nOutputs = 1;
    in_params.op_par.priv_params = &in_params;
    //in_params.op_par.block_num = 1;
    in_params.op_par.block_num = nRoiNumber;
    // in_params.op_par.config.config = (void *)&in_params.cvtcolor_roi_cfg;
    // in_params.op_par.config.size = sizeof(in_params.cvtcolor_roi_cfg);    
    in_params.op_par.config_op_params = config_color_conversion_op;
    in_params.op_par.update_cfg = update_cvtcolor_roi;
    in_params.op_par.config.config = (void *)&in_params.cvtcolor_roi_cfg;
    in_params.op_par.config.size = sizeof(in_params.cvtcolor_roi_cfg);
    // stream->op_async->block_num = in_params.op_par.block_num;
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    }   
    free(in_params.pSizeROI); 
    return VAPP_SUCCESS;       
}

VappStatus vappiRGB2RGBP_8u_C3P3R_Ctx(unsigned int devID, const Vapp8u * pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, int nDstStep, int nRoiNumber, VappiRect *pSizeROI,  vastStream_t vastStreamCtx)
{
    rtError_t vaccRet;
    csc_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }

    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    if( !pSrc|| !pDst || !nSrcStep || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }    
    VastStream * stream = (VastStream *)vastStreamCtx;
    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }   

    vaccRet =  vapp_find_op_entry(OP_CVTCOLOR_ROI_EXTOP, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    }       
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t)pDst;

    in_params.cvtcolor_roi_cfg.image_width        = oSrcSize.width;    
    in_params.cvtcolor_roi_cfg.image_height       = oSrcSize.height;
    in_params.cvtcolor_roi_cfg.iimage_width_pitch  = nSrcStep;
    in_params.cvtcolor_roi_cfg.iimage_height_pitch = oSrcSize.height;  
    in_params.cvtcolor_roi_cfg.oimage_width_pitch  = nDstStep;
    in_params.cvtcolor_roi_cfg.oimage_height_pitch = oSrcSize.height;  
    in_params.cvtcolor_roi_cfg.cvt_type = COLOR_RGB2RGB_INTERLEAVE2PLANAR;  

    // in_params.cvtcolor_roi_cfg.roi.x = oSizeROI.x;
    // in_params.cvtcolor_roi_cfg.roi.y = oSizeROI.x;
    // in_params.cvtcolor_roi_cfg.roi.w = oSizeROI.width;
    // in_params.cvtcolor_roi_cfg.roi.h = oSizeROI.height;
    in_params.cvtcolor_roi_cfg.color_space = RGB_PLANAR;
    in_params.pSizeROI =  calloc(nRoiNumber, sizeof(VappiRect));
    memcpy(in_params.pSizeROI, pSizeROI, nRoiNumber* sizeof(VappiRect));      
    in_params.nOutputs = 1;
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num = nRoiNumber;
    // in_params.op_par.config.config = (void *)&in_params.cvtcolor_roi_cfg;
    // in_params.op_par.config.size = sizeof(in_params.cvtcolor_roi_cfg);    
    in_params.op_par.config_op_params = config_color_conversion_op;
    in_params.op_par.update_cfg = update_cvtcolor_roi;
    in_params.op_par.config.config = (void *)&in_params.cvtcolor_roi_cfg;
    in_params.op_par.config.size = sizeof(in_params.cvtcolor_roi_cfg);    

    // stream->op_async->block_num = in_params.op_par.block_num;
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    }   
    free(in_params.pSizeROI); 
    return VAPP_SUCCESS;       
}


VappStatus vappiRGBToYUV420_8u_C3P3R(const Vapp8u * pSrc, int nSrcStep, Vapp8u * pDst[3], int rDstStep[3], VappiSize oSizeROI)
{

       return VAPP_SUCCESS;
}



