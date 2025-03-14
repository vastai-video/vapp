#include <vapp.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

//#include "readerwritercircularbuffer.h"
//using namespace moodycamel;
class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });

                        if (stop && tasks.empty()) {
                            return;
                        }

                        task = std::move(tasks.front());
                        tasks.pop();
                    }

                    task();
                }
                });
        }
    }

    template <class F, class... Args>
    auto enqueue(F&& f, Args &&...args) -> std::future<decltype(f(args...))> {
        using return_type = decltype(f(args...));

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex);

            if (stop) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            tasks.emplace([task]() { (*task)(); });
        }

        condition.notify_one();
        return result;
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }

        condition.notify_all();

        for (std::thread& worker : workers) {
            worker.join();
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};

static std::vector<VappiRect> get_roi(int x, int y, int w, int h, int roi_num,
    bool divide_w = true) {
    std::vector<VappiRect> rois;
    if (divide_w) {  // divide width
        int roi_w = w / roi_num;
        int remainder = w % roi_num;
        int x_offset = 0;
        for (int i = 0; i < roi_num; ++i) {
            VappiRect roi;
            roi.y = y;
            roi.height = h;
            roi.width = i < remainder ? roi_w + 1 : roi_w;
            roi.x = x + x_offset;
            x_offset += roi.width;
            rois.push_back(roi);
            // std::cout << roi.x << ", " << roi.y << ", " << roi.width << ", "
            //           << roi.height << std::endl;
        }
    }
    else {  // divide height
        int roi_h = h / roi_num;
        int remainder = h % roi_num;
        int y_offset = 0;
        for (int i = 0; i < roi_num; ++i) {
            VappiRect roi;
            roi.x = x;
            roi.width = w;
            roi.height = i < remainder ? roi_h + 1 : roi_h;
            roi.y = y + y_offset;
            y_offset += roi.height;
            rois.push_back(roi);
            // std::cout << roi.x << ", " << roi.y << ", " << roi.width << ", "
            //           << roi.height << std::endl;
        }
    }
    return rois;
}
// static int frame_num = 0;
// int test_remap_threshold_one_stream(vastStream_t stream, int repeat_time) {
//     int device_id = 0;

//     int width = 4096;
//     int height = 2160;
//     int map_w = 3900;
//     int map_h = 2095;
//     int map_size = map_w * map_h;
//     int in_image_size = width * height;
//     int out_image_size = map_size;

//     double nThreshold = 1.0;
//     int nBlockSize = 19;
//     int nMaxValue = 255;

//     VappiSize src_size, dst_remap_size;
//     src_size.width = width;
//     src_size.height = height;
//     dst_remap_size.width = map_w;
//     dst_remap_size.height = map_h;
//     int roi_num = 3;
//     auto remap_rois =
//         get_roi(0, 0, dst_remap_size.width, dst_remap_size.height, roi_num);

//     auto thres_rois =
//         get_roi(0, 0, dst_remap_size.width, dst_remap_size.height, roi_num);

//     // VappiRect remap_rois[3] = {
//     //     {0, 0, 901, dst_remap_size.height},
//     //     {901, 0, 901, dst_remap_size.height},
//     //     {1802, 0, dst_remap_size.width - 1802, dst_remap_size.height}};

//     // VappiRect thres_rois[4] = {
//     //     {0, 0, dst_remap_size.width / 3, dst_remap_size.height},
//     //     {dst_remap_size.width / 3, 0, dst_remap_size.width / 3,
//     //      dst_remap_size.height},
//     //     {dst_remap_size.width / 3 * 2, 0,
//     //      dst_remap_size.width - dst_remap_size.width / 3 * 2,
//     //      dst_remap_size.height},
//     // };

//     std::vector<std::string> filenames = {
//         "C:\\Users\\Acemake\\Desktop\\vsx\\Release\\data\\800w\\Pic\\OrgBin\\OrgA.bin",
//         "C:\\Users\\Acemake\\Desktop\\vsx\\Release\\data\\800w\\Pic\\OrgBin\\OrgB.bin" };

//     FILE* fp_map1 = fopen(
//         "C:\\Users\\Acemake\\Desktop\\vsx\\Release\\data\\800w\\Pic\\OrgBin\\OrgA_fixed_mapRx_3900_w_2095_h.bin",
//         "rb");
//     FILE* fp_map2 = fopen(
//         "C:\\Users\\Acemake\\Desktop\\vsx\\Release\\data\\800w\\Pic\\OrgBin\\OrgA_fixed_mapRy_3900_w_2095_h.bin",
//         "rb");


