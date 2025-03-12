#include <stdio.h>
#include <stdint.h>

#ifdef __linux__
#include <dlfcn.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>  
#include <string.h> 
#include <pthread.h>
int vapp_usleep(unsigned usec);
static pthread_mutex_t vacc_init_mutex = PTHREAD_MUTEX_INITIALIZER;
#elif _WIN32
#include "compat.h"
CRITICAL_SECTION vacc_init_cs = {0};
#endif
#include <inttypes.h>
//#include <sys/stat.h>
#include "vapp.h"
#include "vdsp_op.h"
#include "md5.h"
//#include "uuid4.h"
#include <sys/stat.h>
#include "xxhash32.h"
//static void* vaccrt_open_handle = NULL;
int vapp_log_level;
int g_stream_id = 0; 
#ifdef __linux__
#define LIB_VACCRT_PATH "libvaccrt.so"
#elif _WIN32
#include "compat.h"
#define LIB_VACCRT_PATH "vaccrt.dll"
static int op_register_once = 0;
static char uni_code_once[37];
#endif

#define MAX_VACC_NUM 64

typedef rtError_t (*vaccrt_init_t)(uint32_t dev_id);
typedef rtError_t (*vaccrt_malloc_video_t)(uint32_t dev_id, uint32_t align, uint64_t size, uint64_t *addr);
typedef rtError_t (*vaccrt_free_video_t)(uint32_t dev_id, uint64_t addr);
typedef rtError_t (*vaccrt_malloc_host_t)(uint32_t dev_id, size_t size, void **addr);
typedef rtError_t (*vaccrt_free_host_t)(uint32_t dev_id, void *addr);
typedef rtError_t (*vaccrt_get_current_pid_t)(uint32_t dev_id, uint32_t *pid);
typedef rtError_t (*vaccrt_get_process_status_t)(uint32_t dev_id, uint32_t pid, uint32_t *status);
typedef rtError_t (*vaccrt_get_video_reserver_ddr_t)(uint32_t dev_id, addr_ext_t *addr_ext);
typedef rtError_t (*vaccrt_program_create_t)(uint32_t dev_id, const void *elf_buff, uint64_t size, const char *identifier_name,
                                   uint32_t op_type, vastai_op_version_t *dsp_op_version);
typedef rtError_t (*vaccrt_program_get_function_t)(uint32_t dev_id, const char *op_name, uint32_t *func_entry);
typedef rtError_t (*vaccrt_create_stream_t)(uint32_t dev_id, const create_stream_t *stream);
typedef rtError_t (*vaccrt_memcpy_t)(uint32_t dev_id, const void *addr_from, size_t size, void *addr_to,
                        memcpy_direction_t direction);
typedef rtError_t (*vaccrt_malloc_inout_t)(uint32_t dev_id, uint32_t align, size_t size, void **addr);					
typedef rtError_t (*vaccrt_free_inout_t)(uint32_t dev_id, void *addr);
typedef rtError_t (*vaccrt_sync_run_stream_t)(uint32_t dev_id, const op_info_t *op_info, uint32_t op_num, uint32_t timeout,
                                 uint32_t *error_no);
typedef rtError_t (*vaccrt_destroy_stream_t)(uint32_t dev_id, uint32_t stream_id);
typedef rtError_t (*vaccrt_program_destroy_t)(uint32_t dev_id, const char * identifier_name);
typedef rtError_t (*vaccrt_async_run_stream_t)(uint32_t dev_id, const run_stream_t *stream, const op_info_t *op_info,
                                                  uint32_t op_num);
typedef rtError_t (*vaccrt_program_get_function_list_t)(uint32_t dev_id, const char *identifier_name,
                                           vastai_op_entry_list_t *op_entry_list);     
typedef rtError_t (*vaccrt_program_free_function_list_t)( vastai_op_entry_list_t *op_entry_list);   


typedef struct  {
	unsigned int die_id;
	int core_id;
	int vacc_id;
	
	uint32_t pid;
	void *vaccrt_open_handle;
    int vacc_init[MAX_VACC_NUM];  
	vaccrt_init_t					vaccrt_init;
	vaccrt_malloc_video_t			vaccrt_malloc_video;
	vaccrt_free_video_t				vaccrt_free_video;
	vaccrt_malloc_host_t			vaccrt_malloc_host;
	vaccrt_free_host_t				vaccrt_free_host;
	vaccrt_get_current_pid_t		vaccrt_get_current_pid;
	vaccrt_get_process_status_t		vaccrt_get_process_status;
	vaccrt_get_video_reserver_ddr_t vaccrt_get_video_reserver_ddr;
	vaccrt_program_create_t			vaccrt_program_create;
	vaccrt_program_get_function_t	vaccrt_program_get_function;
    vaccrt_program_get_function_list_t vaccrt_program_get_function_list;
    vaccrt_program_free_function_list_t vaccrt_program_free_function_list;
	vaccrt_create_stream_t 			vaccrt_create_stream;
	vaccrt_memcpy_t 				vaccrt_memcpy;
	vaccrt_malloc_inout_t 			vaccrt_malloc_inout;
	vaccrt_free_inout_t 			vaccrt_free_inout;	
	vaccrt_sync_run_stream_t 		vaccrt_sync_run_stream;
	vaccrt_destroy_stream_t 		vaccrt_destroy_stream;	
	vaccrt_program_destroy_t 		vaccrt_program_destroy;
	vaccrt_async_run_stream_t		vaccrt_async_run_stream;
    
    
}rt_functions;
static rt_functions s_rt_functions = {0};



