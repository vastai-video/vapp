 
#ifndef VAST_VAPP_H
#define VAST_VAPP_H

/**
 * \file vapp.h
 * Main include file for NPP library.
 *      Aggregates all other include files.
 */

/**
 * Major version
 */
#define VAPP_VER_MAJOR 12
/**
 * Minor version
 */
#define VAPP_VER_MINOR 2
/**
 * Patch version
 */
#define VAPP_VER_PATCH 3
/**
 * Build version
 */
#define VAPP_VER_BUILD 2

/**
 * Full version
 */
#define VAPP_VERSION (VAPP_VER_MAJOR * 1000 +     \
                     VAPP_VER_MINOR *  100 +     \
                     VAPP_VER_PATCH)

/**
 * Major version
 */
#define VAPP_VERSION_MAJOR  VAPP_VER_MAJOR
/**
 * Minor version
 */
#define VAPP_VERSION_MINOR  VAPP_VER_MINOR
/**
 * Patch version
 */
#define VAPP_VERSION_PATCH  VAPP_VER_PATCH
/**
 * Build version
 */
#define VAPP_VERSION_BUILD  VAPP_VER_BUILD

#include <vappdefs.h>
// #include <vappcore.h>
#include <vappi.h>
// #include <vapps.h>
#endif //VAST_VAPP_H
