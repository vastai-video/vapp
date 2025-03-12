#include "misc_params.h"

static int config_rotate_op(void * priv_params, uint32_t entry)
{
    geometry_input_params *in_params = (geometry_input_params *)priv_params;
    op_info_t * op_info = &in_params->op_par.op_info;
    rotate_param_t *cfg = &in_params->rotete_cfg;
    int i = 0;

    cfg->rotate_param[0].in_img = in_params->in_img;

    //cfg->image_type = in_params->image_format;
    if(cfg->image_type == IMAGE_NV12){
        cfg->channel = 1;
    }else if (cfg->image_type == IMAGE_DEFAULT){
        cfg->channel = 3;   //yuv420p
    }
    else{
        fprintf(stderr, "unsupported rotate image type:%d\n", cfg->image_type);
        return -1;  
    }
    cfg->data_type = DATA_TYPE_U8;


    if(cfg->rotate_degree == ROTATE_DEGREE_90 || cfg->rotate_degree == ROTATE_DEGREE_NEG90 
        || cfg->rotate_degree == ROTATE_DEGREE_270 || cfg->rotate_degree==ROTATE_DEGREE_NEG270){
        cfg->rotate_param[0].out_img.width  = in_params->out_img.height;
        cfg->rotate_param[0].out_img.height = in_params->out_img.width;  
        cfg->rotate_param[0].out_img.h_pitch = in_params->out_img.w_pitch;//in_params->i_dst_step;
        cfg->rotate_param[0].out_img.w_pitch = in_params->out_img.h_pitch; 
    }else if(cfg->rotate_degree == ROTATE_DEGREE_180){
        cfg->rotate_param[0].out_img.width  = in_params->out_img.width;  
        cfg->rotate_param[0].out_img.height = in_params->out_img.height;  
        cfg->rotate_param[0].out_img.w_pitch = in_params->out_img.w_pitch;
        cfg->rotate_param[0].out_img.h_pitch = in_params->out_img.h_pitch;;                  
    }else{
        fprintf(stderr, "unsupported degree\n");
        return -1;          
    }  

    // printf("cfg->rotate_param[0].in_img.width:%d\n",cfg->rotate_param[0].in_img.width); 
    // printf("cfg->rotate_param[0].in_img.height:%d\n",cfg->rotate_param[0].in_img.height); 
    // printf(" cfg->rotate_param[0].in_img.w_pitch:%d\n", cfg->rotate_param[0].in_img.w_pitch); 
    // printf("cfg->rotate_param[0].in_img.h_pitch:%d\n",cfg->rotate_param[0].in_img.h_pitch); 

    // printf("cfg->rotate_param[0].out_img.width:%d\n",cfg->rotate_param[0].out_img.width); 
    // printf("cfg->rotate_param[0].out_img.height:%d\n",cfg->rotate_param[0].out_img.height); 
    // printf(" cfg->rotate_param[0].out_img.w_pitch:%d\n", cfg->rotate_param[0].out_img.w_pitch); 
    // printf("cfg->rotate_param[0].out_img.h_pitch:%d\n",cfg->rotate_param[0].out_img.h_pitch); 
    if(cfg->rotate_param[0].out_img.w_pitch < cfg->rotate_param[0].out_img.width 
    || cfg->rotate_param[0].out_img.h_pitch < cfg->rotate_param[0].out_img.height){
        fprintf(stderr, "invalid dst step pw:%d w:%d ph:%d h:%d\n", cfg->rotate_param[0].out_img.w_pitch, cfg->rotate_param[0].out_img.width,cfg->rotate_param[0].out_img.h_pitch,cfg->rotate_param[0].out_img.height);
        return -1;  
    }

    /*printf("in data w:%d h:%d wp:%d hp:%d out data w:%d h:%d wp:%d hp:%d\n",
        cfg->rotate_param[0].in_img.width, cfg->rotate_param[0].in_img.height, cfg->rotate_param[0].in_img.w_pitch, cfg->rotate_param[0].in_img.h_pitch,
        cfg->rotate_param[0].out_img.width, cfg->rotate_param[0].out_img.height, cfg->rotate_param[0].out_img.w_pitch, cfg->rotate_param[0].out_img.h_pitch
    );*/

    for(i = 1; i < cfg->channel; i++){
        cfg->rotate_param[i].out_img.width  = cfg->rotate_param[0].out_img.width/2;
        cfg->rotate_param[i].out_img.height = cfg->rotate_param[0].out_img.height/2;
        cfg->rotate_param[i].out_img.w_pitch = cfg->rotate_param[0].out_img.w_pitch/2;
        cfg->rotate_param[i].out_img.h_pitch = cfg->rotate_param[0].out_img.h_pitch/2;

        cfg->rotate_param[i].in_img.width 	= cfg->rotate_param[0].in_img.width 	/2;
        cfg->rotate_param[i].in_img.height 	= cfg->rotate_param[0].in_img.height 	/2;
        cfg->rotate_param[i].in_img.w_pitch = cfg->rotate_param[0].in_img.w_pitch   /2;
        cfg->rotate_param[i].in_img.h_pitch = cfg->rotate_param[0].in_img.h_pitch   /2;
    }

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
    p->inputCount = cfg->channel;
    p->outputCount = cfg->channel;
    p->config = 0;//runtime 分配完config内存填写地址

    if (cfg->channel == 3)
    {
        p->inout_addr[0] = in_params->op_par.inout_addr[0];
        p->inout_addr[1] = p->inout_addr[0] + cfg->rotate_param[0].in_img.w_pitch * cfg->rotate_param[0].in_img.h_pitch;
        p->inout_addr[2] = p->inout_addr[1] +  cfg->rotate_param[1].in_img.w_pitch * cfg->rotate_param[1].in_img.h_pitch;

        p->inout_addr[3] = in_params->op_par.inout_addr[1];
        p->inout_addr[4] = p->inout_addr[3] + cfg->rotate_param[0].out_img.w_pitch * cfg->rotate_param[0].out_img.h_pitch;
        p->inout_addr[5] = p->inout_addr[4] +  cfg->rotate_param[1].out_img.w_pitch * cfg->rotate_param[1].out_img.h_pitch;
    }
    else{
         p->inout_addr[0] = in_params->op_par.inout_addr[0];
         p->inout_addr[1] = in_params->op_par.inout_addr[1];
    }
    /* config */
    in_params->op_par.config.config = (void *)cfg;
    in_params->op_par.config.size = sizeof(*cfg);
    op_info->config_array = &in_params->op_par.config;
    op_info->config_num = 1;
    return 0;
}  

