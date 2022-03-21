#include "ras.h"

/* this file provides platform specific implementation
 * 
 * Anything added to the file which replaces this should be platform
 * specific. If it is not, please consider adding it to obmc-ras.c
 * or even better, having it as a library of its own.
 */

/* Have a dummy define to stop the compiler from complaining if
 * we decide to use the stubbed library */
void _ras_dummy(void)
{

}
