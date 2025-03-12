#include "detect.h"

//#define DUMP_OUTPUT_IMAGE

static Vapp8s laplacian_kernel[3][3] = {
                {0,  1, 0},
                {1, -4, 1},
                {0,  1, 0}
                 };

static Vapp8s sobel_x[3][3] = {
                {-1,  0, 1},
                {-2,  0, 2},
                {-1,  0, 1}
                 };


static Vapp8s sobel_y[3][3] = {
                {-1, -2, -1},
                {0,   0,  0},
                {1,   2,  1}
                 };

#ifdef DUMP_OUTPUT_IMAGE
static void drawTextArea(Vapp8u *image, Vapp32s width, Vapp32s height, Vapp32s block_size, Vapp8u* roi_map) {
    Vapp32s rows = 0, cols = 0;
    Vapp32s pix_index, m, n;
    Vapp32s block_row = 0, block_col = 0;
    int text_block_count = 0;

    rows = height / block_size;
    cols = width / block_size;

    for (block_row = 0; block_row < rows; block_row++) {

        for (block_col = 0; block_col < cols; block_col++) {

            if (roi_map[block_row * cols + block_col] == 1) {
                        text_block_count++;
                for (m = 0; m < block_size; m++){
                    for (n = 0; n < block_size; n++) {
                        if(m == 0 || n == 0 || m == block_size - 1 || n == block_size - 1) {
                            pix_index = width * (block_row * block_size + m) + (block_col * block_size + n);
                            image[pix_index] = 128;
                        }

                    }
                }

            }

        }
    }
    //printf("text_block_count=%d\n", text_block_count);
}
#endif

static void alloc_roimap_buffer(Vapp8u** roi_map, Vapp32s rows, Vapp32s cols) {

    Vapp8u* roi_map_tmp = NULL;

    roi_map_tmp = (Vapp8u*)calloc(1, sizeof(Vapp8u) * rows * cols);
    if (roi_map_tmp == NULL) {
        printf("alloc roi map failed!\n");
        return;
    }

    *roi_map = roi_map_tmp;
}

static void free_roimap_buffer(Vapp8u** roi_map) {

    if (roi_map == NULL)
        return;

    free(*roi_map);
    *roi_map = NULL;
}


static void setup_tile(xvpTile xvTitle, const void* data, Vapp32u pitch, Vapp32u width, Vapp32u height, Vapp32u pad) {
    XV_TILE_SET_DATA_PTR(xvTitle, (Vapp8u*)data);
    XV_TILE_SET_BUFF_SIZE(xvTitle, pitch * (height + 2 * pad));
    XV_TILE_SET_WIDTH(xvTitle, width);
    XV_TILE_SET_HEIGHT(xvTitle, height);
    XV_TILE_SET_PITCH(xvTitle, pitch);
    XV_TILE_SET_TYPE(xvTitle, XV_TYPE_TILE_BIT);
    XV_TILE_SET_EDGE_WIDTH(xvTitle, pad);
    XV_TILE_SET_EDGE_HEIGHT(xvTitle, pad);
    XV_TILE_SET_X_COORD(xvTitle, 0);
    XV_TILE_SET_Y_COORD(xvTitle, 0);
}


static void copy_and_extend_plane(const Vapp8u* src, Vapp32s src_pitch,
    Vapp8u* dst, Vapp32s dst_pitch, Vapp32s w, Vapp32s h,
    Vapp32s extend_top, Vapp32s extend_left,
    Vapp32s extend_bottom, Vapp32s extend_right) {

    Vapp32s i, linesize;

    // copy the left and right most columns out
    const Vapp8u* src_ptr1 = src;
    const Vapp8u* src_ptr2 = src + w - 1;
    Vapp8u* dst_ptr1 = dst - extend_left;
    Vapp8u* dst_ptr2 = dst + w;

    for (i = 0; i < h; i++) {
        memset(dst_ptr1, src_ptr1[0], extend_left);
        memcpy(dst_ptr1 + extend_left, src_ptr1, w);
        memset(dst_ptr2, src_ptr2[0], extend_right);
        src_ptr1 += src_pitch;
        src_ptr2 += src_pitch;
        dst_ptr1 += dst_pitch;
        dst_ptr2 += dst_pitch;
    }

    // Now copy the top and bottom lines into each line of the respective
    // borders
    src_ptr1 = dst - extend_left;
    src_ptr2 = dst + dst_pitch * (h - 1) - extend_left;
    dst_ptr1 = dst + dst_pitch * (-extend_top) - extend_left;
    dst_ptr2 = dst + dst_pitch * (h)-extend_left;
    linesize = extend_left + extend_right + w;

    for (i = 0; i < extend_top; i++) {
        memcpy(dst_ptr1, src_ptr1, linesize);
        dst_ptr1 += dst_pitch;
    }

    for (i = 0; i < extend_bottom; i++) {
        memcpy(dst_ptr2, src_ptr2, linesize);
        dst_ptr2 += dst_pitch;
    }
}