static int config_filtering_op(void * priv_params, uint32_t entry)
{
    int i = 0;
    geometry_input_params *in_params = (geometry_input_params *)priv_params;
    int nb_outputs = in_params->nOutputs;
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

static VappStatus 
inner_vappiUnsharp_8u_Px_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape,int nImageFormat, float nLumaAmount, float nChromAmount, vastStream_t vastStreamCtx)
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
    vaccRet =  vapp_find_op_entry(OP_UNSHARP, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "find unsharp failed.\n");
        return vaccRet;           
    } 

    in_params.op_par.custom_op_name = UNSHARP_OP_FUNC;     
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )pDst;
    
    in_params.unsharp_cfg.in_image_shape.width = oSrcShape.width;
    in_params.unsharp_cfg.in_image_shape.height = oSrcShape.height;
    in_params.unsharp_cfg.in_image_shape.w_pitch = oSrcShape.wPitch;
    in_params.unsharp_cfg.in_image_shape.h_pitch = oSrcShape.hPitch;    

    in_params.nOutputs = 1;
    in_params.unsharp_cfg.out_image_shape.width   = oDstShape.width;
    in_params.unsharp_cfg.out_image_shape.height  = oDstShape.height;
    in_params.unsharp_cfg.out_image_shape.w_pitch = oDstShape.wPitch;
    in_params.unsharp_cfg.out_image_shape.h_pitch = oDstShape.hPitch;   

    if(oSrcShape.width != oDstShape.width || oSrcShape.height != oDstShape.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    in_params.unsharp_cfg.img_type = nImageFormat;
    in_params.unsharp_cfg.image_unsharp_param.lamount = nLumaAmount;
    in_params.unsharp_cfg.image_unsharp_param.camount = nChromAmount;

    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num  = 1;
    in_params.op_par.config.config = (void *)&in_params.unsharp_cfg;
    in_params.op_par.config.size = sizeof(in_params.unsharp_cfg);          
    in_params.op_par.config_op_params = config_filtering_op;
       
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op unsharp failed.\n");
        return vaccRet;           
    } 
    return VAPP_SUCCESS;     
}

VappStatus 
vappiNV12Unsharp_8u_Px_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, float nLumaAmount, float nChromAmount, vastStream_t vastStreamCtx)
{
    
    return inner_vappiUnsharp_8u_Px_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, YUV_NV12, nLumaAmount, nChromAmount, vastStreamCtx);     
}

VappStatus 
vappiYUV420Unsharp_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, float nLumaAmount, float nChromAmount, vastStream_t vastStreamCtx)
{
    return inner_vappiUnsharp_8u_Px_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, YUV_I420, nLumaAmount, nChromAmount, vastStreamCtx);  
}

