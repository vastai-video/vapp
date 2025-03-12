#pragma once

 #ifdef _WIN32
 #include "Windows.h"
 #include <stdint.h>
#define RTLD_LAZY	0x00001
int vapp_usleep(unsigned usec);

#define pthread_self() NULL
#define getpid() GetCurrentProcessId();
static inline HMODULE win32_dlopen(const char* name)
 {
	#if _WIN32_WINNT < 0x0602
		    // Need to check if KB2533623 is available
		if (!GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetDefaultDllDirectories")) {
		HMODULE module = NULL;
		wchar_t* path = NULL, *name_w = NULL;
		DWORD pathlen;
		if (utf8towchar(name, &name_w))
			 goto exit;
		path = (wchar_t*)av_mallocz_array(MAX_PATH, sizeof(wchar_t));
		        // Try local directory first
			pathlen = GetModuleFileNameW(NULL, path, MAX_PATH);
		pathlen = wcsrchr(path, '\\') - path;
		if (pathlen == 0 || pathlen + wcslen(name_w) + 2 > MAX_PATH)
			 goto exit;
		path[pathlen] = '\\';
		wcscpy(path + pathlen + 1, name_w);
		module = LoadLibraryExW(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
		if (module == NULL) {
			            // Next try System32 directory
			pathlen = GetSystemDirectoryW(path, MAX_PATH);
			if (pathlen == 0 || pathlen + wcslen(name_w) + 2 > MAX_PATH)
			 goto exit;
			path[pathlen] = '\\';
			wcscpy(path + pathlen + 1, name_w);
			module = LoadLibraryExW(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
			
		}
		if(module == NULL){
			module = LoadLibraryExW(name_w, NULL, 0);
		}		
		 exit:
		av_free(path);
		av_free(name_w);
		return module;
		
	}
	 #endif
		 #ifndef LOAD_LIBRARY_SEARCH_APPLICATION_DIR
		 #   define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x00000200
		 #endif
		 #ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
		 #   define LOAD_LIBRARY_SEARCH_SYSTEM32        0x00000800
		 #endif
		
		return LoadLibraryExA(name, NULL, 0);
	
		}

#define dlopen(name, flags) win32_dlopen(name)
 #define dlclose FreeLibrary
 #define dlsym GetProcAddress
 #define dlerror() NULL
#elif __linux__

#endif