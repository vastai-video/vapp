#include "vapp.h"
#include "vdsp_op.h"
#include "compat.h"
#if __linux__
#include <unistd.h>
#include <sys/time.h>
#endif
#include "misc_params.h"
extern int g_stream_id;

VappOpEntrys  vapp_op_entrys[OP_RESERVED] = 
{
    [OP_SCALE_U8]       		= {RESIZE_ELF_NAME, RESIZE_OP_FUNC,  0},
	[OP_RESIZE_MULTICORE_OP]   	= {RESIZE_ELF_NAME_MULTI, RESIZE_OP_FUNC_MULTI, 0},
	[OP_IMAGE_CROP_U8]          = {CROP_ELF_NAME, CROP_OP_FUNC, 0},
	[OP_SIMPLE_ROTATE]          = {ROTATE_ELF_NAME, ROTATE_OP_FUNC, 0},
    [OP_FLIP]                   = {FLIP_ELF_NAME, FLIP_OP_FUNC, 0},
	[OP_FLIP_ROI]               = {ROIFLIP_ELF_NAME, ROIFLIP_OP_FUNC, 0}, 
	[OP_REMAP]                  = {REMAP_ELF_NAME, REMAP_OP_FUNC},
	[OP_CROP_ROI]               = {ROICROP_ELF_NAME, ROICROP_OP_FUNC},
	[OP_WARPPERSPECTIVE]        = {WARPPERSPECTIVE_ELF_NAME, WARPPERSPECTIVE_OP_FUNC},
    [OP_TRANSLATE]              = {TRANSLATE_ELF_NAME, TRANSLATE_OP_FUNC},
    [OP_PERMUTE]                = {PERMUTE_ELF_NAME, PERMUTE_OP_FUNC},
    [OP_TRANSPOSE]              = {TRANSPOSE_ELF_NAME, TRANSPOSE_OP_FUNC},
    [OP_CROPSCALE]              = {CROPSCALE_ELF_NAME, CROPSCALE_OP_FUNC},
	

    [OP_CVTCOLOR_U8]            = {CVTCOLOR_ELF_NAME, CVTCOLOR_OP_FUNC, 0},
    [OP_CVTCOLOR_ROI_EXTOP]     = {CVTCOLOR_ELF_NAME_ASYNC, CVTCOLOR_OP_FUNC_ASYNC, 0}, 
    [OP_COLOR_SPACE ]           = {NV12CSC_ELF_NAME, NV12CSC_OP_FUNC, 0},
	[OP_BAYERTONV12]            = {BAYERTONV12_ELF_NAME, BAYERTONV12_OP_FUNC, 0},
	
    [OP_FFMPEG_OVERLAY]         = {NV12OVERLAY_ELF_NAME, NV12OVERLAY_OP_FUNC, 0},
    [OP_BIT10TO8]               = {BITDEPTHCVT_ELF_NAME, BITDEPTHCVT_OP_FUNC, 0},  
    [OP_ADAPTIVETHRESHOLD]      = {ADAPTIVETHRESHOLD_ELF_NAME, ADAPTIVETHRESHOLD_OP_FUNC},  

    [OP_EQ]                     = {EQ_ELF_NAME, EQ_OP_FUNC, 0},
    [OP_UNSHARP]                = {UNSHARP_ELF_NAME, UNSHARP_OP_FUNC, 0},
    [OP_HQDN3D]                 = {HQDN3D_ELF_NAME, HQDN3D_OP_FUNC, 0},
    [OP_CAS]                    = {CAS_ELF_NAME, CAS_OP_FUNC, 0},
    [OP_SAD]                    = {SAD_ELF_NAME, SAD_OP_FUNC, 0},
    [OP_ROTATE_RGBA]            = {ROTATE_RGBA_INTERLEAVED_ROI_ELF_NAME, ROTATE_RGBA_INTERLEAVED_ROI_OP_FUNC, 0},
    [OP_DETECTION]              = {DETECTION_ELF_NAME, DETECTION_OP_FUNC, 0},
    [OP_ARGB2NV12]              = {CVTCOLOR_U8_ELF_NAME,CVTCOLOR_U8_OP_FUNC,0},

};



//int64_t time_usec(void)
//{
//	struct timeval tv_date;
//	gettimeofday(&tv_date, NULL);
//	return (int64_t)tv_date.tv_sec * 1000000 + (int64_t)tv_date.tv_usec;
//}

