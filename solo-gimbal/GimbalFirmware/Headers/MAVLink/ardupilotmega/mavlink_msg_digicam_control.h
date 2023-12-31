// MESSAGE DIGICAM_CONTROL PACKING

#if MAVLINK_C2000
#include "protocol_c2000.h"
#endif

#define MAVLINK_MSG_ID_DIGICAM_CONTROL 155

typedef struct __mavlink_digicam_control_t
{
 float extra_value; ///< Correspondent value to given extra_param
 uint8_t target_system; ///< System ID
 uint8_t target_component; ///< Component ID
 uint8_t session; ///< 0: stop, 1: start or keep it up //Session control e.g. show/hide lens
 uint8_t zoom_pos; ///< 1 to N //Zoom's absolute position (0 means ignore)
 int8_t zoom_step; ///< -100 to 100 //Zooming step value to offset zoom from the current position
 uint8_t focus_lock; ///< 0: unlock focus or keep unlocked, 1: lock focus or keep locked, 3: re-lock focus
 uint8_t shot; ///< 0: ignore, 1: shot or start filming
 uint8_t command_id; ///< Command Identity (incremental loop: 0 to 255)//A command sent multiple times will be executed or pooled just once
 uint8_t extra_param; ///< Extra parameters enumeration (0 means ignore)
} mavlink_digicam_control_t;

#define MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN 13
#define MAVLINK_MSG_ID_155_LEN 13

#define MAVLINK_MSG_ID_DIGICAM_CONTROL_CRC 22
#define MAVLINK_MSG_ID_155_CRC 22



#define MAVLINK_MESSAGE_INFO_DIGICAM_CONTROL { \
	"DIGICAM_CONTROL", \
	10, \
	{  { "extra_value", NULL, MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_digicam_control_t, extra_value) }, \
         { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_digicam_control_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 5, offsetof(mavlink_digicam_control_t, target_component) }, \
         { "session", NULL, MAVLINK_TYPE_UINT8_T, 0, 6, offsetof(mavlink_digicam_control_t, session) }, \
         { "zoom_pos", NULL, MAVLINK_TYPE_UINT8_T, 0, 7, offsetof(mavlink_digicam_control_t, zoom_pos) }, \
         { "zoom_step", NULL, MAVLINK_TYPE_INT8_T, 0, 8, offsetof(mavlink_digicam_control_t, zoom_step) }, \
         { "focus_lock", NULL, MAVLINK_TYPE_UINT8_T, 0, 9, offsetof(mavlink_digicam_control_t, focus_lock) }, \
         { "shot", NULL, MAVLINK_TYPE_UINT8_T, 0, 10, offsetof(mavlink_digicam_control_t, shot) }, \
         { "command_id", NULL, MAVLINK_TYPE_UINT8_T, 0, 11, offsetof(mavlink_digicam_control_t, command_id) }, \
         { "extra_param", NULL, MAVLINK_TYPE_UINT8_T, 0, 12, offsetof(mavlink_digicam_control_t, extra_param) }, \
         } \
}