static VappStatus 
inner_vappiHqdn3d_8u_Px_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, int nImageFormat,float luma_temporal  , float chroma_temporal   , vastStream_t vastStreamCtx)
{
    VappStatus vaccRet;
    int vppRet = 0;
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
    vaccRet =  vapp_find_op_entry(OP_HQDN3D, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "find hqdn3d failed.\n");
        return vaccRet;           
    }
    
    static int first_flags = 1;
    if(first_flags == 1){
        in_params.hqdn3d_cfg.first_flag = 1;
        stream->hqdn3d_buffer = NULL;  
        first_flags = 0;
        // printf("in_params.hqdn3d_cfg.first_flag:%d\n",in_params.hqdn3d_cfg.first_flag);
    }else{
        in_params.hqdn3d_cfg.first_flag = 0;
        // printf("in_params.hqdn3d_cfg.first_flag:%d\n",in_params.hqdn3d_cfg.first_flag);
    }

    uint32_t input_buffsize =  oSrcShape.wPitch*oSrcShape.hPitch*3/2;
    uint32_t hqdn3d_buffer_size = input_buffsize * sizeof(uint16_t) + (8 * 1024 * sizeof(int16_t)) * 4;
    if(stream->hqdn3d_buffer == NULL){
        vppRet = vastMalloc(devID, (void**)&stream->hqdn3d_buffer,hqdn3d_buffer_size);
        if(vppRet != VAPP_SUCCESS){
            VAPP_LOG(VAPP_LOG_ERROR, "vastMalloc failed.\n");
            return vppRet;
        }
    }  
    in_params.hqdn3d_cfg.hqdn3d_enum = denoise_temporal; 

    in_params.op_par.custom_op_name = HQDN3D_OP_FUNC;     
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )stream->hqdn3d_buffer;
    in_params.op_par.inout_addr[2] = (uint64_t )pDst;
    
    in_params.hqdn3d_cfg.in_image_shape.width = oSrcShape.width;
    in_params.hqdn3d_cfg.in_image_shape.height = oSrcShape.height;
    in_params.hqdn3d_cfg.in_image_shape.w_pitch = oSrcShape.wPitch;
    in_params.hqdn3d_cfg.in_image_shape.h_pitch = oSrcShape.hPitch;    

    in_params.nOutputs = 1;
    in_params.hqdn3d_cfg.out_image_shape.width   = oDstShape.width;
    in_params.hqdn3d_cfg.out_image_shape.height  = oDstShape.height;
    in_params.hqdn3d_cfg.out_image_shape.w_pitch = oDstShape.wPitch;
    in_params.hqdn3d_cfg.out_image_shape.h_pitch = oDstShape.hPitch;   

    if(oSrcShape.width != oDstShape.width || oSrcShape.height != oDstShape.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    in_params.hqdn3d_cfg.image_type = nImageFormat;
    in_params.hqdn3d_cfg.luma_temporal = luma_temporal;
    in_params.hqdn3d_cfg.chroma_temporal = chroma_temporal;

    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num  = 1;
    in_params.op_par.config.config = (void *)&in_params.hqdn3d_cfg;
    in_params.op_par.config.size = sizeof(in_params.hqdn3d_cfg);          
    in_params.op_par.config_op_params = config_filtering_op;
       
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op hqdn3d failed.\n");
        return vaccRet;           
    } 
    return VAPP_SUCCESS;     
}

VappStatus 
vappiNV12Hqdn3d_8u_Px_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, float luma_temporal  , float chroma_temporal   , vastStream_t vastStreamCtx)
{
   
    return inner_vappiHqdn3d_8u_Px_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, YUV_NV12, luma_temporal,chroma_temporal, vastStreamCtx);     
}

VappStatus 
vappiYUV420Hqdn3d_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, float luma_temporal  , float chroma_temporal   , vastStream_t vastStreamCtx)
{
   return inner_vappiHqdn3d_8u_Px_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, YUV_I420, luma_temporal,chroma_temporal, vastStreamCtx);     
}

