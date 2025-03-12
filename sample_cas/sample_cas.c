#include <stdio.h>
#include <sys/time.h>
#include <inttypes.h>

#include "vappi.h"

#define vappSafeCall(expr, status)  {if(status == 0){ VappStatus code = expr ; if(code < 0) { fprintf (stderr, "Error code : %d (%d)\n", code, __LINE__); status = code;} } else{return -1;}} 

int64_t time_usec(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

int main(void) 
{
    int width = 1080;//图像宽度
    int height = 1920;//图像高度
    float strength = 0.9;//锐化强度，和ffmepg原生cas filter一致，范围[0,1]
    int planes = 5;//平面数量，和ffmepg原生cas filter一致，范围[0,15]
    int device_id = 0;//设备id，从0起，一张卡有2个
    int device_id_cross = 0;//设备id，从0起，一张卡有2个
    vastStream_t stream;//流上下文，当ctx使用
    Vapp8u* src_img = NULL;//输入地址，卡上空间，如果是硬件解码，相当于frame->data[3]，直接使用即可，不需要额外申请。
    Vapp8u* src_img_cross = NULL;//输入地址，卡上空间，如果是硬件解码，相当于frame->data[3]，直接使用即可，不需要额外申请。
    Vapp8u* dst_img = NULL;//输出地址，卡上空间，如果是硬件解码，相当于frame->data[3]，直接使用即可，不需要额外申请。
    Vapp8u *pSrcImg = NULL;//sample测试使用的保存输入数据的host空间，malloc申请
    Vapp8u *pDstImg = NULL;//sample测试使用的保存输出数据的host空间，malloc申请
    int frame_count = 1;
    int64_t start = 0;
    int64_t end = 0;
    int64_t sum = 0;
    int64_t elapsed = 0;
    size_t reads = 0;
    VappStatus status = VAPP_SUCCESS;

    int width_pitch = width;//宽度pitch，sample为host输入，所以和width一致，如果是硬件解码则需要按实际对齐情况填写
    int height_pitch = height;//高度pitch，sample为host输入，所以和height一致，如果是硬件解码则需要按实际对齐情况填写
    int image_size = width_pitch * height_pitch * 3/2;//nv12和yuv420p都是*3/2，也就是1.5倍的width_pitch*height_pitch，rgb plannar则是3倍
    VappiShape2D img_shape = {//宽、高、宽pitch、高pitch
        width,
        height,
        width_pitch,
        height_pitch            
    };

    FILE *fp_input = fopen("/home/vastai/wxhu/media/nv12_1080x1920.yuv", "rb");
    if(!fp_input){
        fprintf(stderr, "open in failed.\n");
        return -1;
    }    
    FILE *fp_out = fopen("out_nv12_1080x1920_09_5.yuv", "wb");
    if(!fp_out){
        fprintf(stderr, "open out failed.\n");
        return -1;
    }    

    pSrcImg = (Vapp8u*)malloc(image_size);
    pDstImg = (Vapp8u*)malloc(image_size);

    vappSafeCall(vastStreamCreate(device_id, 1, &stream, NULL), status);//创建stream，后面每帧的处理逻辑都需要使用
    if(device_id == 0)
        device_id_cross = 1;
    else
        device_id_cross = 0;
    vappSafeCall(vastMalloc(device_id_cross, (void**)&src_img_cross, image_size), status);//申请卡上空间，跨die拷贝使用
    vappSafeCall(vastMalloc(device_id, (void**)&src_img, image_size), status);//申请卡上空间，如果使用硬件解码后的地址则不需要这个步骤
    vappSafeCall(vastMalloc(device_id, (void**)&dst_img, image_size), status); //申请卡上空间，如果使用硬件解码后的地址则不需要这个步骤 

    for(int i = 0; i < frame_count; i++){
        reads = fread((void *)pSrcImg, image_size, 1, fp_input);
        if(reads != 1){
            fprintf(stderr, "fread failed. %zu\n", reads);
            return -1;
        }
        // for(int i = 0; i < 1920*1080; i++){
        //     pSrcImg[i] = 0;
        // }
        rewind(fp_input);   
        vappSafeCall(vastMemcpy(device_id_cross, src_img_cross, pSrcImg, image_size, vastMemcpyHostToDevice), status);//将host地址内容拷贝到卡上地址，如果使用硬件解码后的地址则不需要这个步骤
        vappSafeCall(vastMemcpyCross(device_id, src_img, device_id_cross, src_img_cross, image_size), status);//跨die拷贝
        // vappSafeCall(vastMemcpyCrossCPU(device_id, src_img, device_id_cross, src_img_cross, image_size), status);//跨die拷贝
        start = time_usec(); 
        // vappSafeCall(vappiRGBPLANARCas_8u_P3_Ctx(device_id, 
        //                     src_img, img_shape, dst_img, img_shape, strength, planes, stream), status);//rgb planar输入格式的cas运算接口
        vappSafeCall(vappiNV12Cas_8u_P3_Ctx(device_id, 
                            src_img, img_shape, dst_img, img_shape, strength, planes, stream), status);//nv12输入格式的cas运算接口
        // vappSafeCall(vappiYUV420Cas_8u_P3_Ctx(device_id, 
                            // src_img, img_shape, dst_img, img_shape, strength, planes, stream), status);//yuv420p输入格式的cas运算接口 
        vappSafeCall(vastStreamSynchronize(device_id, stream), status);//同步结果
        end = time_usec();
        elapsed =  end - start;
        printf("elapsed %" PRId64 " us\n", elapsed);                                                                   
        vappSafeCall(vastMemcpy(device_id, pDstImg, dst_img, image_size, vastMemcpyDeviceToHost), status);//将cas运算后的结果从卡上拷贝到host上方便验证，如果结果送给硬件编码则不需要
        fwrite(pDstImg, image_size, 1,fp_out);
        sum += elapsed;                        
    }
    printf("avg %" PRId64 " us\n", sum/frame_count);
    vappSafeCall(vastStreamDestroy(device_id, stream), status);//处理完最后一帧后销毁stream
    if(src_img)
        vappSafeCall(vastFree(device_id, src_img), status);//销毁申请的卡上空间
    if(dst_img)
        vappSafeCall(vastFree(device_id, dst_img), status);//销毁申请的卡上空间

    fclose(fp_input);
    fclose(fp_out);
    return 0;
}
