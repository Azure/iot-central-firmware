// Copyright (c) 2019 Microsoft. All rights reserved.

// Update Notes: Reduced .bss usage, fixed issues with latest driver

/*
 * Amazon FreeRTOS Secure Sockets for STM32L4 Discovery kit IoT node V1.0.0 Beta 1
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */


/**
 * @file aws_secure_sockets.c
 * @brief WiFi and Secure Socket interface implementation for ST board.
 */


#include <stdbool.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "cmsis_os.h"


/* TLS includes. */
#include "aws_tls.h"

/* Socket interface includes. */
#include "aws_secure_sockets.h"

/* Socket and BG96 modem includes. */
#include "../../../lib/iotc/bg96_modem.h"
#include "com_sockets_ip_modem.h"
#include "com_sockets_net_compat.h"
#include "com_sockets_err_compat.h"
#include "cellular_service.h"



/**
 * @brief A Flag to indicate whether or not a socket is
 * secure i.e. it uses TLS or not.
 */
#define stsecuresocketsSOCKET_SECURE_FLAG          ( 1UL << 0 )

/**
 * @brief A flag to indicate whether or not a socket is closed
 * for receive.
 */
#define stsecuresocketsSOCKET_READ_CLOSED_FLAG     ( 1UL << 1 )

/**
 * @brief A flag to indicate whether or not a socket is closed
 * for send.
 */
#define stsecuresocketsSOCKET_WRITE_CLOSED_FLAG    ( 1UL << 2 )

/**
 * @brief The maximum timeout accepted by the Inventek module.
 *
 * This value is dictated by the hardware and should not be
 * modified.
 */
#define stsecuresocketsMAX_TIMEOUT                 ( 30000 )

/**
 * @brief Delay used between network read attempts when effecting a receive timeout.
 *
 * Receive sockets are either blocking until the receive buffer is filled up to
 * the number of bytes set in the call or the timeout period is reached.
 * To avoid blocking any other task of same or lower priority this constant sets
 * between each read attempt during the receive timeout period.
 */
#define stsecuresocketsFIVE_MILLISECONDS           ( pdMS_TO_TICKS( 5 ) )

/**
 * @brief Minimum Receive and Send Timeout values.
 *
 * Receive sockets are either blocking until the receive buffer is filled up to
 * the number of bytes set in the call or the timeout period is reached.
 * Send sockets are either blocking until the number of bytes to send in
 * the buffer are entirely sent or the timeout period is reached.
 */
#define stsecuresocketsHUNDRED_MILLISECONDS        ( pdMS_TO_TICKS( 100 ))
#define stsecuresocketsTWO_HUNDRED_MILLISECONDS    ( pdMS_TO_TICKS( 200 ))

#define minMESSAGE_SIZE                                (5)

/**
 * @brief Represents a secure socket.
 */
typedef struct STSecureSocket
{
    uint8_t ucInUse;                    /**< Tracks whether the socket is in use or not. */
    uint32_t ulFlags;                   /**< Various properties of the socket (secured etc.). */
    uint32_t ulSendTimeout;             /**< Send timeout. */
    uint32_t ulReceiveTimeout;          /**< Receive timeout. */
    char * pcDestination;               /**< Destination URL. Set using SOCKETS_SO_SERVER_NAME_INDICATION option in SOCKETS_SetSockOpt function. */
    void * pvTLSContext;                /**< The TLS Context. */
    char * pcServerCertificate;         /**< Server certificate. Set using SOCKETS_SO_TRUSTED_SERVER_CERTIFICATE option in SOCKETS_SetSockOpt function. */
    uint32_t ulServerCertificateLength; /**< Length of the server certificate. */
    int32_t ST_socket_handle;           /**< This is the socket handle created by the BG96 modem*/
} STSecureSocket_t;

// Support for ST sockets

//End
/**
 * @brief Secure socket objects.
 *
 * An index in this array is returned to the user from SOCKETS_Socket
 * function.
 */
static STSecureSocket_t xSockets[ CELLULAR_MAX_SOCKETS ];

/**
 * @brief Get a free socket from the free socket pool.
 *
 * Iterates over the xSockets array to see if it can find
 * a free socket. A free or unused socket is indicated by
 * the zero value of the ucInUse member of STSecureSocket_t.
 *
 * @return Index of the socket in the xSockets array, if it is
 * able to find a free socket, SOCKETS_INVALID_SOCKET otherwise.
 */