/**
 * @brief Pack a digicam_control message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param session 0: stop, 1: start or keep it up //Session control e.g. show/hide lens
 * @param zoom_pos 1 to N //Zoom's absolute position (0 means ignore)
 * @param zoom_step -100 to 100 //Zooming step value to offset zoom from the current position
 * @param focus_lock 0: unlock focus or keep unlocked, 1: lock focus or keep locked, 3: re-lock focus
 * @param shot 0: ignore, 1: shot or start filming
 * @param command_id Command Identity (incremental loop: 0 to 255)//A command sent multiple times will be executed or pooled just once
 * @param extra_param Extra parameters enumeration (0 means ignore)
 * @param extra_value Correspondent value to given extra_param
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_digicam_control_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t target_system, uint8_t target_component, uint8_t session, uint8_t zoom_pos, int8_t zoom_step, uint8_t focus_lock, uint8_t shot, uint8_t command_id, uint8_t extra_param, float extra_value)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN];
	_mav_put_float(buf, 0, extra_value);
	_mav_put_uint8_t(buf, 4, target_system);
	_mav_put_uint8_t(buf, 5, target_component);
	_mav_put_uint8_t(buf, 6, session);
	_mav_put_uint8_t(buf, 7, zoom_pos);
	_mav_put_int8_t(buf, 8, zoom_step);
	_mav_put_uint8_t(buf, 9, focus_lock);
	_mav_put_uint8_t(buf, 10, shot);
	_mav_put_uint8_t(buf, 11, command_id);
	_mav_put_uint8_t(buf, 12, extra_param);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN);
#elif MAVLINK_C2000
		mav_put_float_c2000(&(msg->payload64[0]), 0, extra_value);
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 4, target_system);
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 5, target_component);
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 6, session);
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 7, zoom_pos);
		mav_put_int8_t_c2000(&(msg->payload64[0]), 8, zoom_step);
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 9, focus_lock);
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 10, shot);
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 11, command_id);
		mav_put_uint8_t_c2000(&(msg->payload64[0]), 12, extra_param);
	
	
#else
	mavlink_digicam_control_t packet;
	packet.extra_value = extra_value;
	packet.target_system = target_system;
	packet.target_component = target_component;
	packet.session = session;
	packet.zoom_pos = zoom_pos;
	packet.zoom_step = zoom_step;
	packet.focus_lock = focus_lock;
	packet.shot = shot;
	packet.command_id = command_id;
	packet.extra_param = extra_param;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_DIGICAM_CONTROL;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN, MAVLINK_MSG_ID_DIGICAM_CONTROL_CRC);
#else
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN);
#endif
}

/**
 * @brief Pack a digicam_control message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param session 0: stop, 1: start or keep it up //Session control e.g. show/hide lens
 * @param zoom_pos 1 to N //Zoom's absolute position (0 means ignore)
 * @param zoom_step -100 to 100 //Zooming step value to offset zoom from the current position
 * @param focus_lock 0: unlock focus or keep unlocked, 1: lock focus or keep locked, 3: re-lock focus
 * @param shot 0: ignore, 1: shot or start filming
 * @param command_id Command Identity (incremental loop: 0 to 255)//A command sent multiple times will be executed or pooled just once
 * @param extra_param Extra parameters enumeration (0 means ignore)
 * @param extra_value Correspondent value to given extra_param
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_digicam_control_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t target_system,uint8_t target_component,uint8_t session,uint8_t zoom_pos,int8_t zoom_step,uint8_t focus_lock,uint8_t shot,uint8_t command_id,uint8_t extra_param,float extra_value)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN];
	_mav_put_float(buf, 0, extra_value);
	_mav_put_uint8_t(buf, 4, target_system);
	_mav_put_uint8_t(buf, 5, target_component);
	_mav_put_uint8_t(buf, 6, session);
	_mav_put_uint8_t(buf, 7, zoom_pos);
	_mav_put_int8_t(buf, 8, zoom_step);
	_mav_put_uint8_t(buf, 9, focus_lock);
	_mav_put_uint8_t(buf, 10, shot);
	_mav_put_uint8_t(buf, 11, command_id);
	_mav_put_uint8_t(buf, 12, extra_param);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN);
#else
	mavlink_digicam_control_t packet;
	packet.extra_value = extra_value;
	packet.target_system = target_system;
	packet.target_component = target_component;
	packet.session = session;
	packet.zoom_pos = zoom_pos;
	packet.zoom_step = zoom_step;
	packet.focus_lock = focus_lock;
	packet.shot = shot;
	packet.command_id = command_id;
	packet.extra_param = extra_param;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_DIGICAM_CONTROL;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN, MAVLINK_MSG_ID_DIGICAM_CONTROL_CRC);
#else
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN);
#endif
}

/**
 * @brief Encode a digicam_control struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param digicam_control C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_digicam_control_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_digicam_control_t* digicam_control)
{
	return mavlink_msg_digicam_control_pack(system_id, component_id, msg, digicam_control->target_system, digicam_control->target_component, digicam_control->session, digicam_control->zoom_pos, digicam_control->zoom_step, digicam_control->focus_lock, digicam_control->shot, digicam_control->command_id, digicam_control->extra_param, digicam_control->extra_value);
}

/**
 * @brief Encode a digicam_control struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param digicam_control C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_digicam_control_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_digicam_control_t* digicam_control)
{
	return mavlink_msg_digicam_control_pack_chan(system_id, component_id, chan, msg, digicam_control->target_system, digicam_control->target_component, digicam_control->session, digicam_control->zoom_pos, digicam_control->zoom_step, digicam_control->focus_lock, digicam_control->shot, digicam_control->command_id, digicam_control->extra_param, digicam_control->extra_value);
}

/**
 * @brief Send a digicam_control message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param session 0: stop, 1: start or keep it up //Session control e.g. show/hide lens
 * @param zoom_pos 1 to N //Zoom's absolute position (0 means ignore)
 * @param zoom_step -100 to 100 //Zooming step value to offset zoom from the current position
 * @param focus_lock 0: unlock focus or keep unlocked, 1: lock focus or keep locked, 3: re-lock focus
 * @param shot 0: ignore, 1: shot or start filming
 * @param command_id Command Identity (incremental loop: 0 to 255)//A command sent multiple times will be executed or pooled just once
 * @param extra_param Extra parameters enumeration (0 means ignore)
 * @param extra_value Correspondent value to given extra_param
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_digicam_control_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint8_t session, uint8_t zoom_pos, int8_t zoom_step, uint8_t focus_lock, uint8_t shot, uint8_t command_id, uint8_t extra_param, float extra_value)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN];
	_mav_put_float(buf, 0, extra_value);
	_mav_put_uint8_t(buf, 4, target_system);
	_mav_put_uint8_t(buf, 5, target_component);
	_mav_put_uint8_t(buf, 6, session);
	_mav_put_uint8_t(buf, 7, zoom_pos);
	_mav_put_int8_t(buf, 8, zoom_step);
	_mav_put_uint8_t(buf, 9, focus_lock);
	_mav_put_uint8_t(buf, 10, shot);
	_mav_put_uint8_t(buf, 11, command_id);
	_mav_put_uint8_t(buf, 12, extra_param);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_DIGICAM_CONTROL, buf, MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN, MAVLINK_MSG_ID_DIGICAM_CONTROL_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_DIGICAM_CONTROL, buf, MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN);
#endif
#else
	mavlink_digicam_control_t packet;
	packet.extra_value = extra_value;
	packet.target_system = target_system;
	packet.target_component = target_component;
	packet.session = session;
	packet.zoom_pos = zoom_pos;
	packet.zoom_step = zoom_step;
	packet.focus_lock = focus_lock;
	packet.shot = shot;
	packet.command_id = command_id;
	packet.extra_param = extra_param;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_DIGICAM_CONTROL, (const char *)&packet, MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN, MAVLINK_MSG_ID_DIGICAM_CONTROL_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_DIGICAM_CONTROL, (const char *)&packet, MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN);
#endif
#endif
}

#if MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This varient of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_digicam_control_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t target_system, uint8_t target_component, uint8_t session, uint8_t zoom_pos, int8_t zoom_step, uint8_t focus_lock, uint8_t shot, uint8_t command_id, uint8_t extra_param, float extra_value)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char *buf = (char *)msgbuf;
	_mav_put_float(buf, 0, extra_value);
	_mav_put_uint8_t(buf, 4, target_system);
	_mav_put_uint8_t(buf, 5, target_component);
	_mav_put_uint8_t(buf, 6, session);
	_mav_put_uint8_t(buf, 7, zoom_pos);
	_mav_put_int8_t(buf, 8, zoom_step);
	_mav_put_uint8_t(buf, 9, focus_lock);
	_mav_put_uint8_t(buf, 10, shot);
	_mav_put_uint8_t(buf, 11, command_id);
	_mav_put_uint8_t(buf, 12, extra_param);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_DIGICAM_CONTROL, buf, MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN, MAVLINK_MSG_ID_DIGICAM_CONTROL_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_DIGICAM_CONTROL, buf, MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN);
#endif
#else
	mavlink_digicam_control_t *packet = (mavlink_digicam_control_t *)msgbuf;
	packet->extra_value = extra_value;
	packet->target_system = target_system;
	packet->target_component = target_component;
	packet->session = session;
	packet->zoom_pos = zoom_pos;
	packet->zoom_step = zoom_step;
	packet->focus_lock = focus_lock;
	packet->shot = shot;
	packet->command_id = command_id;
	packet->extra_param = extra_param;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_DIGICAM_CONTROL, (const char *)packet, MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN, MAVLINK_MSG_ID_DIGICAM_CONTROL_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_DIGICAM_CONTROL, (const char *)packet, MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN);
#endif
#endif
}
#endif

#endif

// MESSAGE DIGICAM_CONTROL UNPACKING


/**
 * @brief Get field target_system from digicam_control message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_digicam_control_get_target_system(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  4);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  4);
#endif
}

/**
 * @brief Get field target_component from digicam_control message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_digicam_control_get_target_component(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  5);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  5);
#endif
}

/**
 * @brief Get field session from digicam_control message
 *
 * @return 0: stop, 1: start or keep it up //Session control e.g. show/hide lens
 */