static Vapp32s rgb2gray_ref(const xvpTile src, xvpTile dst, img_format_t pix_format) {

    Vapp32u src_line_size;
    Vapp32u dst_line_size;
    Vapp8u  R, G, B;
    Vapp32u gray;
    Vapp32u j, k;
    Vapp32u plane = 1;

    if (pix_format == _XI_TILE_RGB888_TYPE_) {
        plane = 3;
    } else if (pix_format == _XI_TILE_RGBA_TYPE_) {
        plane = 4;
    }

    const Vapp8u* src_img = (const Vapp8u*)(XV_TILE_GET_DATA_PTR(src));
    Vapp8u* dst_img = (Vapp8u*)(XV_ARRAY_GET_DATA_PTR(dst));
    Vapp32u src_pitch = XV_TILE_GET_PITCH(src);
    Vapp32u dst_pitch = XV_TILE_GET_PITCH(dst);

    for (j = 0; j < XV_TILE_GET_HEIGHT(src); j++)
    {
        src_line_size = j * src_pitch;
        dst_line_size = j * dst_pitch;
        for (k = 0; k < XV_TILE_GET_WIDTH(src); k++)
        {
            R = src_img[src_line_size + plane * k + 0];
            G = src_img[src_line_size + plane * k + 1];
            B = src_img[src_line_size + plane * k + 2];

            gray = (257 * R + 504 * G + 98 * B ) / 1000;
            dst_img[dst_line_size + k] = gray;
        }

    }
    return 0;
}
#if 0
static void extend_plane(const Vapp8u* src, Vapp32s src_pitch,
    Vapp32s w, Vapp32s h,
    Vapp32s extend_top, Vapp32s extend_left,
    Vapp32s extend_bottom, Vapp32s extend_right) {

    Vapp32s i, linesize;

    // copy the left and right most columns out
    const Vapp8u* src_ptr1 = src;
    const Vapp8u* src_ptr2 = src + w - 1;
    Vapp8u* dst_ptr1 = (Vapp8u*)src - extend_left;
    Vapp8u* dst_ptr2 = (Vapp8u*)src + w;

    for (i = 0; i < h; i++) {
        memset(dst_ptr1, src_ptr1[0], extend_left);
        memset(dst_ptr2, src_ptr2[0], extend_right);

        src_ptr1 += src_pitch;
        src_ptr2 += src_pitch;
        dst_ptr1 += src_pitch;
        dst_ptr2 += src_pitch;

    }

    // Now copy the top and bottom lines into each line of the respective
    // borders
    src_ptr1 = src - extend_left;
    src_ptr2 = src + src_pitch * (h - 1) - extend_left;
    dst_ptr1 = (Vapp8u*)src + src_pitch * (-extend_top) - extend_left;
    dst_ptr2 = (Vapp8u*)src + src_pitch * (h)-extend_left;
    linesize = extend_left + extend_right + w;

    for (i = 0; i < extend_top; i++) {
        memcpy(dst_ptr1, src_ptr1, linesize);
        dst_ptr1 += src_pitch;
    }

    for (i = 0; i < extend_bottom; i++) {
        memcpy(dst_ptr2, src_ptr2, linesize);
        dst_ptr2 += src_pitch;
    }
}

static Vapp32s cmp(const void* a, const void* b)
{
    return *(Vapp32u * )b - *(Vapp32u * )a;
}

static void printROIMap(int8_t** roi_map,  Vapp32u row, Vapp32u col)
{
    for (Vapp32s i = 0; i < row; i++)
    {
        for (Vapp32s j = 0; j < col; j++)
        {
            if (roi_map[i][j] == 1)
            {
                printf("printROIMap [%d][%d]=1 \n", i, j);
            }
        }
    }
}

//histogram
static Vapp32s histogram_ref(Vapp8u* src, Vapp32u* dst, Vapp32u pitch, Vapp32u width, Vapp32u height, Vapp32u HistRanges, Vapp32u HistBinSize)
{
    const Vapp8u* src_img = (const Vapp8u*)src;
    Vapp32u* dst_data = (Vapp32u*)dst;

    for (Vapp32s j = 0; j < height; j++)
    {
        for (Vapp32s k = 0; k < width; k++)
        {
            if (src_img[j * pitch + k] < HistRanges)
                dst_data[src_img[j * pitch + k] / HistBinSize] += 1;
        }
    }
    return 0;
}

//sobel and get abs
static Vapp32s sobel_3x3_ref(const xvpTile src, xvpTile dst, int32_t flag)
{
    int8_t kernel_x[] = { -1,  0,   1,
                          -2,  0,   2,
                          -1,  0,   1
    };

    int8_t kernel_y[] = { -1,  -2,  -1,
                           0,   0,   0,
                           1,   2,   1
    };
    int8_t* kernel = flag ? kernel_x : kernel_y;

    const Vapp8u* src_img = (const Vapp8u*)(XV_TILE_GET_DATA_PTR(src));
    int16_t* dst_img = (int16_t*)(XV_ARRAY_GET_DATA_PTR(dst));
    //printf("src_img:%p pitch:%d\n", src_img, XV_TILE_GET_PITCH(src));

    for (Vapp32s j = 0; j < XV_TILE_GET_HEIGHT(src); j++)
    {
        for (Vapp32s k = 0; k < XV_TILE_GET_WIDTH(src); k++)
        {
            int16_t sum = 0;
            for (Vapp32s i = -1; i <= 1; ++i)
            {
                for (Vapp32s l = -1; l <= 1; ++l)
                {
                    sum += src_img[(j + i) * XV_TILE_GET_PITCH(src) + k + l] * kernel[(i + 1) * 3 + l + 1];
                    //if (j < 2 && k < 16)
                    //printf("tmp_ptr %x  %x sum:%x\n", src_img[(j+i) * XV_TILE_GET_PITCH(src) + k + l], (j + i) * XV_TILE_GET_PITCH(src) + k + l, sum);
                }
            }
            dst_img[j * XV_ARRAY_GET_PITCH(dst) + k] = abs(sum);
            if (16 <= j && j < 32 && 16 <= k && j < 32)
                printf("[%d][%d] %d %d\n", j, k, sum, src_img[j * XV_ARRAY_GET_PITCH(src) + k]);
        }
    }

    return 0;
}

//laplacian
static Vapp32s laplacian_3x3_ref(const xvpTile src, xvpTile dst)
{
    int8_t kernel[] = { 0,  1,   0,
                        1,  -4,  1,
                        0,  1,   0
    };

    const Vapp8u* src_img = (const Vapp8u*)(XV_TILE_GET_DATA_PTR(src));
    uint16_t* dst_img = (uint16_t*)(XV_ARRAY_GET_DATA_PTR(dst));

    for (Vapp32s j = 0; j < XV_TILE_GET_HEIGHT(src); j++)
    {
        for (Vapp32s k = 0; k < XV_TILE_GET_WIDTH(src); k++)
        {
            int16_t sum = 0;
            for (Vapp32s i = -1; i <= 1; ++i)
            {
                for (Vapp32s l = -1; l <= 1; ++l)
                {
                    sum += src_img[(j + i) * XV_TILE_GET_PITCH(src) + k + l] * kernel[(i + 1) * 3 + l + 1];
                }
            }
            dst_img[j * XV_ARRAY_GET_PITCH(dst) + k] = abs(sum);

            //if (j < 5 && k < 16)
            //printf("[%d][%d] %d\n", j, k, sum);
        }
    }

    return 0;
}