static void vaccrt_load()
{
    
    char *error;
    rt_functions *ps_rt_functions = &s_rt_functions;

    char *level = NULL;
#if __linux__
	level = getenv("VAPP_LOG_LEVEL");
    if (NULL != level) {
        vapp_log_level = atoi(level);
    }
    else {
        vapp_log_level = VAPP_LOG_ERROR;
    }
#elif _WIN32
     size_t len;
    int err = _dupenv_s(&level, &len, "VAPP_LOG_LEVEL");
    if (err == 0 && level != NULL) {
        vapp_log_level = atoi(level);
        free(level);
        
    }
     else {
        vapp_log_level = VAPP_LOG_ERROR;
    }
#endif

    if (ps_rt_functions->vaccrt_open_handle) {
        return;
    }
    ps_rt_functions->vaccrt_open_handle = dlopen(LIB_VACCRT_PATH, RTLD_LAZY);
    if (!ps_rt_functions->vaccrt_open_handle) {
        VAPP_LOG(VAPP_LOG_ERROR, "failed to dlopen %s\n", LIB_VACCRT_PATH);
        exit(EXIT_FAILURE);
    } 
	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_init) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_init");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_init, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_malloc_video) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_malloc_video_32bit");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_malloc_video, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_free_video) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_free_video_32bit");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_free_video, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_get_current_pid) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_get_current_pid");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_get_current_pid, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_get_process_status) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_get_process_status");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_get_process_status, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_get_video_reserver_ddr) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_get_video_reserver_ddr");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_get_video_reserver_ddr, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_program_create) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_program_create");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_program_create, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}	

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_program_get_function) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_program_get_function");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_program_get_function, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}	

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_program_get_function_list) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_program_get_function_list");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_program_get_function_list, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}	    


	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_program_free_function_list) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_program_free_function_list");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_program_free_function_list, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}	

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_create_stream) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_create_stream");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_create_stream, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}	

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_memcpy) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_memcpy");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_memcpy, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}	

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_malloc_inout) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_malloc_inout");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_malloc_inout, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}		

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_free_inout) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_free_inout");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_free_inout, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}	

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_sync_run_stream) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_sync_run_stream");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_sync_run_stream, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}	

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_async_run_stream) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_async_run_stream");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_async_run_stream, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}		

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_destroy_stream) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_destroy_stream");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_destroy_stream, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}	

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_program_destroy) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_program_destroy");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_program_destroy, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_malloc_host) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_malloc_host");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_program_destroy, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}

	dlerror();
	*(void **) (&ps_rt_functions->vaccrt_free_host) = dlsym(ps_rt_functions->vaccrt_open_handle, "vaccrt_free_host");
	error = dlerror();
	if (error != NULL) {
		VAPP_LOG(VAPP_LOG_ERROR, "failed to dlsym vaccrt_program_destroy, err: %s\n", error);
		dlclose(ps_rt_functions->vaccrt_open_handle);
		exit(EXIT_FAILURE);
	}	
}


static int vast_vacc_init(uint32_t devID)
{
    rtError_t vaccRet;
#if __linux__
    pthread_mutex_lock(&vacc_init_mutex);
#elif _WIN32
    InitializeCriticalSection(&vacc_init_cs);
    EnterCriticalSection(&vacc_init_cs);
#endif
	if (s_rt_functions.vacc_init[devID] == 0) {
        vaccrt_load();
		vaccRet = s_rt_functions.vaccrt_init(devID);
		if(rtSuccess != vaccRet){
			VAPP_LOG(VAPP_LOG_ERROR, "%s: failed to init mem device, VaccError=%d\n", __func__, vaccRet);
			return VAPP_DEVICE_INIT_ERROR;
		}

		vaccRet = s_rt_functions.vaccrt_get_current_pid(devID, &s_rt_functions.pid);
		if(rtSuccess != vaccRet){
			VAPP_LOG(VAPP_LOG_ERROR, "%s: failed to get pid from vacc, VaccError=%d\n", __func__, vaccRet);
			return VAPP_DEVICE_INIT_ERROR;
		}
		VAPP_LOG(VAPP_LOG_INFO, "%s: current pid : %d \n", __func__, s_rt_functions.pid);
		s_rt_functions.vacc_init[devID] = 1;       
	}    
#if __linux__
    pthread_mutex_unlock(&vacc_init_mutex);
#elif _WIN32
    LeaveCriticalSection(&vacc_init_cs);
#endif     
    return 0;
}


