// MESSAGE REPORT_AXIS_LIMITS_TEST_PROGRESS PACKING

#if MAVLINK_C2000
#include "protocol_c2000.h"
#endif

#define MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS 198

typedef struct __mavlink_report_axis_limits_test_progress_t
{
 uint8_t test_axis; ///< Which gimbal axis we're reporting test progress for
 uint8_t test_section_progress; ///< The progress of the current test section, 0x64=100%
 uint8_t test_status; ///< The status of the currently executing test section
} mavlink_report_axis_limits_test_progress_t;

#define MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN 3
#define MAVLINK_MSG_ID_198_LEN 3

#define MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_CRC 182
#define MAVLINK_MSG_ID_198_CRC 182



#define MAVLINK_MESSAGE_INFO_REPORT_AXIS_LIMITS_TEST_PROGRESS { \
	"REPORT_AXIS_LIMITS_TEST_PROGRESS", \
	3, \
	{  { "test_axis", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_report_axis_limits_test_progress_t, test_axis) }, \
         { "test_section_progress", NULL, MAVLINK_TYPE_UINT8_T, 0, 1, offsetof(mavlink_report_axis_limits_test_progress_t, test_section_progress) }, \
         { "test_status", NULL, MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_report_axis_limits_test_progress_t, test_status) }, \
         } \
}


/**
 * @brief Pack a report_axis_limits_test_progress message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param test_axis Which gimbal axis we're reporting test progress for
 * @param test_section_progress The progress of the current test section, 0x64=100%
 * @param test_status The status of the currently executing test section
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_report_axis_limits_test_progress_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t test_axis, uint8_t test_section_progress, uint8_t test_status)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN];
	_mav_put_uint8_t(buf, 0, test_axis);
	_mav_put_uint8_t(buf, 1, test_section_progress);
	_mav_put_uint8_t(buf, 2, test_status);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN);
#elif MAVLINK_C2000
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 0, test_axis);
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 1, test_section_progress);
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 2, test_status);
	
	
#else
	mavlink_report_axis_limits_test_progress_t packet;
	packet.test_axis = test_axis;
	packet.test_section_progress = test_section_progress;
	packet.test_status = test_status;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_CRC);
#else
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN);
#endif
}

/**
 * @brief Pack a report_axis_limits_test_progress message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param test_axis Which gimbal axis we're reporting test progress for
 * @param test_section_progress The progress of the current test section, 0x64=100%
 * @param test_status The status of the currently executing test section
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_report_axis_limits_test_progress_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t test_axis,uint8_t test_section_progress,uint8_t test_status)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN];
	_mav_put_uint8_t(buf, 0, test_axis);
	_mav_put_uint8_t(buf, 1, test_section_progress);
	_mav_put_uint8_t(buf, 2, test_status);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN);
#else
	mavlink_report_axis_limits_test_progress_t packet;
	packet.test_axis = test_axis;
	packet.test_section_progress = test_section_progress;
	packet.test_status = test_status;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_CRC);
#else
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN);
#endif
}

/**
 * @brief Encode a report_axis_limits_test_progress struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param report_axis_limits_test_progress C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_report_axis_limits_test_progress_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_report_axis_limits_test_progress_t* report_axis_limits_test_progress)
{
	return mavlink_msg_report_axis_limits_test_progress_pack(system_id, component_id, msg, report_axis_limits_test_progress->test_axis, report_axis_limits_test_progress->test_section_progress, report_axis_limits_test_progress->test_status);
}

/**
 * @brief Encode a report_axis_limits_test_progress struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param report_axis_limits_test_progress C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_report_axis_limits_test_progress_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_report_axis_limits_test_progress_t* report_axis_limits_test_progress)
{
	return mavlink_msg_report_axis_limits_test_progress_pack_chan(system_id, component_id, chan, msg, report_axis_limits_test_progress->test_axis, report_axis_limits_test_progress->test_section_progress, report_axis_limits_test_progress->test_status);
}

/**
 * @brief Send a report_axis_limits_test_progress message
 * @param chan MAVLink channel to send the message
 *
 * @param test_axis Which gimbal axis we're reporting test progress for
 * @param test_section_progress The progress of the current test section, 0x64=100%
 * @param test_status The status of the currently executing test section
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_report_axis_limits_test_progress_send(mavlink_channel_t chan, uint8_t test_axis, uint8_t test_section_progress, uint8_t test_status)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN];
	_mav_put_uint8_t(buf, 0, test_axis);
	_mav_put_uint8_t(buf, 1, test_section_progress);
	_mav_put_uint8_t(buf, 2, test_status);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS, buf, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS, buf, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN);
#endif
#else
	mavlink_report_axis_limits_test_progress_t packet;
	packet.test_axis = test_axis;
	packet.test_section_progress = test_section_progress;
	packet.test_status = test_status;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS, (const char *)&packet, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS, (const char *)&packet, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN);
#endif
#endif
}

#if MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This varient of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_report_axis_limits_test_progress_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t test_axis, uint8_t test_section_progress, uint8_t test_status)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char *buf = (char *)msgbuf;
	_mav_put_uint8_t(buf, 0, test_axis);
	_mav_put_uint8_t(buf, 1, test_section_progress);
	_mav_put_uint8_t(buf, 2, test_status);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS, buf, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS, buf, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN);
#endif
#else
	mavlink_report_axis_limits_test_progress_t *packet = (mavlink_report_axis_limits_test_progress_t *)msgbuf;
	packet->test_axis = test_axis;
	packet->test_section_progress = test_section_progress;
	packet->test_status = test_status;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS, (const char *)packet, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS, (const char *)packet, MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN);
#endif
#endif
}
#endif

#endif

// MESSAGE REPORT_AXIS_LIMITS_TEST_PROGRESS UNPACKING


/**
 * @brief Get field test_axis from report_axis_limits_test_progress message
 *
 * @return Which gimbal axis we're reporting test progress for
 */
static inline uint8_t mavlink_msg_report_axis_limits_test_progress_get_test_axis(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  0);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  0);
#endif
}

/**
 * @brief Get field test_section_progress from report_axis_limits_test_progress message
 *
 * @return The progress of the current test section, 0x64=100%
 */
static inline uint8_t mavlink_msg_report_axis_limits_test_progress_get_test_section_progress(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  1);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  1);
#endif
}

/**
 * @brief Get field test_status from report_axis_limits_test_progress message
 *
 * @return The status of the currently executing test section
 */
static inline uint8_t mavlink_msg_report_axis_limits_test_progress_get_test_status(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  2);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  2);
#endif
}

/**
 * @brief Decode a report_axis_limits_test_progress message into a struct
 *
 * @param msg The message to decode
 * @param report_axis_limits_test_progress C-struct to decode the message contents into
 */
static inline void mavlink_msg_report_axis_limits_test_progress_decode(const mavlink_message_t* msg, mavlink_report_axis_limits_test_progress_t* report_axis_limits_test_progress)
{
#if MAVLINK_NEED_BYTE_SWAP || MAVLINK_C2000
	report_axis_limits_test_progress->test_axis = mavlink_msg_report_axis_limits_test_progress_get_test_axis(msg);
	report_axis_limits_test_progress->test_section_progress = mavlink_msg_report_axis_limits_test_progress_get_test_section_progress(msg);
	report_axis_limits_test_progress->test_status = mavlink_msg_report_axis_limits_test_progress_get_test_status(msg);
#else
	memcpy(report_axis_limits_test_progress, _MAV_PAYLOAD(msg), MAVLINK_MSG_ID_REPORT_AXIS_LIMITS_TEST_PROGRESS_LEN);
#endif
}