static uint32_t prvGetFreeSocket( void );

/**
 * @brief Returns the socket back to the free socket pool.
 *
 * Marks the socket as free by setting ucInUse member of the
 * STSecureSocket_t structure as zero.
 */
static void prvReturnSocket( uint32_t ulSocketNumber );

/**
 * @brief Checks whether or not the provided socket number is valid.
 *
 * Ensures that the provided number is less than CELLULAR_MAX_SOCKETS
 * and the socket is "in-use" i.e. ucInUse is set to non-zero in the
 * socket structure.
 *
 * @param[in] ulSocketNumber The provided socket number to check.
 *
 * @return pdTRUE if the socket is valid, pdFALSE otherwise.
 */
static BaseType_t prvIsValidSocket( uint32_t ulSocketNumber );

/**
 * @brief Sends the provided data over WiFi.
 *
 * @param[in] pvContext The caller context. Socket number in our case.
 * @param[in] pucData The data to send.
 * @param[in] xDataLength Length of the data.
 *
 * @return Number of bytes actually sent if successful, SOCKETS_SOCKET_ERROR
 * otherwise.
 */
static BaseType_t prvNetworkSend( void * pvContext,
                                  const unsigned char * pucData,
                                  size_t xDataLength );

/**
 * @brief Receives the data over WiFi.
 *
 * @param[in] pvContext The caller context. Socket number in our case.
 * @param[out] pucReceiveBuffer The buffer to receive the data in.
 * @param[in] xReceiveBufferLength The length of the provided buffer.
 *
 * @return The number of bytes actually received if successful, SOCKETS_SOCKET_ERROR
 * otherwise.
 */
static BaseType_t prvNetworkRecv( void * pvContext,
                                  unsigned char * pucReceiveBuffer,
                                  size_t xReceiveBufferLength );



static uint32_t prvGetFreeSocket( void )
{
    uint32_t ulIndex;

    /* Iterate over xSockets array to see if any free socket
     * is available. */
    for( ulIndex = 0 ; ulIndex < ( uint32_t ) CELLULAR_MAX_SOCKETS ; ulIndex++ )
    {
        /* Since multiple tasks can be accessing this simultaneously,
         * this has to be in critical section. */
        taskENTER_CRITICAL();

        if( xSockets[ ulIndex ].ucInUse == 0U )
        {
            /* Mark the socket as "in-use". */
            xSockets[ ulIndex ].ucInUse = 1;
            taskEXIT_CRITICAL();

            /* We have found a free socket, so stop. */
            break;
        }
        else
        {
            taskEXIT_CRITICAL();
        }
    }

    /* Did we find a free socket? */
    if( ulIndex == ( uint32_t ) CELLULAR_MAX_SOCKETS )
    {
        /* Return SOCKETS_INVALID_SOCKET if we fail to
         * find a free socket. */
        ulIndex = ( uint32_t ) SOCKETS_INVALID_SOCKET;
    }

    return ulIndex;
}


static void prvReturnSocket( uint32_t ulSocketNumber )
{
    /* Since multiple tasks can be accessing this simultaneously,
     * this has to be in critical section. */
    taskENTER_CRITICAL();
    {
        /* Mark the socket as free. */
        xSockets[ ulSocketNumber ].ucInUse = 0;
    }
    taskEXIT_CRITICAL();
}


static BaseType_t prvIsValidSocket( uint32_t ulSocketNumber )
{
    BaseType_t xValid = pdFALSE;

    /* Check that the provided socket number is within the valid
     * index range. */
    if( ulSocketNumber < ( uint32_t ) CELLULAR_MAX_SOCKETS )
    {
        /* Since multiple tasks can be accessing this simultaneously,
         * this has to be in critical section. */
        taskENTER_CRITICAL();
        {
            /* Check that this socket is in use. */
            if( xSockets[ ulSocketNumber ].ucInUse == 1U )
            {
                /* This is a valid socket number. */
                xValid = pdTRUE;
            }
        }
        taskEXIT_CRITICAL();
    }

    return xValid;
}


