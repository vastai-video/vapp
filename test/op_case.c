#include "op_case.h"
#include "inttypes.h"

#define RUN_DETECTION_OP_HOST
#define ALIGN(value, n) (((value) + (n) - 1) & ~((n) - 1))

typedef enum{
    //geometry transforms
    VAPPI_OP_DEFAULT = 0,
    VAPPI_OP_RESIZE_INTERL_CTX,
    VAPPI_OP_RESIZE_CTX,
    VAPPI_OP_YUV420RESIZE,
    VAPPI_OP_NV12RESIZE_CTX,
    VAPPI_OP_YUV420FLIP_CTX,
    VAPPI_OP_NV12FLIP_CTX,
    VAPPI_OP_CROP,
    VAPPI_OP_YUV420ROTATE_CTX,
    VAPPI_OP_ROIFLIP_RGBP_CTX,
    VAPPI_OP_REMAP_RGBP_CTX,
    VAPPI_OP_CROP_RGB_INTERL_CTX,
    VAPPI_OP_CROP_RGB_PLANAR_CTX, 
    VAPPI_OP_WARPPERSPECTIVE_CTX,
    VAPPI_OP_RESIZE_PLANAR_CTX,
    VAPPI_OP_REMAP_GRAY_CTX,
    VAPPI_OP_CROP_GRAY_CTX,
    VAPPI_OP_RESIZE_GRAY_CTX,
    VAPPI_OP_REMAP_GRAY_FIX_CTX,
    VAPPI_OP_ROIFLIP_GRAY_CTX,
    VAPPI_OP_WARPPERSPECTIVE_GRAY_CTX,
    VAPPI_OP_TRANSLATETRANS_GRAY_CTX,
    VAPPI_OP_TRANSPOSE_GRAY_CTX,
    VAPPI_OP_EQ,
    VAPPI_OP_RESIZE_PLANAR_PLUS_CTX, //INCLUDE_CROP
    VAPPI_OP_RESIZE_GRAY_PLUS_CTX, //INCLUDE_CROP  25
    VAPPI_OP_YUV420RESIZE_CTX,
    VAPPI_OP_SAD,
    VAPPI_OP_NV12ROTATE_CTX,   
    VAPPI_OP_RGBA_PLANAR_ROTATE,
    VAPPI_OP_RGBA_PLANAR_ROTATE_CTX,
    VAPPI_OP_DETECTION,
    VAPPI_OP_YUV420TRANSPOSE_CTX,
    VAPPI_OP_NV12TRANSPOSE_CTX,
    VAPPI_OP_YUV420CROPSCALE_CTX,
    VAPPI_OP_NV12CROPSCALE_CTX,
    VAAPI_OP_ARGB2NV12_CTX, //36
    VAAPI_OP_ARGB2NV12_RESIZE_CTX,

    //color space conversion
    VAPPI_OP_NV12CSC_CTX = VAPPI_OP_DEFAULT + SUB_MODULE_MAX_OPS,
    VAPPI_OP_COLORTOGRAY,
    VAPPI_OP_BAYERTONV12_CTX, //102
    VAPPI_OP_COLORTOGRAY_CTX,
    VAPPI_OP_COLORP2C_CTX, //104
    VAPPI_OP_COLORC2P_CTX,
    VAPPI_OP_COLOR_NV12TORGBP_CTX, //106 same as VAPPI_OP_COLOR_NV12TORGB_CTX
    VAPPI_OP_COLOR_RGBPTONV12_CTX, //107 same as VAPPI_OP_COLOR_RGBTONV12_CTX
    //arithmetic and logical
    VAPPI_OP_NV12OVERLAY_CTX = VAPPI_OP_NV12CSC_CTX + SUB_MODULE_MAX_OPS,
    VAPPI_OP_BITDEPTHCVT_CTX,
    VAPPI_OP_ADAPTIVETHRESH_CTX,

    //filtering functions
    VAPPI_OP_UNSHARP,
    VAAPI_OP_HQNN3D,
    VAPPI_OP_CAS_CTX, //205   
    VAPPI_OP_RESERVED,

    //test case
    VAPPI_TEST_REMAP_CROP = TEST_START,
    VAPPI_TEST_MULTI_STREAM_REMAP,
    VAPPI_TEST_CYCLE_REMAP,
    VAPPI_TEST_CYCLE_CROP_PLANAR,
    VAPPI_TEST_C2P_REMAP,
    VAPPI_TEST_MULTI_STREAM_8882PLANAR_REMAP,
    VAPPI_TEST_REMAP_THRESHOLD,    
}VappiOps;

#ifdef _WIN32
LARGE_INTEGER frequency = {0}; 
#endif
int64_t time_usec(void)
{
#if __linux__
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
#elif _WIN32
    //FILETIME ft;
    //int64_t t;
    //GetSystemTimeAsFileTime(&ft);
    //t = (int64_t)ft.dwHighDateTime << 32 | ft.dwLowDateTime;
    //return t / 10 - 11644473600000000; /* Jan 1, 1601 */
    
    LARGE_INTEGER ticks;
    if(!frequency.QuadPart){
        QueryPerformanceFrequency(&frequency);
    }
    
    QueryPerformanceCounter(&ticks);

    return ticks.QuadPart * 1000000/ frequency.QuadPart;
#else
    return -1;
#endif
}

float av_clipf_c(float a, float amin, float amax)
{
    if (a < amin) return amin;
    else if (a > amax) return amax;
    else               return a;
}

//void update_map(Vapp32f* map1, Vapp32f* map2 , int width,int height);

