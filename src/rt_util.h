#ifndef _VAPP_RT_UTIL_H_
#define _VAPP_RT_UTIL_H_
// the base of runtime error code
#ifndef RT_ERR_BASE
#define RT_ERR_BASE 500000
#endif
#define MAX_IN_OUT_ADDR 64
#define BLOCK_NUM 3

#include <stdint.h>
#ifdef __linux__
#include <pthread.h>
#elif _WIN32
#include <Windows.h>
#endif
#pragma pack(push, 1)
typedef struct {
    /* header */
    uint32_t seperator;
    uint32_t loopCount;
    uint32_t batchSize;
    uint32_t configCount;
    uint32_t inputCount;
    uint32_t outputCount;

    uint64_t config; 
    uint64_t inout_addr[64];
} op_args_args_t;
typedef struct get_desc {
    uint32_t dev : 32;
    uint32_t pid : 32;
    uint32_t stream_id : 32;
    uint32_t data_id : 32;
    /* dev soc entry addr */
    uint32_t stream_addr;
    uint32_t context_id;
    uint32_t error_code;
    uint32_t : 32;
} get_desc_t;
typedef struct run_stream {
    uint32_t dev : 32;
    // uint32_t cmd : 32;
    uint32_t pid : 32;
    uint32_t stream_id : 32;
    uint32_t data_id : 32;
    /* dev soc entry addr */
    uint32_t stream_addr : 32;
    uint32_t : 32;
    uint64_t : 64;
} run_stream_t;
#pragma pack(pop)

typedef struct {
    uint32_t p_fun;
    char name[32];
} vastai_vdsp_op_t;

typedef struct vastai_op_entry_list{
    uint32_t op_num;
    void * op_entry_list;
}vastai_op_entry_list_t;

typedef struct ext_addr {
    uint64_t soc_addr;// soc address (ddr address)
    uint64_t bar_addr;// bar address
    void *vir_addr;// virtual address
} addr_ext_t;

typedef struct {
    uint32_t nnlib_version;
    uint32_t sdk_version;
    char dsp_git_hash[48];
    char dsp_build_time[32];
} vastai_op_version_t;



typedef struct get_output_register {
    uint32_t dev_id;
    void *func_arg;
    int (*func_get_output)(uint32_t dev_id, get_desc_t *, void *func_arg);
} get_output_callback_t;

typedef struct create_stream {
    uint32_t stream_id : 32;
    /* 0,balance for stream; 1,balance for run stream */
    uint32_t vdsp_balance_mode : 32;
    uint32_t : 32;
    get_output_callback_t callback;
} create_stream_t;

typedef struct op_confg {
    void *config;
    uint32_t size;
} op_config_t;

typedef struct op_info {
    uint32_t op_uid;
    uint32_t op_addr;
    uint32_t op_type;
    void *argument;
    uint32_t argument_size;
    op_config_t *config_array;
    uint32_t config_num;
} op_info_t;



typedef int (* config_op)(void * priv_params, uint32_t entry);
typedef int (* update_op_cfg)(void * priv_params);
typedef struct{
    op_config_t config;
    op_info_t op_info;
    uint64_t inout_addr[MAX_IN_OUT_ADDR];
    int nOutputs;
    char * elf_file; 
    char* custom_op_name;  
    int32_t block_num;
    int32_t block_id;    
    config_op config_op_params;  
    update_op_cfg update_cfg;
    void *priv_params;
}op_params;

typedef struct {
    uint32_t dev;
    uint32_t pid;
    uint32_t stream_id;
    int frame_ready;  
    int block_num;
    int block_id;
#ifdef __linux__
    pthread_mutex_t mutex;
    pthread_cond_t ready_cond;
#elif _WIN32
    CRITICAL_SECTION  cs_lock;
    CONDITION_VARIABLE ready_cv;
#endif

    short flag;
    void *stream;
    int hope_op_times;
}op_async_data;

typedef struct {
	char * elf_file;
	char uni_code[37];
	int dsp_type;
    int reg_flag;
}OpRegister;