//laplacian, abs and Sqaured result
static Vapp32s laplacian_3x3_sqaured_ref(const xvpTile src, xvpTile dst)
{
    int8_t kernel[] = { 0,  -1,   0,
                        -1,  4,  -1,
                        0,  -1,   0
    };

    const Vapp8u* src_img = (const Vapp8u*)(XV_TILE_GET_DATA_PTR(src));
    Vapp32u* dst_img = (Vapp32u*)(XV_ARRAY_GET_DATA_PTR(dst));

    for (Vapp32s j = 0; j < XV_TILE_GET_HEIGHT(src); j++)
    {
        for (Vapp32s k = 0; k < XV_TILE_GET_WIDTH(src); k++)
        {
            int16_t sum = 0;
            for (Vapp32s i = -1; i <= 1; ++i)
            {
                for (Vapp32s l = -1; l <= 1; ++l)
                {
                    sum += src_img[(j + i) * XV_TILE_GET_PITCH(src) + k + l] * kernel[(i + 1) * 3 + l + 1];
                }
            }
            dst_img[j * XV_ARRAY_GET_PITCH(dst) + k] = abs(sum) * abs(sum);
        }
    }

    return 0;
}

void findTextArea(xvpTile gray_src, xvpTile gray_pad, Vapp32u block_size, Vapp8u* decision)
{
    Vapp32u dst_pitch = XV_TILE_GET_PITCH(gray_src);
    Vapp32u dst_width = XV_TILE_GET_WIDTH(gray_src);
    Vapp32u dst_height = XV_TILE_GET_HEIGHT(gray_src);



    xvTile sobelx_t;
    int16_t* sobelx_data = (int16_t*)malloc(dst_width * dst_height * sizeof(int16_t));
    setup_tile(&sobelx_t, sobelx_data, dst_width, dst_width, dst_height, 0);

    sobel_3x3_ref(gray_pad, &sobelx_t, 1);


    xvTile sobely_t;
    uint16_t* sobely_data = (uint16_t*)malloc(dst_width * dst_height * sizeof(uint16_t));
    setup_tile(&sobely_t, sobely_data, dst_width, dst_width, dst_height, 0);

    sobel_3x3_ref(gray_pad, &sobely_t, 0);


    xvTile laplacian_t;
    uint16_t* laplacian_data = (uint16_t*)malloc(dst_width * dst_height * sizeof(uint16_t));
    setup_tile(&laplacian_t, laplacian_data, dst_width, dst_width, dst_height, 0);

    laplacian_3x3_ref(gray_pad, &laplacian_t);


    Vapp32u rows = dst_height / block_size;
    Vapp32u cols = dst_width / block_size;
    Vapp32u col_index = 0;
    Vapp32u row_index = 0;

    static Vapp32s frame = 1;

    for (row_index = 0; row_index < rows; row_index++)
    {
        for (col_index = 0; col_index < cols; col_index++)
        {
            Vapp32u hist[HIST_RANGE / HIST_BIN_SIZE] = { 0 };
            Vapp32u start_xpos = col_index * block_size;
            Vapp32u start_ypos = row_index * block_size;

            Vapp32u block_width = XVTM_MIN(block_size, dst_width - col_index * block_size);
            //Vapp32u block_pitch = XVTM_MIN(block_size, dst_pitch - col_index * block_size);
            Vapp32u block_height = XVTM_MIN(block_size, dst_height - row_index * block_size);

            Vapp8u* block_img = (Vapp8u*)(XV_TILE_GET_DATA_PTR(gray_src)) + start_xpos + start_ypos * dst_pitch;

            histogram_ref(block_img, hist, dst_pitch, block_width, block_height, HIST_RANGE, HIST_BIN_SIZE);
            qsort(hist, HIST_RANGE / HIST_BIN_SIZE, sizeof(Vapp32u), cmp);

            //printf("histogram_ref row_index[%d]  col_index[%d] %u  %u  %u\n", row_index, col_index, hist[0], hist[1], hist[2]);

            Vapp32f percentage = 0.0;
            Vapp32u sum_hist_max = 0;
            for (Vapp32s index = 0; index < MAX_BIN_SIZE; index++)
            {
                sum_hist_max += hist[index];
            }
            percentage = sum_hist_max * 1.0 / (block_size * block_size);
            //printf("percentage:%f\n", percentage);

            uint16_t* block_sobelx = ((uint16_t*)XV_TILE_GET_DATA_PTR(&sobelx_t)) + start_xpos + start_ypos * dst_pitch;
            uint16_t* block_sobely = ((uint16_t*)XV_TILE_GET_DATA_PTR(&sobely_t)) + start_xpos + start_ypos * dst_pitch;
            uint16_t* block_laplacian = ((uint16_t*)XV_TILE_GET_DATA_PTR(&laplacian_t)) + start_xpos + start_ypos * dst_pitch;
            Vapp32u roi_gradient_x = 0;
            Vapp32u roi_gradient_y = 0;
            Vapp32u square_laplace_sum = 0;
            for (Vapp32s index_h = 0; index_h < block_height; index_h++)
            {
                for (Vapp32s index_w = 0; index_w < block_width; index_w++)
                {
                    roi_gradient_x += *((uint16_t*)(block_sobelx + index_h * dst_pitch + index_w));
                    roi_gradient_y += *((uint16_t*)(block_sobely + index_h * dst_pitch + index_w));
                    Vapp32u  laplace = *((uint16_t*)(block_laplacian + index_h * dst_pitch + index_w));
                    square_laplace_sum += laplace * laplace;
                }
            }

            if (frame == 1)
                printf("row=%d col=%d roi_gradient_x=%d, roi_gradient_y=%d\n", row_index, col_index, roi_gradient_x, roi_gradient_y);

            if (percentage > HIST_PERCENTAGE_TH
                && square_laplace_sum > LAPLACIAN_TH * block_size * block_size
                && roi_gradient_x > (GRADIENT_TH * block_size * block_size)
                && roi_gradient_y > (GRADIENT_TH * block_size * block_size))
            {
                decision[row_index * cols + col_index] = 1;
                 //fprintf("findTextArea 1 [%d][%d] block_size:%d\n",row_index, col_index, block_size);
            }
        }
    }

    frame++;

    if (laplacian_data)
        free(laplacian_data);

    if (sobely_data)
        free(sobely_data);

    if (sobelx_data)
        free(sobelx_data);
}