int run_test_case(CommandLineArgs *args)
{
    OperatorCfg op_cfg = {0};
    vastStream_t stream;
    VappStatus status = VAPP_SUCCESS;
    Vapp8u * pSrcImg = NULL;
    Vapp8u * pSrcImg2 = NULL;
    Vapp8u * pDstImg = NULL;
    FILE *fp_input_2;
    int i = 0;
    size_t reads = 0;
    VappiAxis eAxis = VAPPI_VERTICAL_AXIS;
    VappiTransposeDirection eDirection = TRANSPOSE_CCLOCK_FLIP;
    int64_t start = 0, end = 0, elapsed = 0, sum = 0, max = 0;
    int position = 0;
    int x, y, crop_width, crop_height;
    int blocknum = 3;
    int nAngle = ROTATE_DEGREE_90;
    float strength;
    unsigned int planes;

    FILE *fp_input = fopen(args->input_file[0], "rb");
    if(!fp_input){
        fprintf(stderr, "open %s failed.\n", args->input_file[0]);
        return -1;
    }    
    FILE *fp_out = fopen(args->output_file, "wb");
    if(!fp_out){
        fprintf(stderr, "open %s failed.\n", args->output_file);
        return -1;
    }    
    args->input_size.wPitch = (args->input_size.wPitch == 0) ?  args->input_size.width : args->input_size.wPitch;
    args->input_size.hPitch = (args->input_size.hPitch == 0) ?  args->input_size.height : args->input_size.hPitch;
    args->output_size.wPitch = (args->output_size.wPitch == 0) ?  args->output_size.width : args->output_size.wPitch;
    args->output_size.hPitch = (args->output_size.hPitch == 0) ?  args->output_size.height : args->output_size.hPitch;   

    op_cfg.device_id = args->device_id;   
    op_cfg.plane_size =  args->input_size.wPitch * args->input_size.hPitch;
    op_cfg.out_plane_size = args->output_size.wPitch * args->output_size.hPitch;
    op_cfg.oSrcSize.width = args->input_size.width;
    op_cfg.oSrcSize.height = args->input_size.height;
    op_cfg.oDstSize.width = args->output_size.width;
    op_cfg.oDstSize.height = args->output_size.height;   
    op_cfg.nSrcStep = args->input_size.wPitch;
    op_cfg.nDstStep = args->output_size.wPitch;

    op_cfg.inSrcSize = args->input_size;  
    op_cfg.outSize   = args->output_size;   

    switch (args->test_case)
    {
    case VAPPI_OP_DEFAULT:         
    case VAPPI_OP_RESIZE_INTERL_CTX:

        op_cfg.image_size = op_cfg.plane_size * 3;
        op_cfg.out_image_size = op_cfg.oDstSize.width * op_cfg.oDstSize.height * 3;

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, blocknum, &stream,NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        
        start = time_usec();
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %d\n", (int)reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            vappSafeCall(vappiRGBResize_8u_C3_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.oDstSize, op_cfg.nDstStep, VAPPI_RESIZE_BILINEAR_PILLOW,stream), status);                             
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);                            
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        
        end = time_usec();
        elapsed =  end - start;
        printf("elapsed %" PRId64 " us\n", elapsed/args->frame_count);  
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status);   
  
        break;
    case VAPPI_OP_RESIZE_PLANAR_CTX:

        op_cfg.image_size = op_cfg.plane_size * 3;
        op_cfg.out_image_size = op_cfg.oDstSize.width * op_cfg.oDstSize.height * 3;


        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, blocknum, &stream,NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        

        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            rewind(fp_input);                         
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            start = time_usec();
            vappSafeCall(vappiRGBPResize_8u_P3_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.oDstSize, op_cfg.nDstStep, VAPPI_RESIZE_BILINEAR_PILLOW,stream), status);                             
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);                            
            end = time_usec();
            elapsed =  end - start;
            sum = sum + elapsed;
            printf("elapsed %" PRId64 " us sum %" PRId64 "\n", elapsed, sum);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            // fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out); 
            char filename[100];
            sprintf(filename, "cycle_resize_planar_2560x2048_%d.rgb", i);
            FILE *fp_out1 = fopen(filename, "wb");
            if(!fp_out1){
                fprintf(stderr, "open failed.\n");
                return -1;
            }  
            fwrite(pDstImg, op_cfg.out_image_size, 1, fp_out1);
            fclose(fp_out1);
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status);   
  
        break;
    case VAPPI_OP_RESIZE_PLANAR_PLUS_CTX: //crop: (6, 11) 2576x2032 to 1000x2000, resize:1000x2000, to 400x500, 
        x = 6;
        y = 11;
        crop_width = 1000;
        crop_height = 2000;
        position = y*op_cfg.oSrcSize.width + x;
        op_cfg.image_size = op_cfg.plane_size * 3;
        op_cfg.out_image_size = op_cfg.oDstSize.width * op_cfg.oDstSize.height * 3;

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, blocknum, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        

        for(i = 0; i < args->frame_count; i++){
            int reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %d\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg+position, op_cfg.image_size-position, vastMemcpyHostToDevice), status); 
            start = time_usec();
            vappSafeCall(vappiRGBPResizePlus_8u_P3_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.oDstSize, op_cfg.nDstStep, crop_width, crop_height, VAPPI_RESIZE_BILINEAR_PILLOW, stream), status);                             
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);                            
            end = time_usec();
            elapsed =  end - start;
            printf("elapsed %" PRId64 " us\n", elapsed/args->frame_count);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status);   
  
        break;
    case VAPPI_OP_RESIZE_GRAY_PLUS_CTX: //crop: (6, 11) 2576x2032 to 1000x2000, resize:1000x2000, to 400x500, 
        x = 0;
        y = 5;
        crop_width = 2691;
        crop_height = 2101;
        VappiRect _cropsize = {
            .x = 0,
            .y = 5,
            .height = crop_height,
            .width = crop_width,
        };
        //position = y*op_cfg.oSrcSize.width + x;
        op_cfg.image_size = op_cfg.plane_size;
        op_cfg.out_image_size = op_cfg.oDstSize.width * op_cfg.oDstSize.height;

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, blocknum, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        

        for(i = 0; i < args->frame_count; i++){
            int reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %d\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status); 
            start = time_usec();
            vappSafeCall(vappiGrayResizePlus_8u_P1_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.oDstSize, op_cfg.nDstStep, _cropsize, VAPPI_RESIZE_BILINEAR_PILLOW, stream), status);                             
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);                            
            end = time_usec();
            elapsed = end - start;
            sum = sum + elapsed;
            printf("elapsed %" PRId64 " us sum %" PRId64 "\n", elapsed, sum);
            printf("avg %" PRId64 " us\n", sum / args->frame_count);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status);   
  
        break;
    case VAPPI_OP_RESIZE_GRAY_CTX:

        op_cfg.image_size = op_cfg.plane_size;
        op_cfg.out_image_size = op_cfg.oDstSize.width * op_cfg.oDstSize.height;


        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, blocknum, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        

        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            rewind(fp_input);                         
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            start = time_usec();
            vappSafeCall(vappiGrayResize_8u_P1_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.oDstSize, op_cfg.nDstStep, VAPPI_RESIZE_BILINEAR_PILLOW,stream), status);                             
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);                            
            end = time_usec();
            elapsed =  end - start;
            sum = sum + elapsed;
            printf("elapsed %" PRId64 " us sum %" PRId64 "\n", elapsed, sum);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1, fp_out); 
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status);   
  
        break;
    case VAPPI_OP_RESIZE_CTX:
        op_cfg.image_size = op_cfg.plane_size * 3;
        op_cfg.out_image_size = op_cfg.oDstSize.width * op_cfg.oDstSize.height * 3;

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, blocknum, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  

        start = time_usec();
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            vappSafeCall(vappiRGBResize_8u_C3_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.oDstSize, op_cfg.nDstStep, VAPPI_RESIZE_BILINEAR_PILLOW,stream), status);                                                       
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);      
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status);   
        end = time_usec();
        elapsed =  end - start;
        printf("elapsed %" PRId64 " us\n", elapsed/args->frame_count);
        break;   
    case VAPPI_OP_YUV420RESIZE:

        op_cfg.image_size = op_cfg.plane_size * 3/2;
        op_cfg.out_image_size = op_cfg.oDstSize.width * op_cfg.oDstSize.height * 3/2;

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  

        start = time_usec();
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            vappSafeCall(vappiYUV420Resize_8u_P3(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.oDstSize, op_cfg.nDstStep, VAPPI_RESIZE_BICUBIC), status);                           
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        
        end = time_usec();
        elapsed =  end - start;
        printf("elapsed %" PRId64 " us\n", elapsed/args->frame_count);
        break;   
    case VAPPI_OP_YUV420RESIZE_CTX:
       
        op_cfg.image_size = op_cfg.plane_size * 3/2;
        op_cfg.out_image_size = op_cfg.oDstSize.width * op_cfg.oDstSize.height * 3/2;

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, args->elf_file), status);

        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
         VappiShape2D resize_src_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height            
        };
        VappiShape2D resize_dst_shape = {
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height,
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height            
        }; 
        
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status); 
            start = time_usec(); 
            vappSafeCall(vappiYUV420PResize_8u_P3_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, resize_src_shape, op_cfg.dst_img, resize_dst_shape, VAPPI_RESIZE_BICUBIC, stream), status); 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);    
            end = time_usec();
            elapsed =  end - start;
            printf("elapsed %" PRId64 " us\n", elapsed/args->frame_count);                                                                   
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
                

        break;   
    case VAPPI_OP_UNSHARP:
        op_cfg.image_size = op_cfg.plane_size * 3/2;
        op_cfg.oDstSize = op_cfg.oSrcSize;
        op_cfg.out_image_size = op_cfg.oDstSize.width * op_cfg.oDstSize.height * 3/2;
        VappiShape2D src_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height            
        };
        VappiShape2D dst_shape = {
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height,
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height            
        }; 
        float nLumaAmount = -2.1;
        float nChromAmount = -1.1;       
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);

        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  

        
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status); 
            start = time_usec(); 
            vappSafeCall(vappiNV12Unsharp_8u_P2_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, src_shape,   
                                op_cfg.dst_img, dst_shape, nLumaAmount, nChromAmount, stream), status); 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);    
            end = time_usec();
            elapsed =  end - start;
            printf("elapsed %" PRId64 " us\n", elapsed/args->frame_count);                                                                   
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
                

        break;
    case VAAPI_OP_HQNN3D:
        op_cfg.image_size = op_cfg.plane_size * 3/2;
        op_cfg.oDstSize = op_cfg.oSrcSize;
        op_cfg.out_image_size = op_cfg.oDstSize.width * op_cfg.oDstSize.height * 3/2;
        VappiShape2D src_img_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height            
        };
        VappiShape2D dst_img_shape = {
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height,
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height            
        }; 
        float luma_temporal = 4.0;
        float chroma_temporal = 6.0;       
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);

        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  

        
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status); 
            start = time_usec(); 
            vappSafeCall(vappiNV12Hqdn3d_8u_P2_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, src_img_shape,   
                                op_cfg.dst_img, dst_img_shape, luma_temporal, chroma_temporal, stream), status); 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);    
            end = time_usec();
            elapsed =  end - start;
            printf("elapsed %" PRId64 " us\n", elapsed/args->frame_count);                                                                   
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status);
        break;                        
    case VAPPI_OP_NV12RESIZE_CTX:

         op_cfg.image_size = op_cfg.plane_size * 3/2;
        op_cfg.out_image_size = op_cfg.oDstSize.width * op_cfg.oDstSize.height * 3/2;

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);

        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
         VappiShape2D nv12_resize_src_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height            
        };
        VappiShape2D nv12_resize_dst_shape[1] = {
            {op_cfg.oDstSize.width,
            op_cfg.oDstSize.height,
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height}           
        }; 
        
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status); 
            start = time_usec(); 
            vappSafeCall(vappiNV12Resize_8u_P2_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, nv12_resize_src_shape, &op_cfg.dst_img,&nv12_resize_dst_shape[0], VAPPI_RESIZE_BICUBIC, stream), status); 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);    
            end = time_usec();
            elapsed =  end - start;
            printf("elapsed %" PRId64 " us\n", elapsed/args->frame_count);                                                                   
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
        break;       
    case VAPPI_OP_COLORTOGRAY:
        op_cfg.oDstSize.width = op_cfg.oSrcSize.width;
        op_cfg.oDstSize.height = op_cfg.oSrcSize.height;
        op_cfg.nDstStep = op_cfg.oDstSize.width;
        op_cfg.image_size = op_cfg.plane_size * 3;
        op_cfg.out_image_size = op_cfg.plane_size;

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  

        start = time_usec();
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            vappSafeCall(vappiRGBToGray_8u_C3P1(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.nDstStep), status);                                                 
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        
        end = time_usec();
        elapsed =  end - start;
        printf("elapsed %" PRId64 " us\n", elapsed/args->frame_count);
        break;

    case VAPPI_OP_YUV420FLIP_CTX:
        op_cfg.image_size = op_cfg.plane_size * 3/2;
        op_cfg.oDstSize = op_cfg.oSrcSize;
        op_cfg.out_image_size = op_cfg.image_size;
        VappiShape2D i420_flip_src_img_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height            
        };
        VappiShape2D i420_flip_dst_img_shape = {
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height,
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height            
        };
 
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);
        eAxis = VAPPI_HORIZONTAL_AXIS;
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  

        
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status); 
            start = time_usec(); 
            
            vappSafeCall(vappiYUV420Mirror_8u_P3_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, i420_flip_src_img_shape,   
                                op_cfg.dst_img, i420_flip_dst_img_shape, stream, eAxis), status); 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);    
            end = time_usec();
            elapsed =  end - start;
            printf("elapsed %"PRId64" us\n", elapsed/args->frame_count);                                                                   
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status);
        break;
    case VAPPI_OP_NV12FLIP_CTX:
        op_cfg.image_size = op_cfg.plane_size * 3/2;
        op_cfg.oDstSize = op_cfg.oSrcSize;
        op_cfg.out_image_size = op_cfg.image_size;
        VappiShape2D nv12_flip_src_img_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height            
        };
        VappiShape2D nv12_flip_dst_img_shape = {
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height,
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height            
        };
 
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);
        VappiAxis eAxis_nv12 = VAPPI_VERTICAL_AXIS;
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  

        
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status); 
            start = time_usec(); 
            
            vappSafeCall(vappiNV12Mirror_8u_P2_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, nv12_flip_src_img_shape,   
                                op_cfg.dst_img, nv12_flip_dst_img_shape, stream, eAxis_nv12), status); 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);    
            end = time_usec();
            elapsed =  end - start;
            printf("elapsed %"PRId64" us\n", elapsed/args->frame_count);                                                                   
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status);
        break;               
    case VAPPI_OP_CROP:
        op_cfg.oCropSize.x = 0;
        op_cfg.oCropSize.y = 0;    
        op_cfg.oCropSize.width = args->output_size.width;
        op_cfg.oCropSize.height = args->output_size.height;

        op_cfg.nDstStep = op_cfg.oCropSize.width;
        op_cfg.image_size = op_cfg.plane_size * 3;
        op_cfg.out_image_size = op_cfg.oCropSize.width * op_cfg.oCropSize.height * 3;

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  

        start = time_usec();
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            vappSafeCall(vappiRGBCrop_8u_C3(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.oCropSize, op_cfg.nDstStep), status);                                                                                               
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        
        end = time_usec();
        elapsed =  end - start;
        printf("elapsed %" PRId64 " us\n", elapsed/args->frame_count);
        break;    
    case VAPPI_OP_YUV420ROTATE_CTX:
        op_cfg.image_size = op_cfg.plane_size * 3/2;
        op_cfg.out_image_size = op_cfg.out_plane_size * 3/2;  

        VappiShape2D yuv420_rotate_src_img_shape = op_cfg.inSrcSize;          
        VappiShape2D yuv420_rotate_dst_img_shape = op_cfg.outSize;       

        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
     
        start = time_usec();
        for(i = 0; i < args->frame_count; i++){            
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }

            rewind(fp_input);   
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            vappSafeCall(vappiYUV420Rotate_8u_P3_Ctx(op_cfg.device_id,  op_cfg.src_img, yuv420_rotate_src_img_shape, op_cfg.dst_img, yuv420_rotate_dst_img_shape, ROTATE_DEGREE_90, stream),status);                       
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
           
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);   
            rewind(fp_out); 
        }
        
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
        end = time_usec();
        elapsed =  end - start;
        printf("total frame count:%d elapsed avg %"PRId64" us, sum %"PRId64" ms\n", args->frame_count, elapsed/args->frame_count, elapsed/1000);

        break;  
    case VAPPI_OP_RGBA_PLANAR_ROTATE:
        if(args->degree == 90){
            nAngle = ROTATE_DEGREE_90;
            op_cfg.nDstStep = op_cfg.oSrcSize.height;
        }
        else if(args->degree == 180){
            nAngle = ROTATE_DEGREE_180;
            op_cfg.nDstStep = op_cfg.oSrcSize.width;
        }
        else if(args->degree == 270){
            nAngle = ROTATE_DEGREE_270;
            op_cfg.nDstStep = op_cfg.oSrcSize.height;
        }

        op_cfg.image_size = op_cfg.plane_size * 4;
        op_cfg.out_image_size = op_cfg.image_size;

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  

        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            rewind(fp_input);   
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            start = time_usec();
            vappSafeCall(vappiRGBAPRotate_8u_P4(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.nDstStep, nAngle), status);                                                                                                                             
            end = time_usec();
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);   
            rewind(fp_out); 
            elapsed =  end - start;
            // printf("elapsed %" PRId64 " us\n", elapsed);
            sum = sum + elapsed;
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        break;
    case VAPPI_OP_RGBA_PLANAR_ROTATE_CTX:
        if(args->degree == 90){
            nAngle = ROTATE_DEGREE_90;
            op_cfg.nDstStep = op_cfg.oSrcSize.height;
        }
        else if(args->degree == 180){
            nAngle = ROTATE_DEGREE_180;
            op_cfg.nDstStep = op_cfg.oSrcSize.width;
        }
        else if(args->degree == 270){
            nAngle = ROTATE_DEGREE_270;
            op_cfg.nDstStep = op_cfg.oSrcSize.height;
        }
        // int rotate_roi_num = 2;
        // VappiRect rotateSizeROI[2] = {
        //     {0,                       0, op_cfg.oSrcSize.width/2, op_cfg.oSrcSize.height},
        //     {op_cfg.oSrcSize.width/2, 0, op_cfg.oSrcSize.width/2, op_cfg.oSrcSize.height}
        // };
        int rotate_roi_num = 1;
        VappiRect rotateSizeROI[2] = {
            {0,                       0, op_cfg.oSrcSize.width, op_cfg.oSrcSize.height},
            {op_cfg.oSrcSize.width/2, 0, op_cfg.oSrcSize.width/2, op_cfg.oSrcSize.height}
        };
        op_cfg.image_size = op_cfg.plane_size * 4;
        op_cfg.out_image_size = op_cfg.image_size;

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, rotate_roi_num, &stream, NULL), status);

        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  

        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            rewind(fp_input);
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status); 
            start = time_usec(); 

            vappSafeCall(vappiRGBAPRotate_8u_P4_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.nDstStep, nAngle, rotate_roi_num, rotateSizeROI, stream), status); 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);    
            end = time_usec();
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out); 
            rewind(fp_out);
            elapsed =  end - start;
            // printf("elapsed %" PRId64 " us\n", elapsed);
            if(elapsed > max){
                max = elapsed;
                printf("max %" PRId64 " us\n", max);
            }
            sum = sum + elapsed;
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
        break;   
     case VAPPI_OP_NV12ROTATE_CTX:
        //op_cfg.nDstStep = op_cfg.oSrcSize.height;
        op_cfg.image_size = op_cfg.plane_size * 3/2;
        op_cfg.out_image_size = op_cfg.out_plane_size * 3/2;  

        VappiShape2D nv12_rotate_src_img_shape = op_cfg.inSrcSize;          
        VappiShape2D nv12_rotate_dst_img_shape = op_cfg.outSize;       

        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
     
        start = time_usec();
        for(i = 0; i < args->frame_count; i++){            
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }

            rewind(fp_input);   
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            // vappSafeCall(vappiNV12Rotate_8u_P3_ctx(op_cfg.device_id, 
            //                     op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
            //                     op_cfg.dst_img, op_cfg.nDstStep, ROTATE_DEGREE_90, stream), status); 
            vappSafeCall(vappiNV12Rotate_8u_P2_Ctx(op_cfg.device_id,  op_cfg.src_img, nv12_rotate_src_img_shape, op_cfg.dst_img, nv12_rotate_dst_img_shape, ROTATE_DEGREE_90, stream), status);
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
           
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);   
            rewind(fp_out); 
        }
        
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
        end = time_usec();
        elapsed =  end - start;
        printf("total frame count:%d elapsed avg %"PRId64" us, sum %"PRId64" ms\n", args->frame_count, elapsed/args->frame_count, elapsed/1000);

        break;  
     case VAPPI_OP_CAS_CTX:
        op_cfg.image_size = op_cfg.plane_size * 3/2;// RGB:*3 YUV:*3/2
        VappiShape2D img_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height            
        };

        strength = 0.5;
        planes = 5;       
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);

        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.image_size), status);  

        
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            rewind(fp_input);   
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status); 
            start = time_usec(); 
            // vappSafeCall(vappiRGBPLANARCas_8u_P3_Ctx(op_cfg.device_id, 
            //                     op_cfg.src_img, img_shape, op_cfg.dst_img, img_shape, strength, planes, stream), status); 
            vappSafeCall(vappiNV12Cas_8u_P3_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, img_shape, op_cfg.dst_img, img_shape, strength, planes, stream), status); 
            // vappSafeCall(vappiYUV420Cas_8u_P3_Ctx(op_cfg.device_id, 
            //                     op_cfg.src_img, img_shape, op_cfg.dst_img, img_shape, strength, planes, stream), status); 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);    
            end = time_usec();
            elapsed =  end - start;
            printf("elapsed %" PRId64 " us\n", elapsed);                                                                   
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.image_size, 1,fp_out);
            sum += elapsed;                        
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status);
        break;                        
    case VAPPI_OP_NV12CSC_CTX:
        op_cfg.oDstSize.width = op_cfg.oSrcSize.width;
        op_cfg.oDstSize.height = op_cfg.oSrcSize.height;
        op_cfg.nDstStep = op_cfg.oDstSize.width;
        op_cfg.image_size = op_cfg.plane_size * 3/2;
        op_cfg.out_image_size = op_cfg.image_size;
        VappiColorSpace oSrcColorSpace = {
            VAPPI_REC_601,
            VAPPI_REC_709_T,
            VAPPI_SMPTE_C,
        };
        VappiColorSpace oDstColorSpace = {
            VAPPI_REC_709_M,
            VAPPI_REC_709_T,
            VAPPI_REC_709_C,
        };        
        Vapp8u nSrcFullRange = 1;
        Vapp8u nDstFullRange = 1;
        Vapp64f nPeakLuminance = 255.0;
        Vapp8u nApproximateGamma = 1;
        Vapp8u nSceneReferred = 0;

        VappiShape2D csc_src_img_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height            
        };
        VappiShape2D csc_dst_img_shape = {
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height,
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height            
        };

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        
        start = time_usec();
        for(i = 0; i < args->frame_count; i++){            
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }

            rewind(fp_input);   
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            vappSafeCall(vappiNV12CSC_8u_P2_Ctx(op_cfg.device_id, op_cfg.src_img, csc_src_img_shape, op_cfg.dst_img, csc_dst_img_shape, oSrcColorSpace, nSrcFullRange,
                             oDstColorSpace, nDstFullRange, nPeakLuminance,  nApproximateGamma,  nSceneReferred, stream),status);                      
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
           
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);   
            rewind(fp_out); 
        }
        
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
        end = time_usec();
        elapsed =  end - start;
        printf("total frame count:%d elapsed avg %"PRId64" us, sum %"PRId64" ms\n", args->frame_count, elapsed/args->frame_count, elapsed/1000);

        break;  
    case VAPPI_OP_NV12OVERLAY_CTX:
        
        op_cfg.oDstSize.width = op_cfg.oSrcSize.width;
        op_cfg.oDstSize.height = op_cfg.oSrcSize.height;
        op_cfg.nDstStep = op_cfg.oDstSize.width;
        op_cfg.image_size = op_cfg.plane_size * 3/2;
        op_cfg.out_image_size = op_cfg.image_size;

        VappiShape2D ovl_src_img_shape = {
        op_cfg.oSrcSize.width,
        op_cfg.oSrcSize.height,
        op_cfg.oSrcSize.width,
        op_cfg.oSrcSize.height,           
        };

        VappiShape2D ovl_dst_img_shape = {
            args->layer_width, 
            args->layer_height,
            args->layer_width, 
            args->layer_height,          
        };

        int layer_plane_size = args->layer_width *args->layer_height;
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        char * pLayerImg = (char*)malloc(layer_plane_size *3/2);
        
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img2, layer_plane_size *3/2), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);

        /* Prepare a dummy image.
         */
        /* Y */
        int x = 0, y = 0;
        for (y = 0; y < args->layer_height; y++) {
            for (x = 0; x < args->layer_width; x++) {
                pLayerImg[y * args->layer_width + x] = x + y + i * 3;
            }
        }

        /* Cb and Cr */
        for (y = 0; y < args->layer_height/2; y++) {
            for (x = 0; x < args->layer_width/2; x++) {
                pLayerImg[y * args->layer_width/2 + x + layer_plane_size] = 128 + y + i * 2;
                pLayerImg[y * args->layer_width/2 + x + layer_plane_size + layer_plane_size/4] = 64 + x + i * 5;
            }
        }        
        vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img2, pLayerImg, layer_plane_size *3/2, vastMemcpyHostToDevice), status);  
        start = time_usec();
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            vappSafeCall(vappiNV12Overlay_8u_P2_Ctx(op_cfg.device_id, op_cfg.src_img, ovl_src_img_shape, op_cfg.src_img2,ovl_dst_img_shape, 200, 300,stream), status);    
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);                                                                    
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.src_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        
        end = time_usec();
        elapsed =  end - start;
        printf("elapsed %" PRId64 " us\n", elapsed/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
        vappSafeCall(vastFree(op_cfg.device_id, op_cfg.src_img2), status);  
        free(pLayerImg);
        break;      
    case VAPPI_OP_BAYERTONV12_CTX:
        op_cfg.image_size = op_cfg.plane_size;
        op_cfg.oDstSize = op_cfg.oSrcSize;
        op_cfg.out_image_size = op_cfg.image_size*3/2;
        VappiShape2D bayer_src_img_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height            
        };
        VappiShape2D bayer_dst_img_shape = {
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height,
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height            
        };
 
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  

        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status); 
            start = time_usec(); 
            
            vappSafeCall(vappiBayerGRGBToNV12_8u_C1P2_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, bayer_src_img_shape,   
                                op_cfg.dst_img, bayer_dst_img_shape,stream), status); 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);    
            end = time_usec();
            elapsed =  end - start;
            printf("elapsed %"PRId64" us\n", elapsed/args->frame_count);                                                                   
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status);
        break;
    case VAPPI_OP_BITDEPTHCVT_CTX:
        op_cfg.image_size = op_cfg.plane_size*3;
        op_cfg.oDstSize = op_cfg.oSrcSize;
        op_cfg.out_image_size = op_cfg.image_size*3/2;
        VappiShape2D bitdepth_src_img_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height            
        };
        VappiShape2D bitdepth_dst_img_shape = {
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height,
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height            
        };
 
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  

        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status); 
            start = time_usec(); 
            
            vappSafeCall(vappiP010Bit10ToBit8_8u_P2_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, bitdepth_src_img_shape,   
                                op_cfg.dst_img, bitdepth_dst_img_shape,stream), status); 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);    
            end = time_usec();
            elapsed =  end - start;
            printf("elapsed %"PRId64" us\n", elapsed/args->frame_count);                                                                   
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status);
        break;    
    case VAPPI_OP_ADAPTIVETHRESH_CTX:
        test_adaptive_thresh(args, &op_cfg, fp_input, fp_out);
        break;
    case VAPPI_OP_TRANSLATETRANS_GRAY_CTX:
        test_translate_transform(args, &op_cfg, fp_input);
        break;   
    case VAPPI_OP_TRANSPOSE_GRAY_CTX:
        status = (VappStatus)test_transpose(args, &op_cfg, fp_input);
        break;              
    case VAPPI_OP_COLORTOGRAY_CTX:
        op_cfg.oDstSize.width = op_cfg.oSrcSize.width;
        op_cfg.oDstSize.height = op_cfg.oSrcSize.height;
        op_cfg.nSrcStep = op_cfg.oSrcSize.width;
        op_cfg.nDstStep = op_cfg.oDstSize.width;
        op_cfg.image_size = op_cfg.plane_size*3;    
        op_cfg.out_image_size = op_cfg.plane_size;
        int cvt_roi_num = 3;
        VappiRect cvtSizeROI[4] = {
            {0,   0, 853,                       op_cfg.oDstSize.height},
            {853, 0, 853,                       op_cfg.oDstSize.height},
            {1706, 0, op_cfg.oDstSize.width-1706, op_cfg.oDstSize.height}
        };
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, blocknum, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        

        
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            rewind(fp_input);
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status); 
            start = time_usec();
            vappSafeCall(vappiRGBPToGray_8u_P3P1R_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.nDstStep, cvt_roi_num, cvtSizeROI, stream), status);    
                
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status); 
            end = time_usec();                                                                                      
            elapsed =  end - start;
            sum = sum + elapsed;
            printf("elapsed %" PRId64 " us sum %" PRId64 " us\n", elapsed,sum);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            // fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);      
            char filename[100];
            sprintf(filename, "cycle_resize_8882gray_2560x2048_%d.rgb", i);
            FILE *fp_out1 = fopen(filename, "wb");
            if(!fp_out1){
                fprintf(stderr, "open failed.\n");
                return -1;
            }  
            fwrite(pDstImg, op_cfg.out_image_size, 1, fp_out1);
            fclose(fp_out1);
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
        
        break;   
    case VAPPI_OP_COLORP2C_CTX:
        op_cfg.oDstSize.width = op_cfg.oSrcSize.width;
        op_cfg.oDstSize.height = op_cfg.oSrcSize.height;
        op_cfg.nSrcStep = op_cfg.oSrcSize.width;
        op_cfg.nDstStep = op_cfg.oDstSize.width;
        op_cfg.image_size = op_cfg.plane_size*3;    
        op_cfg.out_image_size = op_cfg.plane_size*3;
        cvt_roi_num = 3;
        VappiRect _104SizeROI[4] = {
            {0,   0, op_cfg.oDstSize.width/3,                       op_cfg.oDstSize.height},
            {op_cfg.oDstSize.width/3, 0, op_cfg.oDstSize.width/3,                       op_cfg.oDstSize.height},
            {op_cfg.oDstSize.width/3*2, 0, op_cfg.oDstSize.width-op_cfg.oDstSize.width/3*2, op_cfg.oDstSize.height}
        };
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, blocknum, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        

        
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);   
            start = time_usec(); 
            vappSafeCall(vappiRGBP2RGB_8u_P3C3R_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.nDstStep, cvt_roi_num, _104SizeROI, stream), status);    
                                        
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);  
            end = time_usec();                                                                                  
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
             
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
        
        elapsed =  end - start;
        printf("elapsed %" PRId64 " us\n", elapsed/args->frame_count);
        break; 
    case VAPPI_OP_COLORC2P_CTX:
        op_cfg.oDstSize.width = op_cfg.oSrcSize.width;
        op_cfg.oDstSize.height = op_cfg.oSrcSize.height;
        op_cfg.nSrcStep = op_cfg.oSrcSize.width;
        op_cfg.nDstStep = op_cfg.oDstSize.width;
        op_cfg.image_size = op_cfg.plane_size*3;    
        op_cfg.out_image_size = op_cfg.plane_size*3;

        cvt_roi_num = 3;
        VappiRect oC2PROI[4] = {
            {0,   0, op_cfg.oDstSize.width/3,                       op_cfg.oDstSize.height},
            {op_cfg.oDstSize.width/3, 0, op_cfg.oDstSize.width/3,                       op_cfg.oDstSize.height},
            {op_cfg.oDstSize.width/3*2, 0, op_cfg.oDstSize.width-op_cfg.oDstSize.width/3*2, op_cfg.oDstSize.height}
        };           
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, blocknum, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        

        
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            rewind(fp_input);
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            start = time_usec(); 
            vappSafeCall(vappiRGB2RGBP_8u_C3P3R_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.nDstStep, cvt_roi_num, oC2PROI, stream), status);                                 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);  
            end = time_usec();
            elapsed =  end - start;
            sum = sum + elapsed;                                                                                                    
            printf("elapsed %" PRId64 " us sum %" PRId64 " us\n", elapsed,sum);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
        
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        break;
        case VAPPI_OP_COLOR_NV12TORGBP_CTX:
        op_cfg.image_size = op_cfg.plane_size*3/2;    
        op_cfg.out_image_size = op_cfg.image_size*2;
        op_cfg.nDstStep = op_cfg.nSrcStep;
        int nv12_to_rgb_roi_num = 1;

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, nv12_to_rgb_roi_num, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        VappiShape2D nv12_src_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height
        };
        VappiShape2D nv12_dst_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height
        };
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            rewind(fp_input);
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status); 
            start = time_usec();
            vappSafeCall(vappiNV12ToRGBP_8u_P2C3_Ctx(op_cfg.device_id, //vappiNV12ToRGB_8u_P2C3_Ctx
                                op_cfg.src_img, nv12_src_shape,  
                                op_cfg.dst_img, nv12_dst_shape, stream), status);    
                
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status); 
            end = time_usec();                                                                                      
            elapsed =  end - start;
            sum = sum + elapsed;
            printf("elapsed %" PRId64 " us sum %" PRId64 " us\n", elapsed,sum);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);      
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
        
        break;   
        case VAPPI_OP_COLOR_RGBPTONV12_CTX:
        op_cfg.image_size = op_cfg.plane_size*3;    
        op_cfg.out_image_size = op_cfg.image_size/2;
        op_cfg.nDstStep = op_cfg.nSrcStep;
        int rgb_to_nv12_roi_num = 1;

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, rgb_to_nv12_roi_num, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        VappiShape2D rgb_src_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height
        };
        VappiShape2D rgb_dst_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height
        };
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            rewind(fp_input);
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status); 
            start = time_usec();
            vappSafeCall(vappiRGBPToNV12_8u_P2C3_Ctx(op_cfg.device_id, //vappiRGBToNV12_8u_P2C3_Ctx
                                op_cfg.src_img, rgb_src_shape,  
                                op_cfg.dst_img, rgb_dst_shape, stream), status);    
                
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status); 
            end = time_usec();                                                                                      
            elapsed =  end - start;
            sum = sum + elapsed;
            printf("elapsed %" PRId64 " us sum %" PRId64 " us\n", elapsed,sum);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);      
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
        
        break;   
    case VAPPI_OP_ROIFLIP_RGBP_CTX:
        op_cfg.oDstSize.width = op_cfg.oSrcSize.width;
        op_cfg.oDstSize.height = op_cfg.oSrcSize.height;
        op_cfg.nSrcStep = op_cfg.oSrcSize.width;
        op_cfg.nDstStep = op_cfg.oDstSize.width;
        op_cfg.image_size = op_cfg.plane_size*3;    
        op_cfg.out_image_size = op_cfg.plane_size*3;
        int roi_num = 3;
        VappiRect rSizeROI[4] = {
            {0,   0, 901,                       op_cfg.oDstSize.height},
            {901, 0, 901,                       op_cfg.oDstSize.height},
            {1802, 0, op_cfg.oDstSize.width-1802, op_cfg.oDstSize.height}
        };
      

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, blocknum, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        

        
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            rewind(fp_input);
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            start = time_usec(); 
            vappSafeCall(vappiRGBPMirror_8u_P3R_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.nDstStep, VAPPI_BOTH_AXIS, roi_num, rSizeROI, stream), status);                                 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);     
            end = time_usec();
            elapsed =  end - start;
            sum = sum + elapsed;
            printf("elapsed %" PRId64 " us sum %" PRId64 " us\n", elapsed,sum);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            // fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);  
            char filename[100];
            sprintf(filename, "cycle_flip_rgbp_2704x2106_%d.rgb", i);
            FILE *fp_out1 = fopen(filename, "wb");
            if(!fp_out1){
                fprintf(stderr, "open failed.\n");
                return -1;
            }  
            fwrite(pDstImg, op_cfg.out_image_size, 1, fp_out1);
            fclose(fp_out1);
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 

        break;   
    case VAPPI_OP_ROIFLIP_GRAY_CTX:
        op_cfg.oDstSize.width = op_cfg.oSrcSize.width;
        op_cfg.oDstSize.height = op_cfg.oSrcSize.height;
        op_cfg.nSrcStep = op_cfg.oSrcSize.width;
        op_cfg.nDstStep = op_cfg.oDstSize.width;
        op_cfg.image_size = op_cfg.plane_size;    
        op_cfg.out_image_size = op_cfg.plane_size;
        int gray_roi_num = 3;
        VappiRect graySizeROI[4] = {
            {0,   0, 901,                       op_cfg.oDstSize.height},
            {901, 0, 901,                       op_cfg.oDstSize.height},
            {1802, 0, op_cfg.oDstSize.width-1802, op_cfg.oDstSize.height}
        };
      

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, blocknum, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        

        
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            rewind(fp_input);
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            start = time_usec(); 
            vappSafeCall(vappiGrayMirror_8u_P1R_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.nDstStep, VAPPI_BOTH_AXIS, gray_roi_num, graySizeROI, stream), status);                                 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);     
            end = time_usec();
            elapsed =  end - start;
            sum = sum + elapsed;
            printf("elapsed %" PRId64 " us sum %" PRId64 " us\n", elapsed,sum);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            // fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);  
            char filename[100];
            sprintf(filename, "cycle_flip_rgbp_2704x2106_%d.rgb", i);
            FILE *fp_out1 = fopen(filename, "wb");
            if(!fp_out1){
                fprintf(stderr, "open failed.\n");
                return -1;
            }  
            fwrite(pDstImg, op_cfg.out_image_size, 1, fp_out1);
            fclose(fp_out1);
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 

        break;           
    case VAPPI_OP_REMAP_RGBP_CTX:
        if(op_cfg.oDstSize.width == 0)
            op_cfg.oDstSize.width = op_cfg.oSrcSize.width;
        if(op_cfg.oDstSize.height == 0)
            op_cfg.oDstSize.height = op_cfg.oSrcSize.height;
        op_cfg.out_plane_size = op_cfg.oDstSize.width*op_cfg.oDstSize.height;
        op_cfg.nSrcStep = op_cfg.oSrcSize.width;
        op_cfg.nDstStep = op_cfg.oDstSize.width;
        op_cfg.image_size = op_cfg.plane_size*3;    
        op_cfg.out_image_size = op_cfg.out_plane_size*3;
        roi_num = 1;
        VappiRect rRemapP3ROI[4] = {
            {0,0,op_cfg.oDstSize.width, op_cfg.oDstSize.height},
            {901, 0, 901, op_cfg.oDstSize.height},
            {1802, 0, op_cfg.oDstSize.width-1802, op_cfg.oDstSize.height}
        };        
        Vapp32f* map1_p3, *map2_p3;
        Vapp32f* map1h_p3, *map2h_p3;
        FILE * fp_map1_p3 = NULL;
        FILE * fp_map2_p3 = NULL;
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        map1h_p3 = (Vapp32f*)malloc(op_cfg.out_plane_size * sizeof(Vapp32f));
        map2h_p3 = (Vapp32f*)malloc(op_cfg.out_plane_size * sizeof(Vapp32f)); 
        if(op_cfg.oDstSize.width == 640){
            fp_map1_p3 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_480_w_640_in.bin", "rb");
            fp_map2_p3 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_480_w_640_in.bin", "rb");
        }else if(op_cfg.oDstSize.width == 1280){
            fp_map1_p3 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_720_w_1280_in.bin", "rb");
            fp_map2_p3 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_720_w_1280_in.bin", "rb");
        }else if(op_cfg.oDstSize.width == 2704){
            fp_map1_p3 = fopen("/home/vastai/simonz/input/data/remap_map1_h_2106_w_2704_in.bin", "rb");
            fp_map2_p3 = fopen("/home/vastai/simonz/input/data/remap_map2_h_2106_w_2704_in.bin", "rb");
        }else{
            fprintf(stderr, "unsupport dst width %d\n", op_cfg.oDstSize.width);
            return -1;
        }
        size_t reads = fread(map1h_p3, op_cfg.out_plane_size* sizeof(Vapp32f), 1, fp_map1_p3);
        if(reads != 1){
            fprintf(stderr, "fread failed. %ld\n", reads);
            return -1;
        }        
        reads = fread(map2h_p3, op_cfg.out_plane_size* sizeof(Vapp32f), 1, fp_map2_p3);
        if(reads != 1){
            fprintf(stderr, "fread failed. %ld\n", reads);
            return -1;
        }
        fclose(fp_map1_p3);
        fclose(fp_map2_p3);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, blocknum, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&map1_p3, op_cfg.out_plane_size * sizeof(Vapp32f)), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&map2_p3, op_cfg.out_plane_size * sizeof(Vapp32f)), status);         
        

        vappSafeCall(vastMemcpy(op_cfg.device_id, map1_p3, map1h_p3, op_cfg.out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 
        vappSafeCall(vastMemcpy(op_cfg.device_id, map2_p3, map2h_p3, op_cfg.out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            start = time_usec(); 
            vappSafeCall(vappiRGBPRemap_8u_P3R_Ctx(op_cfg.device_id,
                                op_cfg.src_img, map1_p3, map2_p3, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.oDstSize, op_cfg.nDstStep, roi_num, rRemapP3ROI, stream), status);                                 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);     
            end = time_usec();
            elapsed =  end - start;
            printf("elapsed %" PRId64 " us\n", elapsed/args->frame_count);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);  
            rewind(fp_input);                         
        }
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
        vappSafeCall(vastFree(op_cfg.device_id, map1_p3), status);  
        vappSafeCall(vastFree(op_cfg.device_id, map2_p3), status);  
        free(map1h_p3);
        free(map2h_p3);

        break;
    case VAPPI_OP_REMAP_GRAY_CTX:
        if(op_cfg.oDstSize.width == 0)
            op_cfg.oDstSize.width = op_cfg.oSrcSize.width;
        if(op_cfg.oDstSize.height == 0)
            op_cfg.oDstSize.height = op_cfg.oSrcSize.height;
        op_cfg.out_plane_size = op_cfg.oDstSize.width*op_cfg.oDstSize.height;
        op_cfg.nSrcStep = op_cfg.oSrcSize.width;
        op_cfg.nDstStep = op_cfg.oDstSize.width;
        op_cfg.image_size = op_cfg.plane_size;    
        op_cfg.out_image_size = op_cfg.out_plane_size;
        roi_num = 3;
        VappiRect rRemapP1ROI[3] = {
            {0,    0, 901,                        op_cfg.oDstSize.height},
            {901,  0, 901,                        op_cfg.oDstSize.height},
            {1802, 0, op_cfg.oDstSize.width-1802, op_cfg.oDstSize.height}
        };        
        Vapp32f* map1_p1, *map2_p1;
        Vapp32f* map1h_p1, *map2h_p1;
        FILE * fp_map1_p1 = NULL;
        FILE * fp_map2_p1 = NULL;
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        map1h_p1 = (Vapp32f*)malloc(op_cfg.out_plane_size * sizeof(Vapp32f));
        map2h_p1 = (Vapp32f*)malloc(op_cfg.out_plane_size * sizeof(Vapp32f)); 
        if(op_cfg.oDstSize.width == 640){
            fp_map1_p1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_480_w_640_in.bin", "rb");
            fp_map2_p1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_480_w_640_in.bin", "rb");
        }else if(op_cfg.oDstSize.width == 1280){
            fp_map1_p1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_720_w_1280_in.bin", "rb");
            fp_map2_p1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_720_w_1280_in.bin", "rb");
        }else if(op_cfg.oDstSize.width == 2704){
            fp_map1_p1 = fopen("C:/Users/Acemake/Desktop/vapp_release/op/OP_REMAP/data/remap_map1_h_2106_w_2704_in.bin", "rb");
            fp_map2_p1 = fopen("C:/Users/Acemake/Desktop/vapp_release/op/OP_REMAP/data/remap_map2_h_2106_w_2704_in.bin", "rb");
        }else{
            fprintf(stderr, "unsupport dst width %d\n", op_cfg.oDstSize.width);
            return -1;
        }
        reads =fread(map1h_p1, op_cfg.out_plane_size* sizeof(Vapp32f), 1, fp_map1_p1);
        if(reads != 1){
            fprintf(stderr, "fread failed. %ld\n", reads);
            return -1;
        }            
        reads = fread(map2h_p1, op_cfg.out_plane_size* sizeof(Vapp32f), 1, fp_map2_p1);
        if(reads != 1){
            fprintf(stderr, "fread failed. %ld\n", reads);
            return -1;
        }       
        fclose(fp_map1_p1);
        fclose(fp_map2_p1);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, roi_num, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&map1_p1, op_cfg.out_plane_size * sizeof(Vapp32f)), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&map2_p1, op_cfg.out_plane_size * sizeof(Vapp32f)), status);         
        

        vappSafeCall(vastMemcpy(op_cfg.device_id, map1_p1, map1h_p1, op_cfg.out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 
        vappSafeCall(vastMemcpy(op_cfg.device_id, map2_p1, map2h_p1, op_cfg.out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            start = time_usec(); 
            vappSafeCall(vappiGrayRemap_8u_P1R_Ctx(op_cfg.device_id,
                                op_cfg.src_img, map1_p1, map2_p1, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.oDstSize, op_cfg.nDstStep, roi_num, rRemapP1ROI, stream), status);                                 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);     
            end = time_usec();
            elapsed =  end - start;
            sum = sum + elapsed;
            printf("elapsed %" PRId64 " us sum %" PRId64 " us\n", elapsed,sum);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);  
            rewind(fp_input);                         
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
        vappSafeCall(vastFree(op_cfg.device_id, map1_p1), status);  
        vappSafeCall(vastFree(op_cfg.device_id, map2_p1), status);  
        free(map1h_p1);
        free(map2h_p1);

        break;
    case VAPPI_OP_REMAP_GRAY_FIX_CTX:
        test_fixed_remap(args, &op_cfg, fp_input, fp_out);
        break;        
    case VAPPI_OP_CROP_RGB_INTERL_CTX:
        op_cfg.oCropSize.x = 0;
        op_cfg.oCropSize.y = 0;    
        op_cfg.oCropSize.width = args->output_size.width;;
        op_cfg.oCropSize.height = args->output_size.height;

        op_cfg.nDstStep = op_cfg.oCropSize.width;
        op_cfg.image_size = op_cfg.plane_size * 3;
        op_cfg.out_image_size = op_cfg.oCropSize.width * op_cfg.oCropSize.height * 3;

        roi_num = 3;
        VappiRect rCropRIOInterl[3] = {
            {0,   0, 100,                        op_cfg.oCropSize.height},
            {100, 0, 100,                        op_cfg.oCropSize.height},
            {200, 0, op_cfg.oCropSize.width-200, op_cfg.oCropSize.height}
        };        

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, blocknum, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        

        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            rewind(fp_input);
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            start = time_usec();
            vappSafeCall(vappiRGBCrop_8u_C3R_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.oCropSize, op_cfg.nDstStep, roi_num, rCropRIOInterl, stream), status);
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);     
            end = time_usec();
            elapsed =  end - start;
            printf("elapsed %" PRId64 " us\n", elapsed/args->frame_count);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);
        }
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
        break;    
    case VAPPI_OP_CROP_RGB_PLANAR_CTX:
        op_cfg.oCropSize.x = 0;
        op_cfg.oCropSize.y = 0;    
        op_cfg.oCropSize.width = args->output_size.width;
        op_cfg.oCropSize.height = args->output_size.height;

        op_cfg.nDstStep = op_cfg.oCropSize.width;
        op_cfg.image_size = op_cfg.plane_size * 3;
        op_cfg.out_image_size = op_cfg.oCropSize.width * op_cfg.oCropSize.height * 3;

        roi_num = 3;
        VappiRect rCropRIOPlanar[3] = {
            {0,    0, 901,                         op_cfg.oCropSize.height},
            {901,  0, 901,                         op_cfg.oCropSize.height},
            {1802, 0, op_cfg.oCropSize.width-1802, op_cfg.oCropSize.height}
        };        

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, blocknum, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        

        reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
        if(reads != 1){
            fprintf(stderr, "fread failed. %zu\n", reads);
            return -1;
        }
        for(i = 0; i < args->frame_count; i++){
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            start = time_usec();
            vappSafeCall(vappiRGBPCrop_8u_P3R_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.oCropSize, op_cfg.nDstStep, roi_num, rCropRIOPlanar, stream), status);
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);     
            end = time_usec();
            elapsed =  end - start;
            printf("elapsed %" PRId64 " us\n", elapsed/args->frame_count);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);
        }
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
        break;    
    case VAPPI_OP_CROP_GRAY_CTX:
        op_cfg.oCropSize.x = 0;
        op_cfg.oCropSize.y = 5;    
        op_cfg.oCropSize.width = args->output_size.width;
        op_cfg.oCropSize.height = args->output_size.height;

        op_cfg.nDstStep = op_cfg.oCropSize.width;
        op_cfg.image_size = op_cfg.plane_size;
        op_cfg.out_image_size = op_cfg.oCropSize.width * op_cfg.oCropSize.height;

        roi_num = 3;
        VappiRect rCropRIOGray[3] = {
            {op_cfg.oCropSize.x,    op_cfg.oCropSize.y, 901,                         op_cfg.oCropSize.height - op_cfg.oCropSize.y},
            {901,  op_cfg.oCropSize.y, 901,                         op_cfg.oCropSize.height- op_cfg.oCropSize.y},
            {1802, 0, op_cfg.oCropSize.width-1802, op_cfg.oCropSize.height- op_cfg.oCropSize.y}
        };        

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, blocknum, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        

        reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
        if(reads != 1){
            fprintf(stderr, "fread failed. %zu\n", reads);
            return -1;
        }
        for(i = 0; i < args->frame_count; i++){
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            start = time_usec();
            vappSafeCall(vappiGrayCrop_8u_P1R_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.oCropSize, op_cfg.nDstStep, roi_num, rCropRIOGray, stream), status);
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);     
            end = time_usec();
            elapsed =  end - start;
            sum = sum + elapsed;
            printf("elapsed %" PRId64 " us sum %" PRId64 " us\n", elapsed,sum);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
        break;    
    case VAPPI_OP_WARPPERSPECTIVE_CTX:
        op_cfg.out_plane_size = op_cfg.oDstSize.width*op_cfg.oDstSize.height;
        op_cfg.nSrcStep = op_cfg.oSrcSize.width;
        op_cfg.nDstStep = op_cfg.oDstSize.width;
        op_cfg.image_size = op_cfg.plane_size*3;    
        op_cfg.out_image_size = op_cfg.out_plane_size*3;
        roi_num = 3;
        VappiRect rWarpPerspectiveROI[3] = {
            {0,   0, 901,                       op_cfg.oDstSize.height},
            {901, 0, 901,                       op_cfg.oDstSize.height},
            {1802, 0, op_cfg.oDstSize.width-1802, op_cfg.oDstSize.height}
        };

        Vapp64f* pM = NULL;
        pM = (Vapp64f*)malloc(9 * sizeof(Vapp64f));
        FILE *file_M = fopen("/home/vastai/wxhu/qingying/warpperspective/M.bin", "rb");
        reads = fread((char*)pM, 9 * sizeof(Vapp64f), 1, file_M);
        if(reads != 1){
            fprintf(stderr, "fread failed. %zu\n", reads);
            return -1;
        }
        fclose(file_M);
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, blocknum, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        

        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            rewind(fp_input);                         
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            start = time_usec(); 
            vappSafeCall(vappiRGBPWrapPerspective_8u_P3R_Ctx(op_cfg.device_id,
                                op_cfg.src_img, pM, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                                op_cfg.dst_img, op_cfg.oDstSize, op_cfg.nDstStep, roi_num, rWarpPerspectiveROI, stream), status);                                 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);     
            end = time_usec();
            elapsed =  end - start;
            sum = sum + elapsed;
            printf("elapsed %" PRId64 " us sum %" PRId64 " us\n", elapsed,sum);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            // fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);  
            char filename[100];
            sprintf(filename, "cycle_warpperspective_2720x2125_%d.rgb", i);
            FILE *fp_out1 = fopen(filename, "wb");
            if(!fp_out1){
                fprintf(stderr, "open failed.\n");
                return -1;
            }  
            fwrite(pDstImg, op_cfg.out_image_size, 1, fp_out1);
            fclose(fp_out1);
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
        free(pM);
        break;
   case VAPPI_OP_WARPPERSPECTIVE_GRAY_CTX:
        status = (VappStatus)test_gray_warpperspective(args, &op_cfg, fp_input);
        break;        
    case VAPPI_TEST_REMAP_CROP:
        pSrcImg = NULL;
        pDstImg = NULL;
        test_remap_crop(args, &op_cfg, fp_input, fp_out, pSrcImg, pDstImg);
        break;
    case VAPPI_TEST_MULTI_STREAM_8882PLANAR_REMAP:
        pSrcImg = NULL;
        pDstImg = NULL;
        test_multi_stream2();
        break;
    case VAPPI_TEST_MULTI_STREAM_REMAP:
        pSrcImg = NULL;
        pDstImg = NULL;
        test_multi_stream();
        break;
    case VAPPI_TEST_CYCLE_REMAP:
        pSrcImg = NULL;
        pDstImg = NULL;
        test_cycle_remap(args, &op_cfg, fp_input, fp_out, pSrcImg, pDstImg);
        break;
    case VAPPI_TEST_CYCLE_CROP_PLANAR:
        pSrcImg = NULL;
        pDstImg = NULL;
        test_cycle_crop_planar(args, &op_cfg, fp_input, fp_out, pSrcImg, pDstImg);
        break;
    case VAPPI_TEST_C2P_REMAP:
        test_c2p_remap(args, &op_cfg, fp_input, fp_out, pSrcImg, pDstImg);   
        break;
    case VAPPI_TEST_REMAP_THRESHOLD:
        status = (VappStatus)test_multi_stream_remap_thresh(args, &op_cfg, fp_input, fp_out);
        //test_remap_threshold(args, &op_cfg, fp_input, fp_out);
        break;  
     case VAPPI_OP_EQ:
        op_cfg.EqParam.brightness = av_clipf_c(args->brightness, -1.0, 1.0);   //0
        op_cfg.EqParam.contrast = av_clipf_c(args->contrast, -1000, 1000);     //1.0
        op_cfg.EqParam.saturation = av_clipf_c(args->saturation, 0, 3.0);      //1.0
        printf("b:%f c:%f s:%f\n",args->brightness,args->contrast,args->saturation);

        op_cfg.image_size = op_cfg.plane_size * 3/2;
        op_cfg.oDstSize = op_cfg.oSrcSize;
        op_cfg.out_image_size = op_cfg.oDstSize.width * op_cfg.oDstSize.height * 3/2;
        VappiShape2D eq_src_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height            
        };
        VappiShape2D eq_dst_shape = {
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height,
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height            
        };     
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);

        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  

        
        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status); 
            start = time_usec(); 
            vappSafeCall(vappiYUV420EQ_8u_P3_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, eq_src_shape,   
                                op_cfg.dst_img, eq_dst_shape, op_cfg.EqParam.brightness, op_cfg.EqParam.contrast, op_cfg.EqParam.saturation, stream), status); 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);    
            end = time_usec();
            elapsed =  end - start;
            printf("elapsed %" PRId64 " us\n", elapsed/args->frame_count);                                                                   
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
                

        break;

     case VAPPI_OP_SAD:

        fp_input_2 = fopen(args->input_file[1], "rb");
        if(!fp_input_2){
            fprintf(stderr, "open %s failed.\n", args->input_file[1]);
            return -1;
        }

        op_cfg.image_size = op_cfg.plane_size * 3/2;
        op_cfg.out_image_size = op_cfg.oDstSize.width * op_cfg.oDstSize.height * 3/2;

        pSrcImg  = (Vapp8u*)malloc(op_cfg.image_size);
        pSrcImg2 = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg  = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img2, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);

        for(i = 0; i < args->frame_count; i++){
            int reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread 1 failed. %d\n", reads);
                return -1;
            }

            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);

            int reads2 = fread((void *)pSrcImg2, op_cfg.image_size, 1, fp_input_2);
            if(reads2 != 1){
                fprintf(stderr, "fread 2 failed. %d\n", reads2);
                return -1;
            }

            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img2, pSrcImg2, op_cfg.image_size, vastMemcpyHostToDevice), status);

            start = time_usec();
            vappSafeCall(vappiYUV420SAD_8u_P3(op_cfg.device_id,
                                op_cfg.src_img, op_cfg.src_img2, op_cfg.oSrcSize, op_cfg.nSrcStep, op_cfg.dst_img), status);
            end = time_usec();
            elapsed =  end - start;
            sum = sum + elapsed;
            printf("elapsed %" PRId64 " us sum %" PRId64 " us\n", elapsed,sum);
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status);

            fwrite(pDstImg, op_cfg.out_image_size, 1, fp_out);
        }

        if(fp_input_2)
            fclose(fp_input_2);

        printf("avg elapsed %" PRId64 " us\n", sum/args->frame_count);

        break;

     case VAPPI_OP_DETECTION:
     {
        test_static_text_detection(args, &op_cfg, fp_input, fp_out);
        break;
     }
    case VAPPI_OP_YUV420TRANSPOSE_CTX:
        op_cfg.image_size = op_cfg.plane_size * 3/2;
        op_cfg.oDstSize = op_cfg.oSrcSize;
        op_cfg.out_image_size = op_cfg.image_size;
        VappiShape2D i420_transpose_src_img_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height
        };
        VappiShape2D i420_transpose_dst_img_shape = {
            op_cfg.oDstSize.height,
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height,
            op_cfg.oDstSize.width
        };
 
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);
        eDirection = TRANSPOSE_CLOCK_FLIP;
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  

        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);
            start = time_usec(); 
            vappSafeCall(vappiYUV420Transpose_8u_P3_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, i420_transpose_src_img_shape,   
                                op_cfg.dst_img, i420_transpose_dst_img_shape, stream, eDirection), status);
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);
            end = time_usec();
            elapsed =  end - start;
            printf("elapsed %"PRId64" us\n", elapsed/args->frame_count);                                                                   
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status);
        break;
    case VAPPI_OP_NV12TRANSPOSE_CTX:
        op_cfg.image_size = op_cfg.plane_size * 3/2;
        op_cfg.oDstSize = op_cfg.oSrcSize;
        op_cfg.out_image_size = op_cfg.image_size;
        VappiShape2D nv12_transpose_src_img_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height
        };
        VappiShape2D nv12_transpose_dst_img_shape = {
            op_cfg.oDstSize.height,
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height,
            op_cfg.oDstSize.width
        };
 
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);
        eDirection = TRANSPOSE_CLOCK_FLIP;
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  

        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status); 
            start = time_usec(); 
            
            vappSafeCall(vappiNV12Transpose_8u_P2_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, nv12_transpose_src_img_shape,   
                                op_cfg.dst_img, nv12_transpose_dst_img_shape, stream, eDirection), status); 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);    
            end = time_usec();
            elapsed =  end - start;
            printf("elapsed %"PRId64" us\n", elapsed/args->frame_count);                                                                   
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status);
        break;
    case VAPPI_OP_NV12CROPSCALE_CTX:
        op_cfg.plane_size =  op_cfg.oSrcSize.width * op_cfg.oSrcSize.height;
        op_cfg.out_plane_size = op_cfg.oDstSize.width * op_cfg.oDstSize.height;
        int offsetWidth = 300;
        int offsetHeight = 400;
        int cropWidth = 500;
        int cropHeight = 500;
        op_cfg.image_size = op_cfg.plane_size * 3/2;
        op_cfg.out_image_size = op_cfg.out_plane_size * 3/2;
        VappiShape2D nv12_cropscale_src_img_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height
        };
        VappiShape2D nv12_cropscale_dst_img_shape[1] = {
            {op_cfg.oDstSize.width,
            op_cfg.oDstSize.height,
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height}
        };
 
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  

        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status); 
            start = time_usec(); 
            
            vappSafeCall(vappiNV12Nout_Cropscale_8u_P2_Ctx(op_cfg.device_id, 
                                op_cfg.src_img, nv12_cropscale_src_img_shape,   
                                &op_cfg.dst_img, &nv12_cropscale_dst_img_shape[0], offsetWidth, offsetHeight, cropWidth, cropHeight, 1, 0, stream), status); 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);    
            end = time_usec();
            elapsed =  end - start;
            printf("elapsed %"PRId64" us\n", elapsed/args->frame_count);                                                                   
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);                           
        }
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status);
        break;
    case VAAPI_OP_ARGB2NV12_CTX:
        op_cfg.image_size = op_cfg.plane_size * 4;
        op_cfg.out_image_size = op_cfg.oDstSize.width * op_cfg.oDstSize.height * 3/2;
        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  

        VappiShape2D rgba2nv12_src_img_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height
        };
        VappiShape2D rgba2nv12_dst_img_shape[1] = {{
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height
        }};

        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            rewind(fp_input);   
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            start = time_usec();
            vappSafeCall(vappiRGBA2NV12_8u_C4P2R_Ctx(op_cfg.device_id, op_cfg.src_img,rgba2nv12_src_img_shape,
                            &op_cfg.dst_img, &rgba2nv12_dst_img_shape[0],COLOR_SPACE_BT601,stream), status); 
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);                                                                                                                           
            end = time_usec();
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);   
            rewind(fp_out); 
            elapsed =  end - start;
            // printf("elapsed %" PRId64 " us\n", elapsed);
            sum = sum + elapsed;
        }
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status);
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        break;
    case VAAPI_OP_ARGB2NV12_RESIZE_CTX:
        op_cfg.image_size = op_cfg.plane_size * 4;
        op_cfg.out_image_size = op_cfg.image_size*3/2;
        op_cfg.out_image_size2 = op_cfg.oDstSize.width * op_cfg.oDstSize.height * 3/2;

        pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
        pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size2);
        vappSafeCall(vastStreamCreate(op_cfg.device_id, 1, &stream, NULL), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
        vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img2, op_cfg.out_image_size2), status);  
        printf("src ddr:%p, dst1 ddr:%p,dst2 ddr:%p\n",op_cfg.src_img,op_cfg.dst_img,op_cfg.dst_img2);
        VappiShape2D rgba2_src_img_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height
        };
        VappiShape2D rgba2_dst_img_shape[1] = {
            {op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height}
        };

        VappiShape2D rgba2nv12_resize_src_shape = {
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height,
            op_cfg.oSrcSize.width,
            op_cfg.oSrcSize.height          
        };

        VappiShape2D rgba2nv12_resize_dst_shape[1] = {{
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height,
            op_cfg.oDstSize.width,
            op_cfg.oDstSize.height            
        }}; 

        for(i = 0; i < args->frame_count; i++){
            reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }
            rewind(fp_input);   
            vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
            start = time_usec();
            vappSafeCall(vappiRGBA2NV12_8u_C4P2R_Ctx(op_cfg.device_id, op_cfg.src_img,rgba2_src_img_shape,
                            &op_cfg.dst_img, &rgba2_dst_img_shape[0],COLOR_SPACE_BT601,stream), status);  
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);    
            vappSafeCall(vappiNV12Resize_8u_P2_Ctx(op_cfg.device_id, 
                                op_cfg.dst_img, rgba2nv12_resize_src_shape, &op_cfg.dst_img2,&rgba2nv12_resize_dst_shape[0], VAPPI_RESIZE_BICUBIC, stream), status);     
            vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);                                                                                                                    
            end = time_usec();
            vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img2, op_cfg.out_image_size2, vastMemcpyDeviceToHost), status); 
            fwrite(pDstImg, op_cfg.out_image_size2, 1,fp_out);   
            rewind(fp_out); 
            elapsed =  end - start;
            // printf("elapsed %" PRId64 " us\n", elapsed);
            sum = sum + elapsed;
        }
        vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status);
        printf("avg %" PRId64 " us\n", sum/args->frame_count);
        break;
    default:
        status = VAPP_NOT_SUPPORTED_MODE_ERROR;
        break;
    }
    if(op_cfg.src_img)
        vappSafeCall(vastFree(op_cfg.device_id, op_cfg.src_img), status);
    if(op_cfg.dst_img)
        vappSafeCall(vastFree(op_cfg.device_id, op_cfg.dst_img), status);
     if(op_cfg.dst_img2)
        vappSafeCall(vastFree(op_cfg.device_id, op_cfg.dst_img2), status);
    if(fp_out)
        fclose(fp_out);
    if (status == VAPP_SUCCESS) {
        printf("run op successful.\n");
    } else {
        fprintf(stderr, "run op : %d\n", status);
    }
    if(pSrcImg)
        free(pSrcImg);
    if(pDstImg)
        free(pDstImg); 
    if(fp_input)
        fclose(fp_input); 
    free(args->input_file);
    return 0;
}

