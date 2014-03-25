#ifndef _GET_MEMORY_SIZE_H_
#define _GET_MEMORY_SIZE_H_ 1

/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

#if defined(_WIN32)
  #include <Windows.h>
#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/param.h>
    #if defined(BSD)
      #include <sys/sysctl.h>
    #endif
#endif

size_t getMemorySize();

#endif /* _GET_MEMORY_SIZE_H_ */