VappStatus vapp_run_op(unsigned int devID, op_params *op_par)
{
    rtError_t vaccRet;
    int dsp_type = 0;//vdsp
    char uni_code[33];
    uint32_t func_entry;
    uint32_t op_num = 1;
    
    uint32_t erro_no = 0; 
    op_async_data *op_asynced = malloc(sizeof(*op_asynced));
 
    op_asynced->stream = 0;
    vaccRet = custom_op_register(devID, dsp_type, op_par->elf_file, uni_code);    
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_run_op custom_op_register failed elf_file %s\n", op_par->elf_file);
        return VAPP_REGISTER_ERROR;              
    }
    size_t op_name_len = sizeof(uni_code) + strlen(op_par->custom_op_name) + 2;
    char *op_name = malloc(op_name_len);
    snprintf(op_name, op_name_len,"%s:%s", uni_code, op_par->custom_op_name);
    vaccRet = vapp_get_op_entry(devID, op_name, &func_entry);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_get_op_entry failed, %s \n", op_name);
        free(op_name);
        free(op_asynced);
        return VAPP_GET_ENTRY_ERROR;
    }
#ifdef ASYNC_CASE    
    op_par->block_num = 1;
    op_asynced->block_num = op_par->block_num;
    op_asynced->stream_id = ++g_stream_id;
    op_asynced->dev = devID;
    op_asynced->pid = getpid(); 
    op_asynced->hope_op_times = 0; 
    vaccRet = vapp_create_stream(devID,op_asynced->stream_id, op_asynced);      
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_create_stream failed\n");
        free(op_name);
        free(op_asynced);
        return VAPP_RESIZE_NO_OPERATION_ERROR;     
    }      
#endif 
    if(op_par->config_op_params(op_par->priv_params, func_entry) < 0){
        return VAPP_RESIZE_NO_OPERATION_ERROR;
    }
    op_par->op_info.op_uid = RUN_CUSTOM;
#ifdef ASYNC_CASE
    vaccRet = vapp_op_execute_async(devID, (const op_info_t *)&op_par->op_info, op_num, op_asynced->stream_id, 0);
#else
    uint32_t timeout = 500; // timeout(ms) for waitting output;
    vaccRet = vapp_op_execute(devID, (const op_info_t *)&op_par->op_info, op_num, timeout, &erro_no);
#endif
    printf("vapp_op_execute_async end\n");  
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_op_execute failed, err %d\n", erro_no);
        free(op_name);
        free(op_asynced);
        free(op_par->op_info.argument);
        return VAPP_RESIZE_NO_OPERATION_ERROR;           
    }    



#ifdef ASYNC_CASE 
    while(!op_asynced->frame_ready){
        vapp_usleep(10);
    }
    vaccRet = vapp_destroy_stream(devID, op_asynced->stream_id);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_destroy_stream failed.\n");
        free(op_name);
        free(op_asynced);
        free(op_par->op_info.argument);
        return VAPP_RESIZE_NO_OPERATION_ERROR;
    }    
#endif    
    free(op_name);
    free(op_asynced);
    free(op_par->op_info.argument);
    return VAPP_SUCCESS;    
}   



VappStatus vapp_run_op_multi(unsigned int devID, op_params *op_par)
{
    rtError_t vaccRet;
    int dsp_type = 0;//vdsp
    char uni_code[33];
    uint32_t func_entry;
    uint32_t op_num = 1;
    uint32_t erro_no = 0; 

    int i = 0;
    vaccRet = custom_op_register(devID, dsp_type, op_par->elf_file, uni_code);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "custom_op_register failed elf_file %s\n", op_par->elf_file);
        return VAPP_REGISTER_ERROR;              
    }
    size_t op_name_len = sizeof(uni_code) + strlen(op_par->custom_op_name) + 2;
    char *op_name = malloc(op_name_len);
    snprintf(op_name, op_name_len,"%s:%s", uni_code, op_par->custom_op_name);
    vaccRet = vapp_get_op_entry(devID, op_name, &func_entry);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_get_op_entry failed, %s \n", op_name);
        free(op_name);
        return VAPP_GET_ENTRY_ERROR;
    }


	// int64_t start, end, elapsed;
	// start = time_usec();	
    for(i = 0;i  < op_par->block_num; i++){
        op_par->block_id = i;
        if(op_par->update_cfg){
            op_par->update_cfg(op_par->priv_params);
        }        
        if(op_par->config_op_params(op_par->priv_params, func_entry) < 0){
            return VAPP_RESIZE_NO_OPERATION_ERROR;
        }
        op_par->op_info.op_uid = RUN_CUSTOM;
        uint32_t timeout = 500; // timeout(ms) for waitting output;
        vaccRet = vapp_op_execute(devID, (const op_info_t *)&op_par->op_info, op_num, timeout, &erro_no);       
        if(vaccRet!= rtSuccess){
            VAPP_LOG(VAPP_LOG_ERROR, "vapp_op_execute failed, err %d\n", erro_no);
            free(op_name);
            free(op_par->op_info.argument);
            return VAPP_RESIZE_NO_OPERATION_ERROR;     
        }    
    }

    free(op_name);
    if(op_par->op_info.argument){
        free(op_par->op_info.argument);
        op_par->op_info.argument = NULL;
    }
    return VAPP_SUCCESS;    
}   


