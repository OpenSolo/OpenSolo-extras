// MESSAGE MEMINFO PACKING

#if MAVLINK_C2000
#include "protocol_c2000.h"
#endif

#define MAVLINK_MSG_ID_MEMINFO 152

typedef struct __mavlink_meminfo_t
{
 uint16_t brkval; ///< heap top
 uint16_t freemem; ///< free memory
} mavlink_meminfo_t;

#define MAVLINK_MSG_ID_MEMINFO_LEN 4
#define MAVLINK_MSG_ID_152_LEN 4

#define MAVLINK_MSG_ID_MEMINFO_CRC 208
#define MAVLINK_MSG_ID_152_CRC 208



#define MAVLINK_MESSAGE_INFO_MEMINFO { \
	"MEMINFO", \
	2, \
	{  { "brkval", NULL, MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_meminfo_t, brkval) }, \
         { "freemem", NULL, MAVLINK_TYPE_UINT16_T, 0, 2, offsetof(mavlink_meminfo_t, freemem) }, \
         } \
}


/**
 * @brief Pack a meminfo message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param brkval heap top
 * @param freemem free memory
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_meminfo_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint16_t brkval, uint16_t freemem)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_MEMINFO_LEN];
	_mav_put_uint16_t(buf, 0, brkval);
	_mav_put_uint16_t(buf, 2, freemem);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_MEMINFO_LEN);
#elif MAVLINK_C2000
		mav_put_uint16_t_c2000(&(msg->payload64[0]), 0, brkval);
		mav_put_uint16_t_c2000(&(msg->payload64[0]), 2, freemem);
	
	
#else
	mavlink_meminfo_t packet;
	packet.brkval = brkval;
	packet.freemem = freemem;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_MEMINFO_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_MEMINFO;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_MEMINFO_LEN, MAVLINK_MSG_ID_MEMINFO_CRC);
#else
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_MEMINFO_LEN);
#endif
}

/**
 * @brief Pack a meminfo message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param brkval heap top
 * @param freemem free memory
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_meminfo_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint16_t brkval,uint16_t freemem)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_MEMINFO_LEN];
	_mav_put_uint16_t(buf, 0, brkval);
	_mav_put_uint16_t(buf, 2, freemem);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_MEMINFO_LEN);
#else
	mavlink_meminfo_t packet;
	packet.brkval = brkval;
	packet.freemem = freemem;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_MEMINFO_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_MEMINFO;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_MEMINFO_LEN, MAVLINK_MSG_ID_MEMINFO_CRC);
#else
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_MEMINFO_LEN);
#endif
}

/**
 * @brief Encode a meminfo struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param meminfo C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_meminfo_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_meminfo_t* meminfo)
{
	return mavlink_msg_meminfo_pack(system_id, component_id, msg, meminfo->brkval, meminfo->freemem);
}

/**
 * @brief Encode a meminfo struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param meminfo C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_meminfo_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_meminfo_t* meminfo)
{
	return mavlink_msg_meminfo_pack_chan(system_id, component_id, chan, msg, meminfo->brkval, meminfo->freemem);
}

/**
 * @brief Send a meminfo message
 * @param chan MAVLink channel to send the message
 *
 * @param brkval heap top
 * @param freemem free memory
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_meminfo_send(mavlink_channel_t chan, uint16_t brkval, uint16_t freemem)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_MEMINFO_LEN];
	_mav_put_uint16_t(buf, 0, brkval);
	_mav_put_uint16_t(buf, 2, freemem);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MEMINFO, buf, MAVLINK_MSG_ID_MEMINFO_LEN, MAVLINK_MSG_ID_MEMINFO_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MEMINFO, buf, MAVLINK_MSG_ID_MEMINFO_LEN);
#endif
#else
	mavlink_meminfo_t packet;
	packet.brkval = brkval;
	packet.freemem = freemem;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MEMINFO, (const char *)&packet, MAVLINK_MSG_ID_MEMINFO_LEN, MAVLINK_MSG_ID_MEMINFO_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MEMINFO, (const char *)&packet, MAVLINK_MSG_ID_MEMINFO_LEN);
#endif
#endif
}

#if MAVLINK_MSG_ID_MEMINFO_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This varient of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_meminfo_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint16_t brkval, uint16_t freemem)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char *buf = (char *)msgbuf;
	_mav_put_uint16_t(buf, 0, brkval);
	_mav_put_uint16_t(buf, 2, freemem);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MEMINFO, buf, MAVLINK_MSG_ID_MEMINFO_LEN, MAVLINK_MSG_ID_MEMINFO_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MEMINFO, buf, MAVLINK_MSG_ID_MEMINFO_LEN);
#endif
#else
	mavlink_meminfo_t *packet = (mavlink_meminfo_t *)msgbuf;
	packet->brkval = brkval;
	packet->freemem = freemem;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MEMINFO, (const char *)packet, MAVLINK_MSG_ID_MEMINFO_LEN, MAVLINK_MSG_ID_MEMINFO_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MEMINFO, (const char *)packet, MAVLINK_MSG_ID_MEMINFO_LEN);
#endif
#endif
}
#endif

#endif

// MESSAGE MEMINFO UNPACKING


/**
 * @brief Get field brkval from meminfo message
 *
 * @return heap top
 */
static inline uint16_t mavlink_msg_meminfo_get_brkval(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint16_t(msg,  0);
#else
	return mav_get_uint16_t_c2000(&(msg->payload64[0]),  0);
#endif
}

/**
 * @brief Get field freemem from meminfo message
 *
 * @return free memory
 */
static inline uint16_t mavlink_msg_meminfo_get_freemem(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint16_t(msg,  2);
#else
	return mav_get_uint16_t_c2000(&(msg->payload64[0]),  2);
#endif
}

/**
 * @brief Decode a meminfo message into a struct
 *
 * @param msg The message to decode
 * @param meminfo C-struct to decode the message contents into
 */
static inline void mavlink_msg_meminfo_decode(const mavlink_message_t* msg, mavlink_meminfo_t* meminfo)
{
#if MAVLINK_NEED_BYTE_SWAP || MAVLINK_C2000
	meminfo->brkval = mavlink_msg_meminfo_get_brkval(msg);
	meminfo->freemem = mavlink_msg_meminfo_get_freemem(msg);
#else
	memcpy(meminfo, _MAV_PAYLOAD(msg), MAVLINK_MSG_ID_MEMINFO_LEN);
#endif
}