void findStaticArea(xvpTile gray, xvpTile preImage, Vapp32u block_size, Vapp8u* decision)
{
    Vapp32u dst_pitch = XV_TILE_GET_PITCH(gray);
    Vapp32u dst_width = XV_TILE_GET_WIDTH(gray);
    Vapp32u dst_height = XV_TILE_GET_HEIGHT(gray);

    Vapp32u rows = dst_height / block_size;
    Vapp32u cols = dst_width / block_size;

    for (Vapp32s row_index = 0; row_index < rows; row_index++)
    {
        for (Vapp32s col_index = 0; col_index < cols; col_index++)
        {
            Vapp32u start_xpos = col_index * block_size;
            Vapp32u start_ypos = row_index * block_size;

            Vapp32u block_width = XVTM_MIN(block_size, dst_width - col_index * block_size);
            //Vapp32u block_pitch = XVTM_MIN(block_size, dst_pitch - col_index * block_size);
            Vapp32u block_height = XVTM_MIN(block_size, dst_height - row_index * block_size);

            Vapp8u* block_img = (Vapp8u*)(XV_TILE_GET_DATA_PTR(gray)) + start_xpos + start_ypos * dst_pitch;
            Vapp8u* block_pre_img = (Vapp8u*)(XV_TILE_GET_DATA_PTR(preImage)) + start_xpos + start_ypos * dst_pitch;

            Vapp32u subtract = 0;
            for (Vapp32s index_h = 0; index_h < block_height; index_h++)
            {
                for (Vapp32s index_w = 0; index_w < block_width; index_w++)
                {
                    Vapp8u block_img_data = *((Vapp8u*)(block_img + index_h * dst_pitch + index_w));
                    Vapp8u block_pre_img_data = *((Vapp8u*)(block_pre_img + index_h * dst_pitch + index_w));
                    subtract += abs(block_img_data - block_pre_img_data);
                    //					if (block_img_data != block_pre_img_data)
                    //					{
                    //						decision[row_index][col_index] = 0;
                    //						continue;
                    //					}
                }
            }
            //printf("subtract:%d\n", subtract);
            if (subtract == 0)
            {
                decision[row_index * cols + col_index] = 1;
                //printf("findStaticArea [%d][%d]=1 \n", row_index, col_index);
            }
        }
    }
}



static void erode(Vapp8u* src, Vapp32u width, Vapp32u height, Vapp8u* dst, Vapp8u kernel_width, Vapp8u kernel_height)
{
    Vapp8u  center_x = (kernel_width - 1) / 2;
    Vapp8u  center_y = (kernel_height - 1) / 2;
    Vapp32s      pitch = width + kernel_width;
    Vapp32s      j, k, i, l;
    Vapp8u  min;

    for (j = 0; j < height; j++)
    {
        for (k = 0; k < width; k++)
        {
            min = 255;
            for (i = -center_y; i <= kernel_height - center_y; ++i)
            {
                for (l = -center_x; l <= kernel_width - center_x; ++l)
                {
                    if (min > src[(j + i) * pitch + k + l])
                        min = src[(j + i) * pitch + k + l];
                }
            }
            dst[j * pitch + k] = min;
        }
    }
}

static void dilate(Vapp8u* src, Vapp32u width, Vapp32u height, Vapp8u* dst, Vapp8u kernel_width, Vapp8u kernel_height)
{
    Vapp8u  center_x = (kernel_width - 1) / 2;
    Vapp8u  center_y = (kernel_height - 1) / 2;
    Vapp32s      pitch = width + kernel_width;
    Vapp32s      j, k, i, l;
    Vapp8u  max;

    for (j = 0; j < height; j++)
    {
        for (k = 0; k < width; k++)
        {
            max = 0;
            for (i = -center_y; i <= kernel_height - center_y; ++i)
            {
                for (l = -center_x; l <= kernel_width - center_x; ++l)
                {

                    if (max < src[(j + i) * pitch + k + l])
                        max = src[(j + i) * pitch + k + l];
                }
            }
            dst[j * pitch + k] = max;
        }
    }
}

