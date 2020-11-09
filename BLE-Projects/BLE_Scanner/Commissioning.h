/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2015 Semtech

Description: End device commissioning parameters

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#ifndef __LORA_COMMISSIONING_H__
#define __LORA_COMMISSIONING_H__

/*!
 * When set to 1 the application uses the Over-the-Air activation procedure
 * When set to 0 the application uses the Personalization activation procedure
 */
#define OVER_THE_AIR_ACTIVATION                     0

/*!
 * Indicates if the end-device is to be connected to a private or public network
 */
#define LORAWAN_PUBLIC_NETWORK                      true

/*!
 * IEEE Organizationally Unique Identifier ( OUI ) (big endian)
 * \remark This is unique to a company or organization
 */
#define IEEE_OUI                                    0x00, 0x1E, 0xA3

/*!
 * Mote device IEEE EUI (big endian)
 *
 * \remark In this application the value is automatically generated by calling
 *         BoardGetUniqueId function
 */
#define LORAWAN_DEVICE_EUI                          { IEEE_OUI, 0xBC, 0xA0, 0x97, 0x74, 0x71 }

/*!
 * Application IEEE EUI (big endian)
 */
#define LORAWAN_APPLICATION_EUI                     { 0x70, 0xB3, 0xd5, 0x7E, 0xD0, 0x00, 0x96, 0x5C }
                                                     //70b3d57ef00046a4   70B3D57E D0007DFA
/*!
 * AES encryption/decryption cipher application key
 */
#define LORAWAN_APPLICATION_KEY                     { 0xDF, 0xC6, 0xFC, 0x3F, 0x7D, 0x69, 0x84, 0x8A, 0x3D, 0x21, 0xEA, 0x76, 0x5C, 0x33, 0x2A, 0x04 }
                                                     //a6b08140dae1d795ebfa5a6dee1f4dbd    09A503D6256F9EF612A15181F583880A
/*!
 * Current network ID
 */
#define LORAWAN_NETWORK_ID                          ( uint32_t )0

/*!
 * Device address on the network (big endian)
 *
 * \remark In this application the value is automatically generated using
 *         a pseudo random generator seeded with a value derived from
 *         BoardUniqueId value if LORAWAN_DEVICE_ADDRESS is set to 0
 */
#define LORAWAN_DEVICE_ADDRESS                      ( uint32_t )0x005FDD17

/*!
 * AES encryption/decryption cipher network session key
 */
#define LORAWAN_NWKSKEY                             { 0x60, 0xF3, 0x37, 0x04, 0xCB, 0x6F, 0x5F, 0x4E, 0xB5, 0x94, 0x84, 0xF5, 0xBE, 0x95, 0x92, 0x51 }

/*!
 * AES encryption/decryption cipher application session key0f f4 da ff 6b 2f 12 9d 41 59 13 73 c1 27 41 37
 */
#define LORAWAN_APPSKEY                             { 0x0f, 0xf4, 0xda, 0xff, 0x6b, 0x2f, 0x12, 0x9d, 0x41, 0x59, 0x13, 0x73, 0xc1, 0x27, 0x41, 0x37 }

#endif // __LORA_COMMISSIONING_H__
