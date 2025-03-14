
#include "misc_params.h"
#include "detect.h"

//#define ALIGN_VALUE 64
     
static int config_geometry_op(void * priv_params, uint32_t entry)
{
    int i = 0;
    geometry_input_params * in_params = (geometry_input_params *)priv_params;
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
    op_info->config_array = &in_params->op_par.config;
    op_info->config_num = 1;
    return 0;
}

static int up_geometry_info(void * priv_params)
{
    geometry_input_params * in_params = (geometry_input_params *)priv_params;
    int * value = (int *)(in_params->update_cfg_value[0]);
    if(value){
        *value = in_params->op_par.block_id;
    }
    return 0;
}

  

VappStatus 
vappiYUV420Resize_8u_P3(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, int eInterpolation)
{
    VappStatus vaccRet;
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if( !pSrc|| !pDst || !oDstSize.width || !oDstSize.height || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }
    in_params.op_par.elf_file = malloc(strlen(op_path) + strlen(RESIZE_ELF_NAME) + 2) ;
    snprintf(in_params.op_par.elf_file, strlen(op_path) + strlen(RESIZE_ELF_NAME) + 2, "%s%s", op_path, RESIZE_ELF_NAME);
    in_params.op_par.custom_op_name = RESIZE_OP_FUNC;     
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )pDst;
    
    in_params.scale_cfg.iimage_shape.width = oSrcSize.width;
    in_params.scale_cfg.iimage_shape.height = oSrcSize.height;
    in_params.scale_cfg.iimage_shape.w_pitch = nSrcStep;
    in_params.scale_cfg.iimage_shape.h_pitch = oSrcSize.height;    

    //cfg->iimage_shape  = in_params->in_img;
    in_params.nOutputs = 1;
    for (int i = 0; i < in_params.nOutputs; i++) {
        in_params.scale_cfg.oimage_shape[i].width = oDstSize.width;
        in_params.scale_cfg.oimage_shape[i].height = oDstSize.height;
        in_params.scale_cfg.oimage_shape[i].w_pitch = nDstStep;
        in_params.scale_cfg.oimage_shape[i].h_pitch = oDstSize.height;
    }    
    in_params.scale_cfg.oimage_cnt = in_params.nOutputs;
    in_params.scale_cfg.resize_type = eInterpolation;

    //in_params.image_format = YUV_I420;
    in_params.scale_cfg.image_format = YUV_I420;
    in_params.eInterpolation = eInterpolation;
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num  = 1;
    in_params.op_par.config_op_params = config_geometry_op;
    in_params.op_par.config.config = (void *)&in_params.scale_cfg;
    in_params.op_par.config.size = sizeof(in_params.scale_cfg);           
    vaccRet = vapp_run_op(devID, &in_params.op_par);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op yuv420 resize failed.\n");
        free(in_params.op_par.elf_file);
        return vaccRet;           
    }   
    free(in_params.op_par.elf_file);
    return VAPP_SUCCESS;
}

VappStatus 
vappiNV12Resize_8u_P2(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, int eInterpolation)
{
    rtError_t vaccRet;
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }

    if( !pSrc|| !pDst || !oDstSize.width || !oDstSize.height || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }    
    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }    
    in_params.op_par.elf_file = malloc(strlen(op_path) + strlen(RESIZE_ELF_NAME) + 2);
    snprintf(in_params.op_par.elf_file, strlen(op_path) + strlen(RESIZE_ELF_NAME) + 2, "%s%s", op_path, RESIZE_ELF_NAME);
    in_params.op_par.custom_op_name = RESIZE_OP_FUNC;     
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t)pDst;

    in_params.scale_cfg.iimage_shape.width = oSrcSize.width;
    in_params.scale_cfg.iimage_shape.height = oSrcSize.height;
    in_params.scale_cfg.iimage_shape.w_pitch = nSrcStep;
    in_params.scale_cfg.iimage_shape.h_pitch = oSrcSize.height;    

    //cfg->iimage_shape  = in_params->in_img;
    in_params.nOutputs = 1;
    for (int i = 0; i < in_params.nOutputs; i++) {
        in_params.scale_cfg.oimage_shape[i].width = oDstSize.width;
        in_params.scale_cfg.oimage_shape[i].height = oDstSize.height;
        in_params.scale_cfg.oimage_shape[i].w_pitch = nDstStep;
        in_params.scale_cfg.oimage_shape[i].h_pitch = oDstSize.height;
    }    
    in_params.scale_cfg.oimage_cnt = in_params.nOutputs;
    in_params.scale_cfg.resize_type = eInterpolation;
	in_params.op_par.config_op_params = config_geometry_op;
    in_params.scale_cfg.image_format = YUV_NV12;
    in_params.nOutputs = 1;    
    in_params.eInterpolation = eInterpolation;
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num = 1;
    in_params.op_par.config.config = (void *)&in_params.scale_cfg;
    in_params.op_par.config.size = sizeof(in_params.scale_cfg);    
    vaccRet = vapp_run_op(devID, &in_params.op_par);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        free(in_params.op_par.elf_file);
        return vaccRet;           
    }   

    free(in_params.op_par.elf_file);
    return VAPP_SUCCESS; 
}

VappStatus 
vappiRGBResize_8u_C3(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, int eInterpolation)
{
    rtError_t vaccRet;
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }

    if( !pSrc|| !pDst || !oDstSize.width || !oDstSize.height || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    const char * op_path = getenv("VASTAI_VAPP_PATH");

    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }    
    
    in_params.op_par.elf_file = malloc(strlen(op_path) + strlen(RESIZE_ELF_NAME_MULTI) + 2);
    snprintf(in_params.op_par.elf_file, strlen(op_path) + strlen(RESIZE_ELF_NAME_MULTI) + 2, "%s%s", op_path, RESIZE_ELF_NAME_MULTI);
    in_params.op_par.custom_op_name = RESIZE_OP_FUNC_MULTI;        
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t)pDst;

    in_params.scale_cfg_multi.in_img.width = oSrcSize.width;
    in_params.scale_cfg_multi.in_img.height = oSrcSize.height;
    in_params.scale_cfg_multi.in_img.w_pitch = nSrcStep;
    in_params.scale_cfg_multi.in_img.h_pitch = oSrcSize.height;    

    in_params.scale_cfg_multi.out_img.width = oDstSize.width;
    in_params.scale_cfg_multi.out_img.height = oDstSize.height;
    in_params.scale_cfg_multi.out_img.w_pitch = nDstStep;
    in_params.scale_cfg_multi.out_img.h_pitch = oDstSize.height;  

    in_params.nOutputs = 1;
    in_params.eInterpolation = eInterpolation;
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num = BLOCK_NUM;
    in_params.scale_cfg_multi.in_coding = RGB888;
    in_params.scale_cfg_multi.out_coding = RGB888;
    in_params.scale_cfg_multi.method = eInterpolation;
    in_params.scale_cfg_multi.color_space = COLOR_SPACE_BT601;
    in_params.scale_cfg_multi.block_num = in_params.op_par.block_num;

    in_params.op_par.config_op_params = config_geometry_op;
    in_params.op_par.update_cfg = up_geometry_info;
    in_params.update_cfg_value[0] = &in_params.scale_cfg_multi.block_id;
    in_params.op_par.config.config = (void *)&in_params.scale_cfg_multi;
    in_params.op_par.config.size = sizeof(in_params.scale_cfg_multi);    
    vaccRet = vapp_run_op_multi(devID, &in_params.op_par);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        free(in_params.op_par.elf_file);
        return vaccRet;           
    }   

    free(in_params.op_par.elf_file);
    return VAPP_SUCCESS; 
}


static 
VappStatus 
inner_vappiResizePlus_8u_XXX_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, int nCropwidth, int nCropHeight, int eInterpolation, int nImageFormat, vastStream_t vastStreamCtx)
{
    rtError_t vaccRet;
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }

    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    if( !pSrc|| !pDst || !oDstSize.width || !oDstSize.height || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    VastStream * stream = (VastStream *)vastStreamCtx;
    const char * op_path = getenv("VASTAI_VAPP_PATH");

    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }    
    
    vaccRet =  vapp_find_op_entry(OP_RESIZE_MULTICORE_OP, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    } 


    in_params.scale_cfg_multi.in_img.width = nCropwidth;
    in_params.scale_cfg_multi.in_img.height = nCropHeight;
    in_params.scale_cfg_multi.in_img.w_pitch = nSrcStep;
    in_params.scale_cfg_multi.in_img.h_pitch = oSrcSize.height;    

    in_params.scale_cfg_multi.out_img.width = oDstSize.width;
    in_params.scale_cfg_multi.out_img.height = oDstSize.height;
    in_params.scale_cfg_multi.out_img.w_pitch = nDstStep;
    in_params.scale_cfg_multi.out_img.h_pitch = oDstSize.height;  

    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t)pDst;

    in_params.scale_cfg_multi.in_coding = nImageFormat;
    in_params.scale_cfg_multi.out_coding = nImageFormat;

    in_params.nOutputs = 1;
    in_params.eInterpolation = eInterpolation;
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num = BLOCK_NUM;
    in_params.scale_cfg_multi.method = eInterpolation;
    in_params.scale_cfg_multi.color_space = COLOR_SPACE_BT601;
    in_params.scale_cfg_multi.block_num = in_params.op_par.block_num; 
    in_params.op_par.config_op_params = config_geometry_op;
    in_params.op_par.update_cfg = up_geometry_info;
    in_params.update_cfg_value[0] = &in_params.scale_cfg_multi.block_id;
    in_params.op_par.config.config = (void *)&in_params.scale_cfg_multi;
    in_params.op_par.config.size = sizeof(in_params.scale_cfg_multi);        
    in_params.ctx = stream;
    // stream->op_async->block_num = in_params.op_par.block_num;
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    }   
    return VAPP_SUCCESS; 
}