// void update_map( Vapp32f* map1, Vapp32f* map2 , int width,int height)
// {
//     for( int i = 0; i < height; i++ )
//     {
//         for( int j = 0; j < width; j++ )
//         {
//                 map1[i * height +  j] = (float)j;
//                 map2[i * height +  j] = (float)(height - i);
//         }
//     }
// }

int test_gray_warpperspective(CommandLineArgs *args, OperatorCfg* cfg, FILE *fp_input)
{
    OperatorCfg op_cfg = *cfg;
    vastStream_t stream;
    VappStatus status = VAPP_SUCCESS;
    Vapp8u * pSrcImg = NULL;  
    Vapp8u * pDstImg = NULL;   
    int i = 0;    
    int64_t start, end, elapsed, sum = 0;
    if(!op_cfg.oDstSize.width){
        fprintf(stderr, "line %d: please input dst size\n", __LINE__);
        return -1;
    }
    op_cfg.out_plane_size = op_cfg.oDstSize.width*op_cfg.oDstSize.height;
    op_cfg.nSrcStep = op_cfg.oSrcSize.width;
    op_cfg.nDstStep = op_cfg.oDstSize.width;
    op_cfg.image_size = op_cfg.plane_size;    
    op_cfg.out_image_size = op_cfg.out_plane_size;
    int roi_num = 3;
    VappiRect rWarpPerspectiveROI[3] = {
        {0,   0, 901,                       op_cfg.oDstSize.height},
        {901, 0, 901,                       op_cfg.oDstSize.height},
        {1802, 0, op_cfg.oDstSize.width-1802, op_cfg.oDstSize.height}
    };
    FILE *fp_matrix = NULL;
    if(args->config_file){
        char cfg_line[256];
        char matrix_path[256];
        FILE *fp_cfg = fopen(args->config_file, "rb");
        if(!fgets(cfg_line,255,fp_cfg)){
            return -1;
        }
        if(!fgets(cfg_line,255,fp_cfg)){
            return -1;
        }
        if(!fgets(cfg_line,255,fp_cfg)){
            return -1;
        }
        sscanf(cfg_line,"%s", matrix_path);
        fclose(fp_cfg);
        fp_matrix = fopen(matrix_path,"rb");
    }else{
        fprintf(stderr, "line %d: please input config file\n", __LINE__);
        return -1;
    }
    Vapp64f* pM = NULL;
    pM = (Vapp64f*)malloc(9 * sizeof(Vapp64f));
    size_t reads = fread((char*)pM, 9 * sizeof(Vapp64f), 1, fp_matrix);
    if(reads != 1){
        fprintf(stderr, "fread failed. %zu\n", reads);
        return -1;
    }    

    fclose(fp_matrix);
    pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
    pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
    vappSafeCall(vastStreamCreate(op_cfg.device_id, roi_num, &stream, NULL), status);
    vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
    vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
    

    for(i = 0; i < args->frame_count; i++){
        reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
        if(reads != 1){
            fprintf(stderr, "fread failed. %zu\n", reads);
            return -1;
        }
        rewind(fp_input);                         
        vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
        start = time_usec(); 
        vappSafeCall(vappiGrayWrapPerspective_8u_P1R_Ctx(op_cfg.device_id,
                            op_cfg.src_img, pM, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                            op_cfg.dst_img, op_cfg.oDstSize, op_cfg.nDstStep, roi_num, rWarpPerspectiveROI, stream), status);                                 
        vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);     
        end = time_usec();
        elapsed =  end - start;
        sum = sum + elapsed;
        printf("elapsed %" PRId64 " us sum %" PRId64 "us\n", elapsed,sum);
        vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
        // fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);  


        char filename[100];
        sprintf(filename, "%s_%d.rgb",args->output_file, i);
        FILE *fp_out = fopen(filename,"wb");
        if(!fp_out){
            fprintf(stderr, "fp out open failed.\n");
            return -1;
        }          
        fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);
        fclose(fp_out);
    }
    printf("avg %" PRId64 " us\n", sum/args->frame_count);
    vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
    free(pM);  
    return 0;
}


