// MESSAGE GIMBAL_REQUEST_AXIS_CALIBRATION PACKING

#if MAVLINK_C2000
#include "protocol_c2000.h"
#endif

#define MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION 213

typedef struct __mavlink_gimbal_request_axis_calibration_t
{
 uint8_t target_system; ///< System ID
 uint8_t target_component; ///< Component ID
} mavlink_gimbal_request_axis_calibration_t;

#define MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN 2
#define MAVLINK_MSG_ID_213_LEN 2

#define MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_CRC 14
#define MAVLINK_MSG_ID_213_CRC 14



#define MAVLINK_MESSAGE_INFO_GIMBAL_REQUEST_AXIS_CALIBRATION { \
	"GIMBAL_REQUEST_AXIS_CALIBRATION", \
	2, \
	{  { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_gimbal_request_axis_calibration_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 1, offsetof(mavlink_gimbal_request_axis_calibration_t, target_component) }, \
         } \
}


/**
 * @brief Pack a gimbal_request_axis_calibration message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_gimbal_request_axis_calibration_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN];
	_mav_put_uint8_t(buf, 0, target_system);
	_mav_put_uint8_t(buf, 1, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN);
#elif MAVLINK_C2000
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 0, target_system);
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 1, target_component);
	
	
#else
	mavlink_gimbal_request_axis_calibration_t packet;
	packet.target_system = target_system;
	packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_CRC);
#else
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN);
#endif
}

/**
 * @brief Pack a gimbal_request_axis_calibration message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_gimbal_request_axis_calibration_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t target_system,uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN];
	_mav_put_uint8_t(buf, 0, target_system);
	_mav_put_uint8_t(buf, 1, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN);
#else
	mavlink_gimbal_request_axis_calibration_t packet;
	packet.target_system = target_system;
	packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_CRC);
#else
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN);
#endif
}

/**
 * @brief Encode a gimbal_request_axis_calibration struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param gimbal_request_axis_calibration C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_gimbal_request_axis_calibration_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_gimbal_request_axis_calibration_t* gimbal_request_axis_calibration)
{
	return mavlink_msg_gimbal_request_axis_calibration_pack(system_id, component_id, msg, gimbal_request_axis_calibration->target_system, gimbal_request_axis_calibration->target_component);
}

/**
 * @brief Encode a gimbal_request_axis_calibration struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param gimbal_request_axis_calibration C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_gimbal_request_axis_calibration_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_gimbal_request_axis_calibration_t* gimbal_request_axis_calibration)
{
	return mavlink_msg_gimbal_request_axis_calibration_pack_chan(system_id, component_id, chan, msg, gimbal_request_axis_calibration->target_system, gimbal_request_axis_calibration->target_component);
}

/**
 * @brief Send a gimbal_request_axis_calibration message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_gimbal_request_axis_calibration_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN];
	_mav_put_uint8_t(buf, 0, target_system);
	_mav_put_uint8_t(buf, 1, target_component);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION, buf, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION, buf, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN);
#endif
#else
	mavlink_gimbal_request_axis_calibration_t packet;
	packet.target_system = target_system;
	packet.target_component = target_component;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION, (const char *)&packet, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION, (const char *)&packet, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN);
#endif
#endif
}

#if MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This varient of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_gimbal_request_axis_calibration_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char *buf = (char *)msgbuf;
	_mav_put_uint8_t(buf, 0, target_system);
	_mav_put_uint8_t(buf, 1, target_component);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION, buf, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION, buf, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN);
#endif
#else
	mavlink_gimbal_request_axis_calibration_t *packet = (mavlink_gimbal_request_axis_calibration_t *)msgbuf;
	packet->target_system = target_system;
	packet->target_component = target_component;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION, (const char *)packet, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION, (const char *)packet, MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN);
#endif
#endif
}
#endif

#endif

// MESSAGE GIMBAL_REQUEST_AXIS_CALIBRATION UNPACKING


/**
 * @brief Get field target_system from gimbal_request_axis_calibration message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_gimbal_request_axis_calibration_get_target_system(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  0);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  0);
#endif
}

/**
 * @brief Get field target_component from gimbal_request_axis_calibration message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_gimbal_request_axis_calibration_get_target_component(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  1);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  1);
#endif
}

/**
 * @brief Decode a gimbal_request_axis_calibration message into a struct
 *
 * @param msg The message to decode
 * @param gimbal_request_axis_calibration C-struct to decode the message contents into
 */
static inline void mavlink_msg_gimbal_request_axis_calibration_decode(const mavlink_message_t* msg, mavlink_gimbal_request_axis_calibration_t* gimbal_request_axis_calibration)
{
#if MAVLINK_NEED_BYTE_SWAP || MAVLINK_C2000
	gimbal_request_axis_calibration->target_system = mavlink_msg_gimbal_request_axis_calibration_get_target_system(msg);
	gimbal_request_axis_calibration->target_component = mavlink_msg_gimbal_request_axis_calibration_get_target_component(msg);
#else
	memcpy(gimbal_request_axis_calibration, _MAV_PAYLOAD(msg), MAVLINK_MSG_ID_GIMBAL_REQUEST_AXIS_CALIBRATION_LEN);
#endif
}