VappStatus 
vappiRGBPResizePlus_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, int nCropwidth, int nCropHeight, int eInterpolation, vastStream_t vastStreamCtx)
{
    return inner_vappiResizePlus_8u_XXX_Ctx(devID, pSrc, oSrcSize, nSrcStep, pDst, oDstSize, nDstStep, nCropwidth, nCropHeight, eInterpolation, RGB_PLANAR, vastStreamCtx);
}

VappStatus 
vappiGrayResizePlus_8u_P1_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, VappiRect oCropSize, int eInterpolation, vastStream_t vastStreamCtx)
{
    const Vapp8u * const pSrcOff = pSrc + oCropSize.x + oCropSize.y *  oSrcSize.width;
    return inner_vappiResizePlus_8u_XXX_Ctx(devID, pSrcOff, oSrcSize, nSrcStep, pDst, oDstSize, nDstStep, oCropSize.width, oCropSize.height, eInterpolation, GRAY, vastStreamCtx);
}

static 
VappStatus 
inner_vappiResize_8u_XXX_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, int eInterpolation, int nImageFormat, vastStream_t vastStreamCtx)
{
    rtError_t vaccRet;
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }

    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    if( !pSrc|| !pDst || !oDstSize.width || !oDstSize.height || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    VastStream * stream = (VastStream *)vastStreamCtx;
    const char * op_path = getenv("VASTAI_VAPP_PATH");

    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }    
    
    vaccRet =  vapp_find_op_entry(OP_RESIZE_MULTICORE_OP, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    } 
    in_params.scale_cfg_multi.in_img.width = oSrcSize.width;
    in_params.scale_cfg_multi.in_img.height = oSrcSize.height;
    in_params.scale_cfg_multi.in_img.w_pitch = nSrcStep;
    in_params.scale_cfg_multi.in_img.h_pitch = oSrcSize.height;    

    in_params.scale_cfg_multi.out_img.width = oDstSize.width;
    in_params.scale_cfg_multi.out_img.height = oDstSize.height;
    in_params.scale_cfg_multi.out_img.w_pitch = nDstStep;
    in_params.scale_cfg_multi.out_img.h_pitch = oDstSize.height;  
    
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t)pDst;

    in_params.scale_cfg_multi.in_coding = nImageFormat;

    in_params.nOutputs = 1;
    in_params.eInterpolation = eInterpolation;
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num = BLOCK_NUM;
    in_params.scale_cfg_multi.method = eInterpolation;
    in_params.scale_cfg_multi.color_space = COLOR_SPACE_BT601;
    in_params.scale_cfg_multi.block_num = in_params.op_par.block_num;

    in_params.op_par.update_cfg = up_geometry_info;
    in_params.update_cfg_value[0] = &in_params.scale_cfg_multi.block_id;
    in_params.op_par.config.config = (void *)&in_params.scale_cfg_multi;
    in_params.op_par.config.size = sizeof(in_params.scale_cfg_multi);  
    in_params.op_par.config_op_params = config_geometry_op;
    in_params.ctx = stream;
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op crop failed.\n");
        return vaccRet;           
    } 
    return VAPP_SUCCESS; 
}

VappStatus 
vappiRGBResize_8u_C3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, int eInterpolation, vastStream_t vastStreamCtx)
{
    return inner_vappiResize_8u_XXX_Ctx(devID, pSrc, oSrcSize, nSrcStep, pDst, oDstSize, nDstStep, eInterpolation, RGB888, vastStreamCtx);
}

VappStatus 
vappiRGBPResize_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, int eInterpolation, vastStream_t vastStreamCtx)
{
    return inner_vappiResize_8u_XXX_Ctx(devID, pSrc, oSrcSize, nSrcStep, pDst, oDstSize, nDstStep, eInterpolation, RGB_PLANAR, vastStreamCtx);
}

VappStatus 
vappiGrayResize_8u_P1_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiSize oDstSize, int nDstStep, int eInterpolation, vastStream_t vastStreamCtx)
{
    return inner_vappiResize_8u_XXX_Ctx(devID, pSrc, oSrcSize, nSrcStep, pDst, oDstSize, nDstStep, eInterpolation, GRAY, vastStreamCtx);
}


static int update_crop_roi(void * priv_params)
{
    geometry_input_params * in_params = (geometry_input_params *)priv_params;
    crop_roi_para_t *cfg = &in_params->crop_roi_cfg;

    cfg->roi.x = in_params->pSizeROI[in_params->op_par.block_id].x;
    cfg->roi.y = in_params->pSizeROI[in_params->op_par.block_id].y;
    cfg->roi.w = in_params->pSizeROI[in_params->op_par.block_id].width;
    cfg->roi.h = in_params->pSizeROI[in_params->op_par.block_id].height;    

    return 0;  
}
static int update_rotate_rgba_interleaved(void * priv_params)
{
    geometry_input_params * in_params = (geometry_input_params *)priv_params;
    rotate_rgba_interleaved_roi_para_t *cfg = &in_params->rotete_rgba_interleaved_roi_cfg;

    cfg->roi.x = in_params->pSizeROI[in_params->op_par.block_id].x;
    cfg->roi.y = in_params->pSizeROI[in_params->op_par.block_id].y;
    cfg->roi.w = in_params->pSizeROI[in_params->op_par.block_id].width;
    cfg->roi.h = in_params->pSizeROI[in_params->op_par.block_id].height; 
    if(cfg->out_pitch < cfg->out_width || cfg->in_pitch < cfg->in_width){
        fprintf(stderr, "invalid src/dst step srcpw:%d w:%d dstpw:%d w:%d\n", cfg->in_pitch, cfg->in_width, cfg->out_pitch, cfg->out_width);
        return -1;  
    }
    return 0;  
}

static int update_roiflip(void * priv_params)
{
    geometry_input_params *in_params = (geometry_input_params *)priv_params;
    roi_flip_t *cfg = &in_params->roi_flip_cfg;
    cfg->roi.x = in_params->pSizeROI[in_params->op_par.block_id].x;
    cfg->roi.y = in_params->pSizeROI[in_params->op_par.block_id].y;
    cfg->roi.w = in_params->pSizeROI[in_params->op_par.block_id].width;
    cfg->roi.h = in_params->pSizeROI[in_params->op_par.block_id].height;    
    return 0;
}


static int update_remap(void * priv_params)
{
    geometry_input_params *in_params = (geometry_input_params *)priv_params;
    remap_cfg_t *cfg = &in_params->remap_cfg;    
    cfg->roi.c_start = 0;
    cfg->roi.h_start = in_params->pSizeROI[in_params->op_par.block_id].y;
    cfg->roi.w_start = in_params->pSizeROI[in_params->op_par.block_id].x;
    cfg->roi.c_len = in_params->nChaNum;
    cfg->roi.h_len = in_params->pSizeROI[in_params->op_par.block_id].height;
    cfg->roi.w_len = in_params->pSizeROI[in_params->op_par.block_id].width;
    return 0;
}

static int update_warp_perspective(void * priv_params)
{
    geometry_input_params *in_params = (geometry_input_params *)priv_params;
    warp_perspective_cfg_t *cfg = &in_params->warp_perspective_cfg;
    cfg->roi.c_start = 0;
    cfg->roi.h_start = in_params->pSizeROI[in_params->op_par.block_id].y;
    cfg->roi.w_start = in_params->pSizeROI[in_params->op_par.block_id].x;
    cfg->roi.c_len = in_params->nChaNum;
    cfg->roi.h_len = in_params->pSizeROI[in_params->op_par.block_id].height;
    cfg->roi.w_len = in_params->pSizeROI[in_params->op_par.block_id].width;
    return 0;    
}


VappStatus 
vappiRGBCrop_8u_C3(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiRect oCropSize, int nDstStep)
{
   rtError_t vaccRet;
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if( !pSrc|| !pDst || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    if(oCropSize.x + oCropSize.width > oSrcSize.width || oCropSize.y + oCropSize.height > oSrcSize.height || oCropSize.x < 0 || oCropSize.y < 0){
        return VAPP_BAD_ARGUMENT_ERROR;
    }    
    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }    

    in_params.op_par.elf_file = malloc(strlen(op_path) + strlen(CROP_ELF_NAME) + 2);
    snprintf(in_params.op_par.elf_file, strlen(op_path) + strlen(CROP_ELF_NAME) + 2, "%s%s", op_path, CROP_ELF_NAME);
    in_params.op_par.custom_op_name = CROP_OP_FUNC;     
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )pDst;

    in_params.crop_cfg.in_img.width = oSrcSize.width;
    in_params.crop_cfg.in_img.height = oSrcSize.height;
    in_params.crop_cfg.in_img.w_pitch = nSrcStep;
    in_params.crop_cfg.in_img.h_pitch = oSrcSize.height;    

    in_params.crop_cfg.out_img.width = oCropSize.width;
    in_params.crop_cfg.out_img.height = oCropSize.height;
    in_params.crop_cfg.out_img.w_pitch = nDstStep;
    in_params.crop_cfg.out_img.h_pitch = oCropSize.height;  


    //in_params.crop_rect = oCropSize;
    in_params.crop_cfg.x = oCropSize.x;
    in_params.crop_cfg.y = oCropSize.y;

    in_params.crop_cfg.in_coding = _XI_TILE_RGB888_TYPE_;
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.config_op_params = config_geometry_op;
    in_params.op_par.config.config = (void *)&in_params.crop_cfg;
    in_params.op_par.config.size = sizeof(in_params.crop_cfg);      
    in_params.op_par.block_num = 1;

    vaccRet = vapp_run_op(devID, &in_params.op_par);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op crop failed.\n");
        free(in_params.op_par.elf_file);
        return vaccRet;           
    }   

    free(in_params.op_par.elf_file);
    return VAPP_SUCCESS;     

}

