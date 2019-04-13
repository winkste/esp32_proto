/*****************************************************************************************
* FILENAME :        myVersion.h          
*
* DESCRIPTION :
*       Header file to define project specific settings
*
* PUBLIC FUNCTIONS :
*       N/A
*
* NOTES :
*
* Copyright (c) [2017] [Stephan Wink]
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
vAUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
* 
* AUTHOR :    Stephan Wink        START DATE :    07.03.2019
*
*****************************************************************************************/
#ifndef MYVERSION_H
#define MYVERSION_H

/****************************************************************************************/
/* Imported header files: */

#include "esp_err.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum, struct, union): */

/****************************************************************************************/
/* Global data allusions (allows type checking, definition in c file): */

/****************************************************************************************/
/* Global function prototypes: */

/**---------------------------------------------------------------------------------------
 * @brief     Initialization function for myVersion
 * @author    S. Wink
 * @date      08. Mar. 2019
 * @return    esp error code (ESP_OK)
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t myVersion_Initialize_st(void);

/**---------------------------------------------------------------------------------------
 * @brief     Provides read access to the firmware identifier
 * @author    S. Wink
 * @date      08. Mar. 2019
 * @return    pointer to the firmware idenitfier
*//*-----------------------------------------------------------------------------------*/
extern const char * myVersion_GetFwIdentifier_cch(void);

/**---------------------------------------------------------------------------------------
 * @brief     Provides read access to the firmware version
 * @author    S. Wink
 * @date      08. Mar. 2019
 * @return    pointer to the firmware version
*//*-----------------------------------------------------------------------------------*/
extern const char * myVersion_GetFwVersion_cch(void);

/**---------------------------------------------------------------------------------------
 * @brief     Provides read access to the firmware description
 * @author    S. Wink
 * @date      08. Mar. 2019
 * @return    pointer to the firmware description
*//*-----------------------------------------------------------------------------------*/
extern const char * myVersion_GetFwDescription_cch(void);

#endif /* MYVERSION_H */
/****************************************************************************************/

