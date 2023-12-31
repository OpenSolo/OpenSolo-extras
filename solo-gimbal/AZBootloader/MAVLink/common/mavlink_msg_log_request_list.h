// MESSAGE LOG_REQUEST_LIST PACKING

#if MAVLINK_C2000
#include "protocol_c2000.h"
#endif

#define MAVLINK_MSG_ID_LOG_REQUEST_LIST 117

typedef struct __mavlink_log_request_list_t
{
 uint16_t start; ///< First log id (0 for first available)
 uint16_t end; ///< Last log id (0xffff for last available)
 uint8_t target_system; ///< System ID
 uint8_t target_component; ///< Component ID
} mavlink_log_request_list_t;

#define MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN 6
#define MAVLINK_MSG_ID_117_LEN 6

#define MAVLINK_MSG_ID_LOG_REQUEST_LIST_CRC 128
#define MAVLINK_MSG_ID_117_CRC 128



#define MAVLINK_MESSAGE_INFO_LOG_REQUEST_LIST { \
	"LOG_REQUEST_LIST", \
	4, \
	{  { "start", NULL, MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_log_request_list_t, start) }, \
         { "end", NULL, MAVLINK_TYPE_UINT16_T, 0, 2, offsetof(mavlink_log_request_list_t, end) }, \
         { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_log_request_list_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 5, offsetof(mavlink_log_request_list_t, target_component) }, \
         } \
}


/**
 * @brief Pack a log_request_list message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param start First log id (0 for first available)
 * @param end Last log id (0xffff for last available)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_log_request_list_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t target_system, uint8_t target_component, uint16_t start, uint16_t end)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN];
	_mav_put_uint16_t(buf, 0, start);
	_mav_put_uint16_t(buf, 2, end);
	_mav_put_uint8_t(buf, 4, target_system);
	_mav_put_uint8_t(buf, 5, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN);
#elif MAVLINK_C2000
		mav_put_uint16_t_c2000(&(msg->payload64[0]), 0, start);
		mav_put_uint16_t_c2000(&(msg->payload64[0]), 2, end);
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 4, target_system);
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 5, target_component);
	
	
#else
	mavlink_log_request_list_t packet;
	packet.start = start;
	packet.end = end;
	packet.target_system = target_system;
	packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_LOG_REQUEST_LIST;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN, MAVLINK_MSG_ID_LOG_REQUEST_LIST_CRC);
#else
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN);
#endif
}

/**
 * @brief Pack a log_request_list message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param start First log id (0 for first available)
 * @param end Last log id (0xffff for last available)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_log_request_list_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t target_system,uint8_t target_component,uint16_t start,uint16_t end)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN];
	_mav_put_uint16_t(buf, 0, start);
	_mav_put_uint16_t(buf, 2, end);
	_mav_put_uint8_t(buf, 4, target_system);
	_mav_put_uint8_t(buf, 5, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN);
#else
	mavlink_log_request_list_t packet;
	packet.start = start;
	packet.end = end;
	packet.target_system = target_system;
	packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_LOG_REQUEST_LIST;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN, MAVLINK_MSG_ID_LOG_REQUEST_LIST_CRC);
#else
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN);
#endif
}

/**
 * @brief Encode a log_request_list struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param log_request_list C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_log_request_list_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_log_request_list_t* log_request_list)
{
	return mavlink_msg_log_request_list_pack(system_id, component_id, msg, log_request_list->target_system, log_request_list->target_component, log_request_list->start, log_request_list->end);
}

/**
 * @brief Encode a log_request_list struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param log_request_list C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_log_request_list_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_log_request_list_t* log_request_list)
{
	return mavlink_msg_log_request_list_pack_chan(system_id, component_id, chan, msg, log_request_list->target_system, log_request_list->target_component, log_request_list->start, log_request_list->end);
}

/**
 * @brief Send a log_request_list message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param start First log id (0 for first available)
 * @param end Last log id (0xffff for last available)
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_log_request_list_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint16_t start, uint16_t end)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN];
	_mav_put_uint16_t(buf, 0, start);
	_mav_put_uint16_t(buf, 2, end);
	_mav_put_uint8_t(buf, 4, target_system);
	_mav_put_uint8_t(buf, 5, target_component);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LOG_REQUEST_LIST, buf, MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN, MAVLINK_MSG_ID_LOG_REQUEST_LIST_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LOG_REQUEST_LIST, buf, MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN);
#endif
#else
	mavlink_log_request_list_t packet;
	packet.start = start;
	packet.end = end;
	packet.target_system = target_system;
	packet.target_component = target_component;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LOG_REQUEST_LIST, (const char *)&packet, MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN, MAVLINK_MSG_ID_LOG_REQUEST_LIST_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LOG_REQUEST_LIST, (const char *)&packet, MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN);
#endif
#endif
}

#if MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This varient of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_log_request_list_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t target_system, uint8_t target_component, uint16_t start, uint16_t end)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char *buf = (char *)msgbuf;
	_mav_put_uint16_t(buf, 0, start);
	_mav_put_uint16_t(buf, 2, end);
	_mav_put_uint8_t(buf, 4, target_system);
	_mav_put_uint8_t(buf, 5, target_component);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LOG_REQUEST_LIST, buf, MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN, MAVLINK_MSG_ID_LOG_REQUEST_LIST_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LOG_REQUEST_LIST, buf, MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN);
#endif
#else
	mavlink_log_request_list_t *packet = (mavlink_log_request_list_t *)msgbuf;
	packet->start = start;
	packet->end = end;
	packet->target_system = target_system;
	packet->target_component = target_component;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LOG_REQUEST_LIST, (const char *)packet, MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN, MAVLINK_MSG_ID_LOG_REQUEST_LIST_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LOG_REQUEST_LIST, (const char *)packet, MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN);
#endif
#endif
}
#endif

#endif

// MESSAGE LOG_REQUEST_LIST UNPACKING


/**
 * @brief Get field target_system from log_request_list message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_log_request_list_get_target_system(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  4);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  4);
#endif
}

/**
 * @brief Get field target_component from log_request_list message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_log_request_list_get_target_component(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  5);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  5);
#endif
}

/**
 * @brief Get field start from log_request_list message
 *
 * @return First log id (0 for first available)
 */
static inline uint16_t mavlink_msg_log_request_list_get_start(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint16_t(msg,  0);
#else
	return mav_get_uint16_t_c2000(&(msg->payload64[0]),  0);
#endif
}

/**
 * @brief Get field end from log_request_list message
 *
 * @return Last log id (0xffff for last available)
 */
static inline uint16_t mavlink_msg_log_request_list_get_end(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint16_t(msg,  2);
#else
	return mav_get_uint16_t_c2000(&(msg->payload64[0]),  2);
#endif
}

/**
 * @brief Decode a log_request_list message into a struct
 *
 * @param msg The message to decode
 * @param log_request_list C-struct to decode the message contents into
 */
static inline void mavlink_msg_log_request_list_decode(const mavlink_message_t* msg, mavlink_log_request_list_t* log_request_list)
{
#if MAVLINK_NEED_BYTE_SWAP || MAVLINK_C2000
	log_request_list->start = mavlink_msg_log_request_list_get_start(msg);
	log_request_list->end = mavlink_msg_log_request_list_get_end(msg);
	log_request_list->target_system = mavlink_msg_log_request_list_get_target_system(msg);
	log_request_list->target_component = mavlink_msg_log_request_list_get_target_component(msg);
#else
	memcpy(log_request_list, _MAV_PAYLOAD(msg), MAVLINK_MSG_ID_LOG_REQUEST_LIST_LEN);
#endif
}
