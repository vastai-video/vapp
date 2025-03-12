
#include "op_case.h"
#include <inttypes.h>
#ifdef __linux__
#elif _WIN32
#define pthread_self() 0x0
#endif
#define MAX_OPS 8
typedef struct {
#ifdef __linux__
    pthread_t thread_id;
#elif _WIN32
    DWORD thread_id;
#endif
    int thread_num;
    int frame_count;
    OperatorCfg op_cfg[MAX_OPS];
    vastStream_t stream;
    Vapp8u* src_img;
    Vapp8u* dst_img;
    char * config_file;
    char * input_file;
    char * output_file;
}ThreadCtx;

char map1file[500];
char map2file[500];
char case_path[500];
static int parse_config(char * path)
{
    if(!path){
        fprintf(stderr, "error line %d\n",__LINE__);
        exit(0);
    }
    char a_line[500];
    FILE *fp_cfg = fopen(path, "rb");
    if(!fp_cfg){
        fprintf(stderr, "open %s fail\n", path);
    }
    fgets(a_line, 200,fp_cfg); sscanf(a_line, "%s",map1file);
    fgets(a_line, 200,fp_cfg); sscanf(a_line, "%s",map2file);
    fgets(a_line, 200,fp_cfg); sscanf(a_line, "%s",case_path);

    fclose(fp_cfg);
    return 0;
}

static ThreadCtx stream_arg[MAX_THREAD_NUM] = { 0 };
int test_multi_stream_remap_thresh(CommandLineArgs *args, OperatorCfg* cfg, FILE *fp_input, FILE *fp_out)
{
#define MAX_STREAM 8
    int i = 0;
#if _WIN32
    HANDLE thread_handle[MAX_STREAM];
#endif
    for(i=0; i< MAX_STREAM; i++){
        stream_arg[i].thread_num = i;
        stream_arg[i].frame_count = 500;
        stream_arg[i].config_file = args->config_file;
        stream_arg[i].input_file = args->input_file[0];
        stream_arg[i].output_file = args->output_file;
        stream_arg[i].op_cfg[0] = *cfg;
        stream_arg[i].op_cfg[0].out_plane_size = cfg->oDstSize.width*cfg->oDstSize.height;
        stream_arg[i].op_cfg[0].nSrcStep = cfg->oSrcSize.width;
        stream_arg[i].op_cfg[0].nDstStep = cfg->oDstSize.width;
        stream_arg[i].op_cfg[0].image_size = cfg->plane_size;    
        stream_arg[i].op_cfg[0].out_image_size = cfg->out_plane_size;
        stream_arg[i].op_cfg[1] = stream_arg[i].op_cfg[0];
#ifdef __linux__
        if ((pthread_create(&stream_arg[i].thread_id, NULL, (void*)test_remap_threshold, (void *)&stream_arg[i])) == -1) {
            printf("create process dec error!\n");
            return -1;
        }
#elif _WIN32
        thread_handle[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)test_remap_threshold, &stream_arg[i], 0, &stream_arg[i].thread_id);
#endif
        // printf("%s %d %s here stream_arg[%d].thread_id=%ld\n",__FILE__,__LINE__,__FUNCTION__,i,stream_arg[i].thread_id);
        // pthread_join(stream_arg[i].thread_id, NULL);
    }
#ifdef __linux__
    for(i=0; i< MAX_STREAM; i++){
        pthread_join(stream_arg[i].thread_id, NULL);
    }
#elif _WIN32
    for (i = 0; i < MAX_STREAM; i++) {
        WaitForSingleObject(thread_handle[i], INFINITE);
    }
#endif
    // while(1){
    //     usleep(1000);
    // }
    return 0;
}