static VappStatus 
inner_vappiMirror_8u_Px_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, int nImageFormat, vastStream_t vastStreamCtx, VappiAxis eAxis)
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
    vaccRet =  vapp_find_op_entry(OP_FLIP, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "find mirror failed.\n");
        return vaccRet;           
    }

    in_params.op_par.custom_op_name = FLIP_OP_FUNC;     
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )pDst;
    
    in_params.flip_cfg.in_img.width = oSrcShape.width;
    in_params.flip_cfg.in_img.height = oSrcShape.height;
    in_params.flip_cfg.in_img.w_pitch = oSrcShape.wPitch;
    in_params.flip_cfg.in_img.h_pitch = oSrcShape.hPitch;    

    in_params.nOutputs = 1;
    in_params.flip_cfg.out_img.width   = oDstShape.width;
    in_params.flip_cfg.out_img.height  = oDstShape.height;
    in_params.flip_cfg.out_img.w_pitch = oDstShape.wPitch;
    in_params.flip_cfg.out_img.h_pitch = oDstShape.hPitch;   

    if(oSrcShape.width != oDstShape.width || oSrcShape.height != oDstShape.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    in_params.flip_cfg.ImageFormat = nImageFormat;
    in_params.flip_cfg.direction = eAxis;
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num  = 1;
    in_params.op_par.config.config = (void *)&in_params.flip_cfg;
    in_params.op_par.config.size = sizeof(in_params.flip_cfg);          
    in_params.op_par.config_op_params = config_filtering_op;
       
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op mirror failed.\n");
        return vaccRet;           
    } 
    return VAPP_SUCCESS;
}
VappStatus 
vappiYUV420Mirror_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx, VappiAxis eAxis)
{
    return inner_vappiMirror_8u_Px_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, FLIP_IMAGE_YUV420P, vastStreamCtx, eAxis);
}

VappStatus 
vappiNV12Mirror_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx, VappiAxis eAxis)
{
    return inner_vappiMirror_8u_Px_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, FLIP_IMAGE_NVI2, vastStreamCtx, eAxis);
}

static VappStatus
inner_vappiEQ_8u_Px_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape,int nImageFormat, float brightness, 
                            float contrast, float saturation, vastStream_t vastStreamCtx)
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
    vaccRet =  vapp_find_op_entry(OP_EQ, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "find eq failed.\n");
        return vaccRet;           
    } 

    in_params.op_par.custom_op_name = EQ_OP_FUNC;     
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )pDst;
    
    in_params.eq_cfg.in_image_shape.width = oSrcShape.width;
    in_params.eq_cfg.in_image_shape.height = oSrcShape.height;
    in_params.eq_cfg.in_image_shape.w_pitch = oSrcShape.wPitch;
    in_params.eq_cfg.in_image_shape.h_pitch = oSrcShape.hPitch;    

    in_params.nOutputs = 1;
    in_params.eq_cfg.out_image_shape.width   = oDstShape.width;
    in_params.eq_cfg.out_image_shape.height  = oDstShape.height;
    in_params.eq_cfg.out_image_shape.w_pitch = oDstShape.wPitch;
    in_params.eq_cfg.out_image_shape.h_pitch = oDstShape.hPitch;   

    if(oSrcShape.width != oDstShape.width || oSrcShape.height != oDstShape.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    
    in_params.eq_cfg.img_type = nImageFormat; 
    in_params.eq_cfg.image_eq.brightness = brightness;
    in_params.eq_cfg.image_eq.contrast = contrast;
    in_params.eq_cfg.image_eq.saturation = saturation;

    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num  = 1;
    in_params.op_par.config.config = (void *)&in_params.eq_cfg;
    in_params.op_par.config.size = sizeof(in_params.eq_cfg);          
    in_params.op_par.config_op_params = config_filtering_op;
       
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op eq failed.\n");
        return vaccRet;           
    } 
    return VAPP_SUCCESS;     
}

VappStatus
vappiYUV420EQ_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, float brightness, 
                            float contrast, float saturation, vastStream_t vastStreamCtx)
{

    return inner_vappiEQ_8u_Px_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, 
                             YUV_I420, brightness,contrast, saturation,vastStreamCtx);     
}

VappStatus
vappiNV12EQ_8u_Px_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, float brightness, 
                            float contrast, float saturation, vastStream_t vastStreamCtx)
{
    return inner_vappiEQ_8u_Px_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, 
                             YUV_NV12, brightness,contrast, saturation,vastStreamCtx);     
}

static VappStatus inner_vappiBitDepthConvert_8u_P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape,int nType, vastStream_t vastStreamCtx)                                    
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
    vaccRet =  vapp_find_op_entry(OP_BIT10TO8, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "find bitDepthConvert failed.\n");
        return vaccRet;           
    } 

    in_params.op_par.custom_op_name = BITDEPTHCVT_OP_FUNC;     
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )pDst;
    
    in_params.bit_cvt_cfg.in_img.width = oSrcShape.width;
    in_params.bit_cvt_cfg.in_img.height = oSrcShape.height;
    in_params.bit_cvt_cfg.in_img.w_pitch = oSrcShape.wPitch;
    in_params.bit_cvt_cfg.in_img.h_pitch = oSrcShape.hPitch;    

    in_params.bit_cvt_cfg.type = nType;
    in_params.nOutputs = 1;
    in_params.bit_cvt_cfg.out_img.width   = oDstShape.width;
    in_params.bit_cvt_cfg.out_img.height  = oDstShape.height;
    in_params.bit_cvt_cfg.out_img.w_pitch = oDstShape.wPitch;
    in_params.bit_cvt_cfg.out_img.h_pitch = oDstShape.hPitch;   

    if(oSrcShape.width != oDstShape.width || oSrcShape.height != oDstShape.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num  = 1;
    in_params.op_par.config.config = (void *)&in_params.bit_cvt_cfg;
    in_params.op_par.config.size = sizeof(in_params.bit_cvt_cfg);          
    in_params.op_par.config_op_params = config_filtering_op;
       
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op bitDepthConvert failed.\n");
        return vaccRet;           
    }   
    return VAPP_SUCCESS;
}