static
VappStatus 
inner_vappiCrop_8u_XXX_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiRect oCropSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, int nImageFormat, vastStream_t vastStreamCtx)
{
   rtError_t vaccRet;
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if( !pSrc|| !pDst || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    if(oCropSize.x + oCropSize.width > oSrcSize.width || oCropSize.y + oCropSize.height > oSrcSize.height || oCropSize.x < 0 || oCropSize.y < 0){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    VastStream * stream = (VastStream *)vastStreamCtx;  
    vaccRet = vapp_find_op_entry(OP_CROP_ROI, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_find_op_entry failed.\n");
        return vaccRet;           
    }      

    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )pDst;

    in_params.crop_roi_cfg.in_img.width = oSrcSize.width;
    in_params.crop_roi_cfg.in_img.height = oSrcSize.height;
    in_params.crop_roi_cfg.in_img.w_pitch = nSrcStep;
    in_params.crop_roi_cfg.in_img.h_pitch = oSrcSize.height;    

    in_params.crop_roi_cfg.out_img.width = oCropSize.width;
    in_params.crop_roi_cfg.out_img.height = oCropSize.height;
    in_params.crop_roi_cfg.out_img.w_pitch = nDstStep;
    in_params.crop_roi_cfg.out_img.h_pitch = oCropSize.height;  

    in_params.crop_roi_cfg.x = oCropSize.x;
    in_params.crop_roi_cfg.y = oCropSize.y;
    in_params.crop_roi_cfg.in_coding = nImageFormat;
 

    in_params.pSizeROI =  calloc(nRoiNumber, sizeof(VappiRect));
    memcpy(in_params.pSizeROI, pSizeROI, nRoiNumber* sizeof(VappiRect));
    for(int i =0; i < nRoiNumber; i++){
        if(in_params.pSizeROI[i].height > oCropSize.height){
            VAPP_LOG(VAPP_LOG_ERROR, "crop roi height exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }
        if(in_params.pSizeROI[i].width > oCropSize.width){
            VAPP_LOG(VAPP_LOG_ERROR, "crop roi width exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }        
        if(in_params.pSizeROI[i].height + in_params.pSizeROI[i].y > oCropSize.height){
            VAPP_LOG(VAPP_LOG_ERROR, "crop roi h+y exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }
        if(in_params.pSizeROI[i].width + in_params.pSizeROI[i].x > oCropSize.width){
            VAPP_LOG(VAPP_LOG_ERROR, "crop roi w+x exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }                  
    }
    in_params.op_par.priv_params = &in_params;
    
    in_params.op_par.block_num = nRoiNumber;
    in_params.op_par.config_op_params = config_geometry_op;
    in_params.op_par.update_cfg = update_crop_roi;
    // stream->op_async->block_num = in_params.op_par.block_num;
    in_params.op_par.config.config = (void *)&in_params.crop_roi_cfg;
    in_params.op_par.config.size = sizeof(in_params.crop_roi_cfg);  

    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op crop failed.\n");
        return vaccRet;           
    }   
    free(in_params.pSizeROI); 
    return VAPP_SUCCESS;     
}

VappStatus 
vappiRGBCrop_8u_C3R_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiRect oCropSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx)
{
    return inner_vappiCrop_8u_XXX_Ctx(devID, pSrc, oSrcSize, nSrcStep, pDst, oCropSize, nDstStep, nRoiNumber, pSizeROI, _XI_TILE_RGB888_TYPE_, vastStreamCtx);
}

VappStatus 
vappiRGBPCrop_8u_P3R_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiRect oCropSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx)
{
    return inner_vappiCrop_8u_XXX_Ctx(devID, pSrc, oSrcSize, nSrcStep, pDst, oCropSize, nDstStep, nRoiNumber, pSizeROI, _XI_TILE_RGB_PLANAR_TYPE_, vastStreamCtx);
}
VappStatus 
vappiGrayCrop_8u_P1R_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u * pDst, VappiRect oCropSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx)
{
    return inner_vappiCrop_8u_XXX_Ctx(devID, pSrc, oSrcSize, nSrcStep, pDst, oCropSize, nDstStep, nRoiNumber, pSizeROI, _XI_TILE_GRAY_TYPE_, vastStreamCtx);
}

VappStatus 
vappiRGBAPRotate_8u_P4(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, int nAngle)
{
    rtError_t vaccRet;
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if( !pSrc|| !pDst || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }    
    in_params.op_par.elf_file = malloc(strlen(op_path) + strlen(ROTATE_RGBA_INTERLEAVED_ELF_NAME) + 2);
    snprintf(in_params.op_par.elf_file, strlen(op_path) + strlen(ROTATE_RGBA_INTERLEAVED_ELF_NAME) + 2, "%s%s", op_path, ROTATE_RGBA_INTERLEAVED_ELF_NAME);
    in_params.op_par.custom_op_name = ROTATE_RGBA_INTERLEAVED_OP_FUNC;     
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )pDst;

    in_params.rotete_rgba_interleaved_cfg.in_width = oSrcSize.width;
    in_params.rotete_rgba_interleaved_cfg.in_height = oSrcSize.height;
    
    in_params.rotete_rgba_interleaved_cfg.in_pitch = nSrcStep;
    in_params.rotete_rgba_interleaved_cfg.out_pitch = nDstStep;

    if(nAngle == ROTATE_DEGREE_90){
        in_params.rotete_rgba_interleaved_cfg.angle = ROTATE_ANGLE_90;
        in_params.rotete_rgba_interleaved_cfg.out_width = in_params.rotete_rgba_interleaved_cfg.in_height;
        in_params.rotete_rgba_interleaved_cfg.out_height = in_params.rotete_rgba_interleaved_cfg.in_width;
    }else if(nAngle == ROTATE_DEGREE_180){
        in_params.rotete_rgba_interleaved_cfg.angle = ROTATE_ANGLE_180;
        in_params.rotete_rgba_interleaved_cfg.out_width = in_params.rotete_rgba_interleaved_cfg.in_width;
        in_params.rotete_rgba_interleaved_cfg.out_height = in_params.rotete_rgba_interleaved_cfg.in_height;
    }else if(nAngle == ROTATE_DEGREE_270){
        in_params.rotete_rgba_interleaved_cfg.angle = ROTATE_ANGLE_270;
        in_params.rotete_rgba_interleaved_cfg.out_width = in_params.rotete_rgba_interleaved_cfg.in_height;
        in_params.rotete_rgba_interleaved_cfg.out_height = in_params.rotete_rgba_interleaved_cfg.in_width;
    }else{
        VAPP_LOG(VAPP_LOG_ERROR, "rotate degree error %d\n", nAngle);
        return VAPP_BAD_ARGUMENT_ERROR;     
    }
    in_params.rotete_rgba_interleaved_cfg.block_num = 1;
    in_params.rotete_rgba_interleaved_cfg.block_id = 0;
    if(in_params.rotete_rgba_interleaved_cfg.out_pitch < in_params.rotete_rgba_interleaved_cfg.out_width 
    || in_params.rotete_rgba_interleaved_cfg.in_pitch < in_params.rotete_rgba_interleaved_cfg.in_width){
        fprintf(stderr, "invalid src/dst step srcpw:%d w:%d dstpw:%d w:%d\n", in_params.rotete_rgba_interleaved_cfg.in_pitch, in_params.rotete_rgba_interleaved_cfg.in_width,
         in_params.rotete_rgba_interleaved_cfg.out_pitch, in_params.rotete_rgba_interleaved_cfg.out_width);
        return VAPP_BAD_ARGUMENT_ERROR;  
    }
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.config_op_params = config_geometry_op;
    in_params.op_par.config.config = (void *)&in_params.rotete_rgba_interleaved_cfg;
    in_params.op_par.config.size = sizeof(in_params.rotete_rgba_interleaved_cfg);      

    vaccRet = vapp_run_op(devID, &in_params.op_par);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rotate failed.\n");
        free(in_params.op_par.elf_file);
        return vaccRet;           
    }   

    free(in_params.op_par.elf_file);
    return VAPP_SUCCESS;     

}

VappStatus 
vappiRGBAPRotate_8u_P4_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, int nAngle, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx)
{
    rtError_t vaccRet;
    geometry_input_params  in_params = {0};

    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if( !pSrc|| !pDst || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    
    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    VastStream * stream = (VastStream *)vastStreamCtx;
    vaccRet = vapp_find_op_entry(OP_ROTATE_RGBA, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_find_op_entry failed.\n");
        return vaccRet;           
    }

    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )pDst;
    
    in_params.rotete_rgba_interleaved_roi_cfg.in_width = oSrcSize.width;
    in_params.rotete_rgba_interleaved_roi_cfg.in_height = oSrcSize.height;
    
    in_params.rotete_rgba_interleaved_roi_cfg.in_pitch = nSrcStep;
    in_params.rotete_rgba_interleaved_roi_cfg.out_pitch = nDstStep;

    if(nAngle == ROTATE_DEGREE_90){
        in_params.rotete_rgba_interleaved_roi_cfg.angle = ROTATE_ANGLE_90;
        in_params.rotete_rgba_interleaved_roi_cfg.out_width = in_params.rotete_rgba_interleaved_roi_cfg.in_height;
        in_params.rotete_rgba_interleaved_roi_cfg.out_height = in_params.rotete_rgba_interleaved_roi_cfg.in_width;
    }else if(nAngle == ROTATE_DEGREE_180){
        in_params.rotete_rgba_interleaved_roi_cfg.angle = ROTATE_ANGLE_180;
        in_params.rotete_rgba_interleaved_roi_cfg.out_width = in_params.rotete_rgba_interleaved_roi_cfg.in_width;
        in_params.rotete_rgba_interleaved_roi_cfg.out_height = in_params.rotete_rgba_interleaved_roi_cfg.in_height;
    }else if(nAngle == ROTATE_DEGREE_270){
        in_params.rotete_rgba_interleaved_roi_cfg.angle = ROTATE_ANGLE_270;
        in_params.rotete_rgba_interleaved_roi_cfg.out_width = in_params.rotete_rgba_interleaved_roi_cfg.in_height;
        in_params.rotete_rgba_interleaved_roi_cfg.out_height = in_params.rotete_rgba_interleaved_roi_cfg.in_width;
    }else{
        VAPP_LOG(VAPP_LOG_ERROR, "rotate degree error %d\n", nAngle);
        return VAPP_BAD_ARGUMENT_ERROR;     
    }

    if(in_params.rotete_rgba_interleaved_roi_cfg.out_pitch < in_params.rotete_rgba_interleaved_roi_cfg.out_width 
    || in_params.rotete_rgba_interleaved_roi_cfg.in_pitch < in_params.rotete_rgba_interleaved_roi_cfg.in_width){
        fprintf(stderr, "invalid src/dst step srcpw:%d w:%d dstpw:%d w:%d\n", 
        in_params.rotete_rgba_interleaved_roi_cfg.in_pitch, in_params.rotete_rgba_interleaved_roi_cfg.in_width, 
        in_params.rotete_rgba_interleaved_roi_cfg.out_pitch, in_params.rotete_rgba_interleaved_roi_cfg.out_width);
        return VAPP_BAD_ARGUMENT_ERROR;  
    }

    in_params.pSizeROI = calloc(nRoiNumber, sizeof(VappiRect));
    memcpy(in_params.pSizeROI, pSizeROI, nRoiNumber * sizeof(VappiRect));

    in_params.op_par.priv_params = &in_params;

    in_params.op_par.block_num = nRoiNumber;
    in_params.op_par.config_op_params = config_geometry_op;
    in_params.op_par.config.config = (void *)&in_params.rotete_rgba_interleaved_roi_cfg;
    in_params.op_par.config.size = sizeof(in_params.rotete_rgba_interleaved_roi_cfg);  
	in_params.op_par.update_cfg = update_rotate_rgba_interleaved;


    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op_multi_async rotate failed.\n");
        free(in_params.op_par.elf_file);
        return vaccRet;           
    }   

    free(in_params.op_par.elf_file);
    free(in_params.pSizeROI); 
    return VAPP_SUCCESS;     
}


/**
 * 2 channel 8-bit unsigned image mirror.
 *
 * For common parameter descriptions, see \ref CommonMirrorParameters.
 *
 */

VappStatus 
vappiYUV420Mirror_8u_P3(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, VappiAxis eAxis)
{
    rtError_t vaccRet;
    geometry_input_params  in_params = {0};
    yuv_nv12_flip_t *cfg = &in_params.flip_cfg;
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if( !pSrc|| !pDst || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }    
    in_params.op_par.elf_file = malloc(strlen(op_path) + strlen(FLIP_ELF_NAME) + 2);
    snprintf(in_params.op_par.elf_file, strlen(op_path) + strlen(FLIP_ELF_NAME) + 2, "%s%s", op_path, FLIP_ELF_NAME);
    in_params.op_par.custom_op_name = FLIP_OP_FUNC;     
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )pDst;
    in_params.in_img.width = oSrcSize.width;
    in_params.in_img.height = oSrcSize.height;
    in_params.in_img.w_pitch = nSrcStep;
    in_params.in_img.h_pitch = oSrcSize.height;    
    in_params.out_img.w_pitch = nDstStep;

    in_params.image_format = FLIP_IMAGE_YUV420P;
    in_params.op_par.priv_params = &in_params;
    in_params.flip_cfg.direction = eAxis;
    cfg->in_img = in_params.in_img;
    cfg->out_img = in_params.in_img; 

    if(cfg->out_img.w_pitch < cfg->out_img.width){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    cfg->ImageFormat = in_params.image_format;    
    in_params.op_par.config_op_params = config_geometry_op;
    in_params.op_par.block_num = 1;
    in_params.op_par.config.config = (void *)&in_params.flip_cfg;
    in_params.op_par.config.size = sizeof(in_params.flip_cfg);  

    vaccRet = vapp_run_op(devID, &in_params.op_par);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op mirror failed.\n");
        free(in_params.op_par.elf_file);
        return vaccRet;           
    }   

    free(in_params.op_par.elf_file);
    return VAPP_SUCCESS;     

}