static int dump_current_op_data(RTStream * stream, op_params *op_par)
{
    int i = 0;
    for(i = 0; i < MAX_IN_OUT_ADDR; i++){
        if(stream->inout_addr[i]){
            stream->inout_addr[i] = 0;
        }else{
            break;
        }
    }    
    stream->op_cfg_size = op_par->config.size;
    stream->op_addr  = op_par->op_info.op_addr;
    if (!stream->op_cfg) {
        stream->op_cfg = malloc(stream->op_cfg_size);   
    }else if(stream->op_cfg_size > stream->last_op_cfg_size ){
        free(stream->op_cfg);
        stream->op_cfg = malloc(stream->op_cfg_size);
    }
    memcpy(stream->op_cfg, op_par->config.config, stream->op_cfg_size);
    stream->last_op_cfg_size = stream->op_cfg_size;

    for (i = 0; i < MAX_IN_OUT_ADDR; i++) {
        if (op_par->inout_addr[i]) {      
            stream->inout_addr[i] = op_par->inout_addr[i];
        }
        else {
            break;
        }
    }
    return 0;
}

void print_op_params(RTStream* stream)
{
    int i = 0;
    switch (stream->op_type) {
        //geometry transforms
        case OP_SCALE_U8               :
        break;
        case OP_RESIZE_MULTICORE_OP    :
        {
            resize_multi_core_op_t* cfg = (resize_multi_core_op_t*)stream->op_cfg;
            fprintf(stderr, "resize op_stream_id  %d op_data_id %d\n", stream->stream_id, stream->op_async->hope_op_times);
            for (i = 0; i < MAX_IN_OUT_ADDR; i++) {
                if (stream->inout_addr[i]) {
                    fprintf(stderr, "resize inout_addr %d  %"PRIx64" \n", i, stream->inout_addr[i]);
                }
                else {
                    break;
                }
            }
            fprintf(stderr, "resize op_addr  %x \n", stream->op_addr);
            fprintf(stderr, "resize cfg inshape   h w:  %d %d  pitch_  h w: %d %d\n", cfg->in_img.height, cfg->in_img.width, cfg->in_img.h_pitch, cfg->in_img.w_pitch);
            fprintf(stderr, "resize cfg outshape   h w:  %d %d  pitch_  h w: %d %d\n", cfg->out_img.height, cfg->out_img.width, cfg->out_img.h_pitch, cfg->out_img.w_pitch);
            fprintf(stderr, "resize cfg in coding  %d\n", cfg->in_coding);
            fprintf(stderr, "resize cfg out coding  %d\n", cfg->out_coding);
            fprintf(stderr, "resize cfg method  %d\n", cfg->method);
            fprintf(stderr, "resize cfg colorspace  %d\n", cfg->color_space);
            fprintf(stderr, "resize cfg blocknum  %d\n", cfg->block_num);
            fprintf(stderr, "resize cfg block id  %d\n", cfg->block_id);
        }
        break;
        case OP_IMAGE_CROP_U8          :
        break;
        case OP_SIMPLE_ROTATE          :
        break;
        case OP_FLIP                   :
        break;
        case OP_FLIP_ROI               :
        break;
        case OP_REMAP                  :
        {
            remap_cfg_t *cfg = (remap_cfg_t *)stream->op_cfg;
            fprintf(stderr, "remap op_stream_id  %d op_data_id %d\n", stream->stream_id, stream->op_async->hope_op_times);
            fprintf(stderr, "remap cfg roi c_start %d h_start %d w_start %d c %d h %d w %d\n", 
            cfg->roi.c_start,cfg->roi.h_start,cfg->roi.w_start,
            cfg->roi.c_len,cfg->roi.h_len,cfg->roi.w_len);
            fprintf(stderr, "remap op_addr  %x \n", stream->op_addr);
            for(i =0; i < MAX_IN_OUT_ADDR; i++){
                if(stream->inout_addr[i]){
                    fprintf(stderr, "remap inout_addr %d   %"PRIx64"\n", i, stream->inout_addr[i]);
                }else{
                    break;
                }
            }
            fprintf(stderr, "remap cfg inshape  c h w: %d %d %d  pitch_ c h w:%d %d %d\n", cfg->in_shape.channel, cfg->in_shape.height,cfg->in_shape.width,
                cfg->in_shape.c_pitch, cfg->in_shape.h_pitch, cfg->in_shape.w_pitch);
            fprintf(stderr, "remap cfg outshape  c h w: %d %d %d  pitch_ c h w:%d %d %d\n", cfg->out_shape.channel, cfg->out_shape.height, cfg->out_shape.width,
                cfg->out_shape.c_pitch, cfg->out_shape.h_pitch, cfg->out_shape.w_pitch);
            fprintf(stderr, "remap cfg map1  h %d w %d pitch h w %d %d \n", cfg->map1.height, cfg->map1.width, cfg->map1.h_pitch, cfg->map1.w_pitch);
            fprintf(stderr, "remap cfg map2  h %d w %d pitch h w %d %d \n", cfg->map2.height, cfg->map2.width, cfg->map2.h_pitch, cfg->map2.w_pitch);
            fprintf(stderr, "remap cfg map1_type %d\n", cfg->map1_type);
            fprintf(stderr, "remap cfg map2_type %d\n", cfg->map2_type);
            fprintf(stderr, "remap cfg inter_type %d\n", cfg->inter_type);
            fprintf(stderr, "remap cfg border_type %d\n", cfg->border_type);
            fprintf(stderr, "remap cfg border_value %d\n\n\n", cfg->border_value);  
        }
      
        break;        
        case OP_CROP_ROI               :
        break;


        case OP_WARPPERSPECTIVE        :
        {
            warp_perspective_cfg_t *cfg = (warp_perspective_cfg_t *)stream->op_cfg;
            fprintf(stderr, "warpperspe op_stream_id  %d op_data_id %d\n", stream->stream_id, stream->op_async->hope_op_times);
            fprintf(stderr, "warpperspe roi c_start %d h_start %d w_start %d c %d h %d w %d\n", 
            cfg->roi.c_start,cfg->roi.h_start,cfg->roi.w_start,
            cfg->roi.c_len,cfg->roi.h_len,cfg->roi.w_len);
            fprintf(stderr, "warpperspe op_addr  %x \n", stream->op_addr);
                
            for(i =0; i < MAX_IN_OUT_ADDR; i++){
                if(stream->inout_addr[i]){
                    fprintf(stderr, "warpperspe inout_addr %d   %"PRIx64"\n", i, stream->inout_addr[i]);
                }else{
                    break;
                }
            }        
            
            fprintf(stderr, "warpperspe cfg inshape  c h w: %d %d %d  pitch_ c h w:%d %d %d\n", cfg->in_image_shape.channel, cfg->in_image_shape.height, cfg->in_image_shape.width,
                cfg->in_image_shape.c_pitch, cfg->in_image_shape.h_pitch, cfg->in_image_shape.w_pitch);
            fprintf(stderr, "warpperspe cfg outshape  c h w: %d %d %d  pitch_ c h w:%d %d %d\n", cfg->out_image_shape.channel, cfg->out_image_shape.height, cfg->out_image_shape.width,
                cfg->out_image_shape.c_pitch, cfg->out_image_shape.h_pitch, cfg->out_image_shape.w_pitch);
            fprintf(stderr, "warpperspe cfg image_type %d\n", cfg->image_type);
            for (i = 0; i < 9; i++) {
                fprintf(stderr, "warpperspe cfg %d M  %f\n", i, cfg->M[i]);
            }
            fprintf(stderr, "warpperspe cfg inter_type %d\n", cfg->inter_type);
            fprintf(stderr, "warpperspe cfg border_type %d\n", cfg->border_type);
            fprintf(stderr, "warpperspe cfg border_value %d\n", cfg->border_value);
            fprintf(stderr, "warpperspe cfg flag %d\n\n\n", cfg->flag);
        }

        break;        
        case OP_TRANSLATE  :   
        {
            translateTransform_cfg_t *cfg = (translateTransform_cfg_t *)stream->op_cfg;
            fprintf(stderr, "translate op_stream_id  %d op_data_id %d\n", stream->stream_id, stream->op_async->hope_op_times);
            fprintf(stderr, "translate cfg roi pos_x %d pos_y %d pos_z %d c %d h %d w %d\n", 
            cfg->in_image.pos_x,cfg->in_image.pos_y,cfg->in_image.pos_z,
            cfg->in_image.channel,cfg->in_image.height,cfg->in_image.width);
            fprintf(stderr, "translate op_addr  %x \n", stream->op_addr);
            
            for(int i =0; i < MAX_IN_OUT_ADDR; i++){
                if(stream->inout_addr[i]){
                    fprintf(stderr, "translate inout_addr %d  %"PRIx64" \n", i, stream->inout_addr[i]);
                }else{
                    break;
                }
            }

            fprintf(stderr, "translate cfg inshape  c h w: %d %d %d  pitch_ c h w:%d %d %d\n", cfg->pic.channel, cfg->pic.height, cfg->pic.width,
                cfg->pic.c_pitch, cfg->pic.h_pitch, cfg->pic.w_pitch);
            fprintf(stderr, "translate cfg image_offset x %f\n", cfg->image_offset.offset_x);
            fprintf(stderr, "translate cfg image_offset y %f\n", cfg->image_offset.offset_y);              
        }         
      
        break;
        case OP_PERMUTE                :
        break;    
        //color space conversion  :
        case OP_CVTCOLOR_U8            :
        break;
        case OP_CVTCOLOR_ROI_EXTOP     :
        break;
        case OP_COLOR_SPACE            :
        break;
        case OP_BAYERTONV12            :
        break;
                                    
        //arithmetic and logical  :
        case OP_FFMPEG_OVERLAY         :
        break;
        case OP_BIT10TO8               :
        break;
        case OP_ADAPTIVETHRESHOLD      :
        {
            adaptiveThreshold_cfg_t *cfg = (adaptiveThreshold_cfg_t *)stream->op_cfg;
            fprintf(stderr, "adaptive op_stream_id  %d op_data_id %d\n", stream->stream_id, stream->op_async->hope_op_times);
            fprintf(stderr, "adaptive roi c_start %d h_start %d w_start %d c %d h %d w %d\n", 
            cfg->roi.z,cfg->roi.y,cfg->roi.x,
            cfg->roi.c,cfg->roi.h,cfg->roi.w);
            fprintf(stderr, "adaptive op_addr  %x \n", stream->op_addr);
            for(i =0; i < MAX_IN_OUT_ADDR; i++){
                if(stream->inout_addr[i]){
                    fprintf(stderr, "adaptive inout_addr %d  %"PRIx64"x \n", i, stream->inout_addr[i]);	
                }else{
                    break;
                }
            }
            fprintf(stderr, "adaptive cfg inshape  c h w: %d %d %d  pitch_ c h w:%d %d %d\n", cfg->in_shape.channel, cfg->in_shape.height, cfg->in_shape.width,
                cfg->in_shape.c_pitch, cfg->in_shape.h_pitch, cfg->in_shape.w_pitch);
            fprintf(stderr, "adaptive cfg threshold  %f\n", cfg->meanPara.threshold);
            fprintf(stderr, "adaptive cfg blockSize  %d\n", cfg->meanPara.blockSize);
            fprintf(stderr, "adaptive cfg maxValue  %d\n", cfg->meanPara.maxValue);
            fprintf(stderr, "adaptive cfg method  %d\n", cfg->meanPara.method);
            fprintf(stderr, "adaptive cfg type  %d\n", cfg->meanPara.type);            
        }
        break;
        case OP_EQ                     :                       
        break;
        case OP_SAD                    :
            break;
        default:
        break;
    }    

}