int test_translate_transform(CommandLineArgs *args, OperatorCfg* op_cfg, FILE *fp_input)
{
    OperatorCfg cfg = *op_cfg;
    int status = 0, i;
    vastStream_t stream;
    int64_t start, end, elapsed, sum = 0;
    Vapp8u * pSrcImg, *pDstImg;
    int roi_num = 1;
    Vapp64f nOffx = -22; //Block size for average, must be odd, 3 ~ 27,
    Vapp64f nOffy = 11; //Max value in output, actually uint8, 0 ~ 255

    
    cfg.oDstSize.width = cfg.oSrcSize.width;
    cfg.oDstSize.height = cfg.oSrcSize.height;
    cfg.plane_size = cfg.oSrcSize.width* cfg.oSrcSize.height;
    cfg.nSrcStep = cfg.oSrcSize.width;
    cfg.nDstStep = cfg.oDstSize.width;
    cfg.image_size = cfg.plane_size;    
    cfg.out_image_size = cfg.plane_size;    
    pSrcImg = (Vapp8u*)malloc(cfg.image_size);
    pDstImg = (Vapp8u*)malloc(cfg.out_image_size);
    vappSafeCall(vastStreamCreate(cfg.device_id, roi_num, &stream, NULL), status);
    vappSafeCall(vastMalloc(cfg.device_id, (void**)&cfg.src_img, cfg.image_size), status);
    vappSafeCall(vastMalloc(cfg.device_id, (void**)&cfg.dst_img, cfg.out_image_size), status);  
    

    for(i = 0; i < args->frame_count; i++){
        int reads = fread((void *)pSrcImg, cfg.image_size, 1, fp_input);
        if(reads != 1){
            fprintf(stderr, "fread failed. %d\n", reads);
            return -1;
        }
        rewind(fp_input);  
        VappiRect oRoi[4] = {
            {0,   0, cfg.oDstSize.width,    cfg.oDstSize.height},
        };                                 
        vappSafeCall(vastMemcpy(cfg.device_id, cfg.src_img, pSrcImg, cfg.image_size, vastMemcpyHostToDevice), status);  
        start = time_usec();
        vappSafeCall(vappiGrayTranslateTransform_8u_P1R_Ctx(cfg.device_id, 
                            cfg.src_img, cfg.oSrcSize, cfg.nSrcStep,  
                            cfg.dst_img, cfg.nDstStep, roi_num, oRoi, nOffx, nOffy, stream), status);
        vappSafeCall(vastStreamSynchronize(cfg.device_id, stream), status);                            
        end = time_usec();
        elapsed =  end - start;
        sum = sum + elapsed;
        printf("elapsed %" PRId64 " us sum %" PRId64 "\n", elapsed, sum);
        vappSafeCall(vastMemcpy(cfg.device_id, pDstImg, cfg.dst_img, cfg.out_image_size, vastMemcpyDeviceToHost), status); 

        char filename[100];
        sprintf(filename, "%s_%d.rgb",args->output_file, i);
        FILE *fp_out = fopen(filename,"wb");
        if(!fp_out){
            fprintf(stderr, "fp out open failed.\n");
            return -1;
        }          
        fwrite(pDstImg, cfg.out_image_size, 1,fp_out);
        fclose(fp_out);
    }
    printf("avg %" PRId64 " us\n", sum/args->frame_count);
    vappSafeCall(vastStreamDestroy(cfg.device_id, stream), status);      
    if(pSrcImg)
        free(pSrcImg);
    if(pDstImg)
        free(pDstImg);     
    return 0;
}