//     Vapp16s *map1 = NULL;
//     Vapp16u *map2 = NULL;
//     Vapp16s *map1h = NULL;
//     Vapp16u *map2h = NULL;
//     int ma1_channel = 2;
//     map1h = reinterpret_cast<Vapp16s*>(malloc(map_size * sizeof(Vapp16s) * ma1_channel));
//     map2h = reinterpret_cast<Vapp16u*>(malloc(map_size * sizeof(Vapp16u)));
//     fread(map1h, map_size * sizeof(Vapp16s), ma1_channel, fp_map1);
//     fread(map2h, map_size * sizeof(Vapp16u), 1, fp_map2);
//     fclose(fp_map1);
//     fclose(fp_map2);
//     vastMalloc(device_id, (void**)&map1, map_size * sizeof(Vapp16s) *ma1_channel);
//     vastMalloc(device_id, (void**)&map2, map_size * sizeof(Vapp16u));
//     vastMemcpy(device_id, map1, map1h, map_size * sizeof(Vapp16s) * ma1_channel,
//         vastMemcpyHostToDevice);
//     vastMemcpy(device_id, map2, map2h, map_size * sizeof(Vapp16u),
//         vastMemcpyHostToDevice);

//     Vapp8u* pSrcImg;
//     Vapp8u* src_img;
//     Vapp8u* pRemap_DstImg;
//     Vapp8u* remap_dst_img;
//     Vapp8u* pThresImg;
//     Vapp8u* thres_img;
//     pSrcImg = reinterpret_cast<Vapp8u*>(malloc(in_image_size));
//     pRemap_DstImg = reinterpret_cast<Vapp8u*>(malloc(out_image_size));
//     pThresImg = reinterpret_cast<Vapp8u*>(malloc(out_image_size));

//     vastMalloc(device_id, (void**)&src_img, in_image_size);
//     vastMalloc(device_id, (void**)&remap_dst_img, out_image_size);
//     vastMalloc(device_id, (void**)&thres_img, out_image_size);

//     // std::cout << "Current thread id: " << std::this_thread::get_id()
//     //     << ", stream id = " << stream << std::endl;
//     for (int i = 0; i < repeat_time; ++i) {
//         std::cout << "index = " << i << std::endl;
//         auto input_name = filenames[i % 2];
//         std::cout << "filename: " << input_name << std::endl;
//         printf("frame_num %d\n", frame_num++);
//         FILE* fp_input = fopen(input_name.c_str(), "rb");

//         int reads = fread((void*)pSrcImg, in_image_size, 1, fp_input);
//         if (reads != 1) {
//             fprintf(stderr, "fread failed. %d\n", reads);
//             return -1;
//         }
//         fclose(fp_input);
//         vastMemcpy(device_id, src_img, pSrcImg, in_image_size,
//             vastMemcpyHostToDevice);

//         // remap
//         const auto start_time = std::chrono::high_resolution_clock::now();
//         vappiGrayRemapFixedMap_8u_P1R_Ctx(device_id, src_img, map1, map2, src_size,
//             src_size.width, remap_dst_img, dst_remap_size,
//             dst_remap_size.width, roi_num, remap_rois.data(),
//             stream);

//         vastStreamSynchronize(device_id, stream);
//         const auto end_time = std::chrono::high_resolution_clock::now();
//         auto time_delay =
//             std::chrono::duration_cast<std::chrono::microseconds>(
//                 std::chrono::high_resolution_clock::now() - start_time)
//             .count();
//         std::cout << "remap time_delay: " << time_delay << "us" << std::endl;

//          // threshold
//          vappiAdaptiveThreshold_8u_P1R_Ctx(
//              device_id, remap_dst_img, dst_remap_size, dst_remap_size.width,
//              thres_img, dst_remap_size.width, roi_num, thres_rois.data(), nThreshold,
//              nBlockSize, nMaxValue, stream);
//          vastStreamSynchronize(device_id, stream);
//          vastMemcpy(device_id, pThresImg, thres_img, out_image_size,
//                     vastMemcpyDeviceToHost);

//          //auto filename = std::to_string(i) + "test_remap_thres_2106x2704.rgb";
//         // FILE *fp_out = fopen(filename.c_str(), "wb");
//          //fwrite(pThresImg, out_image_size, 1, fp_out);

//         // fclose(fp_out);
//     }
//     vastFree(device_id, src_img);
//     vastFree(device_id, remap_dst_img);
//     vastFree(device_id, thres_img);

//     free(pSrcImg);
//     free(pRemap_DstImg);
//     free(pThresImg);