VappStatus vapp_run_op_multi_async(unsigned int devID, op_params *op_par, VastStream * stream_ctx)
{
    rtError_t vaccRet;
    uint32_t op_num = 1;
    int i = 0;
    
    
    for(i = 0;i  < op_par->block_num; i++){
        RTStream * stream = (RTStream *)&stream_ctx->rt_stream[i];
        
        stream->fun = stream_ctx->fun;
        stream->op_type = stream_ctx->cur_op;
        stream->block_id = i;
        stream->data_id = stream->frame_num;
#ifdef __linux__
    pthread_mutex_lock(&stream->op_async->mutex);
#elif _WIN32
    EnterCriticalSection(&stream->op_async->cs_lock);
#endif          
        stream->op_async->hope_op_times = stream->frame_num;
#ifdef __linux__
    pthread_mutex_unlock(&stream->op_async->mutex);
#elif _WIN32
    LeaveCriticalSection(&stream->op_async->cs_lock);
#endif         
        op_par->block_id = i;
        if(op_par->update_cfg){
            op_par->update_cfg(op_par->priv_params);
        }
        if(op_par->config_op_params(op_par->priv_params, stream->fun) < 0){
            return VAPP_RESIZE_NO_OPERATION_ERROR;
        }
        dump_current_op_data(stream, op_par);
        op_par->op_info.op_uid = RUN_CUSTOM;
        //fprintf(stderr, "thread()=%ld stream->stream_id %d send_data_id=%d\n", GetCurrentThreadId(), stream->stream_id, stream->frame_num);
        vaccRet = vapp_op_execute_async(devID, (const op_info_t *)&op_par->op_info, op_num, stream->stream_id, stream->frame_num);
        if(vaccRet!= rtSuccess){
            VAPP_LOG(VAPP_LOG_ERROR, "vapp_op_execute failed, err \n");
            return VAPP_RESIZE_NO_OPERATION_ERROR;     
        }    
        stream_ctx->issued++; 
        //stream->issued++;
        if(op_par->op_info.argument){
            free(op_par->op_info.argument);
            op_par->op_info.argument = NULL;
        }    
        stream->frame_num++;
        //VAPP_LOG(VAPP_LOG_INFO,"%s %d %s here pthread_self()=%ld stream->frame_num=%d\n",__FILE__,__LINE__,__FUNCTION__,pthread_self(), stream->frame_num);        
    }

    return VAPP_SUCCESS;    
}   