int test_transpose(CommandLineArgs *args, OperatorCfg* op_cfg, FILE *fp_input)
{
    OperatorCfg cfg = *op_cfg;
    int status = 0, i;
    vastStream_t stream;
    int64_t start, end, elapsed, sum = 0;
    Vapp8u * pSrcImg, *pDstImg;
    int roi_num = 4;

    
    cfg.oDstSize.width = cfg.oSrcSize.height;
    cfg.oDstSize.height = cfg.oSrcSize.width;
    cfg.plane_size = cfg.oSrcSize.width* cfg.oSrcSize.height;
    cfg.nSrcStep = cfg.oSrcSize.width;
    cfg.nDstStep = cfg.oDstSize.width;
    cfg.image_size = cfg.plane_size;    
    cfg.out_image_size = cfg.plane_size;    
    pSrcImg = (Vapp8u*)malloc(cfg.image_size);
    pDstImg = (Vapp8u*)malloc(cfg.out_image_size);
    vappSafeCall(vastStreamCreate(cfg.device_id, roi_num, &stream, NULL), status);
    vappSafeCall(vastMalloc(cfg.device_id, (void**)&cfg.src_img, cfg.image_size), status);
    vappSafeCall(vastMalloc(cfg.device_id, (void**)&cfg.dst_img, cfg.out_image_size), status);  
    

    for(i = 0; i < args->frame_count; i++){
        int reads = fread((void *)pSrcImg, cfg.image_size, 1, fp_input);
        if(reads != 1){
            fprintf(stderr, "fread failed. %d\n", reads);
            return -1;
        }
        rewind(fp_input);  
        VappiRect oRoi[4] = {
            {0,   0, cfg.oSrcSize.width/4,    cfg.oSrcSize.height},
            {cfg.oSrcSize.width/4,   0, cfg.oSrcSize.width/4,    cfg.oSrcSize.height},
            {cfg.oSrcSize.width/2,   0, cfg.oSrcSize.width/4,    cfg.oSrcSize.height},
            {cfg.oSrcSize.width/4*3,   0, cfg.oSrcSize.width/4,    cfg.oSrcSize.height}
        };                                 
        vappSafeCall(vastMemcpy(cfg.device_id, cfg.src_img, pSrcImg, cfg.image_size, vastMemcpyHostToDevice), status);  
        start = time_usec();
        vappSafeCall(vappiGrayTranspose_8u_P1R_Ctx(cfg.device_id, 
                            cfg.src_img, cfg.oSrcSize, cfg.nSrcStep,  
                            cfg.dst_img, cfg.nDstStep, roi_num, oRoi,stream), status);
        vappSafeCall(vastStreamSynchronize(cfg.device_id, stream), status);                            
        end = time_usec();
        elapsed =  end - start;
        sum = sum + elapsed;
        printf("elapsed %" PRId64 " us sum %" PRId64 "\n", elapsed, sum);
        vappSafeCall(vastMemcpy(cfg.device_id, pDstImg, cfg.dst_img, cfg.out_image_size, vastMemcpyDeviceToHost), status); 

        char filename[100];
        sprintf(filename, "%s_%d.rgb",args->output_file, i);
        FILE *fp_out = fopen(filename,"wb");
        if(!fp_out){
            fprintf(stderr, "fp out open failed.\n");
            return -1;
        }          
        fwrite(pDstImg, cfg.out_image_size, 1,fp_out);
        fclose(fp_out);
    }
    printf("avg %" PRId64 " us\n", sum/args->frame_count);
    vappSafeCall(vastStreamDestroy(cfg.device_id, stream), status);      
    if(pSrcImg)
        free(pSrcImg);
    if(pDstImg)
        free(pDstImg);     
    return 0;
}

int test_fixed_remap(CommandLineArgs *args, OperatorCfg* cfg, FILE *fp_input, FILE *fp_out)
{
    OperatorCfg op_cfg = *cfg;
    vastStream_t stream;
    VappStatus status = VAPP_SUCCESS;
    Vapp8u * pSrcImg = NULL;  
    Vapp8u * pDstImg = NULL;   
    int i = 0;    
    int ma1_channel = 2;
    int64_t start, end, elapsed, sum = 0;
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
    Vapp16s *map1_p1 = NULL;
    Vapp16u *map2_p1 = NULL;
    Vapp16s *map1h_p1 = NULL;
    Vapp16u *map2h_p1 = NULL;
    FILE * fp_map1_p1 = NULL;
    FILE * fp_map2_p1 = NULL;
    pSrcImg = (Vapp8u*)malloc(op_cfg.image_size);
    pDstImg = (Vapp8u*)malloc(op_cfg.out_image_size);
    map1h_p1 = (Vapp16s*)malloc(op_cfg.out_plane_size * sizeof(Vapp16s) * ma1_channel);
    map2h_p1 = (Vapp16u*)malloc(op_cfg.out_plane_size * sizeof(Vapp16u)); 
    if(op_cfg.oDstSize.width == 2704){
        fp_map1_p1 = fopen("/home/vastai/simonz/input/fixed_remap/remap_map1_h_2106_w_2704_out_DSP.bin", "rb");
        fp_map2_p1 = fopen("/home/vastai/simonz/input/fixed_remap/remap_map2_h_2106_w_2704_out_DSP.bin", "rb");
    }else{
        fprintf(stderr, "unsupport dst width %d\n", op_cfg.oDstSize.width);
        return -1;
    }
    size_t reads = fread(map1h_p1, op_cfg.out_plane_size* sizeof(Vapp16s)*ma1_channel, 1, fp_map1_p1);
    if(reads != 1){
        fprintf(stderr, "fread failed. %zu\n", reads);
        return -1;
    }   
    reads = fread(map2h_p1, op_cfg.out_plane_size* sizeof(Vapp16u), 1, fp_map2_p1);
    if(reads != 1){
        fprintf(stderr, "fread failed. %zu\n", reads);
        return -1;
    }       
    fclose(fp_map1_p1);
    fclose(fp_map2_p1);
    vappSafeCall(vastStreamCreate(op_cfg.device_id, roi_num, &stream, NULL), status);
    vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.src_img, op_cfg.image_size), status);
    vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&op_cfg.dst_img, op_cfg.out_image_size), status);  
    vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&map1_p1, op_cfg.out_plane_size * sizeof(Vapp16s) * ma1_channel), status);
    vappSafeCall(vastMalloc(op_cfg.device_id, (void**)&map2_p1, op_cfg.out_plane_size * sizeof(Vapp16u)), status);         
    

    vappSafeCall(vastMemcpy(op_cfg.device_id, map1_p1, map1h_p1, op_cfg.out_plane_size * sizeof(Vapp16s) * ma1_channel, vastMemcpyHostToDevice), status); 
    vappSafeCall(vastMemcpy(op_cfg.device_id, map2_p1, map2h_p1, op_cfg.out_plane_size * sizeof(Vapp16u), vastMemcpyHostToDevice), status); 
    for(i = 0; i < args->frame_count; i++){
        reads = fread((void *)pSrcImg, op_cfg.image_size, 1, fp_input);
        if(reads != 1){
            fprintf(stderr, "fread failed. %zu\n", reads);
            return -1;
        }
        vappSafeCall(vastMemcpy(op_cfg.device_id, op_cfg.src_img, pSrcImg, op_cfg.image_size, vastMemcpyHostToDevice), status);  
        start = time_usec(); 
        vappSafeCall(vappiGrayRemapFixedMap_8u_P1R_Ctx(op_cfg.device_id,
                            op_cfg.src_img, map1_p1, map2_p1, op_cfg.oSrcSize, op_cfg.nSrcStep,  
                            op_cfg.dst_img, op_cfg.oDstSize, op_cfg.nDstStep, roi_num, rRemapP1ROI, stream), status);                                 
        vappSafeCall(vastStreamSynchronize(op_cfg.device_id, stream), status);     
        end = time_usec();
        elapsed =  end - start;
        sum = sum + elapsed;
        printf("elapsed %" PRId64 " us sum %" PRId64 " us\n", elapsed,sum);
        vappSafeCall(vastMemcpy(op_cfg.device_id, pDstImg, op_cfg.dst_img, op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
        fwrite(pDstImg, op_cfg.out_image_size, 1,fp_out);  
        rewind(fp_input);                         
    }
    printf("avg %" PRId64 " us\n", sum/args->frame_count);
    vappSafeCall(vastStreamDestroy(op_cfg.device_id, stream), status); 
    vappSafeCall(vastFree(op_cfg.device_id, map1_p1), status);  
    vappSafeCall(vastFree(op_cfg.device_id, map2_p1), status);  
    vappSafeCall(vastFree(op_cfg.device_id, pSrcImg), status);  
    vappSafeCall(vastFree(op_cfg.device_id, pDstImg), status);      
    free(map1h_p1);
    free(map2h_p1);    
    return 0;
}
#define ABSOLUTE_PATH_PREFIX "C:/Users/Acemake/Desktop/vapp_release/op/OP_ADDAPTIVE_THRESHOLD/Vector/Case0/Input/"
static int read_data(const char *filename, int size, double *out_data) {
    char filepath[1024];
    FILE *fp = NULL;
    int data = 0;
    size_t reads = 0;

    snprintf(filepath, sizeof(filepath), "%s%s.dat", ABSOLUTE_PATH_PREFIX, filename);

    fp = fopen(filepath, "rb");
    if (fp != NULL) {
        if(size == 8){
            reads = fread(out_data, size, 1, fp);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }            
            printf("%s: %lf\n", filename, *out_data);
        }else{
            reads = fread(&data, size, 1, fp);
            if(reads != 1){
                fprintf(stderr, "fread failed. %zu\n", reads);
                return -1;
            }            
            printf("%s: %d\n", filename, data);
        }
        
        fclose(fp);
    } else {
        fprintf(stderr, "Failed to open file: %s\n", filepath);
    }
    

    return data;
}
#define READ_DATA(param,size) read_data(#param, size, 0)
#define READ_DATA_DOUBLE(param,size) read_data(#param, size, param)

void static get_adaptiveth_input_params(OperatorCfg* op_cfg, double * threshold, int * blockSize, int * maxValue,char *input)
{
    op_cfg->oSrcSize.width  = READ_DATA(width, sizeof(int));
    op_cfg->oSrcSize.height = READ_DATA(height, sizeof(int));
    READ_DATA_DOUBLE(threshold, sizeof(Vapp64f));
    *blockSize = READ_DATA(blockSize, sizeof(int));  
    *maxValue = READ_DATA(maxValue, sizeof(int));
    int channel = READ_DATA(channel, sizeof(int));
    printf("channel %d\n", channel);

    sprintf(input, "%sgrayA.dat", ABSOLUTE_PATH_PREFIX);
    if(channel > 1){
        exit(-1);
    }
}

int test_adaptive_thresh(CommandLineArgs *args, OperatorCfg* op_cfg, FILE *fp_input, FILE *fp_out)
{
    OperatorCfg cfg = *op_cfg;
    int status = 0, i;
    vastStream_t stream;
    int64_t start, end, elapsed, sum = 0;
    Vapp8u * pSrcImg, *pDstImg;
    int roi_num = 1;
    double nThreshold = 127.0;
    int nBlockSize = 13; //Block size for average, must be odd, 3 ~ 27,
    int nMaxValue = 250; //Max value in output, actually uint8, 0 ~ 255
    char input_path[1000];
    get_adaptiveth_input_params(&cfg, &nThreshold, &nBlockSize,  &nMaxValue, input_path);

    printf("nThreshold %lf nBlockSize %d nMaxValue %d\n", nThreshold, nBlockSize, nMaxValue);
    FILE * fp_input1 = fopen(input_path, "rb");
    
    cfg.oDstSize.width = cfg.oSrcSize.width;
    cfg.oDstSize.height = cfg.oSrcSize.height;
    cfg.plane_size = cfg.oSrcSize.width* cfg.oSrcSize.height;
    cfg.nSrcStep = cfg.oSrcSize.width;
    cfg.nDstStep = cfg.oDstSize.width;
    cfg.image_size = cfg.plane_size;    
    cfg.out_image_size = cfg.plane_size;    
    pSrcImg = (Vapp8u*)malloc(cfg.image_size);
    pDstImg = (Vapp8u*)malloc(cfg.out_image_size);
    vappSafeCall(vastStreamCreate(cfg.device_id, roi_num, &stream, NULL), status);
    vappSafeCall(vastMalloc(cfg.device_id, (void**)&cfg.src_img, cfg.image_size), status);
    vappSafeCall(vastMalloc(cfg.device_id, (void**)&cfg.dst_img, cfg.out_image_size), status);  
    

    for(i = 0; i < args->frame_count; i++){
        int reads = fread((void *)pSrcImg, cfg.image_size, 1, fp_input1);
        if(reads != 1){
            fprintf(stderr, "fread failed. %d\n", reads);
            return -1;
        }
        rewind(fp_input1);  
        VappiRect oRoi[4] = {
            {0,   0, cfg.oDstSize.width,    cfg.oDstSize.height},
        };                                 
        vappSafeCall(vastMemcpy(cfg.device_id, cfg.src_img, pSrcImg, cfg.image_size, vastMemcpyHostToDevice), status);  
        start = time_usec();
        vappSafeCall(vappiGrayAdaptiveThreshold_8u_P1R_Ctx(cfg.device_id, 
                            cfg.src_img, cfg.oSrcSize, cfg.nSrcStep,  
                            cfg.dst_img, cfg.nDstStep, roi_num, oRoi, nThreshold, nBlockSize, nMaxValue, stream), status);
        vappSafeCall(vastStreamSynchronize(cfg.device_id, stream), status);                            
        end = time_usec();
        elapsed =  end - start;
        sum = sum + elapsed;
        printf("elapsed %" PRId64 " us sum %" PRId64 "\n", elapsed, sum);
        vappSafeCall(vastMemcpy(cfg.device_id, pDstImg, cfg.dst_img, cfg.out_image_size, vastMemcpyDeviceToHost), status); 

        fwrite(pDstImg, cfg.out_image_size, 1, fp_out);
    }
    printf("avg %" PRId64 " us\n", sum/args->frame_count);
    vappSafeCall(vastStreamDestroy(cfg.device_id, stream), status);      
    fclose(fp_input1);
    if(pSrcImg)
        free(pSrcImg);
    if(pDstImg)
        free(pDstImg);     
    return 0;
}