int test_remap_threshold(void *arg)
{
    ThreadCtx *stream_arg = (ThreadCtx *)arg;
    OperatorCfg op_cfg =  stream_arg->op_cfg[0];
    OperatorCfg cfg = stream_arg->op_cfg[1];
    Vapp8u* tmp_img;
    int status = 0, i;
    vastStream_t stream;
    int64_t start, end, elapsed, sum = 0;
    Vapp8u * pSrcImg, *pDstImg;    
    parse_config(stream_arg->config_file);
    if(op_cfg.oDstSize.width == 0)
        op_cfg.oDstSize.width = op_cfg.oSrcSize.width;
    if(op_cfg.oDstSize.height == 0)
        op_cfg.oDstSize.height = op_cfg.oSrcSize.height;
    op_cfg.out_plane_size = op_cfg.oDstSize.width*op_cfg.oDstSize.height;
    op_cfg.nSrcStep = op_cfg.oSrcSize.width;
    op_cfg.nDstStep = op_cfg.oDstSize.width;
    op_cfg.image_size = op_cfg.plane_size;    
    op_cfg.out_image_size = op_cfg.out_plane_size;
    int roi_num = 3;
    VappiRect rRemapP1ROI[3] = {
        {0,    0, 901,                        op_cfg.oDstSize.height},
        {901,  0, 901,                        op_cfg.oDstSize.height},
        {1802, 0, op_cfg.oDstSize.width-1802, op_cfg.oDstSize.height}
    };    

    FILE * fp_in = fopen(stream_arg->input_file,"rb");

    cfg.oSrcSize.width = op_cfg.oDstSize.width;
    cfg.oSrcSize.height = op_cfg.oDstSize.height;
    cfg.oDstSize.width = cfg.oSrcSize.width;
    cfg.oDstSize.height = cfg.oSrcSize.height;
    cfg.plane_size = cfg.oSrcSize.width* cfg.oSrcSize.height;
    cfg.nSrcStep = cfg.oSrcSize.width;
    cfg.nDstStep = cfg.oDstSize.width;
    cfg.image_size = cfg.plane_size;    
    cfg.out_image_size = cfg.plane_size;  
    double nThreshold = 1.0;
    int nBlockSize =19;
    int nMaxValue = 255;
    VappiRect oRoi[4] = {
        {0,   0, cfg.oDstSize.width/3,    cfg.oDstSize.height},
        {cfg.oDstSize.width/3,   0, cfg.oDstSize.width/3,    cfg.oDstSize.height},
        {cfg.oDstSize.width/3*2,   0, cfg.oDstSize.width - cfg.oDstSize.width/3*2,    cfg.oDstSize.height},
    };          
    Vapp32f* map1_p1, *map2_p1;
    Vapp32f* map1h_p1, *map2h_p1;
    FILE * fp_map1_p1 = NULL;
    FILE * fp_map2_p1 = NULL;
    pSrcImg = malloc(op_cfg.image_size);
    pDstImg = malloc(op_cfg.out_image_size);
    map1h_p1 = malloc(op_cfg.out_plane_size * sizeof(Vapp32f));
    map2h_p1 = malloc(op_cfg.out_plane_size * sizeof(Vapp32f)); 
    fp_map1_p1 = fopen(map1file, "rb");
    fp_map2_p1 = fopen(map2file, "rb");
    fread(map1h_p1, op_cfg.out_plane_size* sizeof(Vapp32f), 1, fp_map1_p1);
    fread(map2h_p1, op_cfg.out_plane_size* sizeof(Vapp32f), 1, fp_map2_p1);

    fclose(fp_map1_p1);
    fclose(fp_map2_p1);

    vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
    vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&tmp_img, op_cfg.out_image_size), status);
    vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
    vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&map1_p1, op_cfg.out_plane_size * sizeof(Vapp32f)), status);
    vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&map2_p1, op_cfg.out_plane_size * sizeof(Vapp32f)), status);         
    vappSafeCall(vastStreamCreate(op_cfg.device_id,roi_num, &stream, NULL), status);

    vappSafeCall(vastMemcpy(op_cfg.device_id, map1_p1, map1h_p1, op_cfg.out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 
    vappSafeCall(vastMemcpy(op_cfg.device_id, map2_p1, map2h_p1, op_cfg.out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 
    for(i = 0; i < stream_arg->frame_count; i++){
        size_t reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_in);
        if(reads != 1){
            fprintf(stderr, "fread failed. %zu\n", reads);
            return -1;
        }
        vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
        start = time_usec(); 
        vappSafeCall(vappiRemap_8u_P1R_Ctx(op_cfg.device_id,
                            op_cfg.src_img, map1_p1, map2_p1, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                            tmp_img, op_cfg.oDstSize, op_cfg.nDstStep, roi_num, rRemapP1ROI, stream), status);                                 
        vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);  
        end = time_usec();
        elapsed =  end - start;
        sum = sum + elapsed;
        printf("elapsed %"PRId64" us sum %"PRId64" us\n", elapsed,sum);

        start = time_usec();
        vappSafeCall(vappiAdaptiveThreshold_8u_P1R_Ctx(cfg.device_id, 
                            tmp_img, cfg.oSrcSize, cfg.nSrcStep,  
                            op_cfg.dst_img, cfg.nDstStep, roi_num, oRoi, nThreshold, nBlockSize, nMaxValue, stream), status);
        vappSafeCall(vastStreamSynchronize(cfg.device_id, stream), status);                            
        end = time_usec();
        elapsed =  end - start;
        sum = sum + elapsed;
        printf("elapsed %"PRId64" us sum %"PRId64"\n", elapsed, sum);

        vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
        char filename[100];
        sprintf(filename, "%s_thread_%lx_%d.rgb",stream_arg->output_file, pthread_self(), i);
        FILE *fp_out = fopen(filename,"wb");
        fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);
        fclose(fp_out);        
        rewind(fp_in);                     
    }
    //printf("avg %ld us\n", sum/stream_arg->frame_count);
    vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
    vappSafeCall(vastFree(op_cfg.device_id, op_cfg.src_img), status);  
    vappSafeCall(vastFree(op_cfg.device_id, tmp_img), status);  
    vappSafeCall(vastFree(op_cfg.device_id, op_cfg.dst_img), status);      
    vappSafeCall(vastFree(op_cfg.device_id, map1_p1), status);  
    vappSafeCall(vastFree(op_cfg.device_id, map2_p1), status);  
    free(map1h_p1);
    free(map2h_p1);
    free(pSrcImg);
    free(pDstImg);
    fclose(fp_in);
    return 0;
}


