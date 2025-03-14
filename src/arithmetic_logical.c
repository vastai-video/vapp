#include <stdint.h> 
#include <string.h>
#include "vapp.h"
#include "vdsp_op.h"
#include "misc_params.h"

#if 0
static int config_al_op(void * priv_params, uint32_t entry)
{
    int i = 0;
    al_input_params * in_params = (al_input_params *)priv_params;
    op_info_t * op_info = &in_params->op_par.op_info;
             
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
    p->outputCount = 1;
    p->config = 0;//runtime 分配完config内存填写地址
    for(i =0; i < MAX_IN_OUT_ADDR; i++){
        if(in_params->op_par.inout_addr[i]){
            p->inout_addr[i] = in_params->op_par.inout_addr[i];
        }else{
            break;
        }
    }
    /* config */
    // in_params->op_par.config.config = (void *)&in_params->csc_cfg;
    // in_params->op_par.config.size = sizeof(in_params->csc_cfg);
    op_info->config_array = &in_params->op_par.config;
    op_info->config_num = 1;
    return 0;
}
#endif

static int config_overlay_op(void * priv_params, uint32_t entry)
{
    int i = 0;
    al_input_params * in_params = (al_input_params *)priv_params;
    op_info_t * op_info = &in_params->op_par.op_info;
             
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
    p->inputCount = 2;
    p->outputCount = 0;
    p->config = 0;//runtime 分配完config内存填写地址
    for(i =0; i < MAX_IN_OUT_ADDR; i++){
        if(in_params->op_par.inout_addr[i]){
            p->inout_addr[i] = in_params->op_par.inout_addr[i];
        }else{
            break;
        }
    }
    /* config */
    // in_params->op_par.config.config = (void *)&in_params->csc_cfg;
    // in_params->op_par.config.size = sizeof(in_params->csc_cfg);
    op_info->config_array = &in_params->op_par.config;
    op_info->config_num = 1;
    return 0;
}

VappStatus vappiNV12Overlay_8u_P2_Ctx(unsigned int devID, const Vapp8u * const pSrc1, VappiShape2D oSrc1Shape,
                            Vapp8u *pSrc2, VappiShape2D  oSrc2Shape, int nX, int nY, vastStream_t vastStreamCtx)
{  
    VappStatus vaccRet;
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    if( !pSrc1|| !pSrc2 || !oSrc1Shape.width || !oSrc1Shape.height || !oSrc1Shape.wPitch || !oSrc2Shape.width || !oSrc2Shape.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    VastStream * stream = (VastStream *)vastStreamCtx;
    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }  
    vaccRet =  vapp_find_op_entry(OP_FFMPEG_OVERLAY, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    } 

    in_params.op_par.custom_op_name = NV12OVERLAY_OP_FUNC;    
    in_params.ovl_cfg.back_img.width = oSrc1Shape.width;
    in_params.ovl_cfg.back_img.height = oSrc1Shape.height;
    in_params.ovl_cfg.back_img.w_pitch = oSrc1Shape.wPitch;
    in_params.ovl_cfg.back_img.h_pitch = oSrc1Shape.hPitch;    

    in_params.nOutputs = 1;
    in_params.ovl_cfg.layer_img.img.width   = oSrc2Shape.width;
    in_params.ovl_cfg.layer_img.img.height  = oSrc2Shape.height;
    in_params.ovl_cfg.layer_img.img.w_pitch = oSrc2Shape.wPitch;
    in_params.ovl_cfg.layer_img.img.h_pitch = oSrc2Shape.hPitch;   

    in_params.ovl_cfg.layer_img.x = nX;
    in_params.ovl_cfg.layer_img.y = nY;

    in_params.op_par.inout_addr[0] = (uint64_t)pSrc1;
    in_params.op_par.inout_addr[1] = (uint64_t)pSrc2;
    in_params.op_par.inout_addr[2] = (uint64_t)pSrc1;

    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num  = 1;
    in_params.op_par.config.config = (void *)&in_params.ovl_cfg;
    in_params.op_par.config.size = sizeof(in_params.ovl_cfg);          
    in_params.op_par.config_op_params = config_overlay_op;
       
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op crop failed.\n");
        return vaccRet;           
    } 
    return VAPP_SUCCESS;          
}

