// MESSAGE SET_HOME_OFFSETS PACKING

#if MAVLINK_C2000
#include "protocol_c2000.h"
#endif

#define MAVLINK_MSG_ID_SET_HOME_OFFSETS 192

typedef struct __mavlink_set_home_offsets_t
{
 uint8_t target_system; ///< System ID
 uint8_t target_component; ///< Component ID
} mavlink_set_home_offsets_t;

#define MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN 2
#define MAVLINK_MSG_ID_192_LEN 2

#define MAVLINK_MSG_ID_SET_HOME_OFFSETS_CRC 46
#define MAVLINK_MSG_ID_192_CRC 46



#define MAVLINK_MESSAGE_INFO_SET_HOME_OFFSETS { \
	"SET_HOME_OFFSETS", \
	2, \
	{  { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_set_home_offsets_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 1, offsetof(mavlink_set_home_offsets_t, target_component) }, \
         } \
}


/**
 * @brief Pack a set_home_offsets message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_home_offsets_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN];
	_mav_put_uint8_t(buf, 0, target_system);
	_mav_put_uint8_t(buf, 1, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN);
#elif MAVLINK_C2000
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 0, target_system);
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 1, target_component);
	
	
#else
	mavlink_set_home_offsets_t packet;
	packet.target_system = target_system;
	packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_SET_HOME_OFFSETS;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN, MAVLINK_MSG_ID_SET_HOME_OFFSETS_CRC);
#else
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN);
#endif
}

/**
 * @brief Pack a set_home_offsets message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_home_offsets_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t target_system,uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN];
	_mav_put_uint8_t(buf, 0, target_system);
	_mav_put_uint8_t(buf, 1, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN);
#else
	mavlink_set_home_offsets_t packet;
	packet.target_system = target_system;
	packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_SET_HOME_OFFSETS;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN, MAVLINK_MSG_ID_SET_HOME_OFFSETS_CRC);
#else
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN);
#endif
}

/**
 * @brief Encode a set_home_offsets struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param set_home_offsets C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_set_home_offsets_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_set_home_offsets_t* set_home_offsets)
{
	return mavlink_msg_set_home_offsets_pack(system_id, component_id, msg, set_home_offsets->target_system, set_home_offsets->target_component);
}

/**
 * @brief Encode a set_home_offsets struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param set_home_offsets C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_set_home_offsets_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_set_home_offsets_t* set_home_offsets)
{
	return mavlink_msg_set_home_offsets_pack_chan(system_id, component_id, chan, msg, set_home_offsets->target_system, set_home_offsets->target_component);
}

/**
 * @brief Send a set_home_offsets message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_set_home_offsets_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN];
	_mav_put_uint8_t(buf, 0, target_system);
	_mav_put_uint8_t(buf, 1, target_component);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SET_HOME_OFFSETS, buf, MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN, MAVLINK_MSG_ID_SET_HOME_OFFSETS_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SET_HOME_OFFSETS, buf, MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN);
#endif
#else
	mavlink_set_home_offsets_t packet;
	packet.target_system = target_system;
	packet.target_component = target_component;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SET_HOME_OFFSETS, (const char *)&packet, MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN, MAVLINK_MSG_ID_SET_HOME_OFFSETS_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SET_HOME_OFFSETS, (const char *)&packet, MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN);
#endif
#endif
}

#if MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This varient of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_set_home_offsets_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char *buf = (char *)msgbuf;
	_mav_put_uint8_t(buf, 0, target_system);
	_mav_put_uint8_t(buf, 1, target_component);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SET_HOME_OFFSETS, buf, MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN, MAVLINK_MSG_ID_SET_HOME_OFFSETS_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SET_HOME_OFFSETS, buf, MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN);
#endif
#else
	mavlink_set_home_offsets_t *packet = (mavlink_set_home_offsets_t *)msgbuf;
	packet->target_system = target_system;
	packet->target_component = target_component;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SET_HOME_OFFSETS, (const char *)packet, MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN, MAVLINK_MSG_ID_SET_HOME_OFFSETS_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SET_HOME_OFFSETS, (const char *)packet, MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN);
#endif
#endif
}
#endif

#endif

// MESSAGE SET_HOME_OFFSETS UNPACKING


/**
 * @brief Get field target_system from set_home_offsets message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_set_home_offsets_get_target_system(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  0);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  0);
#endif
}

/**
 * @brief Get field target_component from set_home_offsets message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_set_home_offsets_get_target_component(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  1);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  1);
#endif
}

/**
 * @brief Decode a set_home_offsets message into a struct
 *
 * @param msg The message to decode
 * @param set_home_offsets C-struct to decode the message contents into
 */
static inline void mavlink_msg_set_home_offsets_decode(const mavlink_message_t* msg, mavlink_set_home_offsets_t* set_home_offsets)
{
#if MAVLINK_NEED_BYTE_SWAP || MAVLINK_C2000
	set_home_offsets->target_system = mavlink_msg_set_home_offsets_get_target_system(msg);
	set_home_offsets->target_component = mavlink_msg_set_home_offsets_get_target_component(msg);
#else
	memcpy(set_home_offsets, _MAV_PAYLOAD(msg), MAVLINK_MSG_ID_SET_HOME_OFFSETS_LEN);
#endif
}