static void textAreaMorphologicalOperations(xvpTile gray, Vapp32u block_size, Vapp8u* decision, Vapp8u* roi_map)
{
    Vapp32u dst_width = XV_TILE_GET_WIDTH(gray);
    Vapp32u dst_height = XV_TILE_GET_HEIGHT(gray);

    Vapp32u rows = dst_height  / block_size;
    Vapp32u cols = dst_width / block_size;

    Vapp8u* dilate_pad_out = (Vapp8u*)calloc(1, (dst_width + block_size) * (dst_height + block_size));
    Vapp8u* erode_pad_out  = (Vapp8u*)calloc(1, (dst_width + block_size) * (dst_height + block_size));

    Vapp8u center_x = (block_size - 1) / 2;
    Vapp8u center_y = (block_size - 1) / 2;
    Vapp8u top = center_y;
    Vapp8u left = center_x;
    Vapp8u bottom = block_size - center_y;
    Vapp8u right = block_size - center_x;

    Vapp8u* mask_pad = (Vapp8u*)calloc(1, (dst_width + block_size) * (dst_height + block_size));

    Vapp32u mask_pitch = block_size + dst_width;
    Vapp8u* mask = mask_pad + top * mask_pitch + left;

    for (Vapp32s row_index = 0; row_index < rows; row_index++)
    {
        for (Vapp32s col_index = 0; col_index < cols; col_index++)
        {
            if (roi_map[row_index * cols + col_index] == 1)
            {
                Vapp32u start_xpos = col_index  * block_size;
                Vapp32u start_ypos = row_index * block_size;

                Vapp32u block_width = XVTM_MIN(block_size, dst_width - col_index * block_size);
                Vapp32u block_height = XVTM_MIN(block_size, dst_height - row_index * block_size);

                Vapp8u* block_img = (Vapp8u*)mask + start_xpos + start_ypos * mask_pitch;
                for (Vapp32s index_h = 0; index_h < block_height; index_h++)
                {
                    for (Vapp32s index_w = 0; index_w < block_width; index_w++)
                    {
                        *((Vapp8u*)(block_img + index_h * mask_pitch + index_w)) = 255;
                    }
                }
            }
        }
    }

    extend_plane(mask, mask_pitch, dst_width, dst_height, top, left, bottom, right);

    dilate(mask, dst_width, dst_height, dilate_pad_out+top * mask_pitch + left, block_size, block_size);
    erode(dilate_pad_out + top * mask_pitch + left, dst_width, dst_height, erode_pad_out+top * mask_pitch + left, block_size, block_size);

    for (Vapp32s row_index = 0; row_index < rows; row_index++)
    {
        for (Vapp32s col_index = 0; col_index < cols; col_index++)
        {
            Vapp32u count_nonzero = 0;
            Vapp32u start_xpos = col_index * block_size;
            Vapp32u start_ypos = row_index * block_size;

            Vapp32u block_width = XVTM_MIN(block_size, dst_width - col_index * block_size);
            Vapp32u block_height = XVTM_MIN(block_size, dst_height - row_index * block_size);

            Vapp8u* block_img = (Vapp8u*)erode_pad_out + top * mask_pitch + left + start_xpos + start_ypos * mask_pitch;
            for (Vapp32s index_h = 0; index_h < block_height; index_h++)
            {
                for (Vapp32s index_w = 0; index_w < block_width; index_w++)
                {
                    if (*((Vapp8u*)(block_img + index_h * mask_pitch + index_w)) != 0)
                        count_nonzero++;
                }
            }

            if (count_nonzero >= MORPHOLOGICAL_TH * block_size * block_size)
            {
                decision[row_index * cols + col_index] = 1;
            }
        }
    }

    if (mask_pad) {
        free(mask_pad);
    }

    if (dilate_pad_out) {
        free(dilate_pad_out);
    }

    if (erode_pad_out) {
        free(erode_pad_out);
    }
}
#endif

static Vapp32s textAreaMorphologicalOperations_v1 (Vapp8u *roi_map, dection_para_t *dect_param, Vapp8u *roi_map_out) {

	Vapp32u width  = dect_param->in_image_shape.width;
    Vapp32u height = dect_param->in_image_shape.height;
    Vapp32u block_size = dect_param->params.block_size;
    Vapp32u rows = 0, cols = 0;
	Vapp32u block_row, block_col, m, n, i, j;
	Vapp32u block_index = 0;
    Vapp8u  padding_left, padding_top;
    Vapp8u  *mask_pad, *delation_out_pad, *erison_out_pad;
    Vapp32u mask_pitch;
    Vapp32u pix_index, location;
    Vapp32u none_zero_count = 0;
    Vapp8u  max = 0, min = 255;


    padding_left = padding_top = (block_size - 1) / 2;
	rows = height / block_size;
	cols = width / block_size;

    mask_pitch = width + block_size;
    mask_pad = (Vapp8u*)calloc(1, (width + block_size) * (height + block_size));
    delation_out_pad = (Vapp8u*)calloc(1, (width + block_size) * (height + block_size));
    erison_out_pad   = (Vapp8u*)calloc(1, (width + block_size) * (height + block_size));

	//binarization
	for (block_row = 0; block_row < rows; block_row++) {

		for (block_col = 0; block_col < cols; block_col++) {

			block_index = block_row * cols + block_col;

			if (roi_map[block_index] == 1) {

				for (m = 0; m < block_size; m++) {

					for (n = 0; n < block_size; n++) {

						pix_index = mask_pitch * (padding_top + block_row * block_size + m) + (padding_left + block_col * block_size + n);
						mask_pad[pix_index] = 255;

					}

				}

			}
		}
	}

	//delation
	for (block_row = 0; block_row < rows; block_row++) {

		for (block_col = 0; block_col < cols; block_col++) {

			block_index = block_row * cols + block_col;

            for (m = 0; m < block_size; m++) {

                for (n = 0; n < block_size; n++) {

                    // current pix location
                    pix_index = mask_pitch * (padding_top + block_row * block_size + m) + (padding_left + block_col * block_size + n);
                    max = 0;

                    for (i = 0; i < block_size; i++) {

                        for (j = 0; j < block_size; j++) {

                            location = pix_index - mask_pitch * padding_top - padding_left + i * mask_pitch + j;

                            if (mask_pad[location] >= max) {
                                max = mask_pad[location];
                            }

                        }
                    }

                    delation_out_pad[pix_index] = max;

                }

            }

		}
	}


	//erosion
	for (block_row = 0; block_row < rows; block_row++) {

		for (block_col = 0; block_col < cols; block_col++) {

			block_index = block_row * cols + block_col;

            for (m = 0; m < block_size; m++) {

                for (n = 0; n < block_size; n++) {

                    // current pix location
                    pix_index = mask_pitch * (padding_top + block_row * block_size + m) + (padding_left + block_col * block_size + n);
                    min = 255;

                    for (i = 0; i < block_size; i++) {

                        for (j = 0; j < block_size; j++) {

                            location = pix_index - mask_pitch * padding_top - padding_left + i * mask_pitch + j;

                            if (delation_out_pad[location] < min) {
                                min = delation_out_pad[location];
                            }

                        }
                    }

                    erison_out_pad[pix_index] = min;

                }

            }

        }
	}

    //get final result
	for (block_row = 0; block_row < rows; block_row++) {

		for (block_col = 0; block_col < cols; block_col++) {

			block_index = block_row * cols + block_col;

            none_zero_count = 0;

            for (m = 0; m < block_size; m++) {

                for (n = 0; n < block_size; n++) {

                    pix_index = mask_pitch * (padding_top + block_row * block_size + m) + (padding_left + block_col * block_size + n);
                    if (erison_out_pad[pix_index]) {
                        none_zero_count++;
                    }

                }

            }
            //printf("none_zero_count=%d\n", none_zero_count);
			if (none_zero_count >= dect_param->params.morphological_th * block_size*block_size) {
                roi_map_out[block_index] = 1;
            }

		}
	}

    if (mask_pad)
	    free(mask_pad);
    if (delation_out_pad)
	    free(delation_out_pad);
    if (erison_out_pad)
	    free(erison_out_pad);

	return 0;

}