//     vastFree(device_id, map1);
//     vastFree(device_id, map2);
//     free(map1h);
//     free(map2h);
//     return 0;
// }

#define vappSafeCall(expr, status)  {if(status == 0){ VappStatus code = expr ; if(code < 0) { fprintf (stderr, "Error code : %d (%d)\n", code, __LINE__); status = code;} } else{return -1;}} 

int test_remap_threshold_one_stream(vastStream_t stream, int repeat_time, int stream_id) {
  int device_id = 0;

  int src_width = 4096;
  int src_height = 3072;
  int map_w = 3965;
  int map_h = 2951;
  int warp_w = 3983;
  int warp_h = 2985;
  VappStatus status = VAPP_NO_ERROR;

  double tx = 16;
  double ty = -57;

  int resize_w = 4096;
  int resize_h = 3072;

  double nThreshold = 1.0;
  int nBlockSize = 19;
  int nMaxValue = 255;
  double m_ptr[9] = {
      1.0184072416031675, -0.0022846972155636, 21.7504433537153440,
       0.0055802038999428, 1.0060900169473954, -57.6893785798429160,
       0.0000048206418357, -0.0000018932944637, 1.0002111193328480};

  VappiSize src_size, remap_size, warp_size, translate_size, resize_size, thres_size;
  src_size.width = src_width;
  src_size.height = src_height;
  remap_size.width = map_w;
  remap_size.height = map_h;
  warp_size.width = warp_w;
  warp_size.height = warp_h;
  translate_size.width = warp_w;
  translate_size.height = warp_h;
  resize_size.width = resize_w;
  resize_size.height = resize_h;
  thres_size.width = resize_w;
  thres_size.height = resize_h;

  int map_size = map_w * map_h;
  int in_image_size = src_width * src_height;
  int remap_image_size = map_size;
  int warp_image_size = warp_w*warp_h;
  int translate_image_size = warp_image_size;
  int resize_image_size = resize_w * resize_h;
  int thres_image_size = resize_image_size;

  int roi_num = 3;
  auto remap_rois =
      get_roi(0, 0, remap_size.width, remap_size.height, roi_num);
  auto warp_rois =
      get_roi(0, 0, warp_size.width, warp_size.height, roi_num);
  auto translate_rois =
      get_roi(0, 0, translate_size.width, translate_size.height, roi_num);
  auto resize_rois =
      get_roi(0, 0, resize_size.width, resize_size.height, roi_num);
  auto thres_rois =
      get_roi(0, 0, thres_size.width, thres_size.height, roi_num);


    // std::vector<std::string> filenames = {
    //     "D:\\vapp_release\\input\\GrayA.bin",
    //     "D:\\vapp_release\\input\\GrayB.bin" };

    // FILE* fp_map1 = fopen(
    //     "D:\\vapp_release\\input\\OrgA_fixed_mapRx_3965_w_2951_h.bin",
    //     "rb");
    // FILE* fp_map2 = fopen(
    //     "D:\\vapp_release\\input\\OrgA_fixed_mapRy_3965_w_2951_h.bin",
    //     "rb");
    std::vector<std::string> filenames = {
        "/home/vastai/simonz/input/GrayA.bin",
        "/home/vastai/simonz/input/GrayB.bin" };

    FILE* fp_map1 = fopen(
        "/home/vastai/simonz/input/OrgA_fixed_mapRx_3965_w_2951_h.bin",
        "rb");
    FILE* fp_map2 = fopen(
        "/home/vastai/simonz/input/OrgA_fixed_mapRy_3965_w_2951_h.bin",
        "rb");    

    Vapp16s* map1;
    Vapp16u* map2;
    Vapp16s* map1h;
    Vapp16u* map2h;

    map1h = reinterpret_cast<Vapp16s*>(malloc(2 * map_size * sizeof(Vapp16s)));
    map2h = reinterpret_cast<Vapp16u*>(malloc(map_size * sizeof(Vapp16u)));
    fread(map1h, 2 * map_size * sizeof(Vapp16s), 1, fp_map1);
    fread(map2h, map_size * sizeof(Vapp16u), 1, fp_map2);
    fclose(fp_map1);
    fclose(fp_map2);
    vappSafeCall(vastMalloc(device_id, (void**)&map1, 2 * map_size * sizeof(Vapp16s) ), status);
    vappSafeCall(vastMalloc(device_id, (void**)&map2, map_size * sizeof(Vapp16u)), status);
    vappSafeCall(vastMemcpy(device_id, map1, map1h, 2 * map_size * sizeof(Vapp16s), vastMemcpyHostToDevice), status);
    vappSafeCall(vastMemcpy(device_id, map2, map2h, map_size * sizeof(Vapp16u), vastMemcpyHostToDevice), status);

    for (int i = 0; i < repeat_time; ++i) {
        auto input_name = filenames[i % 2];
        std::cout << "filename: " << input_name << std::endl;

        Vapp8u* pSrcImg, * pTranslateImg, * pResizeImg, * pThresImg;
        Vapp8u* src_img, * remap_img, * warp_img, * translate_img, * resize_img, * thres_img;

        pSrcImg = reinterpret_cast<Vapp8u*>(malloc(in_image_size));
        pTranslateImg = reinterpret_cast<Vapp8u*>(malloc(translate_image_size));
        pResizeImg = reinterpret_cast<Vapp8u*>(malloc(resize_image_size));
        pThresImg = reinterpret_cast<Vapp8u*>(malloc(thres_image_size));

        vappSafeCall(vastMalloc(device_id, (void**)&src_img, in_image_size ), status);
        vappSafeCall(vastMalloc(device_id, (void**)&remap_img, remap_image_size), status);
        vappSafeCall(vastMalloc(device_id, (void**)&warp_img, warp_image_size), status);
        vappSafeCall(vastMalloc(device_id, (void**)&translate_img, translate_image_size), status);
        vappSafeCall(vastMalloc(device_id, (void**)&resize_img, resize_image_size), status);
        vappSafeCall(vastMalloc(device_id, (void**)&thres_img, thres_image_size), status);

        FILE* fp_input = fopen(input_name.c_str(), "rb");

        // size_t reads = fread((void*)pSrcImg, in_image_size, 1, fp_input);
        // if (reads != 1) {
        //   fprintf(stderr, "fread failed. %d\n", reads);
        //   vastFree(device_id, src_img);
        //   vastFree(device_id, remap_img);
        //   vastFree(device_id, warp_img);
        //   vastFree(device_id, translate_img);
        //   vastFree(device_id, resize_img);
        //   vastFree(device_id, thres_img);

        //   free(pSrcImg);
        //   free(pTranslateImg);
        //   free(pResizeImg);
        //   free(pThresImg);
        //   return 0;
        // }
        fclose(fp_input);

        vappSafeCall(vastMemcpy(device_id, src_img, pSrcImg, in_image_size,
            vastMemcpyHostToDevice), status);
        // remap
        vappSafeCall(vappiGrayRemapFixedMap_8u_P1R_Ctx(device_id, src_img, map1, map2, src_size,
            src_size.width, remap_img, remap_size,
            remap_size.width, roi_num, remap_rois.data(),
            stream), status);

        vappSafeCall(vastStreamSynchronize(device_id, stream), status);
        printf("thread = %d, vappiGrayRemapFixedMap_8u_P1R_Ctx\n", stream_id);
           // warp
        vappSafeCall(vappiWrapPerspective_8u_P1R_Ctx(device_id, remap_img, m_ptr, remap_size,
            remap_size.width, warp_img, warp_size,
            warp_size.width, roi_num, warp_rois.data(), stream), status);
        vappSafeCall(vastStreamSynchronize(device_id, stream), status);
        printf("thread = %d, vappiWrapPerspective_8u_P3R_Ctx\n", stream_id);
            // translate_transform
        vappSafeCall(vappiTranslateTransform_8u_P1R_Ctx(
            device_id, warp_img, warp_size, warp_size.width, translate_img,
            translate_size.width, roi_num, translate_rois.data(), tx, ty, stream), status);

        vappSafeCall(vastStreamSynchronize(device_id, stream), status);
        printf("thread = %d, vappiTranslateTransform_8u_P1R_Ctx\n", stream_id);
            // vastMemcpy(device_id, pTranslateImg, translate_img, translate_image_size,
            //            vastMemcpyDeviceToHost);

            // auto trans_filename = std::to_string(i) + "translate_2720x2125.bin";
            // FILE *fp_trans_out = fopen(trans_filename.c_str(), "wb");
            // fwrite(pTranslateImg, translate_image_size, 1, fp_trans_out);
            // fclose(fp_trans_out);

            // resize
            vappiResize_8u_P1_Ctx(device_id, translate_img, translate_size,
                                  translate_size.width, resize_img, resize_size,
                                  resize_size.width, VAPPI_RESIZE_BILINEAR_PILLOW,
                                  stream);
            vastStreamSynchronize(device_id, stream);
        printf("thread = %d, vappiResize_8u_P1_Ctx\n", stream_id);
            // vastMemcpy(device_id, pResizeImg, resize_img, resize_image_size,
            //            vastMemcpyDeviceToHost);

            // auto resize_filename = std::to_string(i) + "resize_2560x2048.bin";
            // FILE *fp_resize_out = fopen(resize_filename.c_str(), "wb");
            // fwrite(pResizeImg, resize_image_size, 1, fp_resize_out);
            // fclose(fp_resize_out);

            // threshold
        vappSafeCall(vappiAdaptiveThreshold_8u_P1R_Ctx(
            device_id, resize_img, resize_size, resize_size.width, thres_img,
            thres_size.width, roi_num, thres_rois.data(), nThreshold,
            nBlockSize, nMaxValue, stream), status);
        vappSafeCall(vastStreamSynchronize(device_id, stream), status);
        printf("thread = %d, vappiAdaptiveThreshold_8u_P1R_Ctx\n", stream_id);
            // vastMemcpy(device_id, pThresImg, thres_img, thres_image_size,
            //            vastMemcpyDeviceToHost);

            // auto filename = std::to_string(i) + "thres_2560x2048.bin";
            // FILE *fp_out = fopen(filename.c_str(), "wb");
            // fwrite(pThresImg, thres_image_size, 1, fp_out);
            // fclose(fp_out);

        vappSafeCall(vastFree(device_id, src_img), status);
        vappSafeCall(vastFree(device_id, remap_img), status);
        vappSafeCall(vastFree(device_id, warp_img), status);
        vappSafeCall(vastFree(device_id, translate_img), status);
        vappSafeCall(vastFree(device_id, resize_img), status);
        vappSafeCall(vastFree(device_id, thres_img), status);

        free(pSrcImg);
        free(pTranslateImg);
        free(pResizeImg);
        free(pThresImg);
    }

    vappSafeCall(vastFree(0, map1), status);
    vastFree(0, map2);
    free(map1h);
    free(map2h);

    return 0;
}
int corenum = 3;
extern "C" {
    void test_remap_threshold_multistream2(int stream_num);
}