static inline uint8_t mavlink_msg_digicam_control_get_session(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  6);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  6);
#endif
}

/**
 * @brief Get field zoom_pos from digicam_control message
 *
 * @return 1 to N //Zoom's absolute position (0 means ignore)
 */
static inline uint8_t mavlink_msg_digicam_control_get_zoom_pos(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  7);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  7);
#endif
}

/**
 * @brief Get field zoom_step from digicam_control message
 *
 * @return -100 to 100 //Zooming step value to offset zoom from the current position
 */
static inline int8_t mavlink_msg_digicam_control_get_zoom_step(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_int8_t(msg,  8);
#else
	return mav_get_int8_t_c2000(&(msg->payload64[0]),  8);
#endif
}

/**
 * @brief Get field focus_lock from digicam_control message
 *
 * @return 0: unlock focus or keep unlocked, 1: lock focus or keep locked, 3: re-lock focus
 */
static inline uint8_t mavlink_msg_digicam_control_get_focus_lock(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  9);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  9);
#endif
}

/**
 * @brief Get field shot from digicam_control message
 *
 * @return 0: ignore, 1: shot or start filming
 */
static inline uint8_t mavlink_msg_digicam_control_get_shot(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  10);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  10);
#endif
}

/**
 * @brief Get field command_id from digicam_control message
 *
 * @return Command Identity (incremental loop: 0 to 255)//A command sent multiple times will be executed or pooled just once
 */