VappStatus vastMalloc(uint32_t devID, void **devPtr, size_t size)
{
	rtError_t vaccRet;
    uint64_t soc_addr = 0;
    rt_functions *ps_rt_functions = &s_rt_functions;
    // ps_rt_functions->vacc_id = devID;
    if(size <= 0){
        VAPP_LOG(VAPP_LOG_ERROR, "%s: vastMalloc failed, size is 0 \n", __func__);
        return VAPP_MEMORY_ALLOCATION_ERR;
    }
	// if (ps_rt_functions->vacc_init[devID] == 0) {
    //     vaccrt_load();
	// 	vaccRet = ps_rt_functions->vaccrt_init(devID);
	// 	if(rtSuccess != vaccRet){
	// 		VAPP_LOG(VAPP_LOG_ERROR, "%s: failed to init mem device, VaccError=%d\n", __func__, vaccRet);
	// 		return VAPP_DEVICE_INIT_ERROR;
	// 	}

	// 	vaccRet = ps_rt_functions->vaccrt_get_current_pid(devID, &ps_rt_functions->pid);
	// 	if(rtSuccess != vaccRet){
	// 		VAPP_LOG(VAPP_LOG_ERROR, "%s: failed to get pid from vacc, VaccError=%d\n", __func__, vaccRet);
	// 		return VAPP_DEVICE_INIT_ERROR;
	// 	}
	// 	VAPP_LOG(VAPP_LOG_INFO, "%s: current pid : %d \n", __func__, ps_rt_functions->pid);
	// 	ps_rt_functions->vacc_init[devID] = 1;
	// } 
    vaccRet = vast_vacc_init(devID);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vast_vacc_init failed\n");
        return VAPP_RESIZE_NO_OPERATION_ERROR;     
    }     
    if(ps_rt_functions->vacc_init[devID] == 0){
        VAPP_LOG(VAPP_LOG_ERROR, " vastMalloc error, please call vastStreamCreate first.\n");
        return VAPP_MEMORY_ALLOCATION_ERR;
    }
    vaccRet = ps_rt_functions->vaccrt_malloc_video(devID, 4096, size, &soc_addr);
    //vaccRet = ps_rt_functions->vaccrt_malloc_inout(devID, 4096, size, &soc_addr);
    VAPP_LOG(VAPP_LOG_DEBUG, "%s: vastMalloc devid=%d, soc_addr=%"PRId64"\n", __func__, devID, soc_addr);
    // printf("%s %d %s here pthread_self()=%ld soc_addr=0x%lx size=%d\n",__FILE__,__LINE__,__FUNCTION__,pthread_self(), soc_addr, size);
    if (vaccRet != rtSuccess) {
        VAPP_LOG(VAPP_LOG_ERROR, "%s: vastMalloc failed, ret=%d\n", __func__, vaccRet);
        return VAPP_MEMORY_ALLOCATION_ERR;
    }    
    *devPtr = (void *)soc_addr;
    return VAPP_SUCCESS;
}

VappStatus vastMemcpy(uint32_t devID, void *dst, const void *src, size_t size, memcpy_direction_t direction)
{
    rtError_t vaccRet;
    rt_functions *ps_rt_functions = &s_rt_functions;
    if (ps_rt_functions->vacc_init[devID] == 1) {
        vaccRet = ps_rt_functions->vaccrt_memcpy(devID, src, size, dst,direction);
        if (vaccRet != rtSuccess) {
            VAPP_LOG(VAPP_LOG_ERROR, "%s: vastMemcpy failed, ret=%d\n", __func__, vaccRet);
            return VAPP_MEMCPY_ERROR;
        }  
    }else{
        return VAPP_DEVICE_STATUS_ERROR;
    }
    return VAPP_SUCCESS;
}

VappStatus vastMemcpyCross(uint32_t dstDevID, void *dst, uint32_t srcDevID, const void *src, size_t size)
{
    rtError_t vaccRet;
    rt_functions *ps_rt_functions = &s_rt_functions;
    if (ps_rt_functions->vacc_init[dstDevID] == 1 && ps_rt_functions->vacc_init[srcDevID] == 1) {
        void* tmpBuffer = NULL;
        vaccRet = ps_rt_functions->vaccrt_malloc_host(srcDevID, size, &tmpBuffer);
        if (vaccRet != rtSuccess) {
            VAPP_LOG(VAPP_LOG_ERROR, "%s: failed to allocate temporary buffer\n", __func__);
            return VAPP_MEMORY_ALLOCATION_ERR;
        }
        vaccRet = ps_rt_functions->vaccrt_memcpy(srcDevID, src, size, tmpBuffer, vastMemcpyDeviceToHost);
        if (vaccRet != rtSuccess) {
            VAPP_LOG(VAPP_LOG_ERROR, "%s: vastMemcpyCross failed, ret=%d\n", __func__, vaccRet);
            ps_rt_functions->vaccrt_free_host(srcDevID, tmpBuffer);
            return VAPP_MEMCPY_ERROR;
        }
        vaccRet = ps_rt_functions->vaccrt_memcpy(dstDevID, tmpBuffer, size, dst, vastMemcpyHostToDevice);
        if (vaccRet != rtSuccess) {
            VAPP_LOG(VAPP_LOG_ERROR, "%s: vastMemcpyCross failed, ret=%d\n", __func__, vaccRet);
            ps_rt_functions->vaccrt_free_host(srcDevID, tmpBuffer);
            return VAPP_MEMCPY_ERROR;
        }  
        vaccRet = ps_rt_functions->vaccrt_free_host(srcDevID, tmpBuffer);
        if (vaccRet != rtSuccess) {
            VAPP_LOG(VAPP_LOG_ERROR, "%s: failed to free temporary buffer\n", __func__);
            return VAPP_MEMORY_ALLOCATION_ERR;
        }
    } else {
        return VAPP_DEVICE_STATUS_ERROR;
    }
    return VAPP_SUCCESS;
}