VappStatus vappiBitDepthConvert_Bit10ToBit8_8u_P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx)
{
    return inner_vappiBitDepthConvert_8u_P2_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape,BIT_10_TO_8,vastStreamCtx);  
}

VappStatus vappiBitDepthConvert_Bit8ToBit10_8u_P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx)
{
    return inner_vappiBitDepthConvert_8u_P2_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape,BIT_8_TO_10,vastStreamCtx);  
}

VappStatus inner_vappiBayerConvert_8u_C1P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, VappiBayerType eBayerType,vastStream_t vastStreamCtx)
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
    vaccRet =  vapp_find_op_entry(OP_BAYERTONV12, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "find bayerConvert failed.\n");
        return vaccRet;           
    } 

    in_params.op_par.custom_op_name = BAYERTONV12_OP_FUNC;     
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )pDst;
    
    in_params.bayer_cfg.iimage_width = oSrcShape.width;
    in_params.bayer_cfg.iimage_height = oSrcShape.height;
    in_params.bayer_cfg.iimage_width_pitch = oSrcShape.wPitch;
    in_params.bayer_cfg.iimage_height_pitch = oSrcShape.hPitch;    

    in_params.bayer_cfg.bType = eBayerType;
    in_params.nOutputs = 1;
    in_params.bayer_cfg.oimage_width   = oDstShape.width;
    in_params.bayer_cfg.oimage_height  = oDstShape.height;
    in_params.bayer_cfg.oimage_width_pitch = oDstShape.wPitch;
    in_params.bayer_cfg.oimage_height_pitch = oDstShape.hPitch;   

    if(oSrcShape.width != oDstShape.width || oSrcShape.height != oDstShape.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num  = 1;
    in_params.op_par.config.config = (void *)&in_params.bayer_cfg;
    in_params.op_par.config.size = sizeof(in_params.bayer_cfg);          
    in_params.op_par.config_op_params = config_filtering_op;
       
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op bayerConvert failed.\n");
        return vaccRet;           
    }   
    return VAPP_SUCCESS;              
}

VappStatus vappiBayerGRGBToNV12_8u_C1P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx)
{
    return inner_vappiBayerConvert_8u_C1P2_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, VAPPI_BAYER_GRBG, vastStreamCtx);  
}

VappStatus vappiBayerRGGBToNV12_8u_C1P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx)
{
    return inner_vappiBayerConvert_8u_C1P2_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, VAPPI_BAYER_RGGB, vastStreamCtx);  
}

VappStatus vappiBayerBGGRToNV12_8u_C1P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx)
{
    return inner_vappiBayerConvert_8u_C1P2_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, VAPPI_BAYER_BGGR, vastStreamCtx);  
}

VappStatus vappiBayerGBGRToNV12_8u_C1P2_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx)
{
    return inner_vappiBayerConvert_8u_C1P2_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, VAPPI_BAYER_GBRG, vastStreamCtx);
}


static VappStatus  
inner_vappiRotate_8u_P3_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u *pDst, VappiShape2D oDstShape,int nImageFormat, int nAngle, vastStream_t vastStreamCtx)
{
    
    rtError_t vaccRet;
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }

    if( !pSrc|| !pDst || !oSrcShape.width || !oSrcShape.height || !oSrcShape.wPitch || !oDstShape.width || !oDstShape.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

     if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
   
    VastStream * stream = (VastStream *)vastStreamCtx;  
    vaccRet = vapp_find_op_entry(OP_SIMPLE_ROTATE, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "find rotate failed.\n");
        return vaccRet;           
    }      

    // const char * op_path = getenv("VASTAI_VAPP_PATH");
    // if(!op_path){
    //     op_path = DEFAULT_OP_PATH;
    // }    
    // in_params.op_par.elf_file = malloc(strlen(op_path) + strlen(ROTATE_ELF_NAME) + 2);
    // snprintf(in_params.op_par.elf_file, strlen(op_path) + strlen(ROTATE_ELF_NAME) + 2, "%s%s", op_path, ROTATE_ELF_NAME);
    in_params.op_par.custom_op_name = ROTATE_OP_FUNC;     
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )pDst;

    in_params.in_img.width = oSrcShape.width;
    in_params.in_img.height = oSrcShape.height;
    in_params.in_img.w_pitch = oSrcShape.wPitch;
    in_params.in_img.h_pitch = oSrcShape.hPitch; 

    in_params.out_img.width = oDstShape.width;
    in_params.out_img.height = oDstShape.height;
    in_params.out_img.w_pitch = oDstShape.wPitch;
    in_params.out_img.h_pitch = oDstShape.hPitch; 

    in_params.rotete_cfg.image_type = nImageFormat;
    in_params.op_par.priv_params = &in_params;
    in_params.rotete_cfg.rotate_degree = nAngle;
    in_params.op_par.config_op_params = config_rotate_op;
    in_params.op_par.block_num = 1;

    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    //vaccRet = vapp_run_op(devID, &in_params.op_par);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rotate failed.\n");
        free(in_params.op_par.elf_file);
        return vaccRet;           
    }
 
    free(in_params.op_par.elf_file);
    return VAPP_SUCCESS;     

}