static BaseType_t prvNetworkSend( void * pvContext,
                                  const unsigned char * pucData,
                                  size_t xDataLength )
{
    uint32_t ulSocketNumber = ( uint32_t ) pvContext; /*lint !e923 cast is necessary for port. */
    BaseType_t xRetVal = SOCKETS_SOCKET_ERROR;

    /* Sends the data. Note that this is a blocking function and the send timeout must be set properly
     * using SOCKETS_SetSockOpt. If the timeout expires when sending data, the function will return
     * an ERROR, so please be wise in setting the value. Also note that you should not use the option
     * COM_MSG_DONTWAIT as it does not handle sending buffers greater than the MSS (1460).
     */
    xRetVal = ( BaseType_t )com_send_ip_modem ((uint32_t)xSockets[ulSocketNumber].ST_socket_handle,
                (const com_char_t *) &pucData[0], (int32_t) xDataLength, 0);

    /* If the data was successfully sent, return the actual
     * number of bytes sent. Otherwise return SOCKETS_SOCKET_ERROR. */
    if ( xRetVal <= 0 )
    {
        xRetVal = SOCKETS_SOCKET_ERROR;
    }

    /* To allow other tasks of equal priority that are using this API to run as
     * a switch to an equal priority task that is waiting for the mutex will
     * only otherwise occur in the tick interrupt - at which point the mutex
     * might have been taken again by the currently running task.
     */

    taskYIELD();

    return xRetVal;
}