VappStatus vastMemcpyCrossCPU(uint32_t dstDevID, void *dst, uint32_t srcDevID, const void *src, size_t size)
{
    rtError_t vaccRet;
    rt_functions *ps_rt_functions = &s_rt_functions;
    if (ps_rt_functions->vacc_init[dstDevID] == 1 && ps_rt_functions->vacc_init[srcDevID] == 1) {
        Vapp8u *tmp = (Vapp8u*)malloc(size);
        if (!tmp) {
            VAPP_LOG(VAPP_LOG_ERROR, "%s: failed to allocate temporary buffer\n", __func__);
            return VAPP_MEMORY_ALLOCATION_ERR;
        }
        vaccRet = ps_rt_functions->vaccrt_memcpy(srcDevID, src, size, tmp, vastMemcpyDeviceToHost);
        if (vaccRet != rtSuccess) {
            VAPP_LOG(VAPP_LOG_ERROR, "%s: vastMemcpyCrossCPU failed, ret=%d\n", __func__, vaccRet);
            free(tmp);
            return VAPP_MEMCPY_ERROR;
        }
        vaccRet = ps_rt_functions->vaccrt_memcpy(dstDevID, tmp, size, dst, vastMemcpyHostToDevice);
        if (vaccRet != rtSuccess) {
            VAPP_LOG(VAPP_LOG_ERROR, "%s: vastMemcpyCrossCPU failed, ret=%d\n", __func__, vaccRet);
            free(tmp);
            return VAPP_MEMCPY_ERROR;
        }  
        free(tmp);      
    }else{
        return VAPP_DEVICE_STATUS_ERROR;
    }
    return VAPP_SUCCESS;
}

VappStatus vastFree(uint32_t devID, const void *addr)
{
    rtError_t vaccRet;
    rt_functions *ps_rt_functions = &s_rt_functions;
    if (ps_rt_functions->vacc_init[devID] == 1) {
        vaccRet =  ps_rt_functions->vaccrt_free_video(devID, (uint64_t)addr);
        if (vaccRet != rtSuccess) {
            VAPP_LOG(VAPP_LOG_ERROR, "%s: vastFree failed, ret=%d\n", __func__, vaccRet);
            return VAPP_MEMFREE_ERROR;
        }          
    }else{
        return VAPP_DEVICE_STATUS_ERROR;
    }
    return VAPP_SUCCESS;    
}




VappStatus vastStreamCreate(uint32_t devID, uint32_t coreNum, void ** pStream,void *reserved)
{

    rtError_t vaccRet;
    //op_async_data *op_asynced = calloc(coreNum, sizeof(*op_asynced));
	VastStream * stream = (VastStream *)calloc(1, sizeof(VastStream));
    if (!stream) {
        return VAPP_ERROR;
    }
    stream->vacc_id = devID;
    stream->block_num = coreNum;
    stream->rt_stream = (RTStream *)calloc(coreNum, sizeof(RTStream));
    if (!stream->rt_stream) {
        return VAPP_NO_MEMORY_ERROR;
    }
    vaccRet = vast_vacc_init(devID);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vast_vacc_init failed\n");
        return VAPP_RESIZE_NO_OPERATION_ERROR;     
    }  
    if (coreNum > 12) {
        return VAPP_NO_MEMORY_ERROR;
    }
    for(int i = 0; i < (int)coreNum; i++){
        stream->rt_stream[i].stream_id = ++g_stream_id;
        stream->rt_stream[i].op_async = calloc(1, sizeof(op_async_data));
        if (!stream->rt_stream[i].op_async) {
            return VAPP_NO_MEMORY_ERROR;
        }
#if __linux__
        if (pthread_mutex_init(&stream->rt_stream[i].op_async->mutex, NULL) != 0) {
            fprintf(stderr, "Mutex initialization failed\n");
            return 1;
        }    
        pthread_cond_init(&stream->rt_stream[i].op_async->ready_cond, NULL);  
#elif _WIN32
        InitializeCriticalSection(&stream->rt_stream[i].op_async->cs_lock);
        InitializeConditionVariable(&stream->rt_stream[i].op_async->ready_cv);
#endif   
        
        stream->rt_stream[i].op_async->stream = stream;
        stream->rt_stream[i].op_async->pid = getpid();
        stream->rt_stream[i].op_async->block_num = 1;
        stream->rt_stream[i].op_async->block_id = i;
        stream->rt_stream[i].op_async->stream_id = stream->rt_stream[i].stream_id;
        vaccRet = vapp_create_stream(devID, stream->rt_stream[i].stream_id, stream->rt_stream[i].op_async);
        if(vaccRet!= rtSuccess){
            VAPP_LOG(VAPP_LOG_ERROR, "vapp_create_stream failed\n");
            free(stream->rt_stream[i].op_async);
            return VAPP_RESIZE_NO_OPERATION_ERROR;     
        }   
        stream->rt_stream[i].frame_num = 0;    
    }
    
    //stream->op_async = op_asynced;

    //stream->frame_num = 0;
    //stream->stream_id = g_stream_id++;


    vaccRet = vapp_register_op_stream(devID, stream, reserved);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vapp_create_stream failed\n");
        return VAPP_RESIZE_NO_OPERATION_ERROR;     
    }     
    *pStream = stream;

    return VAPP_SUCCESS;  	

}