static inline uint8_t mavlink_msg_digicam_control_get_command_id(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  11);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  11);
#endif
}

/**
 * @brief Get field extra_param from digicam_control message
 *
 * @return Extra parameters enumeration (0 means ignore)
 */
static inline uint8_t mavlink_msg_digicam_control_get_extra_param(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_uint8_t(msg,  12);
#else
	return mav_get_uint8_t_c2000(&(msg->payload64[0]),  12);
#endif
}

/**
 * @brief Get field extra_value from digicam_control message
 *
 * @return Correspondent value to given extra_param
 */
static inline float mavlink_msg_digicam_control_get_extra_value(const mavlink_message_t* msg)
{
#if !MAVLINK_C2000
	return _MAV_RETURN_float(msg,  0);
#else
	return mav_get_float_c2000(&(msg->payload64[0]),  0);
#endif
}

/**
 * @brief Decode a digicam_control message into a struct
 *
 * @param msg The message to decode
 * @param digicam_control C-struct to decode the message contents into
 */
static inline void mavlink_msg_digicam_control_decode(const mavlink_message_t* msg, mavlink_digicam_control_t* digicam_control)
{
#if MAVLINK_NEED_BYTE_SWAP || MAVLINK_C2000
	digicam_control->extra_value = mavlink_msg_digicam_control_get_extra_value(msg);
	digicam_control->target_system = mavlink_msg_digicam_control_get_target_system(msg);
	digicam_control->target_component = mavlink_msg_digicam_control_get_target_component(msg);
	digicam_control->session = mavlink_msg_digicam_control_get_session(msg);
	digicam_control->zoom_pos = mavlink_msg_digicam_control_get_zoom_pos(msg);
	digicam_control->zoom_step = mavlink_msg_digicam_control_get_zoom_step(msg);
	digicam_control->focus_lock = mavlink_msg_digicam_control_get_focus_lock(msg);
	digicam_control->shot = mavlink_msg_digicam_control_get_shot(msg);
	digicam_control->command_id = mavlink_msg_digicam_control_get_command_id(msg);
	digicam_control->extra_param = mavlink_msg_digicam_control_get_extra_param(msg);
#else
	memcpy(digicam_control, _MAV_PAYLOAD(msg), MAVLINK_MSG_ID_DIGICAM_CONTROL_LEN);
#endif
}