typedef struct{
    int x;          /**<  x-coordinate of upper left corner (lowest memory address). */
    int y;          /**<  y-coordinate of upper left corner (lowest memory address). */
    int z;
    int width;      /**<  Rectangle width. */
    int height;     /**<  Rectangle height. */
    int channels;
}VappiShape;
typedef enum {
    //geometry transforms
    OP_DEFAULT = 0,
    OP_SCALE_U8,
    OP_RESIZE_MULTICORE_OP,
    OP_IMAGE_CROP_U8,
    OP_SIMPLE_ROTATE,
    OP_FLIP,
    OP_FLIP_ROI,
    OP_REMAP,
    OP_CROP_ROI,
    OP_WARPPERSPECTIVE,
    OP_TRANSLATE,
    OP_PERMUTE,
    OP_TRANSPOSE,
    OP_CROPSCALE,

    //color space conversion
    OP_CVTCOLOR_U8,
    OP_CVTCOLOR_ROI_EXTOP,
    OP_COLOR_SPACE,
    OP_BAYERTONV12,

    //arithmetic and logical
    OP_FFMPEG_OVERLAY,
    OP_BIT10TO8,
    OP_ADAPTIVETHRESHOLD,
    OP_EQ,
    OP_SAD,
    OP_DETECTION,

    //filtering functions
    OP_UNSHARP,
    OP_HQDN3D,
    OP_CAS,
    
    OP_ROTATE_RGBA,
    OP_ARGB2NV12,
    OP_RESERVED
}VappOpFunc;
typedef struct{
    int stream_id;
    op_async_data *op_async; 
    int frame_num; 
    uint32_t fun;
    void *stream_ctx;
    VappOpFunc op_type;
    int block_id;
    int data_id;
    void* op_cfg;
    int op_cfg_size;
    int last_op_cfg_size;
    uint64_t inout_addr[MAX_IN_OUT_ADDR];
    uint32_t op_addr;
}RTStream;
typedef struct{
    RTStream *rt_stream;
	uint32_t vacc_id;
    //int stream_id;
	OpRegister op_reg;
#ifdef __linux__
    pthread_rwlock_t op_entry_lock;    
#elif _WIN32
    SRWLOCK op_entry_lock;
#endif
    int cur_op;
    uint32_t fun;
    //op_async_data *op_async;
    vastai_op_entry_list_t op_entry_list;
    int issued;
    VappiShape * size_roi; 
    Vapp8u * map1_buffer;
    Vapp8u * map2_buffer;
    Vapp8u * hqdn3d_buffer; 
    int block_num;
}VastStream;



/**
 * VAST error types
 */
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
// note: runtime error is 2xxxxx
typedef enum deviceRuntimeError {
    rtSuccess = 0,
    rtReady = 1,
    /* -------- normal error -------- */
    rtErrMalloc = RT_ERR_BASE,
    rtErrAssert,
    rtErrArgument,
    rtErrBatchSize,
    rtErrPthread,
    rtErrPthreadCreate,
    rtErrListFind,
    rtErrDlcSimulator,
    rtErrKeyInvalid,
    /* -------- device error -------- */
    rtErrInitDev = RT_ERR_BASE + 20,
    rtErrStorageType,
    rtErrSramOffset,
    rtErrFindProcess,
    rtErrGetProcStatus,
    rtErrGetFwStatus,
    /* -------- model memory -------- */
    rtErrMallocModel = RT_ERR_BASE + 40,
    rtErrMallocDdrShare,
    rtErrMallocWeight,
    rtErrWeightOversize,
    rtErrMallocInOut,
    rtErrMallocVideo,
    rtErrMallocStreamInfo,
    rtErrMallocHost,
    rtErrMemcpyHost2Dev,
    rtErrMemcpyDev2Host,
    rtErrCopyDie2Die,
    rtErrSocDdr2Bar,
    /* -------- model error -------- */
    rtErrModelName = RT_ERR_BASE + 80,
    rtErrCreateModel,
    rtErrCreateModelNode,
    rtErrLoadModel,
    rtErrRunModel,
    rtErrCreateStream,
    rtErrRunStream,
    rtErrOperatorName,
    rtErrCreateOperator,
    rtErrLoadOperator,
    rtErrInitModelLayer,
    rtErrCopyModelLayer,
    rtErrModelHWConfig,
    rtErrInsertVdspPipe,
    rtErrFindVdspPipe,
    rtErrSchedule,
    rtErrScheduleOdmaSRC,
    rtErrScheduleCdmaSRC,
    rtErrScheduleWdmaFW,
    rtErrScheduleWdmaLock,
    rtErrScheduleGroup,
    rtErrBitCfg,
    rtErrWdmaMstCfg,
    /* -------- ISA error -------- */
    rtErrPipeSequence = RT_ERR_BASE + 120,
    rtErrIsaType,
    rtErrIsaParam,
    rtErrIsaLayer,
    rtErrOthers,
    rtErrEnd
} rtError_t;



#ifdef __cplusplus
extern "C"// C++
{
#endif

rtError_t vapp_op_execute(uint32_t dev_id, const op_info_t *op_info, uint32_t op_num,uint32_t timeout, uint32_t *error_no);
int vapp_check_status(uint32_t dev_id);
rtError_t custom_op_register(int vacc_id, uint32_t type,  char * elf_file,  char *op_id);
rtError_t op_register(int vacc_id, OpRegister *reg_value);
rtError_t vapp_get_op_entry(uint32_t dev_id, const char *op_name, uint32_t *func_entry);
rtError_t vapp_create_stream(uint32_t dev_id, uint32_t stream_id, void *args);
rtError_t vapp_destroy_stream(uint32_t dev_id, uint32_t stream_id);
rtError_t vapp_op_execute_async(uint32_t dev_id, const op_info_t *op_info, uint32_t op_num, uint32_t stream_id, uint32_t data_id);
rtError_t vapp_get_op_entrylist(uint32_t dev_id, const char *id_name, vastai_op_entry_list_t *op_entry_list);
rtError_t vapp_free_op_entrylist(uint32_t dev_id, vastai_op_entry_list_t *op_entry_list);
#ifdef __cplusplus
}
#endif

#endif