static int config_adaptive_threshold(void * priv_params, uint32_t entry)
{
    int i = 0;
    al_input_params * in_params = (al_input_params *)priv_params;
    op_info_t * op_info = &in_params->op_par.op_info;
    adaptiveThreshold_cfg_t *cfg = &in_params->adaptive_threshold_cfg;
    // VastStream* stream_ctx = (VastStream*)in_params->ctx;
    // RTStream* stream = (RTStream*)&stream_ctx->rt_stream[in_params->op_par.block_id];
    
    cfg->roi.x = in_params->p_size_roi[in_params->op_par.block_id].x;
    cfg->roi.y = in_params->p_size_roi[in_params->op_par.block_id].y;
    cfg->roi.z = in_params->p_size_roi[in_params->op_par.block_id].z;
    cfg->roi.c = in_params->p_size_roi[in_params->op_par.block_id].channels;
    cfg->roi.h = in_params->p_size_roi[in_params->op_par.block_id].height;
    cfg->roi.w = in_params->p_size_roi[in_params->op_par.block_id].width;

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
    p->outputCount = 1;
    p->config = 0;//runtime 分配完config内存填写地址
    // fprintf(stderr, "adaptive op_stream_id  %d op_data_id %d\n", stream->stream_id, stream->op_async->hope_op_times);
    // fprintf(stderr, "adaptive roi c_start %d h_start %d w_start %d c %d h %d w %d\n", 
    // cfg->roi.z,cfg->roi.x,cfg->roi.y,
    // cfg->roi.c,cfg->roi.h,cfg->roi.w);
    // fprintf(stderr, "adaptive op_addr  %x \n", op_info->op_addr);
    

    for(i =0; i < MAX_IN_OUT_ADDR; i++){
        if(in_params->op_par.inout_addr[i]){
            p->inout_addr[i] = in_params->op_par.inout_addr[i];
            //fprintf(stderr, "adaptive inout_addr %d  %"PRIx64"x \n", i, p->inout_addr[i]);	
        }else{
            break;
        }
    }

    // fprintf(stderr, "adaptive cfg inshape  c h w: %d %d %d  pitch_ c h w:%d %d %d\n", cfg->in_shape.channel, cfg->in_shape.height, cfg->in_shape.width,
    //     cfg->in_shape.c_pitch, cfg->in_shape.h_pitch, cfg->in_shape.w_pitch);
    // fprintf(stderr, "adaptive cfg threshold  %f\n", cfg->meanPara.threshold);
    // fprintf(stderr, "adaptive cfg blockSize  %d\n", cfg->meanPara.blockSize);
    // fprintf(stderr, "adaptive cfg maxValue  %d\n", cfg->meanPara.maxValue);
    // fprintf(stderr, "adaptive cfg method  %d\n", cfg->meanPara.method);
    // fprintf(stderr, "adaptive cfg type  %d\n", cfg->meanPara.type);
    /* config */
    in_params->op_par.config.config = (void *)&in_params->adaptive_threshold_cfg;
    in_params->op_par.config.size = sizeof(in_params->adaptive_threshold_cfg);
    op_info->config_array = &in_params->op_par.config;
    op_info->config_num = 1;
    return 0;
}


VappStatus 
vappiGrayAdaptiveThreshold_8u_P1R_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiSize oSrcSize, int nSrcStep, 
                            Vapp8u *pDst, int nDstStep, int nRoiNumber, VappiRect *pSizeROI, 
                            Vapp64f nThreshold, int nBlockSize, int nMaxValue, vastStream_t vastStreamCtx)                                                                         
{
    rtError_t vaccRet;
    int i = 0;
    al_input_params  in_params = {0};
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
    vaccRet =  vapp_find_op_entry(OP_ADAPTIVETHRESHOLD, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_find_op_entry failed.\n");
        return vaccRet;           
    }    
    if(nRoiNumber != stream->block_num){
        VAPP_LOG(VAPP_LOG_ERROR, "Invalid block num.\n");
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t)pDst;

    in_params.adaptive_threshold_cfg.in_shape.width = oSrcSize.width;
    in_params.adaptive_threshold_cfg.in_shape.height = oSrcSize.height;
    in_params.adaptive_threshold_cfg.in_shape.w_pitch = nSrcStep;
    in_params.adaptive_threshold_cfg.in_shape.h_pitch = oSrcSize.height;  
    in_params.adaptive_threshold_cfg.in_shape.channel = 1;  
    in_params.adaptive_threshold_cfg.in_shape.c_pitch = 1; 

    in_params.adaptive_threshold_cfg.meanPara.threshold = nThreshold; 
    in_params.adaptive_threshold_cfg.meanPara.blockSize = nBlockSize; 
    in_params.adaptive_threshold_cfg.meanPara.maxValue = nMaxValue; 
    in_params.adaptive_threshold_cfg.meanPara.method = ADAPTIVE_THRESH_MEAN_C;
    in_params.adaptive_threshold_cfg.meanPara.type = CV_THRESH_BINARY_INV;

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
    }    
    in_params.p_size_roi = stream->size_roi;
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num = nRoiNumber;   
    in_params.op_par.config_op_params = config_adaptive_threshold;
    in_params.ctx = stream;
    // stream->op_async->block_num = in_params.op_par.block_num;
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgb24 resize_m failed.\n");
        return vaccRet;           
    }   

    return VAPP_SUCCESS;             
}