VappStatus  
vappiNV12Rotate_8u_P3_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u *pDst, VappiShape2D oDstShape, int nAngle, vastStream_t vastStreamCtx){

    return inner_vappiRotate_8u_P3_Ctx(devID, pSrc,  oSrcShape,pDst,  oDstShape, IMAGE_NV12, nAngle, vastStreamCtx);
}

VappStatus  
vappiYUV420Rotate_8u_P3_Ctx(unsigned int devID, const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u *pDst, VappiShape2D oDstShape, int nAngle, vastStream_t vastStreamCtx){

    return inner_vappiRotate_8u_P3_Ctx(devID, pSrc,  oSrcShape,pDst,  oDstShape, IMAGE_DEFAULT, nAngle, vastStreamCtx);
}

static VappStatus inner_vappiCSC_8u_P2_Ctx(unsigned int devID,const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u *pDst, VappiShape2D oDstShape,VappiColorSpace oSrcColorSpace, Vapp8u nSrcFullRange,
                            VappiColorSpace oDstColorSpace, Vapp8u nDstFullRange,Vapp64f nPeakLuminance,
                            Vapp8u nApproximateGamma, Vapp8u nSceneReferred,vastStream_t vastStreamCtx)
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
    vaccRet =  vapp_find_op_entry(OP_COLOR_SPACE, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "find csc failed.\n");
        return vaccRet;           
    } 

    in_params.op_par.custom_op_name = NV12CSC_OP_FUNC;     
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )pDst;
    
    in_params.csc_cfg.in_img.width = oSrcShape.width;
    in_params.csc_cfg.in_img.height = oSrcShape.height;
    in_params.csc_cfg.in_img.w_pitch = oSrcShape.wPitch;
    in_params.csc_cfg.in_img.h_pitch = oSrcShape.hPitch;    

    in_params.csc_cfg.out_img.width   = oDstShape.width;
    in_params.csc_cfg.out_img.height  = oDstShape.height;
    in_params.csc_cfg.out_img.w_pitch = oDstShape.wPitch;
    in_params.csc_cfg.out_img.h_pitch = oDstShape.hPitch; 

    in_params.csc_cfg.in_color_space = oSrcColorSpace;
    in_params.csc_cfg.out_color_space = oDstColorSpace;
    in_params.csc_cfg.fullrange_in = nSrcFullRange;
    in_params.csc_cfg.fullrange_out = nDstFullRange;
    in_params.csc_cfg.peak_luminance = nPeakLuminance;
    in_params.csc_cfg.approximate_gamma = nApproximateGamma;
    in_params.csc_cfg.scene_referred = nSceneReferred; 

    if(oSrcShape.width != oDstShape.width || oSrcShape.height != oDstShape.height){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    
    in_params.nOutputs = 1;
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num  = 1;
    in_params.op_par.config.config = (void *)&in_params.csc_cfg;
    in_params.op_par.config.size = sizeof(in_params.csc_cfg);          
    in_params.op_par.config_op_params = config_filtering_op;
       
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op csc failed.\n");
        return vaccRet;           
    }   
    return VAPP_SUCCESS;             
}


VappStatus vappiNV12CSC_8u_P2_Ctx(unsigned int devID,const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u *pDst, VappiShape2D oDstShape,VappiColorSpace oSrcColorSpace, Vapp8u nSrcFullRange,
                            VappiColorSpace oDstColorSpace, Vapp8u nDstFullRange,Vapp64f nPeakLuminance,
                            Vapp8u nApproximateGamma, Vapp8u nSceneReferred,vastStream_t vastStreamCtx){
    
    return inner_vappiCSC_8u_P2_Ctx(devID, pSrc, oSrcShape, pDst,oDstShape, oSrcColorSpace, nSrcFullRange,
                            oDstColorSpace, nDstFullRange, nPeakLuminance,nApproximateGamma,  nSceneReferred, vastStreamCtx);
}

