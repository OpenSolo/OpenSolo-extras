// MESSAGE NAMED_VALUE_INT PACKING

#if MAVLINK_C2000
#include "protocol_c2000.h"
#endif

#define MAVLINK_MSG_ID_NAMED_VALUE_INT 252

typedef struct __mavlink_named_value_int_t
{
 uint32_t time_boot_ms; ///< Timestamp (milliseconds since system boot)
 int32_t value; ///< Signed integer value
 char name[10]; ///< Name of the debug variable
} mavlink_named_value_int_t;

#define MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN 18
#define MAVLINK_MSG_ID_252_LEN 18

#define MAVLINK_MSG_ID_NAMED_VALUE_INT_CRC 44
#define MAVLINK_MSG_ID_252_CRC 44

#define MAVLINK_MSG_NAMED_VALUE_INT_FIELD_NAME_LEN 10

#define MAVLINK_MESSAGE_INFO_NAMED_VALUE_INT { \
	"NAMED_VALUE_INT", \
	3, \
	{  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_named_value_int_t, time_boot_ms) }, \
         { "value", NULL, MAVLINK_TYPE_INT32_T, 0, 4, offsetof(mavlink_named_value_int_t, value) }, \
         { "name", NULL, MAVLINK_TYPE_CHAR, 10, 8, offsetof(mavlink_named_value_int_t, name) }, \
         } \
}


/**
 * @brief Pack a named_value_int message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms Timestamp (milliseconds since system boot)
 * @param name Name of the debug variable
 * @param value Signed integer value
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_named_value_int_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint32_t time_boot_ms, const char *name, int32_t value)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN];
	_mav_put_uint32_t(buf, 0, time_boot_ms);
	_mav_put_int32_t(buf, 4, value);
	_mav_put_char_array(buf, 8, name, 10);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN);
#elif MAVLINK_C2000
		mav_put_uint32_t_c2000(&(msg->payload64[0]), 0, time_boot_ms);
		mav_put_int32_t_c2000(&(msg->payload64[0]), 4, value);
	
		mav_put_char_array_c2000(&(msg->payload64[0]), name, 8, 10);
	
#else
	mavlink_named_value_int_t packet;
	packet.time_boot_ms = time_boot_ms;
	packet.value = value;
	mav_array_memcpy(packet.name, name, sizeof(char)*10);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_NAMED_VALUE_INT;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN, MAVLINK_MSG_ID_NAMED_VALUE_INT_CRC);
#else
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN);
#endif
}

/**
 * @brief Pack a named_value_int message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_boot_ms Timestamp (milliseconds since system boot)
 * @param name Name of the debug variable
 * @param value Signed integer value
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_named_value_int_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint32_t time_boot_ms,const char *name,int32_t value)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN];
	_mav_put_uint32_t(buf, 0, time_boot_ms);
	_mav_put_int32_t(buf, 4, value);
	_mav_put_char_array(buf, 8, name, 10);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN);
#else
	mavlink_named_value_int_t packet;
	packet.time_boot_ms = time_boot_ms;
	packet.value = value;
	mav_array_memcpy(packet.name, name, sizeof(char)*10);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_NAMED_VALUE_INT;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN, MAVLINK_MSG_ID_NAMED_VALUE_INT_CRC);
#else
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN);
#endif
}

/**
 * @brief Encode a named_value_int struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param named_value_int C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_named_value_int_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_named_value_int_t* named_value_int)
{
	return mavlink_msg_named_value_int_pack(system_id, component_id, msg, named_value_int->time_boot_ms, named_value_int->name, named_value_int->value);
}

/**
 * @brief Encode a named_value_int struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param named_value_int C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_named_value_int_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_named_value_int_t* named_value_int)
{
	return mavlink_msg_named_value_int_pack_chan(system_id, component_id, chan, msg, named_value_int->time_boot_ms, named_value_int->name, named_value_int->value);
}

/**
 * @brief Send a named_value_int message
 * @param chan MAVLink channel to send the message
 *
 * @param time_boot_ms Timestamp (milliseconds since system boot)
 * @param name Name of the debug variable
 * @param value Signed integer value
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_named_value_int_send(mavlink_channel_t chan, uint32_t time_boot_ms, const char *name, int32_t value)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN];
	_mav_put_uint32_t(buf, 0, time_boot_ms);
	_mav_put_int32_t(buf, 4, value);
	_mav_put_char_array(buf, 8, name, 10);
#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NAMED_VALUE_INT, buf, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN, MAVLINK_MSG_ID_NAMED_VALUE_INT_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NAMED_VALUE_INT, buf, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN);
#endif
#else
	mavlink_named_value_int_t packet;
	packet.time_boot_ms = time_boot_ms;
	packet.value = value;
	mav_array_memcpy(packet.name, name, sizeof(char)*10);
#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NAMED_VALUE_INT, (const char *)&packet, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN, MAVLINK_MSG_ID_NAMED_VALUE_INT_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NAMED_VALUE_INT, (const char *)&packet, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN);
#endif
#endif
}

#if MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This varient of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_named_value_int_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t time_boot_ms, const char *name, int32_t value)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char *buf = (char *)msgbuf;
	_mav_put_uint32_t(buf, 0, time_boot_ms);
	_mav_put_int32_t(buf, 4, value);
	_mav_put_char_array(buf, 8, name, 10);
#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NAMED_VALUE_INT, buf, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN, MAVLINK_MSG_ID_NAMED_VALUE_INT_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NAMED_VALUE_INT, buf, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN);
#endif
#else
	mavlink_named_value_int_t *packet = (mavlink_named_value_int_t *)msgbuf;
	packet->time_boot_ms = time_boot_ms;
	packet->value = value;
	mav_array_memcpy(packet->name, name, sizeof(char)*10);
#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NAMED_VALUE_INT, (const char *)packet, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN, MAVLINK_MSG_ID_NAMED_VALUE_INT_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NAMED_VALUE_INT, (const char *)packet, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN);
#endif
#endif
}
#endif

#endif

// MESSAGE NAMED_VALUE_INT UNPACKING


/**
 * @brief Get field time_boot_ms from named_value_int message
 *
 * @return Timestamp (milliseconds since system boot)
 */