VappStatus 
vappiNV12Mirror_8u_P2(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, VappiAxis eAxis)
{
    rtError_t vaccRet;
    geometry_input_params  in_params = {0};
    yuv_nv12_flip_t *cfg = &in_params.flip_cfg;
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if( !pSrc|| !pDst || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }    
    in_params.op_par.elf_file = malloc(strlen(op_path) + strlen(FLIP_ELF_NAME) + 2);
    snprintf(in_params.op_par.elf_file, strlen(op_path) + strlen(FLIP_ELF_NAME) + 2, "%s%s", op_path, FLIP_ELF_NAME);
    in_params.op_par.custom_op_name = FLIP_OP_FUNC;     
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )pDst;
    in_params.in_img.width = oSrcSize.width;
    in_params.in_img.height = oSrcSize.height;
    in_params.in_img.w_pitch = nSrcStep;
    in_params.in_img.h_pitch = oSrcSize.height;    
    in_params.out_img.w_pitch = nDstStep;

    in_params.image_format = FLIP_IMAGE_NVI2;
    in_params.op_par.priv_params = &in_params;
    in_params.flip_cfg.direction = eAxis;
    in_params.op_par.config_op_params = config_geometry_op;
    in_params.op_par.block_num = 1;

    cfg->in_img = in_params.in_img;
    cfg->out_img = in_params.in_img; 

    if(cfg->out_img.w_pitch < cfg->out_img.width){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    cfg->ImageFormat = in_params.image_format;
    in_params.op_par.config.config = (void *)&in_params.flip_cfg;
    in_params.op_par.config.size = sizeof(in_params.flip_cfg);      
    vaccRet = vapp_run_op(devID, &in_params.op_par);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op mirror failed.\n");
        free(in_params.op_par.elf_file);
        return vaccRet;           
    }   

    free(in_params.op_par.elf_file);
    return VAPP_SUCCESS;     

}





VappStatus 
vappi_mirror_8u_roi_ctx(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, VappiAxis eAxis, int nRoiNumber, VappiRect *pSizeROI, int nImageFormat, vastStream_t vastStreamCtx)
{
    rtError_t vaccRet;
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if( !pSrc|| !pDst || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
  
    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    VastStream * stream = (VastStream *)vastStreamCtx;
    vaccRet =  vapp_find_op_entry(OP_FLIP_ROI, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    }   
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t)pDst;


    in_params.roi_flip_cfg.in_img.width = oSrcSize.width;
    in_params.roi_flip_cfg.in_img.height = oSrcSize.height;
    in_params.roi_flip_cfg.in_img.w_pitch = nSrcStep;
    in_params.roi_flip_cfg.in_img.h_pitch = oSrcSize.height;    
    in_params.roi_flip_cfg.out_img.width = oSrcSize.width;
    in_params.roi_flip_cfg.out_img.height = oSrcSize.height;
    in_params.roi_flip_cfg.out_img.w_pitch = nDstStep;
    in_params.roi_flip_cfg.out_img.h_pitch = oSrcSize.height;   
    in_params.roi_flip_cfg.direction = eAxis;
    in_params.pSizeROI =  calloc(nRoiNumber, sizeof(VappiRect));
    memcpy(in_params.pSizeROI, pSizeROI, nRoiNumber* sizeof(VappiRect));

    in_params.roi_flip_cfg.ImageFormat = nImageFormat;
    if(in_params.roi_flip_cfg.out_img.w_pitch < in_params.roi_flip_cfg.out_img.width){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    in_params.op_par.priv_params = &in_params;

    in_params.op_par.block_num = nRoiNumber;
    in_params.op_par.config.config = (void *)&in_params.roi_flip_cfg;
    in_params.op_par.config.size = sizeof(in_params.roi_flip_cfg);      
 
    in_params.op_par.config_op_params = config_geometry_op;
    in_params.op_par.config.config = (void *)&in_params.roi_flip_cfg;
    in_params.op_par.config.size = sizeof(in_params.roi_flip_cfg);  
	in_params.op_par.update_cfg = update_roiflip;

    // stream->op_async->block_num = in_params.op_par.block_num;
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    }   
    free(in_params.pSizeROI); 
    return VAPP_SUCCESS;      
}

VappStatus 
vappiRGBPMirror_8u_P3R_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, VappiAxis eAxis, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx)
{
   
    return vappi_mirror_8u_roi_ctx(devID, pSrc, oSrcSize, nSrcStep, pDst, nDstStep, eAxis, nRoiNumber, pSizeROI, RGB_PLANAR, vastStreamCtx);     
}

VappStatus 
vappiGrayMirror_8u_P1R_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, VappiAxis eAxis, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx)
{
   
    return vappi_mirror_8u_roi_ctx(devID, pSrc, oSrcSize, nSrcStep, pDst, nDstStep, eAxis, nRoiNumber, pSizeROI, GRAY, vastStreamCtx);     
}