static BaseType_t prvNetworkRecv( void * pvContext,
                                  unsigned char * pucReceiveBuffer,
                                  size_t xReceiveBufferLength )
{
    uint32_t ulSocketNumber = ( uint32_t ) pvContext; /*lint !e923 cast is needed for portability. */
    BaseType_t xRetVal = 0;
    int32_t xReceiveValue,xTotalBytesReceived = 0;
    unsigned char * tmpReceiveBuffer = pucReceiveBuffer;
    int32_t tmpReceiveBufferLength = xReceiveBufferLength;
    int32_t errorCount = 0;

    for (;;)
    {
        /* Receive the data. Note that this is a blocking function unless COM_MSG_DONTWAIT is specified.
            * The receive timeout must be properly set using SOCKETS_SetSockOpt. If the timeout expires
            * when receiving data, the function will return an ERROR, so please be wise in setting the value.
            * Also note that you should not use the option COM_MSG_DONTWAIT as it does not handle receiving
            * buffers greater than the MSS (1460). If the timeout expires and there is data in the modem
            * receive buffer the data cache may try to flush it and this may result in data loss
            */

        xReceiveValue = (BaseType_t) com_recv_ip_modem ((uint32_t)xSockets[ulSocketNumber].ST_socket_handle,
                tmpReceiveBuffer, (int32_t) tmpReceiveBufferLength, 0);

        if( xReceiveValue < 0 )
        {
            /* check if the receive timeout expired and return 0 as this is a valid case */
            if (xReceiveValue == COM_SOCKETS_ERR_TIMEOUT)
            {
                /* The socket read has timed out too. Returning SOCKETS_EWOULDBLOCK
                    * will cause mBedTLS to fail and so we must return zero. */
                xTotalBytesReceived = 0;
                break;
            }
            else
            {
                if (errorCount++ > 5) {
                    /* We had a communication error status of some sort */
                    xRetVal = SOCKETS_SOCKET_ERROR;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
        else if (( xReceiveValue >= minMESSAGE_SIZE ) || (xReceiveValue == xReceiveBufferLength))
        {
            /* We received enough data and need to pass it back */
            xTotalBytesReceived +=  xReceiveValue;
            break;
        }
        else
        {
            /* we received less than minMESSAGE_SIZE bytes, so wait a little longer to see if something is coming back.
                * This also allows other tasks of equal priority to take the hand.
                */
            tmpReceiveBuffer = tmpReceiveBuffer + xReceiveValue;
            tmpReceiveBufferLength = tmpReceiveBufferLength - xReceiveValue;
            xTotalBytesReceived += xReceiveValue;
            vTaskDelay( stsecuresocketsFIVE_MILLISECONDS );
        }
    }

    if (xRetVal != SOCKETS_SOCKET_ERROR )
    {
        xRetVal = (BaseType_t) xTotalBytesReceived;
    }

    return xRetVal;
}


Socket_t SOCKETS_Socket( int32_t lDomain,
                         int32_t lType,
                         int32_t lProtocol )
{
    uint32_t ulSocketNumber;
    int32_t  ulModemSocketNumber;

    /* Ensure that only supported values are supplied. */
    configASSERT( lDomain == SOCKETS_AF_INET );
    configASSERT( ( lType == SOCKETS_SOCK_STREAM && lProtocol == SOCKETS_IPPROTO_TCP ) );

    /* Try to get a free socket  in our socket pool. */
    ulSocketNumber = prvGetFreeSocket();

    /* Get a free socket from the modem pool */

    ulModemSocketNumber = com_socket_ip_modem(lDomain, lType, lProtocol);

    /* If we get a free socket, set its attributes. */
    if (( ulModemSocketNumber >= 0) && (ulModemSocketNumber < CELLULAR_MAX_SOCKETS))
    {
        /* Initialize all the members to sane values.
         * Not that the timeouts are set by default at 30 seconds, they will need to be changed
         * using SOCKET_SetSockOpt
         */
        xSockets[ ulSocketNumber ].ulFlags = 0;
        xSockets[ ulSocketNumber ].ulSendTimeout = stsecuresocketsMAX_TIMEOUT;
        xSockets[ ulSocketNumber ].ulReceiveTimeout = stsecuresocketsMAX_TIMEOUT;
        xSockets[ ulSocketNumber ].pcDestination = NULL;
        xSockets[ ulSocketNumber ].pvTLSContext = NULL;
        xSockets[ ulSocketNumber ].pcServerCertificate = NULL;
        xSockets[ ulSocketNumber ].ulServerCertificateLength = 0;
        xSockets[ ulSocketNumber ].ST_socket_handle = ulModemSocketNumber;
    } else {
        ulSocketNumber = ( uint32_t ) SOCKETS_INVALID_SOCKET;
    }


    /* If we fail to get a free socket, we return SOCKETS_INVALID_SOCKET. */
    return ( Socket_t ) ulSocketNumber; /*lint !e923 cast required for portability. */
}

int isBG96Online();

int32_t SOCKETS_Connect( Socket_t xSocket,
                         SocketsSockaddr_t * pxAddress,
                         Socklen_t xAddressLength )
{
    uint32_t ulSocketNumber = ( uint32_t ) xSocket; /*lint !e923 cast required for portability. */
    STSecureSocket_t * pxSecureSocket;
    TLSParams_t xTLSParams = { 0 };
    int32_t lRetVal = SOCKETS_SOCKET_ERROR;
    com_sockaddr_in_t  destination_address;
    uint32_t lRecvTimeout,lSendTimeout;

    if (isBG96Online() != 1) return -1;

    /* Ensure that a valid socket was passed. */
    if( prvIsValidSocket( ulSocketNumber ) == pdTRUE )
    {
        /* Shortcut for easy access. */
        pxSecureSocket = &( xSockets[ ulSocketNumber ] );

        /* Start the client connection. */

        destination_address.sin_family      = COM_AF_INET;
        destination_address.sin_port        = pxAddress->usPort;
        destination_address.sin_addr.s_addr =  pxAddress->ulAddress;

        int32_t res = com_connect_ip_modem((uint32_t)xSockets[ulSocketNumber].ST_socket_handle ,
                (com_sockaddr_t *) &destination_address, sizeof(com_sockaddr_in_t));
        if(res == CELLULAR_OK)
        {
            /* Successful connection is established. */
            lRetVal = SOCKETS_ERROR_NONE;
        }
    }

    /* Initialize TLS only if the connection is successful. */
    if( ( lRetVal == SOCKETS_ERROR_NONE ) &&
        ( ( pxSecureSocket->ulFlags & stsecuresocketsSOCKET_SECURE_FLAG ) != 0UL ) )
    {
        /* Setup TLS parameters. */
        xTLSParams.ulSize = sizeof( xTLSParams );
        xTLSParams.pcDestination = pxSecureSocket->pcDestination;
        xTLSParams.pcServerCertificate = pxSecureSocket->pcServerCertificate;
        xTLSParams.ulServerCertificateLength = pxSecureSocket->ulServerCertificateLength;
        xTLSParams.pvCallerContext = ( void * ) xSocket;
        xTLSParams.pxNetworkRecv = &( prvNetworkRecv );
        xTLSParams.pxNetworkSend = &( prvNetworkSend );

        /* Set sockets timeout to default socket Send and Receive Timeouts */
        lSendTimeout = xSockets[ ulSocketNumber ].ulSendTimeout;
        lRecvTimeout = xSockets[ ulSocketNumber ].ulReceiveTimeout;

        /* Both Receive and Send timeouts need to be programmed so the Modem knows when to return
         * a Receive or Send timeout errors to the client application. Typical values are in seconds
         * but in some cases when you are using the same task to read and send you may set them up
         * for a shorter period which what we will do once the connection is established
         */
        if ((com_setsockopt_ip_modem((uint32_t)xSockets[ulSocketNumber].ST_socket_handle,
                COM_SOL_SOCKET, COM_SO_RCVTIMEO,&lRecvTimeout, sizeof(lRecvTimeout)) == COM_SOCKETS_ERR_OK)&&
            (com_setsockopt_ip_modem((uint32_t)xSockets[ulSocketNumber].ST_socket_handle,
                COM_SOL_SOCKET, COM_SO_SNDTIMEO,&lSendTimeout, sizeof(lSendTimeout)) == COM_SOCKETS_ERR_OK))
        {

            /* Initialize TLS. */
            if( TLS_Init( &( pxSecureSocket->pvTLSContext ), &( xTLSParams ) ) == pdFREERTOS_ERRNO_NONE )
            {
                /* Initiate TLS handshake. */
                if( TLS_Connect( pxSecureSocket->pvTLSContext ) != pdFREERTOS_ERRNO_NONE )
                {
                    /* TLS handshake failed. */
                    lRetVal = SOCKETS_TLS_HANDSHAKE_ERROR;
                }
            }
            else
            {
                /* TLS Initialization failed. */
                configPRINTF(("Error: TLS Init failed\r\n"));
                lRetVal = SOCKETS_TLS_INIT_ERROR;
            }
        }
    }

    return lRetVal;
}


int32_t SOCKETS_Recv( Socket_t xSocket,
                      void * pvBuffer,
                      size_t xBufferLength,
                      uint32_t ulFlags )
{
    uint32_t ulSocketNumber = ( uint32_t ) xSocket; /*lint !e923 cast required for portability. */
    STSecureSocket_t * pxSecureSocket;
    int32_t lReceivedBytes = SOCKETS_SOCKET_ERROR;

    /* Remove warning about unused parameters. */
    ( void ) ulFlags;

    /* Ensure that a valid socket was passed and the
     * passed buffer is not NULL. */
    if( ( prvIsValidSocket( ulSocketNumber ) == pdTRUE ) &&
        ( pvBuffer != NULL ) )
    {
        /* Shortcut for easy access. */
        pxSecureSocket = &( xSockets[ ulSocketNumber ] );

        /* Check that receive is allowed on the socket. */
        if( ( pxSecureSocket->ulFlags & stsecuresocketsSOCKET_READ_CLOSED_FLAG ) == 0UL )
        {
            if( ( pxSecureSocket->ulFlags & stsecuresocketsSOCKET_SECURE_FLAG ) != 0UL )
            {
                /* Receive through TLS pipe, if negotiated. */
                lReceivedBytes = TLS_Recv( pxSecureSocket->pvTLSContext, pvBuffer, xBufferLength );

                /* Convert the error code. */
                if( lReceivedBytes < 0 )
                {
                    /* TLS_Recv failed. */
                    lReceivedBytes = SOCKETS_TLS_RECV_ERROR;
                }
            }
            else
            {
                /* Receive un-encrypted. */
                lReceivedBytes = prvNetworkRecv( xSocket, pvBuffer, xBufferLength );
            }

        } else {
            /* The socket has been closed for read. */
            lReceivedBytes = SOCKETS_ECLOSED;
        }
    }


    return lReceivedBytes;
}


int32_t SOCKETS_Send( Socket_t xSocket,
                      const void * pvBuffer,
                      size_t xDataLength,
                      uint32_t ulFlags )
{
    uint32_t ulSocketNumber = ( uint32_t ) xSocket; /*lint !e923 cast required for portability. */
    STSecureSocket_t * pxSecureSocket;
    int32_t lSentBytes = SOCKETS_SOCKET_ERROR;

    /* Remove warning about unused parameters. */
    ( void ) ulFlags;

    /* Ensure that a valid socket was passed and the passed buffer
     * is not NULL. */
    if( ( prvIsValidSocket( ulSocketNumber ) == pdTRUE ) &&
        ( pvBuffer != NULL ) )
    {
        /* Shortcut for easy access. */
        pxSecureSocket = &( xSockets[ ulSocketNumber ] );

        /* Check that send is allowed on the socket. */
        if( ( pxSecureSocket->ulFlags & stsecuresocketsSOCKET_WRITE_CLOSED_FLAG ) == 0UL )
        {
            if( ( pxSecureSocket->ulFlags & stsecuresocketsSOCKET_SECURE_FLAG ) != 0UL )
            {
                /* Send through TLS pipe, if negotiated. */
               lSentBytes = TLS_Send( pxSecureSocket->pvTLSContext, pvBuffer, xDataLength );

                /* Convert the error code. */
                if( lSentBytes < 0 )
                {
                    vLoggingPrintf("SOCKETS_TLS_SEND_ERROR!!\r\n");
                    /* TLS_Send failed. */
                    lSentBytes = SOCKETS_TLS_SEND_ERROR;
                }
            }
            else
            {
                /* Send un-encrypted. */
                lSentBytes = prvNetworkSend( xSocket, pvBuffer, xDataLength );
            }
        }
        else
        {
            vLoggingPrintf("SOCKETS_ECLOSED!!\r\n");
            /* The socket has been closed for write. */
            lSentBytes = SOCKETS_ECLOSED;
        }
    }

    return lSentBytes;
}


int32_t SOCKETS_Shutdown( Socket_t xSocket,
                          uint32_t ulHow )
{
    uint32_t ulSocketNumber = ( uint32_t ) xSocket; /*lint !e923 cast required for portability. */
    STSecureSocket_t * pxSecureSocket;
    int32_t lRetVal = SOCKETS_SOCKET_ERROR;

    /* Ensure that a valid socket was passed. */
    if( prvIsValidSocket( ulSocketNumber ) == pdTRUE )
    {
        /* Shortcut for easy access. */
        pxSecureSocket = &( xSockets[ ulSocketNumber ] );

        switch( ulHow )
        {
            case SOCKETS_SHUT_RD:
                /* Further receive calls on this socket should return error. */
                pxSecureSocket->ulFlags |= stsecuresocketsSOCKET_READ_CLOSED_FLAG;

                /* Return success to the user. */
                lRetVal = SOCKETS_ERROR_NONE;
                break;

            case SOCKETS_SHUT_WR:
                /* Further send calls on this socket should return error. */
                pxSecureSocket->ulFlags |= stsecuresocketsSOCKET_WRITE_CLOSED_FLAG;

                /* Return success to the user. */
                lRetVal = SOCKETS_ERROR_NONE;
                break;

            case SOCKETS_SHUT_RDWR:
                /* Further send or receive calls on this socket should return error. */
                pxSecureSocket->ulFlags |= stsecuresocketsSOCKET_READ_CLOSED_FLAG;
                pxSecureSocket->ulFlags |= stsecuresocketsSOCKET_WRITE_CLOSED_FLAG;

                /* Return success to the user. */
                lRetVal = SOCKETS_ERROR_NONE;
                break;

            default:
                /* An invalid value was passed for ulHow. */
                lRetVal = SOCKETS_EINVAL;
                break;
        }
    }

    return lRetVal;
}


int32_t SOCKETS_Close( Socket_t xSocket )
{
    uint32_t ulSocketNumber = ( uint32_t ) xSocket; /*lint !e923 cast required for portability. */
    STSecureSocket_t * pxSecureSocket;
    int32_t lRetVal = SOCKETS_SOCKET_ERROR;

    /* Ensure that a valid socket was passed. */
    if( prvIsValidSocket( ulSocketNumber ) == pdTRUE )
    {
        /* Shortcut for easy access. */
        pxSecureSocket = &( xSockets[ ulSocketNumber ] );

        /* Mark the socket as closed. */
        pxSecureSocket->ulFlags |= stsecuresocketsSOCKET_READ_CLOSED_FLAG;
        pxSecureSocket->ulFlags |= stsecuresocketsSOCKET_WRITE_CLOSED_FLAG;

        /* Free the space allocated for pcDestination. */
        if( pxSecureSocket->pcDestination != NULL )
        {
            vPortFree( pxSecureSocket->pcDestination );
        }

        /* Free the space allocated for pcServerCertificate. */
        if( pxSecureSocket->pcServerCertificate != NULL )
        {
            vPortFree( pxSecureSocket->pcServerCertificate );
        }

        /* Cleanup TLS. */
        if( ( pxSecureSocket->ulFlags & stsecuresocketsSOCKET_SECURE_FLAG ) != 0UL )
        {
            TLS_Cleanup( pxSecureSocket->pvTLSContext );
        }

        /* Stop the client connection. */
        if(com_closesocket_ip_modem((uint32_t)xSockets[ulSocketNumber].ST_socket_handle) == CELLULAR_OK )
        {
            /* Connection close successful. */
            lRetVal = SOCKETS_ERROR_NONE;
        }

        /* Return the socket back to the free socket pool. */
        prvReturnSocket( ulSocketNumber );
    }

    return lRetVal;
}


int32_t SOCKETS_SetSockOpt( Socket_t xSocket,
                            int32_t lLevel,
                            int32_t lOptionName,
                            const void * pvOptionValue,
                            size_t xOptionLength )
{
    uint32_t ulSocketNumber = ( uint32_t ) xSocket; /*lint !e923 cast required for portability. */
    STSecureSocket_t * pxSecureSocket;
    int32_t lRetVal = SOCKETS_ERROR_NONE;
    uint32_t lTimeout,lRecvTimeout,lSendTimeout;

    /* Ensure that a valid socket was passed. */
    if( prvIsValidSocket( ulSocketNumber ) == pdTRUE )
    {
        /* Shortcut for easy access. */
        pxSecureSocket = &( xSockets[ ulSocketNumber ] );

        switch( lOptionName )
        {
            case SOCKETS_SO_SERVER_NAME_INDICATION:

                /* Non-NULL destination string indicates that SNI extension should
                 * be used during TLS negotiation. */
                pxSecureSocket->pcDestination = ( char * ) pvPortMalloc( 1U + xOptionLength );

                if( pxSecureSocket->pcDestination == NULL )
                {
                    lRetVal = SOCKETS_ENOMEM;
                }
                else
                {
                    memcpy( pxSecureSocket->pcDestination, pvOptionValue, xOptionLength );
                    pxSecureSocket->pcDestination[ xOptionLength ] = '\0';
                }

                break;

            case SOCKETS_SO_TRUSTED_SERVER_CERTIFICATE:

                /* Non-NULL server certificate field indicates that the default trust
                 * list should not be used. */
                pxSecureSocket->pcServerCertificate = ( char * ) pvPortMalloc( xOptionLength );

                if( pxSecureSocket->pcServerCertificate == NULL )
                {
                    lRetVal = SOCKETS_ENOMEM;
                }
                else
                {
                    memcpy( pxSecureSocket->pcServerCertificate, pvOptionValue, xOptionLength );
                    pxSecureSocket->ulServerCertificateLength = xOptionLength;
                }

                break;

            case SOCKETS_SO_REQUIRE_TLS:

                /* Turn on the secure socket flag to indicate that
                 * TLS should be used. */
                pxSecureSocket->ulFlags |= stsecuresocketsSOCKET_SECURE_FLAG;
                break;

            case SOCKETS_SO_SNDTIMEO:

                lTimeout = *( ( const uint32_t * ) pvOptionValue ); /*lint !e9087 pvOptionValue is passed in as an opaque value, and must be casted for setsockopt. */

                /* Makes sure we don't set too short of a timeout otherwise the sender task in the
                 * modem will timeout, purge the data in its buffers and raise an error */

                lTimeout = lTimeout > stsecuresocketsTWO_HUNDRED_MILLISECONDS ? lTimeout : stsecuresocketsTWO_HUNDRED_MILLISECONDS;

                /* Valid timeouts are 0 (no timeout) or 1-30000ms. */
                if( lTimeout <= stsecuresocketsMAX_TIMEOUT )
                {
                    /* Store send timeout. */
                    pxSecureSocket->ulSendTimeout = lTimeout;

                    if (com_setsockopt_ip_modem((uint32_t)xSockets[ulSocketNumber].ST_socket_handle,
                            COM_SOL_SOCKET, COM_SO_SNDTIMEO,&lTimeout, sizeof(lTimeout)) != COM_SOCKETS_ERR_OK)
                    {
                        lRetVal = SOCKETS_SOCKET_ERROR;
                    }
                    else
                    {
                        lRetVal = SOCKETS_ERROR_NONE;
                    }
                }
                else
                {
                    lRetVal = SOCKETS_EINVAL;
                }

                break;

            case SOCKETS_SO_RCVTIMEO:

                lTimeout = *( ( const uint32_t * ) pvOptionValue ); /*lint !e9087 pvOptionValue is passed in as an opaque value, and must be casted for setsockopt. */

                /* Makes sure we don't set too short of a timeout otherwise the receiver task in the
                 * modem will timeout too often and will try to purge the data in its buffers */

                lTimeout = lTimeout > stsecuresocketsHUNDRED_MILLISECONDS ? lTimeout : stsecuresocketsHUNDRED_MILLISECONDS;

                /* Valid timeouts are 0 (no timeout) or 1-30000ms. */
                if( lTimeout <= stsecuresocketsMAX_TIMEOUT )
                {
                    /* Store receive timeout. */
                    pxSecureSocket->ulReceiveTimeout = lTimeout;

                    if (com_setsockopt_ip_modem((uint32_t)xSockets[ulSocketNumber].ST_socket_handle,
                             COM_SOL_SOCKET, COM_SO_RCVTIMEO,&lTimeout, sizeof(lTimeout)) != COM_SOCKETS_ERR_OK)
                    {
                        lRetVal = SOCKETS_SOCKET_ERROR;
                    }
                    else
                    {
                        lRetVal = SOCKETS_ERROR_NONE;
                    }
                }
                else
                {
                    lRetVal = SOCKETS_EINVAL;
                }

                break;

            case SOCKETS_SO_NONBLOCK:

                /* Since the network stack is using these timeouts to detect network errors and the CAT-M
                 * modem is not able to send data as fast as a WI-FI module we need to set the send and receive
                 * timeouts to reasonable values. This is especially important for the send timeout, as the MQTT
                 * doesn't recover from a send_timeout_error
                 */
                pxSecureSocket->ulReceiveTimeout = stsecuresocketsHUNDRED_MILLISECONDS;
                pxSecureSocket->ulSendTimeout = stsecuresocketsTWO_HUNDRED_MILLISECONDS;

                lRecvTimeout = (uint32_t) stsecuresocketsHUNDRED_MILLISECONDS;
                lSendTimeout = (uint32_t) stsecuresocketsTWO_HUNDRED_MILLISECONDS;

                if ((com_setsockopt_ip_modem((uint32_t)xSockets[ulSocketNumber].ST_socket_handle,
                        COM_SOL_SOCKET, COM_SO_RCVTIMEO,&lRecvTimeout, sizeof(lRecvTimeout)) != COM_SOCKETS_ERR_OK)||
                    (com_setsockopt_ip_modem((uint32_t)xSockets[ulSocketNumber].ST_socket_handle,
                        COM_SOL_SOCKET, COM_SO_SNDTIMEO,&lSendTimeout, sizeof(lSendTimeout)) != COM_SOCKETS_ERR_OK))
                {
                    lRetVal = SOCKETS_SOCKET_ERROR;
                }
                else
                {
                    lRetVal = SOCKETS_ERROR_NONE;
                }

                break;

            default:

                lRetVal = SOCKETS_ENOPROTOOPT;
                break;
        }
    }
    else
    {
        lRetVal = SOCKETS_SOCKET_ERROR;
    }

    return lRetVal;
}


uint32_t SOCKETS_GetHostByName( const char * pcHostName )
{
    uint32_t ulIPAddres = 0;
    com_sockaddr_t distantaddr;

    /* The modem needs to be up before going any further */
    if (isBG96Online() != 1) return -1;

    /* Do a DNS Lookup. */
    if (com_gethostbyname_ip_modem ((const com_char_t *) pcHostName, &distantaddr) == 0)
    {
        configPRINTF(("Getting IP for the hostname\r\n"));
        ulIPAddres = ((com_sockaddr_in_t *)&distantaddr)->sin_addr.s_addr;
        if (ulIPAddres == 0) {
            configPRINTF(("ERROR: IP is not correct! dns resolve has failed\r\n"));
        }
    } else {
        configPRINTF(("Something went wrong while getting IP address\r\n"));
    }

    /* caller should check if ulIPAddres != 0 !!!*/

    return ulIPAddres;
}


BaseType_t SOCKETS_Init( void )
{
    uint32_t ulIndex;

    /* Mark all the sockets as free and closed. */
    for( ulIndex = 0 ; ulIndex < ( uint32_t ) CELLULAR_MAX_SOCKETS ; ulIndex++ )
    {
        xSockets[ ulIndex ].ucInUse = 0;
        xSockets[ ulIndex ].ulFlags = 0;

        xSockets[ ulIndex ].ulFlags |= stsecuresocketsSOCKET_READ_CLOSED_FLAG;
        xSockets[ ulIndex ].ulFlags |= stsecuresocketsSOCKET_WRITE_CLOSED_FLAG;
    }

    /* Empty initialization for ST board. */
    return pdPASS;
}