#if 0
static Vapp32s textAreaMorphologicalOperations_v2 (Vapp8u *roi_map, , dection_para_t *dect_param, Vapp8u *roi_map_out) {

	Vapp32u width  = dect_param->in_image_shape.width;
    Vapp32u height = dect_param->in_image_shape.height;
    Vapp32u block_size = dect_param->params.block_size;
	Vapp32s rows = 0, cols = 0;
	Vapp32s block_row, block_col;
	Vapp32s block_index = 0;

	rows = height / block_size;
	cols = width / block_size;

	int8_t *roi_delation = (int8_t *)calloc(1, rows * cols * sizeof(int8_t));

	//delation
	for (block_row = 1; block_row < rows - 1; block_row++) {

		for (block_col = 1; block_col < cols - 1; block_col++) {

			block_index = block_row * cols + block_col;

			if(roi_map[block_index] == 1) {

				roi_delation[block_index] = 1;
				continue;

			} else {

				if (roi_map[block_index-1] || roi_map[block_index+1] ||roi_map[block_index-cols] || roi_map[block_index+cols]
					|| roi_map[block_index-cols - 1] || roi_map[block_index-cols+1]
					|| roi_map[block_index+cols - 1] || roi_map[block_index+cols+1]) {
						roi_delation[block_index] = 1;
					}

			}

		}
	}

	//erosion
	for (block_row = 1; block_row < rows - 1; block_row++) {

		for (block_col = 1; block_col < cols - 1; block_col++) {

			block_index = block_row * cols + block_col;

			if(roi_delation[block_index] == 0) {

				roi_map_out[block_index] = 0;
				continue;

			} else {

				if (roi_delation[block_index-1] && roi_delation[block_index+1] && roi_delation[block_index-cols] && roi_delation[block_index+cols]
					&& roi_delation[block_index-cols - 1] && roi_delation[block_index-cols+1]
					&& roi_delation[block_index+cols - 1] && roi_delation[block_index+cols+1]) {
						roi_map_out[block_index] = 1;
					}

			}

		}
	}

	free(roi_delation);

	return 0;

}
#endif

static void mergeTwoROIMap(Vapp8u* roi_map, Vapp8u* roi_map_extend, Vapp32u row, Vapp32u col, Vapp32u row_extend, Vapp32u col_extend)
{
    Vapp32u i, j;

    for (i = 0; i < row_extend; i++)
    {
        for (j = 0; j < col_extend; j++)
        {
            if (roi_map_extend[i * col_extend + j] == 1)
            {
                roi_map[(i * 2) * col + j * 2] = 1;
                roi_map[(i * 2 + 1) * col + j * 2] = 1;
                roi_map[(i * 2) * col + j * 2 + 1] = 1;
                roi_map[(i * 2 + 1) * col + j * 2 + 1] = 1;
            }
        }
    }
}

static void generateROIMap(roi_map_ptr_t *roi_map_buf,  dection_para_t *dect_param, Vapp32u row, Vapp32u col, int8_t* roi_map_final)
{
    Vapp8u *roi_map_base   = roi_map_buf->roi_map_base;
    Vapp8u *roi_map_morph  = roi_map_buf->roi_map_morph;
    Vapp8u *roi_map_static = roi_map_buf->roi_map_static;

    for (Vapp32s i = 0; i < row; i++)
    {
        for (Vapp32s j = 0; j < col; j++)
        {

            // static area enable
            if (dect_param->params.static_enable) {

                if (roi_map_static[i * col + j] == 1) {
                    roi_map_final[i * col + j] = QP_OFFSET_STATIC;
                }

                // static and text area enable
                if (dect_param->params.text_enable) {

                    if (dect_param->params.morpho_enable) {
                        if (roi_map_morph[i * col + j] == 1) {
                            roi_map_final[i * col + j] = QP_OFFSET_STATIC_TEXT;
                        }
                    } else {
                        if (roi_map_base[i * col + j] == 1) {
                            roi_map_final[i * col + j] = QP_OFFSET_STATIC_TEXT;
                        }
                    }

                }

            //static area disable
            } else {

                //text area enable
                if (dect_param->params.text_enable) {

                    if (dect_param->params.morpho_enable) {
                        if (roi_map_morph[i * col + j] == 1) {
                            roi_map_final[i * col + j] = QP_OFFSET_TEXT;
                        }
                    } else {
                        if (roi_map_base[i * col + j] == 1) {
                            roi_map_final[i * col + j] = QP_OFFSET_TEXT;
                        }
                    }

                }

            }

        }
    }
}


static void findTopN(uint16_t arr[], Vapp32s n, Vapp32s N) {

    Vapp32s i, j, temp;

    for (i = 0; i < N; i++) {

        for (j = i + 1; j < n; j++) {

            if (arr[i] < arr[j]) {

                temp   = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;

            }
        }

    }
}