static inline uint32_t mavlink_msg_named_value_int_get_time_boot_ms(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint32_t(msg,  0);
#else
	return mav_get_uint32_t_c2000(&(msg->payload64[0]),  0);
#endif
}

/**
 * @brief Get field name from named_value_int message
 *
 * @return Name of the debug variable
 */
static inline uint16_t mavlink_msg_named_value_int_get_name(const mavlink_message_t* msg, char *name)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_char_array(msg, name, 10,  8);
#else
	return mav_get_char_array_c2000(&(msg->payload64[0]), name, 10,  8);
#endif
}

/**
 * @brief Get field value from named_value_int message
 *
 * @return Signed integer value
 */
static inline int32_t mavlink_msg_named_value_int_get_value(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_int32_t(msg,  4);
#else
	return mav_get_int32_t_c2000(&(msg->payload64[0]),  4);
#endif
}

/**
 * @brief Decode a named_value_int message into a struct
 *
 * @param msg The message to decode
 * @param named_value_int C-struct to decode the message contents into
 */
static inline void mavlink_msg_named_value_int_decode(const mavlink_message_t* msg, mavlink_named_value_int_t* named_value_int)
{
#if MAVLINK_NEED_BYTE_SWAP || MAVLINK_C2000
	named_value_int->time_boot_ms = mavlink_msg_named_value_int_get_time_boot_ms(msg);
	named_value_int->value = mavlink_msg_named_value_int_get_value(msg);
	mavlink_msg_named_value_int_get_name(msg, named_value_int->name);
#else
	memcpy(named_value_int, _MAV_PAYLOAD(msg), MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN);
#endif
}
