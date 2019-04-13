/*****************************************************************************************
* FILENAME :        myWifi.h
*
* DESCRIPTION :
*      Header file for wifi control module
*
* Date: 24. January 2019
*
* NOTES :
*
* Copyright (c) [2019] [Stephan Wink]
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
*****************************************************************************************/
#ifndef MAIN_MYWIFI_H_
#define MAIN_MYWIFI_H_

/****************************************************************************************/
/* Imported header files: */

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */
typedef struct myWifi_parameter_tag
{
    void (*eventWifiStarted_ptrs)(void);
    void (*eventWifiStartedSta_ptrs)(void);
    void (*eventWifiDisconnected_ptrs)(void);
}myWifi_parameter_t;
/****************************************************************************************/
/* Global function definitions: */

/**---------------------------------------------------------------------------------------
 * @brief     General initialization of wifi module. This function has to be
 *              executed before the other ones.
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     param_stp           wifi parameter structure
*//*-----------------------------------------------------------------------------------*/
extern void myWifi_InitializeWifi_vd(myWifi_parameter_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Function to initialize WIFI to access point mode
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
extern void myWifi_InitializeWifiSoftAp_vd(void);

/**---------------------------------------------------------------------------------------
 * @brief     Function to initialize WIFI to station mode
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
extern void myWifi_InitializeWifiSta_vd(void);

/**---------------------------------------------------------------------------------------
 * @brief     Function to register WIFI commands
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
extern void myWifi_RegisterWifiCommands(void);
/****************************************************************************************/
/* Global data definitions: */

#endif /* MAIN_MYWIFI_H_ */