static Vapp32s calcBlockTextArea(Vapp8u *image, dection_para_t *dect_param, Vapp8u block_size, Vapp32s block_row, Vapp32s block_col, Vapp8u* roi_map) {

    Vapp32s i, j, m, n;
    Vapp32s cols = 0;

    Vapp32u width = dect_param->in_image_shape.width;
    Vapp32u height = dect_param->in_image_shape.height;
    Vapp32u padding_width = dect_param->in_image_shape.width + 2;
    Vapp32u padding_height = dect_param->in_image_shape.height + 2;

    Vapp32s padding_x = (padding_width - width) / 2;
    Vapp32s padding_y = (padding_height - height) / 2;

    Vapp32s hist_size = dect_param->params.hist_bin_size;
    Vapp32s max_bin_size = dect_param->params.max_bin_size;
    Vapp32s bin_width = HIST_RANGE /  hist_size;

    Vapp32s gradient_th = dect_param->params.gradient_th;
    Vapp32s laplacian_th = dect_param->params.laplacian_th;
    Vapp32f hist_percentage_th = dect_param->params.hist_percentage_th;

    Vapp32s pix_index, hist_index;
    Vapp8u pix_value;

    uint16_t hist[HIST_RANGE] = {0};
    Vapp32f percentage = 0.0;

    Vapp32s tmp_x = 0, tmp_y = 0;
    Vapp32f gradient_x = 0, gradient_y = 0;

    Vapp32s laplacian_tmp = 0;
    Vapp32s laplacian_square_sum = 0;

    Vapp8u hist_decision = 0, grident_decision = 0, laplacian_decision = 0;

    cols = width / block_size;

    for (m = 0; m < block_size; m++) {

        for (n = 0; n < block_size; n++) {

            pix_index = padding_width * (padding_y + block_row * block_size + m) + (padding_x + block_col * block_size + n);
            pix_value = image[pix_index];

            //hist
            if (dect_param->params.hist_enable) {

                //calc hist
                hist_index = pix_value / bin_width;
                hist[hist_index]++;

            }

            //calc sobel
            if (dect_param->params.grident_enable) {

                tmp_x = 0;
                tmp_y = 0;
                for (i = 0; i < KSIZE; i++) {
                    for (j = 0; j < KSIZE; j++) {
                        pix_index = padding_width * (padding_y + block_row * block_size + m - 1 + i) + (padding_x + block_col * block_size + n - 1 + j);
                        tmp_x += image[pix_index] * sobel_x[i][j];
                        tmp_y += image[pix_index] * sobel_y[i][j];
                    }
                }

                gradient_x += abs(tmp_x);
                gradient_y += abs(tmp_y);

            }


            //calc laplacian
            if (dect_param->params.laplacian_enable) {

                laplacian_tmp = 0;

                for (i = 0; i < KSIZE; i++) {
                    for (j = 0; j < KSIZE; j++) {
                        pix_index = padding_width * (padding_y + block_row * block_size + m - 1 + i) + (padding_x + block_col * block_size + n - 1 + j);
                        laplacian_tmp += image[pix_index] * laplacian_kernel[i][j];
                    }
                }

                laplacian_square_sum += abs(laplacian_tmp) * abs(laplacian_tmp);

            }


        }

    }

    if (dect_param->params.hist_enable) {

        findTopN(hist, hist_size, max_bin_size);
        percentage = (Vapp32f)(hist[0] + hist[1] + hist[2]) / (block_size * block_size);

    }

    hist_decision = dect_param->params.hist_enable ? (percentage > hist_percentage_th) : 1;
    grident_decision = dect_param->params.grident_enable ? ((gradient_x > gradient_th * block_size * block_size) && (gradient_y > gradient_th * block_size * block_size)) : 1;
    laplacian_decision = dect_param->params.laplacian_enable ? ((laplacian_square_sum > laplacian_th * block_size * block_size)): 1;

    if((dect_param->params.hist_enable||dect_param->params.grident_enable||dect_param->params.laplacian_enable) && (hist_decision) && (grident_decision) && (laplacian_decision)) {
        roi_map[block_row * cols + block_col] = 1;
    } else {
        roi_map[block_row * cols + block_col] = 0;
    }

    return 0;
}

static Vapp32s findTextArea_v1(Vapp8u *image, dection_para_t *dect_param, Vapp8u block_size, Vapp8u* roi_map) {

    Vapp32s rows = 0, cols = 0;
    Vapp32s i, j;
    Vapp32s hist_size = dect_param->params.hist_bin_size;

    if (image == NULL || roi_map == NULL) {
        printf("invalid input image or hist!\n");
        return -1;
    }

    if (hist_size < 0 || hist_size > 256) {
        printf("invalid hist_size!\n");
        return -1;
    }

    rows = dect_param->in_image_shape.height / block_size;
    cols = dect_param->in_image_shape.width / block_size;

    for (i = 0; i < rows; i++) {

        for (j = 0; j < cols; j++) {

            calcBlockTextArea(image, dect_param, block_size, i, j, roi_map);

        }

    }

    return 0;

}

static Vapp32s findStaticArea_v1(const Vapp8u *image, const Vapp8u *pre_image, dection_para_t* dect_param, Vapp8u* roi_map_static) {

    Vapp32u width = dect_param->in_image_shape.width;
    Vapp32u height = dect_param->in_image_shape.height;
    Vapp32u block_size = dect_param->params.block_size;
    Vapp32s rows = 0, cols = 0;
    Vapp32s block_row, block_col, m, n;
    Vapp32s pix_index;
    Vapp32s block_pix_diff = 0;

    if (image == NULL || pre_image == NULL || roi_map_static == NULL) {
        printf("invalid input param!\n");
        return -1;
    }

    rows = height / block_size;
    cols = width / block_size;

    for (block_row = 0; block_row < rows; block_row++) {

        for (block_col = 0; block_col < cols; block_col++) {

            block_pix_diff = 0;

            for (m = 0; m < block_size; m++) {

                for (n = 0; n < block_size; n++) {

                    pix_index = width * (block_row * block_size + m) + (block_col * block_size + n);

                    if (abs(image[pix_index] - pre_image[pix_index]) != 0) {

                        block_pix_diff++;

                    }

                }

            }

            if (block_pix_diff == 0 ) {

                roi_map_static[block_row * cols + block_col] = 1;

            } else {

                roi_map_static[block_row * cols + block_col] = 0;

            }

        }

    }

    return 0;

}