VappStatus vastStreamSynchronize(uint32_t devID, vastStream_t vastStreamCtx)
{
    VastStream * stream = (VastStream *) vastStreamCtx;
    int time_out_cnt = 0;
    if(stream->issued < stream->block_num){
        printf("issued %d block num %d\n", stream->issued, stream->block_num);
        return VAPP_ERROR;
    }
    // printf("%s %d %s wait pthread_self()=%ld stream_id=%d hope_op_times=%d\n",__FILE__,__LINE__,__FUNCTION__,pthread_self(), stream->op_async->stream_id,stream->op_async->hope_op_times);
    //start = time_usec();	
#if __linux__    
    for(int i = 0; i < stream->block_num; i++){
        pthread_mutex_lock(&stream->rt_stream[i].op_async->mutex);
        while(!stream->rt_stream[i].op_async->frame_ready){
            int cond_ret = 0;
            int timeout_second = 5;
            struct timespec abstime;
            struct timespec now;            
            clock_gettime(CLOCK_REALTIME, &now);
            abstime.tv_nsec = now.tv_nsec ;
            abstime.tv_sec  = now.tv_sec + timeout_second;              
            cond_ret = pthread_cond_timedwait(&stream->rt_stream[i].op_async->ready_cond, &stream->rt_stream[i].op_async->mutex, &abstime);
            if (cond_ret == ETIMEDOUT) {
                fprintf(stderr, "\n\n==== stream id %d time out ==== cond_ret %d\n", stream->rt_stream[i].stream_id, cond_ret);
                print_op_params(&stream->rt_stream[i]);
                time_out_cnt++;
                break;   
            } else if (cond_ret != 0) {
                fprintf(stderr, "wait frameready failed:%s\n", strerror(errno));
                print_op_params(&stream->rt_stream[i]);
                pthread_mutex_unlock(&stream->rt_stream[i].op_async->mutex);
                return -1;
            }                  
            //pthread_cond_wait(&stream->rt_stream[i].op_async->ready_cond, &stream->rt_stream[i].op_async->mutex);
        }
        stream->rt_stream[i].op_async->frame_ready = 0;
        pthread_mutex_unlock(&stream->rt_stream[i].op_async->mutex);
    }
#elif _WIN32
    for(int i = 0; i < stream->block_num; i++){
        EnterCriticalSection(&stream->rt_stream[i].op_async->cs_lock);
        while(!stream->rt_stream[i].op_async->frame_ready){
             SleepConditionVariableCS(&stream->rt_stream[i].op_async->ready_cv, &stream->rt_stream[i].op_async->cs_lock, 50000);
             if (ERROR_TIMEOUT == GetLastError()) {
                 fprintf(stderr, "\n\n==== stream id %d time out ====\n", stream->rt_stream[i].stream_id);
                 print_op_params(&stream->rt_stream[i]);
                 time_out_cnt++;
                 break;
             }
        }
        stream->rt_stream[i].op_async->frame_ready = 0;
        LeaveCriticalSection(&stream->rt_stream[i].op_async->cs_lock);
    }


#endif
    if (time_out_cnt > 0) {
        while (1) {
            vapp_usleep(1000);
        }
        return VAPP_ERROR;
    }
    // printf("%s %d %s wait pthread_self()=%ld stream_id=%d hope_op_times=%d\n",__FILE__,__LINE__,__FUNCTION__,pthread_self(), stream->op_async->stream_id,stream->op_async->hope_op_times);
//     for(int i = 0; i < stream->block_num; i++){
// #ifdef __linux__
//             pthread_mutex_lock(&stream->rt_stream[i].op_async->mutex);
// #elif _WIN32
//             EnterCriticalSection(&stream->rt_stream[i].op_async->cs_lock);
// #endif
//             stream->rt_stream[i].op_async->frame_ready = 0;
// #ifdef __linux__
//             pthread_mutex_unlock(&stream->rt_stream[i].op_async->mutex);
// #elif _WIN32
//             LeaveCriticalSection(&stream->rt_stream[i].op_async->cs_lock);
// #endif     
//     }

    // end = time_usec();
    // elapsed = end - start;
    // printf("wait for  elapsed %ld us\n", elapsed); 	
    return VAPP_SUCCESS;  	

}