int test_remap_crop(CommandLineArgs *args, OperatorCfg* op_cfg, FILE *fp_input, FILE *fp_out, Vapp8u * pSrcImg, Vapp8u * pDstImg){
    vastStream_t stream;
    VappStatus status = VAPP_SUCCESS;
    int64_t start, end, elapsed;

    if(op_cfg->oDstSize.width == 0)
        op_cfg->oDstSize.width = op_cfg->oSrcSize.width;
    if(op_cfg->oDstSize.height == 0)
        op_cfg->oDstSize.height = op_cfg->oSrcSize.height;
    op_cfg->out_plane_size = op_cfg->oDstSize.width*op_cfg->oDstSize.height;
    op_cfg->nSrcStep = op_cfg->oSrcSize.width;
    op_cfg->nDstStep = op_cfg->oDstSize.width;
    op_cfg->image_size = op_cfg->plane_size*3;    
    op_cfg->out_image_size = op_cfg->out_plane_size*3;
    int roi_num = 3;
    VappiRect rRemapROI[4] = {
        {0,0,400, op_cfg->oDstSize.height},
        {400, 0, 400, op_cfg->oDstSize.height},
        {800, 0, op_cfg->oDstSize.width-800, op_cfg->oDstSize.height},
        {3000, 3000, 1000,1000}
    };        
    Vapp32f* map1, *map2;
    Vapp32f* map1h, *map2h;
    FILE * fp_map1 = NULL;
    FILE * fp_map2 = NULL;
    pSrcImg = (Vapp8u*)malloc(op_cfg->image_size);
    map1h = (Vapp32f*)malloc(op_cfg->out_plane_size * sizeof(Vapp32f));
    map2h = (Vapp32f*)malloc(op_cfg->out_plane_size * sizeof(Vapp32f)); 
    if(op_cfg->oDstSize.width == 640){
        fp_map1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_480_w_640_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_480_w_640_in.bin", "rb");
    }else if(op_cfg->oDstSize.width == 1280){
        fp_map1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_720_w_1280_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_720_w_1280_in.bin", "rb");
    }else if(op_cfg->oDstSize.width == 2704){
        fp_map1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_2106_w_2704_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_2106_w_2704_in.bin", "rb");
    }else{
        fprintf(stderr, "unsupport dst width %d\n", op_cfg->oDstSize.width);
        return -1;
    }
    size_t reads = fread(map1h, op_cfg->out_plane_size* sizeof(Vapp32f), 1, fp_map1);
    if(reads != 1){
        fprintf(stderr, "fread failed. %zu\n", reads);
        return -1;
    }    
    reads = fread(map2h, op_cfg->out_plane_size* sizeof(Vapp32f), 1, fp_map2);
    if(reads != 1){
        fprintf(stderr, "fread failed. %zu\n", reads);
        return -1;
    }    
    
    fclose(fp_map1);
    fclose(fp_map2);
    vappSafeCall(vastStreamCreate(op_cfg->device_id, roi_num, &stream, NULL), status);
    vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&op_cfg->src_img, op_cfg->image_size), status);
    vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&op_cfg->dst_img, op_cfg->out_image_size), status);  
    vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&map1, op_cfg->out_plane_size * sizeof(Vapp32f)), status);
    vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&map2, op_cfg->out_plane_size * sizeof(Vapp32f)), status);         
    

    vappSafeCall(vastMemcpy(op_cfg->device_id, map1, map1h, op_cfg->out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 
    vappSafeCall(vastMemcpy(op_cfg->device_id, map2, map2h, op_cfg->out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 
    int i = 0;
    Vapp8u* crop_dst_img = NULL;
    for(i=0; i<args->frame_count; i++){
        FILE* fp_input1 = NULL;
        if(i%2 == 0){
            fp_input1 = fopen("/home/vastai/wxhu/qingying/remap_crop/OrgA_rgb_planar.bin", "rb");
            if(!fp_input1){
                fprintf(stderr, "open failed.\n");
                return -1;
            }    
        }else{
            fp_input1 = fopen("/home/vastai/wxhu/qingying/remap_crop/OrgB_rgb_planar.bin", "rb");
            if(!fp_input1){
                fprintf(stderr, "open failed.\n");
                return -1;
            }
        }
        int reads = fread((void *)pSrcImg, op_cfg->image_size, 1, fp_input1);
        if(reads != 1){
            fprintf(stderr, "fread failed. %d\n", reads);
            return -1;
        }
        fclose(fp_input1);
        vappSafeCall(vastMemcpy(op_cfg->device_id, op_cfg->src_img, pSrcImg, op_cfg->image_size, vastMemcpyHostToDevice), status);  
        start = time_usec(); 
        roi_num = 3;
        vappSafeCall(vappiRGBPRemap_8u_P3R_Ctx(op_cfg->device_id,
                            op_cfg->src_img, map1, map2, op_cfg->oSrcSize, op_cfg->nSrcStep,  
                            op_cfg->dst_img, op_cfg->oDstSize, op_cfg->nDstStep, roi_num, rRemapROI, stream), status);                                 
        vappSafeCall(vastStreamSynchronize(op_cfg->device_id, stream), status);
        VappiRect oCropSize = {0};
        oCropSize.x = 0;
        oCropSize.y = 0;
        oCropSize.width = 2501;
        oCropSize.height = 2021;
        VappiRect rCropRIOPlanar[3] = {
            {0,   0, oCropSize.width,                 oCropSize.height},
            {100, 0, 100,                 oCropSize.height},
            {200, 0, oCropSize.width-200, oCropSize.height}
        };
        roi_num = 3;
        int crop_size = oCropSize.width *oCropSize.height * 3;
        vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&crop_dst_img, crop_size), status);
        pDstImg = (Vapp8u*)malloc(crop_size);
        vappSafeCall(vappiRGBPCrop_8u_P3R_Ctx(op_cfg->device_id, 
                            op_cfg->dst_img, op_cfg->oDstSize, op_cfg->nDstStep,  
                            crop_dst_img, oCropSize, oCropSize.width, roi_num, rCropRIOPlanar, stream), status);
        vappSafeCall(vastStreamSynchronize(op_cfg->device_id, stream), status);

        end = time_usec();
        elapsed =  end - start;
        printf("elapsed %" PRId64 " us\n", elapsed);
        vappSafeCall(vastMemcpy(op_cfg->device_id, pDstImg, crop_dst_img, crop_size, vastMemcpyDeviceToHost), status); 
        char filename[100];        
        sprintf(filename, "%s_%d",args->output_file, i);
        FILE *fp_out1 = fopen(filename, "wb");
        if(!fp_out1){
            fprintf(stderr, "open failed.\n");
            return -1;
        }    
        fwrite(pDstImg, crop_size, 1, fp_out1);
        fclose(fp_out1);

    }   
    vappSafeCall(vastStreamDestroy(op_cfg->device_id, stream), status); 
    vappSafeCall(vastFree(op_cfg->device_id, map1), status);  
    vappSafeCall(vastFree(op_cfg->device_id, map2), status);  
    free(map1h);
    free(map2h);
    vappSafeCall(vastFree(op_cfg->device_id, crop_dst_img), status);  
    return 0;
}

int test_cycle_remap(CommandLineArgs *args, OperatorCfg* op_cfg, FILE *fp_input, FILE *fp_out, Vapp8u * pSrcImg, Vapp8u * pDstImg){
    vastStream_t stream;
    VappStatus status = VAPP_SUCCESS;
    int64_t start, end, elapsed, sum = 0;

    if(op_cfg->oDstSize.width == 0)
        op_cfg->oDstSize.width = op_cfg->oSrcSize.width;
    if(op_cfg->oDstSize.height == 0)
        op_cfg->oDstSize.height = op_cfg->oSrcSize.height;
    op_cfg->out_plane_size = op_cfg->oDstSize.width*op_cfg->oDstSize.height;
    op_cfg->nSrcStep = op_cfg->oSrcSize.width;
    op_cfg->nDstStep = op_cfg->oDstSize.width;
    op_cfg->image_size = op_cfg->plane_size*3;    
    op_cfg->out_image_size = op_cfg->out_plane_size*3;
    int roi_num = 3;
    VappiRect rRemapROI[4] = {
        {0,0,901, op_cfg->oDstSize.height},
        {901, 0, 901, op_cfg->oDstSize.height},
        {1802, 0, op_cfg->oDstSize.width-1802, op_cfg->oDstSize.height}
    };        
    Vapp32f* map1, *map2;
    Vapp32f* map1h, *map2h;
    FILE * fp_map1 = NULL;
    FILE * fp_map2 = NULL;
    pSrcImg = (Vapp8u*)malloc(op_cfg->image_size);
    map1h = (Vapp32f*)malloc(op_cfg->out_plane_size * sizeof(Vapp32f));
    map2h = (Vapp32f*)malloc(op_cfg->out_plane_size * sizeof(Vapp32f)); 
    if(op_cfg->oDstSize.width == 640){
        fp_map1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_480_w_640_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_480_w_640_in.bin", "rb");
    }else if(op_cfg->oDstSize.width == 1280){
        fp_map1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_720_w_1280_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_720_w_1280_in.bin", "rb");
    }else if(op_cfg->oDstSize.width == 2704){
        fp_map1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_2106_w_2704_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_2106_w_2704_in.bin", "rb");
    }else{
        fprintf(stderr, "unsupport dst width %d\n", op_cfg->oDstSize.width);
        return -1;
    }
    size_t reads = fread(map1h, op_cfg->out_plane_size* sizeof(Vapp32f), 1, fp_map1);
    if(reads != 1){
        fprintf(stderr, "fread failed. %zu\n", reads);
        return -1;
    }    
    reads = fread(map2h, op_cfg->out_plane_size* sizeof(Vapp32f), 1, fp_map2);
    if(reads != 1){
        fprintf(stderr, "fread failed. %zu\n", reads);
        return -1;
    }     
    
    fclose(fp_map1);
    fclose(fp_map2);

    vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&op_cfg->src_img, op_cfg->image_size), status);
    vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&op_cfg->dst_img, op_cfg->out_image_size), status);  
    vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&map1, op_cfg->out_plane_size * sizeof(Vapp32f)), status);
    vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&map2, op_cfg->out_plane_size * sizeof(Vapp32f)), status);         
    vappSafeCall(vastStreamCreate(op_cfg->device_id,roi_num, &stream, NULL), status);

    vappSafeCall(vastMemcpy(op_cfg->device_id, map1, map1h, op_cfg->out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 
    vappSafeCall(vastMemcpy(op_cfg->device_id, map2, map2h, op_cfg->out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 
    int i = 0;
    for(i=0; i<args->frame_count; i++){
        FILE* fp_input1 = NULL;
        if(i%2 == 0){
            fp_input1 = fopen("/home/vastai/wxhu/qingying/data/OrgA_planar_2576x2032.rgb", "rb");
            if(!fp_input1){
                fprintf(stderr, "open failed.\n");
                return -1;
            }    
        }else{
            fp_input1 = fopen("/home/vastai/wxhu/qingying/data/OrgB_planar_2576x2032.rgb", "rb");
            if(!fp_input1){
                fprintf(stderr, "open failed.\n");
                return -1;
            }
        }
        int reads = fread((void *)pSrcImg, op_cfg->image_size, 1, fp_input1);
        if(reads != 1){
            fprintf(stderr, "fread failed. %d\n", reads);
            return -1;
        }
        fclose(fp_input1);
        vappSafeCall(vastMemcpy(op_cfg->device_id, op_cfg->src_img, pSrcImg, op_cfg->image_size, vastMemcpyHostToDevice), status);  
        start = time_usec(); 
        vappSafeCall(vappiRGBPRemap_8u_P3R_Ctx(op_cfg->device_id,
                            op_cfg->src_img, map1, map2, op_cfg->oSrcSize, op_cfg->nSrcStep,  
                            op_cfg->dst_img, op_cfg->oDstSize, op_cfg->nDstStep, roi_num, rRemapROI, stream), status);                                 
        vappSafeCall(vastStreamSynchronize(op_cfg->device_id, stream), status);
        end = time_usec();
        elapsed =  end - start;
        sum = sum + elapsed;
        printf("elapsed %" PRId64 " us sum %" PRId64 " us\n", elapsed,sum);
        pDstImg = (Vapp8u*)malloc(op_cfg->out_image_size);
        vappSafeCall(vastMemcpy(op_cfg->device_id, pDstImg, op_cfg->dst_img, op_cfg->out_image_size, vastMemcpyDeviceToHost), status); 
        char filename[100];
        sprintf(filename, "cycle_remap_2704x2106_%d.rgbp", i);
        FILE *fp_out1 = fopen(filename, "wb");
        if(!fp_out1){
            fprintf(stderr, "open failed.\n");
            return -1;
        }  
        fwrite(pDstImg, op_cfg->out_image_size, 1, fp_out1);
        fclose(fp_out1);
    }   
    printf("avg %" PRId64 " us\n", sum/args->frame_count);
    vappSafeCall(vastStreamDestroy(op_cfg->device_id, stream), status); 
    vappSafeCall(vastFree(op_cfg->device_id, map1), status);  
    vappSafeCall(vastFree(op_cfg->device_id, map2), status);  
    free(map1h);
    free(map2h);
    return 0;
}

int test_cycle_crop_planar(CommandLineArgs *args, OperatorCfg* op_cfg, FILE *fp_input, FILE *fp_out, Vapp8u * pSrcImg, Vapp8u * pDstImg){
    vastStream_t stream;
    VappStatus status =  VAPP_SUCCESS;
    int64_t start, end, elapsed, sum = 0;
    int i = 0;
    int roi_num = 3;

    op_cfg->oCropSize.x = 0;
    op_cfg->oCropSize.y = 5;    
    op_cfg->oCropSize.width = args->output_size.width;
    op_cfg->oCropSize.height = args->output_size.height;

    op_cfg->nDstStep = op_cfg->oCropSize.width;
    op_cfg->image_size = op_cfg->plane_size * 3;
    op_cfg->out_image_size = op_cfg->oCropSize.width * op_cfg->oCropSize.height * 3;

    VappiRect rCropRIOPlanar[3] = {
        {0,    0, 901,                          op_cfg->oCropSize.height},
        {901,  0, 901,                          op_cfg->oCropSize.height},
        {1802, 0, op_cfg->oCropSize.width-1802, op_cfg->oCropSize.height}
    };        

    pSrcImg = (Vapp8u*)malloc(op_cfg->image_size);
    pDstImg = (Vapp8u*)malloc(op_cfg->out_image_size);
    vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&op_cfg->src_img, op_cfg->image_size), status);
    vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&op_cfg->dst_img, op_cfg->out_image_size), status);  
    vappSafeCall(vastStreamCreate(op_cfg->device_id,roi_num, &stream, NULL), status);

    // int reads = fread((void *)pSrcImg, op_cfg->image_size, 1, fp_input);
    // if(reads != 1){
    //     fprintf(stderr, "fread failed. %d\n", reads);
    //     return -1;
    // }
    for(i = 0; i < args->frame_count; i++){
        FILE* fp_input1 = NULL;
        if(i%2 == 0){
            fp_input1 = fopen("/home/vastai/wxhu/qingying/time/in_crop_planar_2704x2106_OrgA.rgb", "rb");
            if(!fp_input1){
                fprintf(stderr, "open failed.\n");
                return -1;
            }    
        }else{
            fp_input1 = fopen("/home/vastai/wxhu/qingying/time/in_crop_planar_2704x2106_OrgB.rgb", "rb");
            if(!fp_input1){
                fprintf(stderr, "open failed.\n");
                return -1;
            }
        }
        int reads = fread((void *)pSrcImg, op_cfg->image_size, 1, fp_input1);
        if(reads != 1){
            fprintf(stderr, "fread failed. %d\n", reads);
            return -1;
        }
        fclose(fp_input1);
        vappSafeCall(vastMemcpy(op_cfg->device_id, op_cfg->src_img, pSrcImg, op_cfg->image_size, vastMemcpyHostToDevice), status);  
        start = time_usec();
        vappSafeCall(vappiRGBPCrop_8u_P3R_Ctx(op_cfg->device_id, 
                            op_cfg->src_img, op_cfg->oSrcSize, op_cfg->nSrcStep,  
                            op_cfg->dst_img, op_cfg->oCropSize, op_cfg->nDstStep, roi_num, rCropRIOPlanar, stream), status);
        vappSafeCall(vastStreamSynchronize(op_cfg->device_id, stream), status);     
        end = time_usec();
        elapsed =  end - start;
        sum = sum + elapsed;
        printf("elapsed %" PRId64 " us sum %" PRId64 " us\n", elapsed,sum);
          
        vappSafeCall(vastMemcpy(op_cfg->device_id, pDstImg, op_cfg->dst_img, op_cfg->out_image_size, vastMemcpyDeviceToHost), status); 
        char filename[100];
        sprintf(filename, "cycle_crop_planar_2691x2101_%d.rgb", i);
        FILE *fp_out1 = fopen(filename, "wb");
        if(!fp_out1){
            fprintf(stderr, "open failed.\n");
            return -1;
        }  
        fwrite(pDstImg, op_cfg->out_image_size, 1, fp_out1);
        fclose(fp_out1);
    }
    printf("avg %" PRId64 " us\n", sum/args->frame_count);
    vappSafeCall(vastStreamDestroy(op_cfg->device_id, stream), status); 

    return 0;
}

