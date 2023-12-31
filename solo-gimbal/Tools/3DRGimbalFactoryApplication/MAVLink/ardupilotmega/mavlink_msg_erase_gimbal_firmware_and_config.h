// MESSAGE ERASE_GIMBAL_FIRMWARE_AND_CONFIG PACKING

#if MAVLINK_C2000
#include "protocol_c2000.h"
#endif

#define MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG 196

typedef struct __mavlink_erase_gimbal_firmware_and_config_t
{
 uint32_t knock; ///< Knock value to confirm this is a valid request
 uint8_t target_system; ///< System ID
 uint8_t target_component; ///< Component ID
} mavlink_erase_gimbal_firmware_and_config_t;

#define MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN 6
#define MAVLINK_MSG_ID_196_LEN 6

#define MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_CRC 255
#define MAVLINK_MSG_ID_196_CRC 255



#define MAVLINK_MESSAGE_INFO_ERASE_GIMBAL_FIRMWARE_AND_CONFIG { \
	"ERASE_GIMBAL_FIRMWARE_AND_CONFIG", \
	3, \
	{  { "knock", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_erase_gimbal_firmware_and_config_t, knock) }, \
         { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_erase_gimbal_firmware_and_config_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 5, offsetof(mavlink_erase_gimbal_firmware_and_config_t, target_component) }, \
         } \
}


/**
 * @brief Pack a erase_gimbal_firmware_and_config message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param knock Knock value to confirm this is a valid request
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_erase_gimbal_firmware_and_config_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t target_system, uint8_t target_component, uint32_t knock)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN];
	_mav_put_uint32_t(buf, 0, knock);
	_mav_put_uint8_t(buf, 4, target_system);
	_mav_put_uint8_t(buf, 5, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN);
#elif MAVLINK_C2000
		mav_put_uint32_t_c2000(&(msg->payload64[0]), 0, knock);
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 4, target_system);
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 5, target_component);
	
	
#else
	mavlink_erase_gimbal_firmware_and_config_t packet;
	packet.knock = knock;
	packet.target_system = target_system;
	packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_CRC);
#else
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN);
#endif
}

/**
 * @brief Pack a erase_gimbal_firmware_and_config message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param knock Knock value to confirm this is a valid request
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_erase_gimbal_firmware_and_config_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t target_system,uint8_t target_component,uint32_t knock)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN];
	_mav_put_uint32_t(buf, 0, knock);
	_mav_put_uint8_t(buf, 4, target_system);
	_mav_put_uint8_t(buf, 5, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN);
#else
	mavlink_erase_gimbal_firmware_and_config_t packet;
	packet.knock = knock;
	packet.target_system = target_system;
	packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_CRC);
#else
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN);
#endif
}

/**
 * @brief Encode a erase_gimbal_firmware_and_config struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param erase_gimbal_firmware_and_config C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_erase_gimbal_firmware_and_config_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_erase_gimbal_firmware_and_config_t* erase_gimbal_firmware_and_config)
{
	return mavlink_msg_erase_gimbal_firmware_and_config_pack(system_id, component_id, msg, erase_gimbal_firmware_and_config->target_system, erase_gimbal_firmware_and_config->target_component, erase_gimbal_firmware_and_config->knock);
}

/**
 * @brief Encode a erase_gimbal_firmware_and_config struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param erase_gimbal_firmware_and_config C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_erase_gimbal_firmware_and_config_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_erase_gimbal_firmware_and_config_t* erase_gimbal_firmware_and_config)
{
	return mavlink_msg_erase_gimbal_firmware_and_config_pack_chan(system_id, component_id, chan, msg, erase_gimbal_firmware_and_config->target_system, erase_gimbal_firmware_and_config->target_component, erase_gimbal_firmware_and_config->knock);
}

/**
 * @brief Send a erase_gimbal_firmware_and_config message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param knock Knock value to confirm this is a valid request
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_erase_gimbal_firmware_and_config_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint32_t knock)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN];
	_mav_put_uint32_t(buf, 0, knock);
	_mav_put_uint8_t(buf, 4, target_system);
	_mav_put_uint8_t(buf, 5, target_component);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG, buf, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG, buf, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN);
#endif
#else
	mavlink_erase_gimbal_firmware_and_config_t packet;
	packet.knock = knock;
	packet.target_system = target_system;
	packet.target_component = target_component;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG, (const char *)&packet, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG, (const char *)&packet, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN);
#endif
#endif
}

#if MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This varient of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_erase_gimbal_firmware_and_config_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t target_system, uint8_t target_component, uint32_t knock)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char *buf = (char *)msgbuf;
	_mav_put_uint32_t(buf, 0, knock);
	_mav_put_uint8_t(buf, 4, target_system);
	_mav_put_uint8_t(buf, 5, target_component);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG, buf, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG, buf, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN);
#endif
#else
	mavlink_erase_gimbal_firmware_and_config_t *packet = (mavlink_erase_gimbal_firmware_and_config_t *)msgbuf;
	packet->knock = knock;
	packet->target_system = target_system;
	packet->target_component = target_component;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG, (const char *)packet, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG, (const char *)packet, MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN);
#endif
#endif
}
#endif

#endif

// MESSAGE ERASE_GIMBAL_FIRMWARE_AND_CONFIG UNPACKING


/**
 * @brief Get field target_system from erase_gimbal_firmware_and_config message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_erase_gimbal_firmware_and_config_get_target_system(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  4);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  4);
#endif
}

/**
 * @brief Get field target_component from erase_gimbal_firmware_and_config message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_erase_gimbal_firmware_and_config_get_target_component(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  5);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  5);
#endif
}

/**
 * @brief Get field knock from erase_gimbal_firmware_and_config message
 *
 * @return Knock value to confirm this is a valid request
 */
static inline uint32_t mavlink_msg_erase_gimbal_firmware_and_config_get_knock(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint32_t(msg,  0);
#else
	return mav_get_uint32_t_c2000(&(msg->payload64[0]),  0);
#endif
}

/**
 * @brief Decode a erase_gimbal_firmware_and_config message into a struct
 *
 * @param msg The message to decode
 * @param erase_gimbal_firmware_and_config C-struct to decode the message contents into
 */
static inline void mavlink_msg_erase_gimbal_firmware_and_config_decode(const mavlink_message_t* msg, mavlink_erase_gimbal_firmware_and_config_t* erase_gimbal_firmware_and_config)
{
#if MAVLINK_NEED_BYTE_SWAP || MAVLINK_C2000
	erase_gimbal_firmware_and_config->knock = mavlink_msg_erase_gimbal_firmware_and_config_get_knock(msg);
	erase_gimbal_firmware_and_config->target_system = mavlink_msg_erase_gimbal_firmware_and_config_get_target_system(msg);
	erase_gimbal_firmware_and_config->target_component = mavlink_msg_erase_gimbal_firmware_and_config_get_target_component(msg);
#else
	memcpy(erase_gimbal_firmware_and_config, _MAV_PAYLOAD(msg), MAVLINK_MSG_ID_ERASE_GIMBAL_FIRMWARE_AND_CONFIG_LEN);
#endif
}