VappStatus vastStreamDestroy(uint32_t devID, vastStream_t vastStreamCtx)
{
    rtError_t vaccRet;
    VastStream * stream = (VastStream *) vastStreamCtx;
    for(int i = 0; i < stream->block_num; i++){
        vaccRet = vapp_destroy_stream(devID, stream->rt_stream[i].stream_id);
        if(vaccRet!= rtSuccess){
            VAPP_LOG(VAPP_LOG_ERROR, "vapp_destroy_stream failed.\n");
            return VAPP_RESIZE_NO_OPERATION_ERROR;
        }  
#if __linux__
        pthread_mutex_destroy(&stream->rt_stream[i].op_async->mutex);
        pthread_cond_destroy(&stream->rt_stream[i].op_async->ready_cond);
#elif _WIN32
        DeleteCriticalSection(&stream->rt_stream[i].op_async->cs_lock);
#endif     
        if(stream->rt_stream[i].op_async){
            free(stream->rt_stream[i].op_async);
            stream->rt_stream[i].op_async = NULL;
        }  
        if(stream->rt_stream[i].op_cfg){
            free(stream->rt_stream[i].op_cfg);
            stream->rt_stream[i].op_cfg = NULL;
        }       
    }
    if(stream->map1_buffer){
        vastFree(devID, stream->map1_buffer);
        stream->map1_buffer = NULL;
    }
    if(stream->map2_buffer){
        vastFree(devID, stream->map2_buffer);
        stream->map2_buffer = NULL;
    }

    if(stream->hqdn3d_buffer){
        vastFree(devID, stream->hqdn3d_buffer);
        stream->hqdn3d_buffer = NULL;
    }  

    if(stream->size_roi){
        free(stream->size_roi);
        stream->size_roi = NULL;
    }     
    vapp_free_op_entrylist(devID, &stream->op_entry_list);
    if(stream->op_reg.elf_file){
        free(stream->op_reg.elf_file);
        stream->op_reg.elf_file = NULL;
    }
    if(stream->rt_stream){
        free(stream->rt_stream);
        stream->rt_stream = NULL;
    }
    if(stream){
        free(stream);
    }

    return VAPP_SUCCESS;        	
}

int vapp_check_status(uint32_t dev_id)
{
    return s_rt_functions.vacc_init[dev_id];
}

static int file_size_bytes(FILE *fp)
{
    int cur;
    int size;

    cur = ftell(fp);
    fseek(fp, 0, 2);

    size = ftell(fp);
    fseek(fp, cur, 0);

    return size;
}

rtError_t custom_op_register(int vacc_id, uint32_t type,  char * elf_file,  char *op_id)
{
    rtError_t vaccRet;
    if(!elf_file){
        fprintf(stderr, "Invalid Elf file. \n");
        return -1;
    }
    rt_functions *ps_rt_functions = &s_rt_functions;
    FILE *p_file_elf = fopen(elf_file, "rb");
    int32_t size_file;
    if (NULL == p_file_elf) {
        fprintf(stderr, "Failed to open Elf file %s!\n", elf_file);
        return -1;
    }

    size_file = file_size_bytes(p_file_elf);

    if (size_file <= 0) {
        fclose(p_file_elf);
        fprintf(stderr,"File size error!\n");
        return -1;
    }

    char *p_elf_ary = (char *)malloc(size_file);

    if (NULL == p_elf_ary) {
        fclose(p_file_elf);
        fprintf(stderr,"Failed to allocate %d bytes for Elf!\n", size_file);
        return -1;
    }

    size_t read_bytes = fread((void *)&p_elf_ary[0], 1, size_file, p_file_elf);

    fclose(p_file_elf);

    if (read_bytes != size_file) {
        free(p_elf_ary);
        fprintf(stderr,"Failed to read Elf file: %s (not enough bytes)!\n", elf_file);
        return -1;
    }

    vastai_op_version_t op_version;

    // struct stat fileInfo;
    // if (stat(elf_file, &fileInfo) == -1) {
    //     perror("Error getting file information");
    //     return -1;
    // }
    // snprintf(vapp_context->uni_code, sizeof(uni_code), "%llx", fileInfo.st_ino);
    //printf("Inode number: %x hex %s\n", (unsigned long)fileInfo.st_ino, vapp_context->uni_code);   
    //md5((const uint8_t *)p_elf_ary,size_file, op_id);

	//md5_buffer(p_elf_ary, size_file, (unsigned char *)op_id);
    struct stat fileInfo;
    if (stat(elf_file, &fileInfo) == -1) {
        perror("Error getting file information");
        return -1;
    }
     
    snprintf(op_id, 8, "%lx", fileInfo.st_ino);  
    vaccRet = ps_rt_functions->vaccrt_program_create(vacc_id, p_elf_ary, size_file, op_id, type, &op_version);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vaccrt_program_create failed, size %d.\n", size_file);
        return vaccRet;              
    }     
    free(p_elf_ary);
    return rtSuccess;
}