int test_c2p_remap(CommandLineArgs *args, OperatorCfg* iop_cfg, FILE *fp_input, FILE *fp_out, Vapp8u * pSrcImg, Vapp8u * pDstImg)
{
    OperatorCfg op_cfg = *iop_cfg; 
    int status  = 0;
    vastStream_t stream;
    int64_t start, end, elapsed;   
    int i;    
    Vapp8u* cvt_img = NULL;
    
    // cvt888 c2p 
    OperatorCfg cvt_cfg = *iop_cfg;   
    cvt_cfg.oDstSize.width = op_cfg.oSrcSize.width;
    cvt_cfg.oDstSize.height = op_cfg.oSrcSize.height;
    cvt_cfg.nSrcStep = op_cfg.oSrcSize.width;
    cvt_cfg.nDstStep = op_cfg.oSrcSize.width;
    cvt_cfg.image_size = op_cfg.plane_size*3;    
    cvt_cfg.out_image_size = op_cfg.plane_size*3;
    cvt_cfg.out_plane_size = cvt_cfg.oDstSize.width*cvt_cfg.oDstSize.height;
    VappiRect oC2PROI[4] = {
        {0,   0, cvt_cfg.oDstSize.width,                       cvt_cfg.oDstSize.height},
    };  

    if(op_cfg.oDstSize.width == 0)
        op_cfg.oDstSize.width = op_cfg.oSrcSize.width;
    if(op_cfg.oDstSize.height == 0)
        op_cfg.oDstSize.height = op_cfg.oSrcSize.height;
    op_cfg.out_plane_size = op_cfg.oDstSize.width*op_cfg.oDstSize.height;
    op_cfg.nSrcStep = op_cfg.oSrcSize.width;
    op_cfg.nDstStep = op_cfg.oDstSize.width;
    op_cfg.image_size = op_cfg.plane_size*3;    
    op_cfg.out_image_size = op_cfg.out_plane_size*3;
    int roi_num = 1;
    VappiRect rRemapROI[4] = {
        {0,0,op_cfg.oDstSize.width, op_cfg.oDstSize.height},
        {901, 0, 901, op_cfg.oDstSize.height},
        {1802, 0, op_cfg.oDstSize.width-1802, op_cfg.oDstSize.height}
    };        
    Vapp32f* map1, *map2;
    Vapp32f* map1h, *map2h;
    FILE * fp_map1 = NULL;
    FILE * fp_map2 = NULL;
    pSrcImg = malloc(op_cfg.image_size);
    pDstImg = malloc(op_cfg.out_image_size);
    map1h = malloc(op_cfg.out_plane_size * sizeof(Vapp32f));
    map2h = malloc(op_cfg.out_plane_size * sizeof(Vapp32f)); 
    if(op_cfg.oDstSize.width == 640){
        fp_map1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_480_w_640_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_480_w_640_in.bin", "rb");
    }else if(op_cfg.oDstSize.width == 1280){
        fp_map1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_720_w_1280_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_720_w_1280_in.bin", "rb");
    }else if(op_cfg.oDstSize.width == 2704){
        fp_map1 = fopen("/home/vastai/simonz/input/data/remap_map1_h_2106_w_2704_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/simonz/input/data/remap_map2_h_2106_w_2704_in.bin", "rb");
    }else{
        fprintf(stderr, "unsupport dst width %d\n", op_cfg.oDstSize.width);
        return -1;
    }
    fread(map1h, op_cfg.out_plane_size* sizeof(Vapp32f), 1, fp_map1);
    fread(map2h, op_cfg.out_plane_size* sizeof(Vapp32f), 1, fp_map2);
    
    fclose(fp_map1);
    fclose(fp_map2);

    vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
    vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
    vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&map1, op_cfg.out_plane_size * sizeof(Vapp32f)), status);
    vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&map2, op_cfg.out_plane_size * sizeof(Vapp32f)), status);         
    vappSafeCall(vastStreamCreate(op_cfg.device_id,roi_num, &stream, NULL), status);
    vappSafeCall(vastMemcpy(op_cfg.device_id, map1, map1h, op_cfg.out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 
    vappSafeCall(vastMemcpy(op_cfg.device_id, map2, map2h, op_cfg.out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 




    vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&cvt_img, cvt_cfg.out_plane_size*3), status);  
    for(i = 0; i < args->frame_count; i++){
        // int reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
        // if(reads != 1){
        //     fprintf(stderr, "fread failed. %d\n", reads);
        //     return -1;
        // }
        FILE* fp_input1 = NULL;
        if(i%2 == 0){
            fp_input1 = fopen("/home/vastai/simonz/input/data/500w/Pic/OrgBin/OrgA.bin", "rb");
            if(!fp_input1){
                fprintf(stderr, "open failed.\n");
                return -1;
            }    
        }else{
            fp_input1 = fopen("/home/vastai/simonz/input/data/500w/Pic/OrgBin/OrgB.bin", "rb");
            if(!fp_input1){
                fprintf(stderr, "open failed.\n");
                return -1;
            }
        }
        int reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input1);
        if(reads != 1){
            fprintf(stderr, "fread failed. %d\n", reads);
            return -1;
        }
        fclose(fp_input1);              
        vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  


        start = time_usec(); 
        vappSafeCall(vappiColorC2P_8u_C3P3R_Ctx(cvt_cfg.device_id, 
                            op_cfg.src_img, cvt_cfg.oSrcSize, cvt_cfg.nSrcStep,  
                            cvt_img, cvt_cfg.nDstStep, roi_num, oC2PROI, stream), status);                                 
        vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);  
        end = time_usec();  
        elapsed =  end - start;
        printf("elapsed %"PRId64" us\n", elapsed/args->frame_count);
        start = time_usec(); 
        
        // FILE * fp_cvt = fopen("cvtout.rgb", "wb");
        // vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, cvt_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
        // fwrite(pDstImg, cvt_cfg.out_image_size, 1,fp_cvt);          
        // fclose(fp_cvt);

        vappSafeCall(vappiRemap_8u_P3R_Ctx(op_cfg.device_id,
                            cvt_img, map1, map2, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                            op_cfg.dst_img, op_cfg.oDstSize, op_cfg.nDstStep, roi_num, rRemapROI, stream), status);                                 
        vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);     
        end = time_usec();
        elapsed =  end - start;
        printf("elapsed %"PRId64" us\n", elapsed/args->frame_count);
        vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
        //fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);  


        char filename[100];
        sprintf(filename, "aaa_2501x2021_%d.rgbp", i);
        FILE *fp_out1 = fopen(filename, "wb");
        if(!fp_out1){
            fprintf(stderr, "open failed.\n");
            return -1;
        }    
        fwrite(pDstImg, op_cfg.out_image_size, 1, fp_out1);
        fclose(fp_out1);            
        rewind(fp_input);                         
    }
    vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
    vappSafeCall(vastFree(op_cfg.device_id, map1), status);  
    vappSafeCall(vastFree(op_cfg.device_id, map2), status);  
    free(map1h);
    free(map2h);    
    return 0;
}