void test_remap_threshold_multistream2(int stream_num) {
    int device_id = 0;
    //void* data;

    //Vapp16s* map1h;
    //Vapp16u* map2h;
    //int map_w = 3965;
    //int map_h = 2951;
    //int map_size = map_w * map_h;
    //FILE* fp_map1 = fopen(
    //    "C:\\Users\\Acemake\\Desktop\\vsx\\Release\\data\\1200w\\Pic\\OrgBin\\OrgA_fixed_mapRx_3965_w_2951_h.bin",
    //    "rb");
    //FILE* fp_map2 = fopen(
    //    "C:\\Users\\Acemake\\Desktop\\vsx\\Release\\data\\1200w\\Pic\\OrgBin\\OrgA_fixed_mapRy_3965_w_2951_h.bin",
    //    "rb");
    //map1h = reinterpret_cast<Vapp16s*>(malloc(2 * map_size * sizeof(Vapp16s)));
    //map2h = reinterpret_cast<Vapp16u*>(malloc(map_size * sizeof(Vapp16u)));
    //fread(map1h, 2 * map_size * sizeof(Vapp16s), 1, fp_map1);
    //fread(map2h, map_size * sizeof(Vapp16u), 1, fp_map2);
    //fclose(fp_map1);
    //fclose(fp_map2);
    //Vapp16s* map1[4];
    //Vapp16u* map2[4];

    std::vector<vastStream_t> streams;
    streams.resize(stream_num);
    for (int i = 0; i < stream_num; ++i) {
        vastStreamCreate(device_id, 3, &streams[i],NULL);
    }
    int repeat_time = 10000;

    for (int i = 0; i < repeat_time; ++i) {
        std::cout << "index = " << i << std::endl;
        std::vector<std::future<int>> futs;
        for (int j = 0; j < stream_num; ++j) {
            futs.push_back(std::async(
                std::launch::async, test_remap_threshold_one_stream, streams[j], 1, j));
        }

        for (auto& fut : futs) {
            fut.get();
        }
    }

    for (int j = 0; j < stream_num; ++j) {
        vastStreamDestroy(device_id, streams[j]);
        //vastFree(0, map1[j]);
        //vastFree(0, map2[j]);
    }
}

// int main(int argc, char** argv) {
//     int stream_num = 1;
//     bool flag = 0;
//     int device_id = 0;
//     if (argc > 1) {
//         stream_num = std::atoi(argv[1]);
//     }
//     if (argc > 2) {
//         flag = std::atoi(argv[2]);
//     }


//     test_remap_threshold_multistream2(stream_num);

//     return 0;
// }