VappStatus vappiYUV420CSC_8u_P2_Ctx(unsigned int devID,const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u *pDst, VappiShape2D oDstShape,VappiColorSpace oSrcColorSpace, Vapp8u nSrcFullRange,
                            VappiColorSpace oDstColorSpace, Vapp8u nDstFullRange,Vapp64f nPeakLuminance,
                            Vapp8u nApproximateGamma, Vapp8u nSceneReferred,vastStream_t vastStreamCtx){
    
    return inner_vappiCSC_8u_P2_Ctx(devID, pSrc, oSrcShape, pDst,oDstShape, oSrcColorSpace, nSrcFullRange,
                            oDstColorSpace, nDstFullRange, nPeakLuminance,nApproximateGamma,  nSceneReferred, vastStreamCtx);
}

static VappStatus 
inner_vappiTranspose_8u_Px_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, int nImageFormat, vastStream_t vastStreamCtx, VappiTransposeDirection eDirection)
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
    vaccRet =  vapp_find_op_entry(OP_TRANSPOSE, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "find transpose failed.\n");
        return vaccRet;           
    }

    in_params.op_par.custom_op_name = TRANSPOSE_OP_FUNC;     
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )pDst;

    in_params.transpose_cfg.in_width = oSrcShape.width;
    in_params.transpose_cfg.in_height = oSrcShape.height;
    in_params.transpose_cfg.in_w_pitch = oSrcShape.wPitch;
    in_params.transpose_cfg.in_h_pitch = oSrcShape.hPitch;    

    in_params.nOutputs = 1;
    in_params.transpose_cfg.out_width   = oDstShape.width;
    in_params.transpose_cfg.out_height  = oDstShape.height;
    in_params.transpose_cfg.out_w_pitch = oDstShape.wPitch;
    in_params.transpose_cfg.out_h_pitch = oDstShape.hPitch;   

    if(oSrcShape.width != oDstShape.height || oSrcShape.height != oDstShape.width){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    in_params.transpose_cfg.img_type = nImageFormat;
    in_params.transpose_cfg.direction = eDirection;
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num  = 1;
    in_params.op_par.config.config = (void *)&in_params.transpose_cfg;
    in_params.op_par.config.size = sizeof(in_params.transpose_cfg);          
    in_params.op_par.config_op_params = config_filtering_op;

    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op transpose failed.\n");
        return vaccRet;           
    } 
    return VAPP_SUCCESS;
}
VappStatus 
vappiYUV420Transpose_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx, VappiTransposeDirection eDirection)
{
    return inner_vappiTranspose_8u_Px_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, _XI_TILE_YUV_I420_TYPE_, vastStreamCtx, eDirection);
}

VappStatus 
vappiNV12Transpose_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, 
                            Vapp8u * pDst, VappiShape2D oDstShape, vastStream_t vastStreamCtx, VappiTransposeDirection eDirection)
{
    return inner_vappiTranspose_8u_Px_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, _XI_TILE_YUV_NV12_TYPE_, vastStreamCtx, eDirection);
}

static VappStatus inner_vappiRGBA2NV12_8u_Cx_Px_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u **pDst, VappiShape2D *oDstShape, int eCvtType, int eCvtSpace,vastStream_t vastStreamCtx){
    rtError_t vaccRet;
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

    vaccRet =  vapp_find_op_entry(OP_ARGB2NV12, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "find rgbaColorCvt failed.\n");
        return vaccRet;           
    } 

    in_params.op_par.custom_op_name = CVTCOLOR_U8_OP_FUNC;  
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t)pDst[0];

    in_params.cvtcolor_cfg.iimage_width        = oSrcShape.width;    
    in_params.cvtcolor_cfg.iimage_height       = oSrcShape.height;
    in_params.cvtcolor_cfg.iimage_width_pitch  = oSrcShape.wPitch;
    in_params.cvtcolor_cfg.iimage_height_pitch = oSrcShape.hPitch;  
    in_params.cvtcolor_cfg.oimage_width = oDstShape[0].width;
    in_params.cvtcolor_cfg.oimage_height  = oDstShape[0].height;
    in_params.cvtcolor_cfg.oimage_width_pitch = oDstShape[0].wPitch;  
    in_params.cvtcolor_cfg.oimage_height_pitch = oDstShape[0].hPitch;
    in_params.cvtcolor_cfg.cvt_type = eCvtType;  
   
    in_params.cvtcolor_cfg.color_space = eCvtSpace;   
    in_params.nOutputs = 1;
    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num = 1;
  
    in_params.op_par.config.config = (void *)&in_params.cvtcolor_cfg;
    in_params.op_par.config.size = sizeof(in_params.cvtcolor_cfg);          
    in_params.op_par.config_op_params = config_filtering_op;
    
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op rgbaColorCvt failed.\n");
        return vaccRet;           
    }   
    return VAPP_SUCCESS;
 }