int test_static_text_detection(CommandLineArgs *args, OperatorCfg* op_cfg, FILE *fp_input, FILE *fp_out){

    VappStatus status = VAPP_SUCCESS;
    Vapp8u *pSrcImg = NULL;
    Vapp8s *pRoiMap = NULL;
    VappiTextDetectionParam dect_param;
    VappiTextDetectionBuffers buffers = {NULL};
    Vapp64s start = 0, end = 0, elapsed = 0, sum = 0;
    Vapp32u i;
    //Vapp8u first_frame = 1;

    dect_param.plane            = args->plane;
    dect_param.block_size       = 16;
    dect_param.text_enable      = 1;
    dect_param.extend_enable    = 1;
    dect_param.grident_enable   = 1;
    dect_param.laplacian_enable = 1;
    dect_param.hist_enable      = 1;
    dect_param.morpho_enable    = 1;
    dect_param.static_enable    = 1;

    dect_param.qp_offset_static      = -5;
    dect_param.qp_offset_static_text = -7;
    dect_param.hist_bin_size         = 8;
    dect_param.max_bin_size          = 3;
    dect_param.laplacian_th          = 2000;
    dect_param.gradient_th           = 10;
    dect_param.hist_percentage_th    = 0.65;
    dect_param.morphological_th      = 0.5;

    if (dect_param.plane == 1) {

        op_cfg->image_size = op_cfg->plane_size * 3 / 2;
        op_cfg->nSrcStep   = op_cfg->oSrcSize.width;

    } else {
        op_cfg->image_size = op_cfg->plane_size * dect_param.plane;
        op_cfg->nSrcStep   = op_cfg->oSrcSize.width * dect_param.plane;
    }

    op_cfg->out_roimap_size = (ALIGN(op_cfg->oSrcSize.width, 32) * ALIGN(op_cfg->oSrcSize.height, 32)) / (dect_param.block_size * dect_param.block_size);

    pSrcImg  = (Vapp8u*)malloc(op_cfg->image_size);
    pRoiMap  = (Vapp8s*)malloc(op_cfg->out_roimap_size);

#ifndef RUN_DETECTION_OP_HOST
    op_cfg->plane_size = ALIGN(op_cfg->oSrcSize.width, 32) * ALIGN(op_cfg->oSrcSize.height, 32);
    vastStream_t stream;
    vappSafeCall(vastStreamCreate(op_cfg->device_id, 1, &stream, NULL), status);
    Vapp32u detect_buffer_size = ALIGN(op_cfg->oSrcSize.width, 32) * ALIGN(op_cfg->oSrcSize.height, 32) * sizeof(Vapp16s);
    vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&buffers.in_img_addr, op_cfg->image_size), status);
    vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&buffers.out_roi_map_final_addr, op_cfg->out_roimap_size), status);
    vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&buffers.out_gray_addr,      op_cfg->plane_size), status);
    vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&buffers.pre_gray_addr,      op_cfg->plane_size), status);
    vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&buffers.out_sobelx_addr,    detect_buffer_size), status);
    vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&buffers.out_sobely_addr ,   detect_buffer_size), status);
    vappSafeCall(vastMalloc(op_cfg->device_id, (void**)&buffers.out_laplacian_addr, detect_buffer_size), status);

    Vapp8u *out_gray_addr = buffers.out_gray_addr;
    Vapp8u *pre_gray_addr = buffers.pre_gray_addr;
#else
    buffers.in_img_addr = pSrcImg;
    buffers.out_roi_map_final_addr = pRoiMap;
    buffers.out_gray_addr = (Vapp8u*)calloc(1, op_cfg->plane_size);
    buffers.pre_gray_addr = (Vapp8u*)calloc(1, op_cfg->plane_size);
#endif

    for (i = 0; i < args->frame_count; i++) {

        int reads = fread((void *)pSrcImg, op_cfg->image_size, 1, fp_input);
        if(reads != 1){
            fprintf(stderr, "fread 1 failed. %d\n", reads);
            return -1;
        }

        memset(pRoiMap, 0, op_cfg->out_roimap_size);

#ifdef RUN_DETECTION_OP_HOST
        start = time_usec();

        //vappSafeCall(vappiStaticTextDetection_8u_P3_Host(op_cfg->device_id, &buffers, op_cfg->oSrcSize, op_cfg->nSrcStep, &dect_param, first_frame), status);

        end = time_usec();
        elapsed =  end - start;
        sum = sum + elapsed;
        printf("elapsed %lld  us sum %lld us\n", elapsed, sum);

#else
        vappSafeCall(vastMemcpy(op_cfg->device_id, buffers.in_img_addr, pSrcImg, op_cfg->image_size, vastMemcpyHostToDevice), status);

        start = time_usec();
        buffers.out_gray_addr = i % 2 == 0 ? out_gray_addr : pre_gray_addr;
		buffers.pre_gray_addr = i % 2 == 0 ? pre_gray_addr : out_gray_addr;

        vappSafeCall(vappiStaticTextDetection_8u_P3_Ctx(op_cfg->device_id, &buffers, op_cfg->oSrcSize, op_cfg->nSrcStep, &dect_param, first_frame, stream), status);
        vappSafeCall(vastStreamSynchronize(op_cfg->device_id, stream), status);    

        end = time_usec();
        elapsed =  end - start;
        sum = sum + elapsed;
        printf("elapsed %lld  us sum %lld us\n", elapsed, sum);

        vappSafeCall(vastMemcpy(op_cfg->device_id, pRoiMap, buffers.out_roi_map_final_addr, op_cfg->out_roimap_size, vastMemcpyDeviceToHost), status);
#endif
        //first_frame = 0;
        
        if(i > 0)
        {
            Vapp32u cols = ALIGN(op_cfg->oSrcSize.width, 32)/16;
            for (int j = 0; j < op_cfg->out_roimap_size; j++)
            {      
                if (j % cols == 0 && j != 0)
                {                   
                    fprintf(fp_out, "\n");
                }
                fprintf(fp_out, "%2d ", pRoiMap[j]);      
            }                   
            //fwrite(pRoiMap, op_cfg->out_roimap_size , 1, fp_out);
        }
    }


#ifdef RUN_DETECTION_OP_HOST
    if (buffers.out_gray_addr) {
        free(buffers.out_gray_addr);
        buffers.out_gray_addr = NULL;
    }
    if (buffers.pre_gray_addr) {
        free(buffers.pre_gray_addr);
        buffers.pre_gray_addr = NULL;
    }
#else
    vappSafeCall(vastStreamDestroy(op_cfg->device_id, stream), status); 
    if (buffers.in_img_addr) {
        vappSafeCall(vastFree(op_cfg->device_id, buffers.in_img_addr), status);
        buffers.in_img_addr = NULL;
    }
    if (buffers.out_roi_map_final_addr) {
        vappSafeCall(vastFree(op_cfg->device_id, buffers.out_roi_map_final_addr), status);
        buffers.out_roi_map_final_addr = NULL;
    }
    if (buffers.out_gray_addr) {
        vappSafeCall(vastFree(op_cfg->device_id, buffers.out_gray_addr), status);
        buffers.out_gray_addr = NULL;
    }
    if (buffers.pre_gray_addr) {
        vappSafeCall(vastFree(op_cfg->device_id, buffers.pre_gray_addr), status);
        buffers.pre_gray_addr = NULL;
    }
    if (buffers.out_sobelx_addr) {
        vappSafeCall(vastFree(op_cfg->device_id, buffers.out_sobelx_addr), status);
        buffers.out_sobelx_addr = NULL;
    }
    if (buffers.out_sobely_addr) {
        vappSafeCall(vastFree(op_cfg->device_id, buffers.out_sobely_addr), status);
        buffers.out_sobely_addr = NULL;
    }
    if (buffers.out_laplacian_addr) {
        vappSafeCall(vastFree(op_cfg->device_id, buffers.out_laplacian_addr), status);
        buffers.out_laplacian_addr = NULL;
    }
#endif
    if (pRoiMap)
        free(pRoiMap);
    if (pSrcImg)
        free(pSrcImg);
    printf("avg elapsed %lld us\n", sum/args->frame_count);

    return status;

}


#if __linux__
struct thread_info{
    pthread_t thread_id;
    int thread_num;
    int frame_count;
    OperatorCfg op_cfg;
    OperatorCfg op_cfg2;
    vastStream_t stream;
    Vapp8u* src_img;
    Vapp8u* dst_img;
    FILE* fp_in;
    FILE* fp_out;
    char * config_file;
};
struct thread_info stream_arg[MAX_THREAD_NUM] = { 0 };

int test_multi_stream2(){
    int i = 0;
    int stream_num = 4;

    for(i=0; i<stream_num; i++){
        stream_arg[i].thread_num = i;
        stream_arg[i].frame_count = 5000;
        stream_arg[i].op_cfg.device_id = 0;
        stream_arg[i].op_cfg.oSrcSize.width = 2576;
        stream_arg[i].op_cfg.oSrcSize.height = 2032;
        stream_arg[i].op_cfg.oDstSize.width = 2704;
        stream_arg[i].op_cfg.oDstSize.height = 2106; 
        stream_arg[i].op_cfg.nSrcStep = stream_arg[i].op_cfg.oSrcSize.width;
        stream_arg[i].op_cfg.nDstStep = stream_arg[i].op_cfg.oDstSize.width;
        stream_arg[i].op_cfg.plane_size = stream_arg[i].op_cfg.oSrcSize.width * stream_arg[i].op_cfg.oSrcSize.height;
        stream_arg[i].op_cfg.out_plane_size = stream_arg[i].op_cfg.oDstSize.width * stream_arg[i].op_cfg.oDstSize.height;
        stream_arg[i].op_cfg.image_size = stream_arg[i].op_cfg.plane_size * 3;
        stream_arg[i].op_cfg.out_image_size = stream_arg[i].op_cfg.out_plane_size * 3;
        if ((pthread_create(&stream_arg[i].thread_id, NULL, (void*)process_one_stream3, (void *)&stream_arg[i])) == -1) {
            printf("create process dec error!\n");
            return -1;
        }
        // printf("%s %d %s here stream_arg[%d].thread_id=%ld\n",__FILE__,__LINE__,__FUNCTION__,i,stream_arg[i].thread_id);
        // pthread_join(stream_arg[i].thread_id, NULL);
    }
    while(1){
        sleep(1);
        printf("%s %d %s here sleep\n",__FILE__,__LINE__,__FUNCTION__);
    }
    return 0;
}

int test_multi_stream(){
    int i = 0;
    int stream_num = 4;

    for(i=0; i<stream_num; i++){
        stream_arg[i].thread_num = i;
        stream_arg[i].frame_count = 100;
        stream_arg[i].op_cfg.device_id = 0;
        stream_arg[i].op_cfg.oSrcSize.width = 2576;
        stream_arg[i].op_cfg.oSrcSize.height = 2032;
        stream_arg[i].op_cfg.oDstSize.width = 2704;
        stream_arg[i].op_cfg.oDstSize.height = 2106; 
        stream_arg[i].op_cfg.nSrcStep = stream_arg[i].op_cfg.oSrcSize.width;
        stream_arg[i].op_cfg.nDstStep = stream_arg[i].op_cfg.oDstSize.width;
        stream_arg[i].op_cfg.plane_size = stream_arg[i].op_cfg.oSrcSize.width * stream_arg[i].op_cfg.oSrcSize.height;
        stream_arg[i].op_cfg.out_plane_size = stream_arg[i].op_cfg.oDstSize.width * stream_arg[i].op_cfg.oDstSize.height;
        stream_arg[i].op_cfg.image_size = stream_arg[i].op_cfg.plane_size * 3;
        stream_arg[i].op_cfg.out_image_size = stream_arg[i].op_cfg.out_plane_size * 3;
        if ((pthread_create(&stream_arg[i].thread_id, NULL, (void*)process_one_stream, (void *)&stream_arg[i])) == -1) {
            printf("create process dec error!\n");
            return -1;
        }
        // printf("%s %d %s here stream_arg[%d].thread_id=%ld\n",__FILE__,__LINE__,__FUNCTION__,i,stream_arg[i].thread_id);
        // pthread_join(stream_arg[i].thread_id, NULL);
    }
    while(1){
        sleep(1);
    }
    return 0;
}

