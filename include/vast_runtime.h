
#ifndef _VAST_RUNTIME_H_
#define _VAST_RUNTIME_H_
#include <stdint.h> 
#include <stddef.h>
/**
 * \brief ddr memory copy
 *
 * \param devID : device ID
 * \param addr_from The source address
 * \param size The data size
 * \param addr_to The target address
 * \param direction Memory copy direction
 * \return rtError_t, 0(rtSuccess) means success.
 */
#if defined(WIN32) || defined(_WIN32) || defined(_WIN32__) || defined(_WIN64)
#define attr_visibility __declspec(dllexport)
#else
#define attr_visibility __attribute__((visibility("default")))
#endif
/**
 * VAST memory copy types
 */
typedef enum memcpy_direction {
    vastMemcpyHostToDevice      = 0, /**< Host   -> Host */
    vastMemcpyDeviceToHost      = 1, /**< Host   -> Device */
    vastMemcpyHostToHost        = 2, /**< Device -> Host */
    vastMemcpyDeviceToDevice    = 3, /**< Device -> Device */
} memcpy_direction_t;

#ifdef __cplusplus
extern "C" {
#endif
typedef void * vastStream_t; 

/**
 * \brief Allocate memory for video in given device
 *
 * \param devID The device id
 * \param align The alignment
 * \param size The wanted size
 * \param addr The allocated address
 * \return rtError_t, 0(rtSuccess) means success.
 */                                       
attr_visibility VappStatus vastMalloc(uint32_t devID, void **devPtr, size_t size);

/**
 * \brief ddr memory copy
 *
 * \param devID : device ID
 * \param addr_from The source address
 * \param size The data size
 * \param addr_to The target address
 * \param direction Memory copy direction
 * \return rtError_t, 0(rtSuccess) means success.
 */
attr_visibility VappStatus vastMemcpy(uint32_t devID, void *dst, const void *src, size_t size, memcpy_direction_t direction);

/**
 * \brief ddr memory copy cross different devices
 *
 * \param dstDevID : dst device ID
 * \param dst The source address
 * \param SrcDevID : dst device ID
 * \param src The source address
 * \param size The data size
 * \return rtError_t, 0(rtSuccess) means success.
 */
attr_visibility VappStatus vastMemcpyCross(uint32_t dstDevID, void *dst, uint32_t srcDevID, const void *src, size_t size);

/**
 * \brief Free memory for video in given device
 *
 * \param devID The device id
 * \param addr The allocated address
 * \return rtError_t, 0(rtSuccess) means success.
 */

attr_visibility VappStatus vastFree(uint32_t devID, const void *addr);

/**
 * \brief Create an asynchronous stream.
 *
 * \param devID The device id
 * \param pStream  Pointer to new stream identifier
 * \return rtError_t, 0(rtSuccess) means success.
 */

attr_visibility VappStatus vastStreamCreate(uint32_t devID, uint32_t coreNum, vastStream_t* pStream,void *reserved);


/**
 * \brief Synchronize a given stream
 *
 *
 * \param vastStreamCtx Stream handle.
 */

attr_visibility VappStatus vastStreamSynchronize(uint32_t devID, vastStream_t vastStreamCtx);




/**
 * \brief Destroys and cleans up an asynchronous stream.
 *
 *
 * \param vastStreamCtx Stream handle.
 */
 
attr_visibility VappStatus vastStreamDestroy(uint32_t devID, vastStream_t vastStreamCtx);
#ifdef __cplusplus
}
#endif
#endif