VappStatus vapp_register_op(unsigned int devID, OpRegister *op_reg, uint32_t * entry, char * name, char * func)
{
    rtError_t vaccRet;
    const char * op_path = getenv("VASTAI_VAPP_PATH");
    if(!op_path){
        op_path = DEFAULT_OP_PATH;
    }  
    size_t elf_file_len = strlen(op_path) + strlen(name) + 1;
    op_reg->elf_file = malloc(elf_file_len);
    snprintf(op_reg->elf_file, elf_file_len, "%s%s", op_path, name);
    vaccRet = op_register(devID, op_reg);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "op_register failed elf_file %s\n", op_reg->elf_file);
        return VAPP_REGISTER_ERROR;              
    }        

    size_t op_name_len = sizeof(op_reg->uni_code) + strlen(func) + 1;
    char *op_name = malloc(op_name_len);
    snprintf(op_name, op_name_len,"%s:%s", op_reg->uni_code, func);
    vaccRet = vapp_get_op_entry(devID, op_name, entry);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_get_op_entry failed, %s \n", op_name);
        free(op_name);
        return VAPP_GET_ENTRY_ERROR;
    }  
    free(op_name);   
    return VAPP_SUCCESS;  
}


VappStatus vapp_register_op_stream(unsigned int devID, VastStream * stream,void *reserved)
{
    rtError_t vaccRet;
    char *elf_file_path = (char *)reserved;
    if(elf_file_path != NULL && strlen(elf_file_path)){
        size_t elf_file_len = strlen(elf_file_path) + 1;
        stream->op_reg.elf_file = malloc(elf_file_len);
        snprintf(stream->op_reg.elf_file, elf_file_len, "%s", elf_file_path);
        if(access(stream->op_reg.elf_file, 0)){ //for some op is not in multi_core_op file case, printf can be comment if necessary --wxhu
            printf("%s %d %s return, there is no file:%s\n",__FILE__,__LINE__,__FUNCTION__,stream->op_reg.elf_file);
            return VAPP_NO_ELF_FILE_ERROR;  
        }
    }else{
        const char * op_path = getenv("VASTAI_VAPP_PATH");
        if(!op_path){
            op_path = DEFAULT_OP_PATH;
        }
        size_t elf_file_len = strlen(op_path) + strlen(ELF_FILE_NAME) + 2;
        stream->op_reg.elf_file = malloc(elf_file_len);
        snprintf(stream->op_reg.elf_file, elf_file_len, "%s/%s", op_path, ELF_FILE_NAME);
        if(access(stream->op_reg.elf_file, 0)){ //for some op is not in multi_core_op file case, printf can be comment if necessary --wxhu
            printf("%s %d %s return, there is no file:%s\n",__FILE__,__LINE__,__FUNCTION__,stream->op_reg.elf_file);
            return VAPP_NO_ELF_FILE_ERROR;  
        }
    }
    
    vaccRet = op_register(devID, &stream->op_reg);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "op_register failed elf_file %s\n", stream->op_reg.elf_file);
        return VAPP_REGISTER_ERROR;              
    }
    
    vaccRet = vapp_get_op_entrylist(devID, stream->op_reg.uni_code, &stream->op_entry_list);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_get_op_entry failed \n");
        return VAPP_GET_ENTRY_ERROR;
    }      