VappStatus vappiRGBA2NV12_8u_C4P2R_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape,
                            Vapp8u **pDst, VappiShape2D *oDstShape,int eCvtSpace, vastStream_t vastStreamCtx)
{
     return inner_vappiRGBA2NV12_8u_Cx_Px_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, COLOR_ARGB2YUV_NV12_8888,  eCvtSpace, vastStreamCtx);
}

static VappStatus 
inner_vappiCas_8u_Px_Ctx(unsigned int devID, 
                    const Vapp8u * const pSrc, VappiShape2D oSrcShape, Vapp8u * pDst, VappiShape2D oDstShape, 
                    int nImageFormat, float nStrength, int nPlanes, vastStream_t vastStreamCtx)
{
    VappStatus vaccRet;
    geometry_input_params  in_params = {0};
    if(vapp_check_status(devID)!= 1){
        return VAPP_DEVICE_STATUS_ERROR;
    }
    if(!vastStreamCtx){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    if( !pSrc|| !pDst || !oSrcShape.width || !oSrcShape.height || !oSrcShape.wPitch || !oSrcShape.hPitch
                      || !oDstShape.width || !oDstShape.height || !oDstShape.wPitch || !oDstShape.hPitch){
        return VAPP_BAD_ARGUMENT_ERROR;
    }
    if( nStrength < 0 || nStrength > 1 || nPlanes < 0 || nPlanes > 15){
        return VAPP_BAD_ARGUMENT_ERROR;
    }

    VastStream * stream = (VastStream *)vastStreamCtx;
    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }  
    vaccRet =  vapp_find_op_entry(OP_CAS, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "find cas failed.\n");
        return vaccRet;           
    } 

    in_params.op_par.custom_op_name = CAS_OP_FUNC;     
    in_params.op_par.inout_addr[0] = (uint64_t)pSrc;
    in_params.op_par.inout_addr[1] = (uint64_t )pDst;
    
    in_params.cas_cfg.width = oSrcShape.width;
    in_params.cas_cfg.height = oSrcShape.height;
    in_params.cas_cfg.in_w_pitch = oSrcShape.wPitch;
    in_params.cas_cfg.in_h_pitch = oSrcShape.hPitch;    
    in_params.cas_cfg.out_w_pitch = oDstShape.wPitch;
    in_params.cas_cfg.out_h_pitch = oDstShape.hPitch;    
    in_params.cas_cfg.img_type = nImageFormat;
    in_params.cas_cfg.strength = nStrength;
    in_params.cas_cfg.planes = nPlanes;
    
    in_params.nOutputs = 1;

    in_params.op_par.priv_params = &in_params;
    in_params.op_par.block_num  = 1;
    in_params.op_par.config.config = (void *)&in_params.cas_cfg;
    in_params.op_par.config.size = sizeof(in_params.cas_cfg);          
    in_params.op_par.config_op_params = config_filtering_op;
       
    vaccRet = vapp_run_op_multi_async(devID, &in_params.op_par, stream);
    if(vaccRet!= VAPP_SUCCESS){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op cas failed.\n");
        return vaccRet;           
    } 
    return VAPP_SUCCESS;     
}

VappStatus 
vappiRGBPLANARCas_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, Vapp8u * pDst, VappiShape2D oDstShape, 
                             float nStrength, int nPlanes, vastStream_t vastStreamCtx)
{
    return inner_vappiCas_8u_Px_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, _XI_TILE_RGB_PLANAR_TYPE_, nStrength, nPlanes, vastStreamCtx);     
}

VappStatus 
vappiYUV420Cas_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, Vapp8u * pDst, VappiShape2D oDstShape, 
                             float nStrength, int nPlanes, vastStream_t vastStreamCtx)
{
    return inner_vappiCas_8u_Px_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, _XI_TILE_YUV_I420_TYPE_, nStrength, nPlanes, vastStreamCtx);     
}

VappStatus 
vappiNV12Cas_8u_P3_Ctx(unsigned int devID, 
                            const Vapp8u * const pSrc, VappiShape2D oSrcShape, Vapp8u * pDst, VappiShape2D oDstShape, 
                             float nStrength, int nPlanes, vastStream_t vastStreamCtx)
{
    return inner_vappiCas_8u_Px_Ctx(devID, pSrc, oSrcShape, pDst, oDstShape, _XI_TILE_YUV_NV12_TYPE_, nStrength, nPlanes, vastStreamCtx);     
}