rtError_t op_register(int vacc_id, OpRegister *reg_value)
{
    rtError_t vaccRet;
    if(!reg_value->elf_file){
        fprintf(stderr, "Invalid Elf file. \n");
        return -1;
    }
   
    rt_functions *ps_rt_functions = &s_rt_functions;
    FILE *p_file_elf = fopen(reg_value->elf_file, "rb");
    int32_t size_file;
    if (NULL == p_file_elf) {
        fprintf(stderr, "Failed to open Elf file %s!\n", reg_value->elf_file);
        return -1;
    }

    size_file = file_size_bytes(p_file_elf);

    if (size_file <= 0) {
        fclose(p_file_elf);
        fprintf(stderr,"File size error!\n");
        return -1;
    }

    char *p_elf_ary = (char *)malloc(size_file);

    if (NULL == p_elf_ary) {
        fclose(p_file_elf);
        fprintf(stderr,"Failed to allocate %d bytes for Elf!\n", size_file);
        return -1;
    }

    size_t read_bytes = fread((void *)&p_elf_ary[0], 1, size_file, p_file_elf);

    fclose(p_file_elf);

    if (read_bytes != size_file) {
        free(p_elf_ary);
        fprintf(stderr,"Failed to read Elf file: %s (not enough bytes)!\n", reg_value->elf_file);
        return -1;
    }

    vastai_op_version_t op_version;

#ifdef __linux__
    struct stat fileInfo;
    if (stat(reg_value->elf_file, &fileInfo) == -1) {
        perror("Error getting file information");
        return -1;
    }
    uint32_t xxhash32_value = xxhash32(p_elf_ary, size_file,0);
    snprintf(reg_value->uni_code, sizeof(reg_value->uni_code), "%lx", xxhash32_value);  
#elif _WIN32
    if (!op_register_once) {
        GUID guid;
        CoCreateGuid(&guid);
        // snprintf(reg_value->uni_code, sizeof(reg_value->uni_code), "%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x", guid.Data1, guid.Data2, guid.Data3,
        //     guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
        snprintf(uni_code_once, sizeof(reg_value->uni_code), "%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x", guid.Data1, guid.Data2, guid.Data3,
            guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);              
        op_register_once = 1;
    }
    strcpy_s(reg_value->uni_code,sizeof(reg_value->uni_code), uni_code_once);

#endif     
     
    // for (size_t i = 0; i < strlen((char *)uuid); i++) {
    //     snprintf((char *)&reg_value->uni_code[i*2], 3, "%02x", uuid[i]);
    // }      
	//md5_buffer(p_elf_ary, size_file, (unsigned char *)reg_value->uni_code);

    vaccRet = ps_rt_functions->vaccrt_program_create(vacc_id, p_elf_ary, size_file, reg_value->uni_code, reg_value->dsp_type, &op_version);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vaccrt_program_create failed, size %d.\n", size_file);
        return vaccRet;              
    }     
  
    free(p_elf_ary);
    return rtSuccess;
}


rtError_t vapp_get_op_entry(uint32_t dev_id, const char *op_name, uint32_t *func_entry)
{
    rtError_t vaccRet = rtSuccess;
    rt_functions *ps_rt_functions = &s_rt_functions;
    vaccRet = ps_rt_functions->vaccrt_program_get_function(dev_id, op_name, func_entry);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vaccrt_program_get_function failed, op_name %s\n", op_name);
        return vaccRet;              
    }  
    return vaccRet; 
}

rtError_t vapp_get_op_entrylist(uint32_t dev_id, const char *id_name, vastai_op_entry_list_t *op_entry_list)
{
    rtError_t vaccRet = rtSuccess;
    rt_functions *ps_rt_functions = &s_rt_functions;
    vaccRet = ps_rt_functions->vaccrt_program_get_function_list(dev_id, id_name, op_entry_list);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vaccrt_program_get_function failed, op_name %s\n", id_name);
        return vaccRet;              
    }    
    return vaccRet; 
}


rtError_t vapp_free_op_entrylist(uint32_t dev_id, vastai_op_entry_list_t *op_entry_list)
{
    rtError_t vaccRet = rtSuccess;
    rt_functions *ps_rt_functions = &s_rt_functions;
    vaccRet = ps_rt_functions->vaccrt_program_free_function_list(op_entry_list);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vaccrt_program_free_function_list failed.\n");
        return vaccRet;              
    }    
    return vaccRet; 
}


rtError_t vapp_op_execute(uint32_t dev_id, const op_info_t *op_info, uint32_t op_num,
                                                 uint32_t timeout, uint32_t *error_no)
{
    rtError_t vaccRet;
    rt_functions *ps_rt_functions = &s_rt_functions;
	int64_t start, end, elapsed;
	start = time_usec();	
    vaccRet = ps_rt_functions->vaccrt_sync_run_stream(dev_id, op_info, op_num, timeout, error_no);
    end = time_usec();
    elapsed = end - start;
    printf("run elapsed %"PRId64" us\n", elapsed); 	
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vaccrt_sync_run_stream failed.\n");
        return vaccRet;              
    } 
    return rtSuccess;
}