int process_one_stream(void *arg){
    printf("%s %d %s start thread_num=%d thread_id=%ld ,pthread_self()=%ld\n",__FILE__,__LINE__,__FUNCTION__,stream_arg->thread_num,stream_arg->thread_id,pthread_self());
    VappStatus status =  VAPP_SUCCESS;
    int64_t start, end, elapsed;
    int roi_num = 3;
    int i = 0;
    struct thread_info *stream_arg = (struct thread_info *)arg;
    
    VappiRect rRemapROI[4] = {
        {0,    0, 901,                                    stream_arg->op_cfg.oDstSize.height},
        {901,  0, 901,                                    stream_arg->op_cfg.oDstSize.height},
        {1802, 0, stream_arg->op_cfg.oDstSize.width-1802, stream_arg->op_cfg.oDstSize.height}
    };        
    Vapp32f* map1, *map2;
    Vapp32f* map1h, *map2h;
    FILE * fp_map1 = NULL;
    FILE * fp_map2 = NULL;
    Vapp8u* pSrcImg = (Vapp8u*)malloc(stream_arg->op_cfg.image_size);
    Vapp8u* pDstImg = (Vapp8u*)malloc(stream_arg->op_cfg.out_image_size);
    map1h = (Vapp32f*)malloc(stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f));
    map2h = (Vapp32f*)malloc(stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f)); 
    if(stream_arg->op_cfg.oDstSize.width == 640){
        fp_map1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_480_w_640_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_480_w_640_in.bin", "rb");
    }else if(stream_arg->op_cfg.oDstSize.width == 1280){
        fp_map1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_720_w_1280_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_720_w_1280_in.bin", "rb");
    }else if(stream_arg->op_cfg.oDstSize.width == 2704){
        fp_map1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_2106_w_2704_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_2106_w_2704_in.bin", "rb");
    }else{
        fprintf(stderr, "unsupport dst width %d\n", stream_arg->op_cfg.oDstSize.width);
        return -1;
    }
    size_t reads = fread(map1h, stream_arg->op_cfg.out_plane_size* sizeof(Vapp32f), 1, fp_map1);
    if(reads != 1){
        fprintf(stderr, "fread failed. %zu\n", reads);
        return -1;
    }     
    reads = fread(map2h, stream_arg->op_cfg.out_plane_size* sizeof(Vapp32f), 1, fp_map2);
    if(reads != 1){
        fprintf(stderr, "fread failed. %zu\n", reads);
        return -1;
    }      
    fclose(fp_map1);
    fclose(fp_map2);

    vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&stream_arg->op_cfg.src_img, stream_arg->op_cfg.image_size), status);
    vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&stream_arg->op_cfg.dst_img, stream_arg->op_cfg.out_image_size), status);  
    vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&map1, stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f)), status);
    vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&map2, stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f)), status);         
    vappSafeCall(vastStreamCreate(stream_arg->op_cfg.device_id,roi_num, &stream_arg->stream, NULL), status);
    vappSafeCall(vastMemcpy(stream_arg->op_cfg.device_id, map1, map1h, stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 
    vappSafeCall(vastMemcpy(stream_arg->op_cfg.device_id, map2, map2h, stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 
    for(i = 0; i < stream_arg->frame_count; i++){
        stream_arg->fp_in = fopen("/home/vastai/wxhu/qingying/data/OrgA_planar_2576x2032.rgb","rb");
        int reads = fread((void *)pSrcImg, stream_arg->op_cfg.image_size, 1, stream_arg->fp_in);
        if(reads != 1){
            fprintf(stderr, "fread failed. %d\n", reads);
            return -1;
        }
        fclose(stream_arg->fp_in);
        vappSafeCall(vastMemcpy(stream_arg->op_cfg.device_id, stream_arg->op_cfg.src_img, pSrcImg, stream_arg->op_cfg.image_size, vastMemcpyHostToDevice), status);  
        start = time_usec(); 
        vappSafeCall(vappiRGBPRemap_8u_P3R_Ctx(stream_arg->op_cfg.device_id,
                            stream_arg->op_cfg.src_img, map1, map2, stream_arg->op_cfg.oSrcSize, stream_arg->op_cfg.nSrcStep,  
                            stream_arg->op_cfg.dst_img, stream_arg->op_cfg.oDstSize, stream_arg->op_cfg.nDstStep, roi_num, rRemapROI, stream_arg->stream), status);                                 
        vappSafeCall(vastStreamSynchronize(stream_arg->op_cfg.device_id, stream_arg->stream), status);  
        // printf("%s %d %s here pthread_self()=%ld\n",__FILE__,__LINE__,__FUNCTION__,pthread_self());
        end = time_usec();
        elapsed =  end - start;
        printf("elapsed %ld us\n", elapsed/stream_arg->frame_count);                                                                                                              
        vappSafeCall(vastMemcpy(stream_arg->op_cfg.device_id, pDstImg, stream_arg->op_cfg.dst_img, stream_arg->op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
        char filename[100];
        sprintf(filename, "cycle_stream_2704x2106_thread_%ld_%d.rgb", pthread_self(), i);
        stream_arg->fp_out = fopen(filename,"wb");
        fwrite(pDstImg, stream_arg->op_cfg.out_image_size, 1,stream_arg->fp_out);
        fclose(stream_arg->fp_out);
    }
    vappSafeCall(vastStreamDestroy(stream_arg->op_cfg.device_id, stream_arg->stream), status); 
    vappSafeCall(vastFree(stream_arg->op_cfg.device_id, map1), status);  
    vappSafeCall(vastFree(stream_arg->op_cfg.device_id, map2), status);  
    free(map1h);
    free(map2h);
    free(pSrcImg);
    free(pDstImg);
    return 0;
}

int process_one_stream2(void *arg){
    printf("%s %d %s start thread_num=%d thread_id=%ld ,pthread_self()=%ld\n",__FILE__,__LINE__,__FUNCTION__,stream_arg->thread_num,stream_arg->thread_id,pthread_self());
    VappStatus status =  VAPP_SUCCESS;
    int64_t start, end, middle, elapsed;
    int roi_num = 3;
    int i = 0;
    char filename[100];
    struct thread_info *stream_arg = (struct thread_info *)arg;
    // VappiRect oC2PROI[3] = {
    //     {0,    0, stream_arg->op_cfg.oSrcSize.width, stream_arg->op_cfg.oSrcSize.height},
    //     {858,  0, 858,                                    stream_arg->op_cfg.oSrcSize.height},
    //     {1716, 0, stream_arg->op_cfg.oSrcSize.width-1716, stream_arg->op_cfg.oSrcSize.height}
    // };           
    // VappiRect rRemapROI[3] = {
    //     {0,    0, stream_arg->op_cfg.oDstSize.width, stream_arg->op_cfg.oDstSize.height},
    //     {901,  0, 901,                                    stream_arg->op_cfg.oDstSize.height},
    //     {1802, 0, stream_arg->op_cfg.oDstSize.width-1802, stream_arg->op_cfg.oDstSize.height}
    // };  
    VappiRect oC2PROI[3] = {
        {0,    0, 858,                                    stream_arg->op_cfg.oSrcSize.height},
        {858,  0, 858,                                    stream_arg->op_cfg.oSrcSize.height},
        {1716, 0, stream_arg->op_cfg.oSrcSize.width-1716, stream_arg->op_cfg.oSrcSize.height}
    };           
    VappiRect rRemapROI[3] = {
        {0,    0, 901,                                    stream_arg->op_cfg.oDstSize.height},
        {901,  0, 901,                                    stream_arg->op_cfg.oDstSize.height},
        {1802, 0, stream_arg->op_cfg.oDstSize.width-1802, stream_arg->op_cfg.oDstSize.height}
    };  
    Vapp8u* middle_img;      
    Vapp32f* map1, *map2;
    Vapp32f* map1h, *map2h;
    FILE * fp_map1 = NULL;
    FILE * fp_map2 = NULL;
    Vapp8u* pSrcImg = (Vapp8u*)malloc(stream_arg->op_cfg.image_size);
    Vapp8u* pMiddleImg = (Vapp8u*)malloc(stream_arg->op_cfg.image_size);
    Vapp8u* pDstImg = (Vapp8u*)malloc(stream_arg->op_cfg.out_image_size);
    map1h = (Vapp32f*)malloc(stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f));
    map2h = (Vapp32f*)malloc(stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f)); 
    if(stream_arg->op_cfg.oDstSize.width == 640){
        fp_map1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_480_w_640_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_480_w_640_in.bin", "rb");
    }else if(stream_arg->op_cfg.oDstSize.width == 1280){
        fp_map1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_720_w_1280_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_720_w_1280_in.bin", "rb");
    }else if(stream_arg->op_cfg.oDstSize.width == 2704){
        fp_map1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_2106_w_2704_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_2106_w_2704_in.bin", "rb");
    }else{
        fprintf(stderr, "unsupport dst width %d\n", stream_arg->op_cfg.oDstSize.width);
        return -1;
    }
    size_t reads = fread(map1h, stream_arg->op_cfg.out_plane_size* sizeof(Vapp32f), 1, fp_map1);
    if(reads != 1){
        fprintf(stderr, "fread failed. %zu\n", reads);
        return -1;
    }      
    reads = fread(map2h, stream_arg->op_cfg.out_plane_size* sizeof(Vapp32f), 1, fp_map2);
    if(reads != 1){
        fprintf(stderr, "fread failed. %zu\n", reads);
        return -1;
    }      
    fclose(fp_map1);
    fclose(fp_map2);

    vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&stream_arg->op_cfg.src_img, stream_arg->op_cfg.image_size), status);
    vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&middle_img, stream_arg->op_cfg.image_size), status);
    vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&stream_arg->op_cfg.dst_img, stream_arg->op_cfg.out_image_size), status);  
    vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&map1, stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f)), status);
    vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&map2, stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f)), status);         
    vappSafeCall(vastStreamCreate(stream_arg->op_cfg.device_id, roi_num, &stream_arg->stream, NULL), status);
    vappSafeCall(vastMemcpy(stream_arg->op_cfg.device_id, map1, map1h, stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 
    vappSafeCall(vastMemcpy(stream_arg->op_cfg.device_id, map2, map2h, stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 
    stream_arg->fp_in = fopen("/home/vastai/wxhu/qingying/data/OrgA_24_2576x2032.rgb","rb");
    reads = fread((void *)pSrcImg, stream_arg->op_cfg.image_size, 1, stream_arg->fp_in);
    if(reads != 1){
        fprintf(stderr, "fread failed. %zu\n", reads);
        return -1;
    }
    fclose(stream_arg->fp_in);
    for(i = 0; i < stream_arg->frame_count; i++){
        vappSafeCall(vastMemcpy(stream_arg->op_cfg.device_id, stream_arg->op_cfg.src_img, pSrcImg, stream_arg->op_cfg.image_size, vastMemcpyHostToDevice), status);  
        start = time_usec();
        vappSafeCall(vappiRGB2RGBP_8u_C3P3R_Ctx(stream_arg->op_cfg.device_id, 
                            stream_arg->op_cfg.src_img, stream_arg->op_cfg.oSrcSize, stream_arg->op_cfg.nSrcStep,  
                            middle_img, stream_arg->op_cfg.nSrcStep, roi_num, oC2PROI, stream_arg->stream), status);                                 
        vappSafeCall(vastStreamSynchronize(stream_arg->op_cfg.device_id, stream_arg->stream), status);  
        middle = time_usec();
        
        // vappSafeCall(vastMemcpy(stream_arg->op_cfg.device_id, pMiddleImg, middle_img, stream_arg->op_cfg.image_size, vastMemcpyDeviceToHost), status); 
        // memset(filename, 0, 100);
        // sprintf(filename, "cycle_stream2_middle_2576x2032_thread_%ld_%d.rgb", pthread_self(), i);
        // stream_arg->fp_out = fopen(filename,"wb");
        // fwrite(pMiddleImg, stream_arg->op_cfg.image_size, 1,stream_arg->fp_out);
        // fclose(stream_arg->fp_out);
        // printf("%s %d %s here pthread_self()=%ld middle %ld us\n",__FILE__,__LINE__,__FUNCTION__,pthread_self(), middle-start);
        
        vappSafeCall(vappiRGBPRemap_8u_P3R_Ctx(stream_arg->op_cfg.device_id,
                            middle_img, map1, map2, stream_arg->op_cfg.oSrcSize, stream_arg->op_cfg.nSrcStep,  
                            stream_arg->op_cfg.dst_img, stream_arg->op_cfg.oDstSize, stream_arg->op_cfg.nDstStep, roi_num, rRemapROI, stream_arg->stream), status);                                 
        vappSafeCall(vastStreamSynchronize(stream_arg->op_cfg.device_id, stream_arg->stream), status);  
        end = time_usec();
        elapsed =  end - start;
        printf("%s %d %s here pthread_self()=%ld elapsed %ld+%ld=%ld us\n",__FILE__,__LINE__,__FUNCTION__,pthread_self(), middle-start, end-middle, elapsed);
        vappSafeCall(vastMemcpy(stream_arg->op_cfg.device_id, pDstImg, stream_arg->op_cfg.dst_img, stream_arg->op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
        memset(filename, 0, 100);
        sprintf(filename, "cycle_stream2_end_2704x2106_thread_%ld_%d.rgb", pthread_self(), i);
        stream_arg->fp_out = fopen(filename,"wb");
        fwrite(pDstImg, stream_arg->op_cfg.out_image_size, 1,stream_arg->fp_out);
        fclose(stream_arg->fp_out);
    }
    vappSafeCall(vastStreamDestroy(stream_arg->op_cfg.device_id, stream_arg->stream), status); 
    vappSafeCall(vastFree(stream_arg->op_cfg.device_id, map1), status);  
    vappSafeCall(vastFree(stream_arg->op_cfg.device_id, map2), status);  
    vappSafeCall(vastFree(stream_arg->op_cfg.device_id, middle_img), status);  
    free(map1h);
    free(map2h);
    free(pSrcImg);
    free(pDstImg);
    free(pMiddleImg);
    return 0;
}

int process_one_stream3(void *arg){
    printf("%s %d %s start thread_num=%d thread_id=%ld ,pthread_self()=%ld\n",__FILE__,__LINE__,__FUNCTION__,stream_arg->thread_num,stream_arg->thread_id,pthread_self());
    VappStatus status =  VAPP_SUCCESS;
    int64_t start, end, middle, elapsed;
    int roi_num = 3;
    int i = 0;
    //char filename[100];
    struct thread_info *stream_arg = (struct thread_info *)arg;
    // VappiRect oC2PROI[3] = {
    //     {0,    0, stream_arg->op_cfg.oSrcSize.width, stream_arg->op_cfg.oSrcSize.height},
    //     {858,  0, 858,                                    stream_arg->op_cfg.oSrcSize.height},
    //     {1716, 0, stream_arg->op_cfg.oSrcSize.width-1716, stream_arg->op_cfg.oSrcSize.height}
    // };           
    // VappiRect rRemapROI[3] = {
    //     {0,    0, stream_arg->op_cfg.oDstSize.width, stream_arg->op_cfg.oDstSize.height},
    //     {901,  0, 901,                                    stream_arg->op_cfg.oDstSize.height},
    //     {1802, 0, stream_arg->op_cfg.oDstSize.width-1802, stream_arg->op_cfg.oDstSize.height}
    // };  
    VappiRect oC2PROI[3] = {
        {0,    0, 858,                                    stream_arg->op_cfg.oSrcSize.height},
        {858,  0, 858,                                    stream_arg->op_cfg.oSrcSize.height},
        {1716, 0, stream_arg->op_cfg.oSrcSize.width-1716, stream_arg->op_cfg.oSrcSize.height}
    };           
    VappiRect rRemapROI[3] = {
        {0,    0, 901,                                    stream_arg->op_cfg.oDstSize.height},
        {901,  0, 901,                                    stream_arg->op_cfg.oDstSize.height},
        {1802, 0, stream_arg->op_cfg.oDstSize.width-1802, stream_arg->op_cfg.oDstSize.height}
    };  
    Vapp8u* middle_img;      
    Vapp32f* map1, *map2;
    Vapp32f* map1h, *map2h;
    FILE * fp_map1 = NULL;
    FILE * fp_map2 = NULL;
    Vapp8u* pSrcImg = (Vapp8u*)malloc(stream_arg->op_cfg.image_size);
    Vapp8u* pMiddleImg = (Vapp8u*)malloc(stream_arg->op_cfg.image_size);
    Vapp8u* pDstImg = (Vapp8u*)malloc(stream_arg->op_cfg.out_image_size);
    map1h = (Vapp32f*)malloc(stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f));
    map2h = (Vapp32f*)malloc(stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f)); 
    if(stream_arg->op_cfg.oDstSize.width == 640){
        fp_map1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_480_w_640_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_480_w_640_in.bin", "rb");
    }else if(stream_arg->op_cfg.oDstSize.width == 1280){
        fp_map1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_720_w_1280_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_720_w_1280_in.bin", "rb");
    }else if(stream_arg->op_cfg.oDstSize.width == 2704){
        fp_map1 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map1_h_2106_w_2704_in.bin", "rb");
        fp_map2 = fopen("/home/vastai/wxhu/qingying/remap/data/remap_map2_h_2106_w_2704_in.bin", "rb");
    }else{
        fprintf(stderr, "unsupport dst width %d\n", stream_arg->op_cfg.oDstSize.width);
        return -1;
    }
    size_t reads = fread(map1h, stream_arg->op_cfg.out_plane_size* sizeof(Vapp32f), 1, fp_map1);
    if(reads != 1){
        fprintf(stderr, "fread failed. %zu\n", reads);
        return -1;
    }      
    reads = fread(map2h, stream_arg->op_cfg.out_plane_size* sizeof(Vapp32f), 1, fp_map2);
    if(reads != 1){
        fprintf(stderr, "fread failed. %zu\n", reads);
        return -1;
    }      
    
    fclose(fp_map1);
    fclose(fp_map2);

    // vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&stream_arg->op_cfg.src_img, stream_arg->op_cfg.image_size), status);
    // vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&middle_img, stream_arg->op_cfg.image_size), status);
    // vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&stream_arg->op_cfg.dst_img, stream_arg->op_cfg.out_image_size), status);  
    
    vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&map1, stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f)), status);
    vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&map2, stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f)), status);         
    vappSafeCall(vastStreamCreate(stream_arg->op_cfg.device_id, roi_num, &stream_arg->stream, NULL), status);
    vappSafeCall(vastMemcpy(stream_arg->op_cfg.device_id, map1, map1h, stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 
    vappSafeCall(vastMemcpy(stream_arg->op_cfg.device_id, map2, map2h, stream_arg->op_cfg.out_plane_size * sizeof(Vapp32f), vastMemcpyHostToDevice), status); 
    stream_arg->fp_in = fopen("/home/vastai/wxhu/qingying/data/OrgA_24_2576x2032.rgb","rb");
    reads = fread((void *)pSrcImg, stream_arg->op_cfg.image_size, 1, stream_arg->fp_in);
    if(reads != 1){
        fprintf(stderr, "fread failed. %zu\n", reads);
        return -1;
    }
    fclose(stream_arg->fp_in);
    for(i = 0; i < stream_arg->frame_count; i++){
        vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&stream_arg->op_cfg.src_img, stream_arg->op_cfg.image_size), status);
        vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&middle_img, stream_arg->op_cfg.image_size), status);
        vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&stream_arg->op_cfg.dst_img, stream_arg->op_cfg.out_image_size), status);  
        vappSafeCall(vastMemcpy(stream_arg->op_cfg.device_id, stream_arg->op_cfg.src_img, pSrcImg, stream_arg->op_cfg.image_size, vastMemcpyHostToDevice), status);  
        start = time_usec();
        vappSafeCall(vappiRGB2RGBP_8u_C3P3R_Ctx(stream_arg->op_cfg.device_id, 
                            stream_arg->op_cfg.src_img, stream_arg->op_cfg.oSrcSize, stream_arg->op_cfg.nSrcStep,  
                            middle_img, stream_arg->op_cfg.nSrcStep, roi_num, oC2PROI, stream_arg->stream), status);                                 
        vappSafeCall(vastStreamSynchronize(stream_arg->op_cfg.device_id, stream_arg->stream), status);  
        middle = time_usec();
        
        // vappSafeCall(vastMemcpy(stream_arg->op_cfg.device_id, pMiddleImg, middle_img, stream_arg->op_cfg.image_size, vastMemcpyDeviceToHost), status); 
        // memset(filename, 0, 100);
        // sprintf(filename, "cycle_stream2_middle_2576x2032_thread_%ld_%d.rgb", pthread_self(), i);
        // stream_arg->fp_out = fopen(filename,"wb");
        // fwrite(pMiddleImg, stream_arg->op_cfg.image_size, 1,stream_arg->fp_out);
        // fclose(stream_arg->fp_out);
        // printf("%s %d %s here pthread_self()=%ld middle %ld us\n",__FILE__,__LINE__,__FUNCTION__,pthread_self(), middle-start);
        
        // vappSafeCall(vastMalloc(stream_arg->op_cfg.device_id, (void**)&stream_arg->op_cfg.dst_img, stream_arg->op_cfg.out_image_size), status);  
        
        vappSafeCall(vappiRGBPRemap_8u_P3R_Ctx(stream_arg->op_cfg.device_id,
                            middle_img, map1, map2, stream_arg->op_cfg.oSrcSize, stream_arg->op_cfg.nSrcStep,  
                            stream_arg->op_cfg.dst_img, stream_arg->op_cfg.oDstSize, stream_arg->op_cfg.nDstStep, roi_num, rRemapROI, stream_arg->stream), status);                                 
        vappSafeCall(vastStreamSynchronize(stream_arg->op_cfg.device_id, stream_arg->stream), status);  
        end = time_usec();
        elapsed =  end - start;
        printf("%s %d %s here pthread_self()=%ld elapsed %ld+%ld=%ld us\n",__FILE__,__LINE__,__FUNCTION__,pthread_self(), middle-start, end-middle, elapsed);
        vappSafeCall(vastMemcpy(stream_arg->op_cfg.device_id, pDstImg, stream_arg->op_cfg.dst_img, stream_arg->op_cfg.out_image_size, vastMemcpyDeviceToHost), status); 
        // memset(filename, 0, 100);
        // sprintf(filename, "cycle_stream2_end_2704x2106_thread_%ld_%d.rgb", pthread_self(), i);
        // stream_arg->fp_out = fopen(filename,"wb");
        // fwrite(pDstImg, stream_arg->op_cfg.out_image_size, 1,stream_arg->fp_out);
        // fclose(stream_arg->fp_out);
        
        vappSafeCall(vastFree(stream_arg->op_cfg.device_id, stream_arg->op_cfg.src_img), status);  
        vappSafeCall(vastFree(stream_arg->op_cfg.device_id, stream_arg->op_cfg.dst_img), status);  
        vappSafeCall(vastFree(stream_arg->op_cfg.device_id, middle_img), status);  
    }
    vappSafeCall(vastStreamDestroy(stream_arg->op_cfg.device_id, stream_arg->stream), status); 
    vappSafeCall(vastFree(stream_arg->op_cfg.device_id, map1), status);  
    vappSafeCall(vastFree(stream_arg->op_cfg.device_id, map2), status);  
    free(map1h);
    free(map2h);
    free(pSrcImg);
    free(pDstImg);
    free(pMiddleImg);
    // vappSafeCall(vastFree(stream_arg->op_cfg.device_id, stream_arg->op_cfg.src_img), status);  
    // vappSafeCall(vastFree(stream_arg->op_cfg.device_id, stream_arg->op_cfg.dst_img), status);  
    // vappSafeCall(vastFree(stream_arg->op_cfg.device_id, middle_img), status);  
    printf("%s %d %s pthread_self()=%ld exit total %d frames\n",__FILE__,__LINE__,__FUNCTION__,pthread_self(),stream_arg->frame_count);
    return 0;
}
#else
int test_multi_stream2()
{
    return 0;
}
int test_multi_stream()
{
    return 0;
}
#endif