#ifdef __linux__
    pthread_rwlock_init(&stream->op_entry_lock, NULL);
#elif _WIN32
    InitializeSRWLock(&stream->op_entry_lock);
#endif
    // vastai_vdsp_op_t *dsp_item = NULL;
    // int i,j;
    // dsp_item = (vastai_vdsp_op_t *)stream->op_entry_list.op_entry_list;
    // for (i = 0; i < stream->op_entry_list.op_num; i++) {
    //     printf("------>>op_name:%s, op_fun:0x%x\n", dsp_item->name, dsp_item->p_fun);
    //     dsp_item += 1;
    //     // for (j = OP_DEFAULT + 1; j < OP_RESERVED; j++) {
    //     //     if(!strcmp(dsp_item->name, vapp_op_entrys[j].entry)){
    //     //         vapp_op_entrys[j].pfun = dsp_item->p_fun;
    //     //         vapp_op_entrys[j].reg = 1;
    //     //         break;
    //     //     }            
    //     // }
    // }       
    return VAPP_SUCCESS;  
}

VappStatus vapp_find_op_entry(VappOpFunc op, VastStream * stream)
{
#ifdef __linux__
                pthread_rwlock_wrlock(&stream->op_entry_lock);  
#elif _WIN32
                AcquireSRWLockExclusive(&stream->op_entry_lock);
#endif    
    int is_reg = vapp_op_entrys[op].reg;
#ifdef __linux__
                pthread_rwlock_unlock(&stream->op_entry_lock);
#elif _WIN32
                ReleaseSRWLockExclusive(&stream->op_entry_lock);
#endif
    if(!is_reg){
        vastai_vdsp_op_t *dsp_item = NULL;
        dsp_item = (vastai_vdsp_op_t *)stream->op_entry_list.op_entry_list;
       
        for (uint32_t index = 0; index < stream->op_entry_list.op_num; ++index) {  
            if(!strcmp(dsp_item->name, vapp_op_entrys[op].func_name)){
               
#ifdef __linux__
                pthread_rwlock_wrlock(&stream->op_entry_lock);  
#elif _WIN32
                AcquireSRWLockExclusive(&stream->op_entry_lock);
#endif
                vapp_op_entrys[op].pfun = dsp_item->p_fun;
                vapp_op_entrys[op].reg = 1;
                stream->fun = dsp_item->p_fun;
#ifdef __linux__
                pthread_rwlock_unlock(&stream->op_entry_lock);
#elif _WIN32
                ReleaseSRWLockExclusive(&stream->op_entry_lock);
#endif
                stream->cur_op = op;
                printf("------>>op_name:%s, op_fun:0x%x\n", dsp_item->name, dsp_item->p_fun);
                break;
            }
            dsp_item += 1;
        }           
    }else{
#ifdef __linux__
        pthread_rwlock_wrlock(&stream->op_entry_lock);
#elif _WIN32
        AcquireSRWLockShared(&stream->op_entry_lock);
#endif
        stream->fun = vapp_op_entrys[op].pfun;
        stream->cur_op = op;
#ifdef __linux__
        pthread_rwlock_unlock(&stream->op_entry_lock);
#elif _WIN32
        ReleaseSRWLockShared(&stream->op_entry_lock);
#endif
    }
    if(stream->cur_op == OP_DEFAULT){
        return VAPP_ERROR;
    }
    return VAPP_SUCCESS;  
}