rtError_t vapp_op_execute_async(uint32_t dev_id, const op_info_t *op_info, uint32_t op_num, uint32_t stream_id, uint32_t data_id)
{
	/* async run stream */
	run_stream_t run_stream = {0};
	run_stream.stream_id = stream_id;
	run_stream.data_id = data_id;	
    rtError_t vaccRet;
	// int64_t start, end, elapsed;
	// start = time_usec();		
    rt_functions *ps_rt_functions = &s_rt_functions;
    vaccRet = ps_rt_functions->vaccrt_async_run_stream(dev_id, &run_stream, op_info, op_num);
    // end = time_usec();
    // elapsed = end - start;
    //printf("async run elapsed %ld us\n", elapsed); 	
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vaccrt_async_run_stream failed.\n");
        return vaccRet;              
    } 
    return rtSuccess;
}

static int stream_callback(uint32_t dev_id, get_desc_t *get_desc, void *func_arg)
{
	//int i = 0;
	op_async_data *async_data =(op_async_data *)func_arg;
    //VastStream* stream = (VastStream*)async_data->stream;
	// int64_t start, end, elapsed;
	// start = time_usec();
#ifdef __linux__
    pthread_mutex_lock(&async_data->mutex);
#elif _WIN32
    EnterCriticalSection(&async_data->cs_lock);
#endif
    if(get_desc->data_id == async_data->hope_op_times && get_desc->stream_id == async_data->stream_id){
        async_data->flag++;
    }

    // if (get_desc->error_code != 0) {
    //     printf("error_code %d ------------ \n", get_desc->error_code);
    //     if(stream){
    //         print_op_params(&stream->rt_stream[async_data->block_id]);
    //     }        

    //     vapp_usleep(1000);
    // }

    async_data->frame_ready = (async_data->flag == async_data->block_num);
    // printf(" pthread=%ld  stream_id=%d data_id=%d  local stream_id=%d hope_op_times=%d async_data->flag %d async_data->block_num %d\n",
    // pthread_self(), get_desc->stream_id,get_desc->data_id,async_data->stream_id,async_data->hope_op_times, async_data->flag, async_data->block_num);
    if(async_data->frame_ready == 1){
        async_data->flag = 0;
#ifdef __linux__
        pthread_cond_signal(&async_data->ready_cond);
#elif _WIN32
        WakeConditionVariable(&async_data->ready_cv);
#endif        
    }    

#ifdef __linux__
    pthread_mutex_unlock(&async_data->mutex);
#elif _WIN32
    LeaveCriticalSection(&async_data->cs_lock);
#endif 

	// end = time_usec();
    // elapsed = end - start;
    // printf("callback elapsed %ld us\n", elapsed); 		
    // TODO smoething
    return 0;
}

rtError_t vapp_create_stream(uint32_t dev_id, uint32_t stream_id, void *args)
{
	rtError_t vaccRet;
	rt_functions *ps_rt_functions = &s_rt_functions;
    create_stream_t create_stream = {0};
    create_stream.stream_id = stream_id;
    create_stream.vdsp_balance_mode = 0;
    create_stream.callback.dev_id = dev_id;
    create_stream.callback.func_arg = args;
    create_stream.callback.func_get_output = stream_callback;
    vaccRet = ps_rt_functions->vaccrt_create_stream(dev_id, &create_stream);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vaccrt_create_stream failed.\n");
        return vaccRet;              
    } 
    return rtSuccess;
}

rtError_t vapp_destroy_stream(uint32_t dev_id, uint32_t stream_id)
{
	rtError_t vaccRet;
	rt_functions *ps_rt_functions = &s_rt_functions;
    vaccRet = ps_rt_functions->vaccrt_destroy_stream(dev_id, stream_id);
    if(vaccRet!= rtSuccess){
        VAPP_LOG(VAPP_LOG_ERROR, "vaccrt_create_stream failed.\n");
        return vaccRet;              
    } 
    return rtSuccess;
}



// void test_register()
// {
//     char uni_code[32];
//     uint32_t func_entry;

//     rtError_t vaccRet;
//     vaccrt_load();
// 	s_rt_functions.vaccrt_init(0);
//     vaccRet = custom_op_register(0, 0, "/opt/vastai/vaststream/lib/op/ext_op/video/simple_rotate_ext_op", uni_code);
//     if(vaccRet!= rtSuccess){
//         //VAPP_LOG(VAPP_LOG_ERROR, "custom_op_register failed elf_file \n", );
//         return VAPP_REGISTER_ERROR;              
//     }
//     int op_name_len = sizeof(uni_code) + strlen(ROTATE_OP_FUNC) + 2;
//     char *op_name = malloc(op_name_len);
//     snprintf(op_name, op_name_len,"%s:%s", uni_code, ROTATE_OP_FUNC);
//     vaccRet = vapp_get_op_entry(0, op_name, &func_entry);
//     if(vaccRet!= rtSuccess){
//         VAPP_LOG(VAPP_LOG_ERROR, "vapp_get_op_entry failed, %s \n", op_name);
//         free(op_name);
//         return VAPP_GET_ENTRY_ERROR;
//     }
// 	free(op_name);
// }