VappStatus run_detection_test(VappiTextDetectionBuffers *buffers, dection_para_t *dect_param)
{
    Vapp32u width  = dect_param->in_image_shape.width;
    Vapp32u height = dect_param->in_image_shape.height;
    Vapp32u block_size = dect_param->params.block_size;
    Vapp32u extend_block_size = dect_param->params.block_size * 2;
    Vapp32u rows = height  / block_size;
    Vapp32u cols = width / block_size;
    Vapp32u rows_extend = height  / (block_size * 2);
    Vapp32u cols_extend = width / (block_size * 2);
    Vapp8u *srcImg, *preImg, *gray_pad;
    Vapp32u srcPitch;
    img_format_t pix_format = dect_param->img_type;

    xvTile gray_pad_t;
    xvTile src_rgb_t, dst_gray_t;
    roi_map_ptr_t roi_map_buf = {NULL};

    gray_pad = (Vapp8u*)malloc((width + 2) * (height + 2));
    if (gray_pad == NULL) {
        printf("alloc gray_pad mem failed!\n");
        return VAPP_NO_MEMORY_ERROR;
    }

    if (pix_format != _XI_TILE_YUV_NV12_TYPE_ && pix_format != _XI_TILE_YUV_I420_TYPE_) {

        setup_tile(&src_rgb_t, buffers->in_img_addr, dect_param->in_image_shape.w_pitch, width, height, 0);
        setup_tile(&dst_gray_t, buffers->out_gray_addr, width, width, height, 0);
        rgb2gray_ref(&src_rgb_t, &dst_gray_t, pix_format);

    } else {

        memcpy(buffers->out_gray_addr, buffers->in_img_addr, width * height);

    }

    srcImg = (Vapp8u*)buffers->out_gray_addr;
    preImg = (Vapp8u*)buffers->pre_gray_addr;
    srcPitch = width;

    // setup_tile(&src_t, srcImg, srcPitch, width, height, 0);
    // setup_tile(&pre_t, preImg, srcPitch, width, height, 0);

#if 0
    printf("text_enable=%d,  static_enable=%d\n", dect_param->params.text_enable, dect_param->params.static_enable);
    if (dect_param->params.text_enable) {
        printf("extend_enable=%d, morpho_enable=%d, hist_enable=%d, grident_enable=%d, laplacian_enable=%d\n",
            dect_param->params.extend_enable, dect_param->params.morpho_enable, dect_param->params.hist_enable, dect_param->params.grident_enable, dect_param->params.laplacian_enable);
    }
#endif

    //text area detection
    if (dect_param->params.text_enable) {

        alloc_roimap_buffer(&roi_map_buf.roi_map_base, rows, cols);

        if (dect_param->params.extend_enable) {
            alloc_roimap_buffer(&roi_map_buf.roi_map_extend, rows_extend, cols_extend);
        }

        if (dect_param->params.morpho_enable) {
            alloc_roimap_buffer(&roi_map_buf.roi_map_morph, rows, cols);
        }

        copy_and_extend_plane(srcImg, srcPitch, gray_pad + (width + 2) + 1, srcPitch + 2, width, height, 1, 1, 1, 1);

        setup_tile(&gray_pad_t, gray_pad + (width + 2) + 1, srcPitch + 2, width, height, 1);

        findTextArea_v1(gray_pad, dect_param, block_size, roi_map_buf.roi_map_base);

        if (dect_param->params.extend_enable) {
            findTextArea_v1(gray_pad, dect_param, extend_block_size, roi_map_buf.roi_map_extend);
            mergeTwoROIMap(roi_map_buf.roi_map_base, roi_map_buf.roi_map_extend, rows, cols, rows_extend, cols_extend);
        }

        if (dect_param->params.morpho_enable) {
            //textAreaMorphologicalOperations(&src_t, block_size, roi_map_buf.roi_map_morph, roi_map_buf.roi_map_base);
            textAreaMorphologicalOperations_v1(roi_map_buf.roi_map_base, dect_param, roi_map_buf.roi_map_morph);
        }

    }

    //static area detection
    if (dect_param->params.static_enable) {
        alloc_roimap_buffer(&roi_map_buf.roi_map_static, rows, cols);
        if (!dect_param->first_frame) {
            findStaticArea_v1(srcImg, preImg, dect_param, roi_map_buf.roi_map_static);
        }
    }

    generateROIMap(&roi_map_buf, dect_param, rows, cols, buffers->out_roi_map_final_addr);

#ifdef DUMP_OUTPUT_IMAGE
    FILE *fp_out_detection = fopen("/data/src/dsp/vapp/out-736x1296.yuv", "ab+");
    Vapp32u image_size = width * height;
    
    Vapp8u* pOutImg  = (Vapp8u*)malloc(image_size);
    memcpy(pOutImg, srcImg, image_size);

    if (dect_param->params.text_enable&&dect_param->params.morpho_enable)
        drawTextArea(pOutImg, width, height, 16, roi_map_buf.roi_map_morph);
    else if (dect_param->params.text_enable)
        drawTextArea(pOutImg, width, height, 16, roi_map_buf.roi_map_base);
    else
         drawTextArea(pOutImg, width, height, 16, roi_map_buf.roi_map_static);

    fwrite(pOutImg, image_size, 1, fp_out_detection);
    fclose(fp_out_detection);
    free(pOutImg);
#endif

    memcpy(buffers->pre_gray_addr, buffers->out_gray_addr, width * height);

    if (dect_param->params.text_enable) {

        if (dect_param->params.morpho_enable) {
            free_roimap_buffer(&roi_map_buf.roi_map_morph);
        }

        if (dect_param->params.extend_enable) {
            free_roimap_buffer(&roi_map_buf.roi_map_extend);
        }

        free_roimap_buffer(&roi_map_buf.roi_map_base);

    }

    if (dect_param->params.static_enable) {
        free_roimap_buffer(&roi_map_buf.roi_map_static);
    }

    if (gray_pad) {
        free(gray_pad);
    }

    return VAPP_NO_ERROR;
}