static 
VappStatus 
inner_vappiRemap_8u_XXX_Ctx(unsigned int devID, const Vapp8u * const pSrc, const Vapp32f * const pMap1, const Vapp32f * const pMap2, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, VappiSize oDstSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, int nChaNum, vastStream_t vastStreamCtx)
{
    rtError_t vaccRet;
    int vppRet = 0; 
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if( !pSrc|| !pDst || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
  
    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    
    VastStream * stream = (VastStream *)vastStreamCtx;  
    vaccRet =  vapp_find_op_entry(OP_REMAP, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_find_op_entry failed.\n");
        return vaccRet;           
    } 
    
    if(nRoiNumber != stream->block_num){
        VAPP_LOG(VAPP_LOG_ERROR, "Invalid block num.\n");
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    if(stream->map1_buffer == NULL){
        vppRet = vastMalloc(devID, (void**)&stream->map1_buffer, nDstStep * oDstSize.height * 2 *sizeof(short));
        if(vppRet != VAPP_SUCCESS){
            VAPP_LOG(VAPP_LOG_ERROR, "vastMalloc failed.\n");
            return vppRet;
        }
    }  
    if(stream->map2_buffer == NULL){
        vppRet = vastMalloc(devID, (void**)&stream->map2_buffer, nDstStep * oDstSize.height * 2 *sizeof(short));
        if(vppRet != VAPP_SUCCESS){
            VAPP_LOG(VAPP_LOG_ERROR, "vastMalloc failed.\n");
            return vppRet;
        }    
    }  
    /*****************************************
     * inputOutputArrayAddr[0]: input address
     * inputOutputArrayAddr[1]: map1 address
     * inputOutputArrayAddr[2]: map2 address
     * inputOutputArrayAddr[3]: map1 buffer address
     * inputOutputArrayAddr[4]: map2 buffer address
     * inputOutputArrayAddr[5]: output address
     * ****************************************/  

    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t)pMap1;
    in_params.op_par.inout_addr[2] = (uint64_t)pMap2;
    in_params.op_par.inout_addr[3] = (uint64_t)stream->map1_buffer;
    in_params.op_par.inout_addr[4] = (uint64_t)stream->map2_buffer;
    in_params.op_par.inout_addr[5] = (uint64_t)pDst;

    in_params.remap_cfg.in_shape.width = oSrcSize.width;
    in_params.remap_cfg.in_shape.height = oSrcSize.height;
    in_params.remap_cfg.in_shape.w_pitch = nSrcStep;
    in_params.remap_cfg.in_shape.h_pitch = oSrcSize.height;  
    in_params.remap_cfg.in_shape.channel = nChaNum;  
    in_params.remap_cfg.in_shape.c_pitch = nChaNum; 
    in_params.remap_cfg.out_shape.width = oDstSize.width;
    in_params.remap_cfg.out_shape.height = oDstSize.height;
    in_params.remap_cfg.out_shape.w_pitch = nDstStep;
    in_params.remap_cfg.out_shape.h_pitch = oDstSize.height;   
    in_params.remap_cfg.out_shape.channel = nChaNum;  
    in_params.remap_cfg.out_shape.c_pitch = nChaNum; 
    in_params.remap_cfg.image_type = REMAP_PLANAR;
    in_params.remap_cfg.map1.width = oDstSize.width;
    in_params.remap_cfg.map1.height = oDstSize.height;
    in_params.remap_cfg.map1.w_pitch = nDstStep;
    in_params.remap_cfg.map1.h_pitch = oDstSize.height;
    in_params.remap_cfg.map2.width = oDstSize.width;
    in_params.remap_cfg.map2.height = oDstSize.height;
    in_params.remap_cfg.map2.w_pitch = nDstStep;
    in_params.remap_cfg.map2.h_pitch = oDstSize.height;
    in_params.remap_cfg.map1_type = REMAP_MAP_32FC1;
    in_params.remap_cfg.map2_type = REMAP_MAP_32FC1;
    in_params.remap_cfg.inter_type = REMAP_INTER_LINEAR;
    in_params.remap_cfg.border_type = REMAP_BORDER_CONSTANT;
    in_params.remap_cfg.border_value = 0;
    in_params.nChaNum = nChaNum;

    in_params.pSizeROI =  calloc(nRoiNumber, sizeof(VappiRect));
    memcpy(in_params.pSizeROI, pSizeROI, nRoiNumber* sizeof(VappiRect));
    for(int i =0; i < nRoiNumber; i++){
        if(in_params.pSizeROI[i].height > oDstSize.height){
            VAPP_LOG(VAPP_LOG_ERROR, "remap roi height exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }
        if(in_params.pSizeROI[i].width > oDstSize.width){
            VAPP_LOG(VAPP_LOG_ERROR, "remap roi width exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }        
        if(in_params.pSizeROI[i].height + in_params.pSizeROI[i].y > oDstSize.height){
            VAPP_LOG(VAPP_LOG_ERROR, "remap roi h+y exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }
        if(in_params.pSizeROI[i].width + in_params.pSizeROI[i].x > oDstSize.width){
            VAPP_LOG(VAPP_LOG_ERROR, "remap roi w+x exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }                  
    }
    in_params.op_par.priv_params = &in_params;
    
    in_params.op_par.block_num = nRoiNumber;
    in_params.op_par.config.config = (void *)&in_params.remap_cfg;
    in_params.op_par.config.size = sizeof(in_params.remap_cfg);      
    in_params.op_par.config_op_params = config_geometry_op;
    in_params.op_par.config.config = (void *)&in_params.remap_cfg;
    in_params.op_par.config.size = sizeof(in_params.remap_cfg);  
	in_params.op_par.update_cfg = update_remap;    
    // stream->op_async->block_num = in_params.op_par.block_num;
    in_params.ctx = stream;
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    }   
    free(in_params.pSizeROI);
    return VAPP_SUCCESS;     
}

VappStatus 
vappiGrayRemapFixedMap_8u_P1R_Ctx(unsigned int devID, const Vapp8u * const pSrc, const Vapp16s * const pMap1, const Vapp16u * const pMap2, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, VappiSize oDstSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx)
{
    rtError_t vaccRet;
    int vppRet = 0; 
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if( !pSrc|| !pDst || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
  
    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    VastStream * stream = (VastStream *)vastStreamCtx;  
    vaccRet =  vapp_find_op_entry(OP_REMAP, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_find_op_entry failed.\n");
        return vaccRet;           
    }    
    if(nRoiNumber != stream->block_num){
        VAPP_LOG(VAPP_LOG_ERROR, "Invalid block num.\n");
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    if(stream->map1_buffer == NULL){
        vppRet = vastMalloc(devID, (void**)&stream->map1_buffer, nDstStep * oDstSize.height * 2 *sizeof(short));
        if(vppRet != VAPP_SUCCESS){
            VAPP_LOG(VAPP_LOG_ERROR, "vastMalloc failed.\n");
            return vppRet;
        }
    }  
    if(stream->map2_buffer == NULL){
        vppRet = vastMalloc(devID, (void**)&stream->map2_buffer, nDstStep * oDstSize.height * 2 *sizeof(short));
        if(vppRet != VAPP_SUCCESS){
            VAPP_LOG(VAPP_LOG_ERROR, "vastMalloc failed.\n");
            return vppRet;
        }    
    }  
   
    /*****************************************
     * inputOutputArrayAddr[0]: input address
     * inputOutputArrayAddr[1]: map1 address
     * inputOutputArrayAddr[2]: map2 address
     * inputOutputArrayAddr[3]: map1 buffer address
     * inputOutputArrayAddr[4]: map2 buffer address
     * inputOutputArrayAddr[5]: output address
     * ****************************************/  

    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t)pMap1;
    in_params.op_par.inout_addr[2] = (uint64_t)pMap2;
    in_params.op_par.inout_addr[3] = (uint64_t)stream->map1_buffer;
    in_params.op_par.inout_addr[4] = (uint64_t)stream->map2_buffer;
    in_params.op_par.inout_addr[5] = (uint64_t)pDst;

    in_params.remap_cfg.in_shape.width = oSrcSize.width;
    in_params.remap_cfg.in_shape.height = oSrcSize.height;
    in_params.remap_cfg.in_shape.w_pitch = nSrcStep;
    in_params.remap_cfg.in_shape.h_pitch = oSrcSize.height;  
    in_params.remap_cfg.in_shape.channel = 1;  
    in_params.remap_cfg.in_shape.c_pitch = 1; 
    in_params.remap_cfg.out_shape.width = oDstSize.width;
    in_params.remap_cfg.out_shape.height = oDstSize.height;
    in_params.remap_cfg.out_shape.w_pitch = nDstStep;
    in_params.remap_cfg.out_shape.h_pitch = oDstSize.height;   
    in_params.remap_cfg.out_shape.channel = 1;  
    in_params.remap_cfg.out_shape.c_pitch = 1; 
    in_params.remap_cfg.image_type = REMAP_PLANAR;
    in_params.remap_cfg.map1.width = oDstSize.width;
    in_params.remap_cfg.map1.height = oDstSize.height;
    in_params.remap_cfg.map1.w_pitch = nDstStep;
    in_params.remap_cfg.map1.h_pitch = oDstSize.height;
    in_params.remap_cfg.map2.width = oDstSize.width;
    in_params.remap_cfg.map2.height = oDstSize.height;
    in_params.remap_cfg.map2.w_pitch = nDstStep;
    in_params.remap_cfg.map2.h_pitch = oDstSize.height;
    in_params.remap_cfg.map1_type = REMAP_MAP_16SC2;
    in_params.remap_cfg.map2_type = REMAP_MAP_16UC1;
    in_params.remap_cfg.inter_type = REMAP_INTER_LINEAR;
    in_params.remap_cfg.border_type = REMAP_BORDER_CONSTANT;
    in_params.remap_cfg.border_value = 0;
    in_params.nChaNum = 1; //when add RemapFixedMap rgb, change this to : in_params.nChaNum = nChaNum

    in_params.pSizeROI =  calloc(nRoiNumber, sizeof(VappiRect));
    memcpy(in_params.pSizeROI, pSizeROI, nRoiNumber* sizeof(VappiRect));
    for(int i =0; i < nRoiNumber; i++){
        if(in_params.pSizeROI[i].height > oDstSize.height){
            VAPP_LOG(VAPP_LOG_ERROR, "fixremap roi height exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }
        if(in_params.pSizeROI[i].width > oDstSize.width){
            VAPP_LOG(VAPP_LOG_ERROR, "fixremap roi width exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }        
        if(in_params.pSizeROI[i].height + in_params.pSizeROI[i].y > oDstSize.height){
            VAPP_LOG(VAPP_LOG_ERROR, "fixremap roi h+y exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }
        if(in_params.pSizeROI[i].width + in_params.pSizeROI[i].x > oDstSize.width){
            VAPP_LOG(VAPP_LOG_ERROR, "fixremap roi w+x exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }                  
    }
    in_params.op_par.priv_params = &in_params;

    in_params.op_par.block_num = nRoiNumber;
    in_params.op_par.config.config = (void *)&in_params.remap_cfg;
    in_params.op_par.config.size = sizeof(in_params.remap_cfg);      

    in_params.op_par.config_op_params = config_geometry_op;
    in_params.op_par.config.config = (void *)&in_params.remap_cfg;
    in_params.op_par.config.size = sizeof(in_params.remap_cfg);  
	in_params.op_par.update_cfg = update_remap; 

    // stream->op_async->block_num = in_params.op_par.block_num;
    in_params.ctx = stream;
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    }       
    free(in_params.pSizeROI);
    return VAPP_SUCCESS;     
}

VappStatus 
vappiRGBPRemap_8u_P3R_Ctx(unsigned int devID, const Vapp8u * const pSrc, const Vapp32f * const pMap1, const Vapp32f * const pMap2, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, VappiSize oDstSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx)
{
    return inner_vappiRemap_8u_XXX_Ctx(devID, pSrc, pMap1, pMap2, oSrcSize, nSrcStep, pDst, oDstSize, nDstStep, nRoiNumber, pSizeROI, 3, vastStreamCtx);
}

VappStatus 
vappiGrayRemap_8u_P1R_Ctx(unsigned int devID, const Vapp8u * const pSrc, const Vapp32f * const pMap1, const Vapp32f * const pMap2, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, VappiSize oDstSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx)
{
    return inner_vappiRemap_8u_XXX_Ctx(devID, pSrc, pMap1, pMap2, oSrcSize, nSrcStep, pDst, oDstSize, nDstStep, nRoiNumber, pSizeROI, 1, vastStreamCtx);
}

VappStatus 
inner_vappiWrapPerspective_8u_P3R_Ctx(unsigned int devID, const Vapp8u * const pSrc, const Vapp64f * const pM,  VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, VappiSize oDstSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, int nChaNum, vastStream_t vastStreamCtx)
{
    rtError_t vaccRet;
    geometry_input_params in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if( !pSrc|| !pDst || !nDstStep || !oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    VastStream * stream = (VastStream *)vastStreamCtx;  
    vaccRet =  vapp_find_op_entry(OP_WARPPERSPECTIVE, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_find_op_entry failed.\n");
        return vaccRet;           
    }      
    
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t)pDst;
    
    in_params.warp_perspective_cfg.in_image_shape.width = oSrcSize.width;
    in_params.warp_perspective_cfg.in_image_shape.height = oSrcSize.height;
    in_params.warp_perspective_cfg.in_image_shape.w_pitch = nSrcStep;
    in_params.warp_perspective_cfg.in_image_shape.h_pitch = oSrcSize.height;
    in_params.warp_perspective_cfg.in_image_shape.channel = nChaNum;
    in_params.warp_perspective_cfg.in_image_shape.c_pitch = nChaNum;
    in_params.warp_perspective_cfg.out_image_shape.width = oDstSize.width;
    in_params.warp_perspective_cfg.out_image_shape.height = oDstSize.height;
    in_params.warp_perspective_cfg.out_image_shape.w_pitch = nDstStep;
    in_params.warp_perspective_cfg.out_image_shape.h_pitch = oDstSize.height;
    in_params.warp_perspective_cfg.out_image_shape.channel = nChaNum;
    in_params.warp_perspective_cfg.out_image_shape.c_pitch = nChaNum;
    in_params.warp_perspective_cfg.image_type = WARP_PERSPECTIVE_PLANAR;
    in_params.warp_perspective_cfg.inter_type = WARP_PERSPECTIVE_INTER_LINEAR;
    in_params.warp_perspective_cfg.border_type = WARP_PERSPECTIVE_BORDER_CONSTANT;
    in_params.warp_perspective_cfg.border_value = 0;
    in_params.warp_perspective_cfg.flag = WARP_PERSPECTIVE_FORWARD_MAP;
    in_params.nChaNum = nChaNum;
    memcpy(in_params.warp_perspective_cfg.M, pM, 9 * sizeof(Vapp64f));

    in_params.pSizeROI =  calloc(nRoiNumber, sizeof(VappiRect));
    memcpy(in_params.pSizeROI, pSizeROI, nRoiNumber* sizeof(VappiRect));
    for(int i =0; i < nRoiNumber; i++){
        if(in_params.pSizeROI[i].height > oDstSize.height){
            VAPP_LOG(VAPP_LOG_ERROR, "WrapPerspective roi height exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }
        if(in_params.pSizeROI[i].width > oDstSize.width){
            VAPP_LOG(VAPP_LOG_ERROR, "WrapPerspective roi width exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }        
        if(in_params.pSizeROI[i].height + in_params.pSizeROI[i].y > oDstSize.height){
            VAPP_LOG(VAPP_LOG_ERROR, "WrapPerspective roi h+y exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }
        if(in_params.pSizeROI[i].width + in_params.pSizeROI[i].x > oDstSize.width){
            VAPP_LOG(VAPP_LOG_ERROR, "WrapPerspective roi w+x exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }                  
    }
    in_params.op_par.priv_params = &in_params;

    in_params.op_par.block_num = nRoiNumber;
    in_params.op_par.config.config = (void *)&in_params.warp_perspective_cfg;
    in_params.op_par.config.size = sizeof(in_params.warp_perspective_cfg);      

    in_params.op_par.config_op_params = config_geometry_op;
    in_params.op_par.config.config = (void *)&in_params.warp_perspective_cfg;
    in_params.op_par.config.size = sizeof(in_params.warp_perspective_cfg);  
	in_params.op_par.update_cfg = update_warp_perspective;   
    // stream->op_async->block_num = in_params.op_par.block_num;
    in_params.ctx = stream;
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    }   
    free(in_params.pSizeROI); 
    return VAPP_SUCCESS;     
}


VappStatus 
vappiRGBPWrapPerspective_8u_P3R_Ctx(unsigned int devID, const Vapp8u * const pSrc, const Vapp64f * const pM,  VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, VappiSize oDstSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx)
{
    return inner_vappiWrapPerspective_8u_P3R_Ctx(devID, pSrc, pM, oSrcSize, nSrcStep, pDst, oDstSize, nDstStep, nRoiNumber, pSizeROI, 3,vastStreamCtx);   
}


VappStatus 
vappiGrayWrapPerspective_8u_P1R_Ctx(unsigned int devID, const Vapp8u * const pSrc, const Vapp64f * const pM,  VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, VappiSize oDstSize, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, vastStream_t vastStreamCtx)
{
    return inner_vappiWrapPerspective_8u_P3R_Ctx(devID, pSrc, pM, oSrcSize, nSrcStep, pDst, oDstSize, nDstStep, nRoiNumber, pSizeROI, 1, vastStreamCtx);   
}



static int up_translate_roi(void * priv_params)
{
    geometry_input_params * in_params = (geometry_input_params *)priv_params;
    translateTransform_cfg_t *cfg = &in_params->translateTransform_cfg;

    cfg->in_image.pos_x = in_params->p_size_roi[in_params->op_par.block_id].x;
    cfg->in_image.pos_y = in_params->p_size_roi[in_params->op_par.block_id].y;
    cfg->in_image.pos_z = in_params->p_size_roi[in_params->op_par.block_id].z;
    cfg->in_image.channel = in_params->p_size_roi[in_params->op_par.block_id].channels;
    cfg->in_image.height = in_params->p_size_roi[in_params->op_par.block_id].height;
    cfg->in_image.width = in_params->p_size_roi[in_params->op_par.block_id].width;    

    
    for(int i =0; i < MAX_IN_OUT_ADDR; i++){
        if(in_params->op_par.inout_addr[i]){
            //fprintf(stderr, "translate inout_addr %d  %"PRIx64" \n", i, in_params->op_par.inout_addr[i]);
        }else{
            break;
        }
    }
    return 0;  
}


VappStatus 
vappiGrayTranslateTransform_8u_P1R_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, 
                            Vapp64f nOffsetX, Vapp64f nOffsetY, vastStream_t vastStreamCtx)                                                                         
{
    rtError_t vaccRet;
    int i = 0;
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if( !pSrc|| !pDst || !nDstStep || !oSrcSize.width || !oSrcSize.height || !nSrcStep|| !nRoiNumber || !pSizeROI){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
  
    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    VastStream * stream = (VastStream *)vastStreamCtx;  
    vaccRet =  vapp_find_op_entry(OP_TRANSLATE, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_find_op_entry failed.\n");
        return vaccRet;           
    }    
 
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t)pDst;

    in_params.translateTransform_cfg.pic.width = oSrcSize.width;
    in_params.translateTransform_cfg.pic.height = oSrcSize.height;
    in_params.translateTransform_cfg.pic.channel = 1;  
    in_params.translateTransform_cfg.pic.w_pitch = nSrcStep;
    in_params.translateTransform_cfg.pic.h_pitch = oSrcSize.height;  
    in_params.translateTransform_cfg.pic.c_pitch = 1; 
  
    in_params.translateTransform_cfg.image_offset.offset_x = nOffsetX;
    in_params.translateTransform_cfg.image_offset.offset_y = nOffsetY;
    if(!stream->size_roi){
        stream->size_roi = calloc(nRoiNumber, sizeof(VappiShape));
    }
    for(i = 0; i <nRoiNumber; i++){
        stream->size_roi[i].x = pSizeROI[i].x;
        stream->size_roi[i].y = pSizeROI[i].y;
        stream->size_roi[i].z = 0;            
        stream->size_roi[i].width = pSizeROI[i].width;
        stream->size_roi[i].height = pSizeROI[i].height;
        stream->size_roi[i].channels = 1;
        if (pSizeROI[i].height > oSrcSize.height) {
            VAPP_LOG(VAPP_LOG_ERROR, "TranslateTransform roi height exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }
        if (pSizeROI[i].width > oSrcSize.width) {
            VAPP_LOG(VAPP_LOG_ERROR, "TranslateTransform roi width exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }
        if (pSizeROI[i].height + pSizeROI[i].y > oSrcSize.height) {
            VAPP_LOG(VAPP_LOG_ERROR, "TranslateTransform roi h+y exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }
        if (pSizeROI[i].width + pSizeROI[i].x > oSrcSize.width) {
            VAPP_LOG(VAPP_LOG_ERROR, "TranslateTransform roi w+x exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }
    }    
    in_params.p_size_roi = stream->size_roi;
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num = nRoiNumber;   
    in_params.op_par.config_op_params = config_geometry_op;
    in_params.op_par.update_cfg = up_translate_roi;
    in_params.ctx = stream;
    in_params.op_par.config.config = (void *)&in_params.translateTransform_cfg;
    in_params.op_par.config.size = sizeof(in_params.translateTransform_cfg);
    in_params.ctx = stream;
    // stream->op_async->block_num = in_params.op_par.block_num;
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    }   

    return VAPP_SUCCESS;             
}


static int up_transpose_roi(void * priv_params)
{
    geometry_input_params * in_params = (geometry_input_params *)priv_params;
    permute_roi_op_t *cfg = &in_params->permute_roi_op_cfg;
    cfg->roi.x = in_params->pSizeROI[in_params->op_par.block_id].x;
    cfg->roi.y = in_params->pSizeROI[in_params->op_par.block_id].y;
    cfg->roi.h = in_params->pSizeROI[in_params->op_par.block_id].height;
    cfg->roi.w = in_params->pSizeROI[in_params->op_par.block_id].width;
    return 0;  
}

VappStatus 
vappiGrayTranspose_8u_P1R_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                        Vapp8u *pDst,  int nDstStep, int nRoiNumber, VappiRect *pSizeROI,vastStream_t vastStreamCtx)                                                                         
{
    rtError_t vaccRet;
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if( !pSrc|| !pDst || !nDstStep || !oSrcSize.width || !oSrcSize.height || !nSrcStep|| !nRoiNumber || !pSizeROI){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
  
    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    VastStream * stream = (VastStream *)vastStreamCtx;  
    vaccRet =  vapp_find_op_entry(OP_PERMUTE, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_find_op_entry failed.\n");
        return vaccRet;           
    }    
    VAPP_LOG(VAPP_LOG_TRACE, "vappiTranspose_8u_P1R_Ctx. find op ok. \n");
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t)pDst;

    in_params.permute_roi_op_cfg.iimage_shape.iimage_width = oSrcSize.width;
    in_params.permute_roi_op_cfg.iimage_shape.iimage_height = oSrcSize.height;
    in_params.permute_roi_op_cfg.oimage_shape.iimage_width = oSrcSize.height;
    in_params.permute_roi_op_cfg.oimage_shape.iimage_height = oSrcSize.width;
    if(nDstStep < (int)in_params.permute_roi_op_cfg.oimage_shape.iimage_width){
        VAPP_LOG(VAPP_LOG_ERROR, "vappiTranspose_8u_P1R_Ctx Invalid DstStep %d.\n",nDstStep);
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    in_params.permute_roi_op_cfg.ele_bytes = sizeof(unsigned char); 
    in_params.permute_roi_op_cfg.channel = 1;
    in_params.pSizeROI =  calloc(nRoiNumber, sizeof(VappiRect));

    memcpy(in_params.pSizeROI, pSizeROI, nRoiNumber* sizeof(VappiRect));
    for(int i =0; i < nRoiNumber; i++){
        if(in_params.pSizeROI[i].height > oSrcSize.height){
            VAPP_LOG(VAPP_LOG_ERROR, "vappiTranspose_8u_P1R_Ctx roi height exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }
        if(in_params.pSizeROI[i].width > oSrcSize.width){
            VAPP_LOG(VAPP_LOG_ERROR, "vappiTranspose_8u_P1R_Ctx roi width exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }        
        if(in_params.pSizeROI[i].height + in_params.pSizeROI[i].y > oSrcSize.height){
            VAPP_LOG(VAPP_LOG_ERROR, "vappiTranspose_8u_P1R_Ctx roi h+y exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }
        if(in_params.pSizeROI[i].width + in_params.pSizeROI[i].x > oSrcSize.width){
            VAPP_LOG(VAPP_LOG_ERROR, "vappiTranspose_8u_P1R_Ctx roi w+x exceed limit .\n");
            return VAPP_BAD_ARGUMENT_ERROR;
        }                  
    }
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num = nRoiNumber;   
    in_params.op_par.config_op_params = config_geometry_op;
    in_params.op_par.update_cfg = up_transpose_roi;
    
    in_params.op_par.config.config = (void *)&in_params.permute_roi_op_cfg;
    in_params.op_par.config.size = sizeof(in_params.permute_roi_op_cfg);  
    // stream->op_async->block_num = in_params.op_par.block_num;
    VAPP_LOG(VAPP_LOG_DEBUG, "vappiTranspose_8u_P1R_Ctx. param config done. \n");    
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    }   
    VAPP_LOG(VAPP_LOG_TRACE, "vappiTranspose_8u_P1R_Ctx. run op done. \n"); 
    free(in_params.pSizeROI);
    in_params.pSizeROI = NULL;
    return VAPP_SUCCESS;             
}

float av_clipf_c(float a, float amin, float amax)
{
    if (a < amin) return amin;
    else if (a > amax) return amax;
    else               return a;
}


VAPP_API VappStatus
vappiYUV420SAD_8u_P3(unsigned int devID, const Vapp8u * const pSrc, const Vapp8u * const pSrc2,
                        VappiSize oSrcSize, int nSrcStep, const Vapp8u * const pDst)
{
    VappStatus vaccRet;
    geometry_input_params  in_params = {0};
    sad_para_t *cfg = &in_params.sad_cfg;
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if( !pSrc || !pSrc2 ||!oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }
    in_params.op_par.elf_file = malloc(strlen(op_path) + strlen(ELF_FILE_NAME) + 2) ;
    snprintf(in_params.op_par.elf_file, strlen(op_path) + strlen(ELF_FILE_NAME) + 2, "%s%s", op_path, ELF_FILE_NAME);
    in_params.op_par.custom_op_name = SAD_OP_FUNC;
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t)pSrc2;
    in_params.op_par.inout_addr[2] = (uint64_t)pDst;

    in_params.in_img.width = oSrcSize.width;
    in_params.in_img.height = oSrcSize.height;
    in_params.in_img.w_pitch = nSrcStep;
    in_params.in_img.h_pitch = oSrcSize.height;

    in_params.out_img.width = oSrcSize.width;
    in_params.out_img.height = oSrcSize.height;
    in_params.out_img.w_pitch = nSrcStep;
    in_params.out_img.h_pitch = oSrcSize.height;

    in_params.image_format = YUV_I420;
    in_params.nOutputs = 1;
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num = 1;
    in_params.op_par.config_op_params = config_geometry_op;
    cfg->in_image_shape = in_params.in_img;
    cfg->img_type = in_params.image_format;
    in_params.op_par.config.config = (void *)&in_params.sad_cfg;
    in_params.op_par.config.size = sizeof(in_params.sad_cfg);      
    vaccRet = vapp_run_op(devID, &in_params.op_par);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op yuv420 sad failed.\n");
        free(in_params.op_par.elf_file);
        return vaccRet;
    }
    free(in_params.op_par.elf_file);
    return VAPP_SUCCESS;
}

VAPP_API VappStatus
vappiStaticTextDetection_8u_P3_Host(unsigned int devID, VappiTextDetectionBuffers *buffers, VappiSize oSrcSize, Vapp32u nSrcStep,
                        VappiTextDetectionParam *param, Vapp8u first_frame)
{
    //VappStatus vaccRet;

    if( !buffers->in_img_addr || !buffers->out_roi_map_final_addr || !buffers->out_gray_addr ||!oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    dection_para_t dect_param;

    dect_param.img_type = param->plane == 4 ? _XI_TILE_RGBA_TYPE_ : (param->plane == 3 ? _XI_TILE_RGB888_TYPE_ : _XI_TILE_YUV_NV12_TYPE_);
    dect_param.in_image_shape.width   = oSrcSize.width;
    dect_param.in_image_shape.height  = oSrcSize.height;
    dect_param.in_image_shape.w_pitch = oSrcSize.width * param->plane;
    dect_param.in_image_shape.h_pitch = oSrcSize.height;
    dect_param.params                 = *param;
    dect_param.first_frame            = first_frame;

    run_detection_test(buffers, &dect_param);

    return VAPP_SUCCESS;
}


VAPP_API VappStatus
vappiStaticTextDetection_8u_P3_Ctx(unsigned int devID, VappiTextDetectionBuffers *buffers, VappiSize oSrcSize, Vapp32u nSrcStep,
                        VappiTextDetectionParam *param, Vapp8u first_frame, vastStream_t vastStreamCtx)
{
    VappStatus vaccRet;
    geometry_input_params  in_params = {0};
    dection_para_t *cfg = &in_params.dect_param;
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if( !buffers || !buffers->in_img_addr || !buffers->out_gray_addr || !buffers->out_roi_map_final_addr ||!oSrcSize.width || !oSrcSize.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }

    VastStream * stream = (VastStream *)vastStreamCtx;  
    vaccRet =  vapp_find_op_entry(OP_DETECTION, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_find_op_entry failed.\n");
        return vaccRet;           
    }    


    in_params.op_par.inout_addr[0] = (uint64_t)buffers->in_img_addr;
    in_params.op_par.inout_addr[1] = (uint64_t)buffers->out_roi_map_final_addr;
    in_params.op_par.inout_addr[2] = (uint64_t)buffers->out_gray_addr;
    in_params.op_par.inout_addr[3] = (uint64_t)buffers->pre_gray_addr;
    in_params.op_par.inout_addr[4] = (uint64_t)buffers->out_sobelx_addr;
    in_params.op_par.inout_addr[5] = (uint64_t)buffers->out_sobely_addr;
    in_params.op_par.inout_addr[6] = (uint64_t)buffers->out_laplacian_addr;


    in_params.in_img.width = oSrcSize.width;
    in_params.in_img.height = oSrcSize.height;
    in_params.in_img.w_pitch = nSrcStep;
    in_params.in_img.h_pitch = oSrcSize.height;

    in_params.out_img.width = oSrcSize.width;
    in_params.out_img.height = oSrcSize.height;
    in_params.out_img.w_pitch = nSrcStep;
    in_params.out_img.h_pitch = oSrcSize.height;

    in_params.image_format = param->plane == 4 ? RGBA_INTER : (param->plane == 3 ? RGB888 : YUV_I420);
    in_params.nOutputs = 1;
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num = 1;
    in_params.op_par.config_op_params = config_geometry_op;
    in_params.dect_param.params = *param;
    in_params.dect_param.first_frame = first_frame;
    in_params.op_par.config.config = (void *)&in_params.dect_param;
    in_params.op_par.config.size = sizeof(in_params.dect_param);      
    cfg->in_image_shape = in_params.in_img;
    cfg->img_type = in_params.image_format;
    in_params.ctx = stream;
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    //vaccRet = vapp_run_op(devID, &in_params.op_par);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op_multi_async text detect failed.\n");
        //free(in_params.op_par.elf_file);
        return vaccRet;
    }
    //free(in_params.op_par.elf_file); 

    return VAPP_SUCCESS;
}




static VappStatus 
inner_vappiResize_8u_P_X_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u **pDst, VappiShape2D *oDstShape,int nImageFormat, int oOutnumber, int eInterpolation, vastStream_t vastStreamCtx)
{
    VappStatus vaccRet;
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
     if( !pSrc|| !pDst || !oSrcShape.width || !oSrcShape.height || !oSrcShape.wPitch || !oDstShape[0].width || !oDstShape[0].height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    VastStream * stream = (VastStream *)vastStreamCtx;
    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }  
    vaccRet =  vapp_find_op_entry(OP_SCALE_U8, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    } 
    in_params.nOutputs = oOutnumber;
    in_params.op_par.custom_op_name = RESIZE_OP_FUNC;
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    for (int i = 1; i <= in_params.nOutputs; i++) {
        in_params.op_par.inout_addr[i] = (uint64_t )pDst[i-1];
    }
    
    in_params.scale_cfg.iimage_shape.width = oSrcShape.width;
    in_params.scale_cfg.iimage_shape.height = oSrcShape.height;
    in_params.scale_cfg.iimage_shape.w_pitch = oSrcShape.wPitch;
    in_params.scale_cfg.iimage_shape.h_pitch = oSrcShape.hPitch;    

    //cfg->iimage_shape  = in_params->in_img;
    
    for (int i = 0; i < in_params.nOutputs; i++) {
        in_params.scale_cfg.oimage_shape[i].width = oDstShape[i].width;
        in_params.scale_cfg.oimage_shape[i].height = oDstShape[i].height;
        in_params.scale_cfg.oimage_shape[i].w_pitch = oDstShape[i].wPitch;
        in_params.scale_cfg.oimage_shape[i].h_pitch = oDstShape[i].hPitch;
    }    

    //in_params.image_format = YUV_I420;
    in_params.scale_cfg.oimage_cnt = in_params.nOutputs;
    in_params.scale_cfg.resize_type = eInterpolation;
	in_params.op_par.config_op_params = config_geometry_op;    
    in_params.scale_cfg.image_format = nImageFormat;
    in_params.eInterpolation = eInterpolation;
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num  = 1;
    in_params.op_par.config.config = (void *)&in_params.scale_cfg;
    in_params.op_par.config.size = sizeof(in_params.scale_cfg);    

    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op crop failed.\n");
        return vaccRet;           
    } 
    return VAPP_SUCCESS;     
}



VappStatus 
vappiYUV420PResize_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u *pDst, VappiShape2D oDstShape,int eInterpolation, vastStream_t vastStreamCtx)
{
   VappStatus vaccRet;
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
     if( !pSrc|| !pDst || !oSrcShape.width || !oSrcShape.height || !oSrcShape.wPitch || !oDstShape.width || !oDstShape.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    VastStream * stream = (VastStream *)vastStreamCtx;
    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }  
    vaccRet =  vapp_find_op_entry(OP_SCALE_U8, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    } 
    in_params.nOutputs = 1;
    in_params.eInterpolation = eInterpolation;
    in_params.op_par.custom_op_name = RESIZE_OP_FUNC;
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )pDst;
    
    in_params.scale_cfg.iimage_shape.width = oSrcShape.width;
    in_params.scale_cfg.iimage_shape.height = oSrcShape.height;
    in_params.scale_cfg.iimage_shape.w_pitch = oSrcShape.wPitch;
    in_params.scale_cfg.iimage_shape.h_pitch = oSrcShape.hPitch;    

    in_params.scale_cfg.oimage_shape[0].width = oDstShape.width;
    in_params.scale_cfg.oimage_shape[0].height = oDstShape.height;
    in_params.scale_cfg.oimage_shape[0].w_pitch = oDstShape.wPitch;
    in_params.scale_cfg.oimage_shape[0].h_pitch = oDstShape.hPitch;   

    in_params.scale_cfg.image_format = YUV_I420;
    in_params.scale_cfg.oimage_cnt = in_params.nOutputs;
    in_params.scale_cfg.resize_type = in_params.eInterpolation;
    
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num  = 1;
    in_params.op_par.config_op_params = config_geometry_op;
    in_params.op_par.config.config = (void *)&in_params.scale_cfg;
    in_params.op_par.config.size = sizeof(in_params.scale_cfg);  

    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op crop failed.\n");
        return vaccRet;           
    } 
    return VAPP_SUCCESS;        
}

VappStatus vappiNV12Resize_8u_P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u **pDst, VappiShape2D *oDstShape, int eInterpolation, vastStream_t vastStreamCtx)
{
    return inner_vappiResize_8u_P_X_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape,YUV_NV12, 1, eInterpolation, vastStreamCtx);     
}

VappStatus 
vappiYUV420Nout_Resize_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u **pDst, VappiShape2D *oDstShape,int oOutnumber,int eInterpolation, vastStream_t vastStreamCtx)
{
    return inner_vappiResize_8u_P_X_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, YUV_I420,oOutnumber,eInterpolation, vastStreamCtx);     
}

VappStatus 
vappiNV12NoutResize_8u_P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u **pDst, VappiShape2D *oDstShape,int oOutnumber,int eInterpolation, vastStream_t vastStreamCtx)
{
    return inner_vappiResize_8u_P_X_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, YUV_NV12,oOutnumber,eInterpolation, vastStreamCtx);     
}

static VappStatus 
inner_vappiCropscale_8u_P_X_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u **pDst, VappiShape2D *oDstShape, int offsetWidth, int offsetHeight, int cropWidth, int cropHeight,
                            int oOutnumber, int eInterpolation, vastStream_t vastStreamCtx)
{
    VappStatus vaccRet;
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
     if( !pSrc|| !pDst || !oSrcShape.width || !oSrcShape.height || !oSrcShape.wPitch || !oDstShape[0].width || !oDstShape[0].height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    VastStream * stream = (VastStream *)vastStreamCtx;
    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }
    vaccRet =  vapp_find_op_entry(OP_CROPSCALE, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 cropscale failed.\n");
        return vaccRet;           
    } 
    in_params.nOutputs = oOutnumber;
    in_params.op_par.custom_op_name = CROPSCALE_OP_FUNC;
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    for (int i = 1; i <= in_params.nOutputs; i++) {
        in_params.op_par.inout_addr[i] = (uint64_t )pDst[i-1];
    }
   
    in_params.cropscale_cfg.input_shape.height = oSrcShape.height;
    in_params.cropscale_cfg.input_shape.width = oSrcShape.width;
    in_params.cropscale_cfg.input_shape.h_pitch = oSrcShape.hPitch;    
    in_params.cropscale_cfg.input_shape.w_pitch = oSrcShape.wPitch;
    
    for (int i = 0; i < in_params.nOutputs; i++) {
        in_params.cropscale_cfg.output_shape[i].height = oDstShape[i].height;
        in_params.cropscale_cfg.output_shape[i].width = oDstShape[i].width;
        in_params.cropscale_cfg.output_shape[i].h_pitch = oDstShape[i].hPitch;
        in_params.cropscale_cfg.output_shape[i].w_pitch = oDstShape[i].wPitch;
    }
    in_params.cropscale_cfg.offset_w = offsetWidth;
    in_params.cropscale_cfg.offset_h = offsetHeight;
    in_params.cropscale_cfg.crop_width = cropWidth;
    in_params.cropscale_cfg.crop_height = cropHeight;
    in_params.cropscale_cfg.scale_method = eInterpolation;
    in_params.cropscale_cfg.oimage_cnt   = oOutnumber;
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num  = 1;
    in_params.op_par.config_op_params = config_geometry_op;
    in_params.op_par.config.config = (void *)&in_params.cropscale_cfg;
    in_params.op_par.config.size = sizeof(in_params.cropscale_cfg);      

    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op crop failed.\n");
        return vaccRet;           
    } 
    return VAPP_SUCCESS;     
}

VappStatus 
vappiYUV420Nout_Cropscale_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u **pDst, VappiShape2D *oDstShape, int offsetWidth, int offsetHeight, int cropWidth, int cropHeight,
                            int oOutnumber,int eInterpolation, vastStream_t vastStreamCtx)
{
    return inner_vappiCropscale_8u_P_X_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, offsetWidth, offsetHeight, cropWidth, cropHeight, oOutnumber, eInterpolation, vastStreamCtx);
}

VappStatus 
vappiNV12Nout_Cropscale_8u_P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u **pDst, VappiShape2D *oDstShape, int offsetWidth, int offsetHeight, int cropWidth, int cropHeight,
                            int oOutnumber,int eInterpolation, vastStream_t vastStreamCtx)
{
    return inner_vappiCropscale_8u_P_X_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, offsetWidth, offsetHeight, cropWidth, cropHeight, oOutnumber, eInterpolation, vastStreamCtx);
}