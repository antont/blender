/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contributor(s):
 *   Mike Erwin
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include "GHOST_NDOFManager.h"
#include "GHOST_EventNDOF.h"
#include "GHOST_EventKey.h"
#include "GHOST_WindowManager.h"
#include <string.h> // for memory functions
#include <stdio.h> // for error/info reporting

#ifdef DEBUG_NDOF_BUTTONS
static const char* ndof_button_names[] = {
	// used internally, never sent
	"NDOF_BUTTON_NONE",
	// these two are available from any 3Dconnexion device
	"NDOF_BUTTON_MENU",
	"NDOF_BUTTON_FIT",
	// standard views
	"NDOF_BUTTON_TOP",
	"NDOF_BUTTON_BOTTOM",
	"NDOF_BUTTON_LEFT",
	"NDOF_BUTTON_RIGHT",
	"NDOF_BUTTON_FRONT",
	"NDOF_BUTTON_BACK",
	// more views
	"NDOF_BUTTON_ISO1",
	"NDOF_BUTTON_ISO2",
	// 90 degree rotations
	"NDOF_BUTTON_ROLL_CW",
	"NDOF_BUTTON_ROLL_CCW",
	"NDOF_BUTTON_SPIN_CW",
	"NDOF_BUTTON_SPIN_CCW",
	"NDOF_BUTTON_TILT_CW",
	"NDOF_BUTTON_TILT_CCW",
	// device control
	"NDOF_BUTTON_ROTATE",
	"NDOF_BUTTON_PANZOOM",
	"NDOF_BUTTON_DOMINANT",
	"NDOF_BUTTON_PLUS",
	"NDOF_BUTTON_MINUS",
	// general-purpose buttons
	"NDOF_BUTTON_1",
	"NDOF_BUTTON_2",
	"NDOF_BUTTON_3",
	"NDOF_BUTTON_4",
	"NDOF_BUTTON_5",
	"NDOF_BUTTON_6",
	"NDOF_BUTTON_7",
	"NDOF_BUTTON_8",
	"NDOF_BUTTON_9",
	"NDOF_BUTTON_10",
	};
#endif

static const NDOF_ButtonT SpaceNavigator_HID_map[] =
	{
	NDOF_BUTTON_MENU,
	NDOF_BUTTON_FIT
	};

static const NDOF_ButtonT SpaceExplorer_HID_map[] =
	{
	NDOF_BUTTON_1,
	NDOF_BUTTON_2,
	NDOF_BUTTON_TOP,
	NDOF_BUTTON_LEFT,
	NDOF_BUTTON_RIGHT,
	NDOF_BUTTON_FRONT,
	NDOF_BUTTON_NONE, // esc key
	NDOF_BUTTON_NONE, // alt key
	NDOF_BUTTON_NONE, // shift key
	NDOF_BUTTON_NONE, // ctrl key
	NDOF_BUTTON_FIT,
	NDOF_BUTTON_MENU,
	NDOF_BUTTON_PLUS,
	NDOF_BUTTON_MINUS,
	NDOF_BUTTON_ROTATE
	};

static const NDOF_ButtonT SpacePilotPro_HID_map[] =
	{
	NDOF_BUTTON_MENU,
	NDOF_BUTTON_FIT,
	NDOF_BUTTON_TOP,
	NDOF_BUTTON_LEFT,
	NDOF_BUTTON_RIGHT,
	NDOF_BUTTON_FRONT,
	NDOF_BUTTON_BOTTOM,
	NDOF_BUTTON_BACK,
	NDOF_BUTTON_ROLL_CW,
	NDOF_BUTTON_ROLL_CCW,
	NDOF_BUTTON_ISO1,
	NDOF_BUTTON_ISO2,
	NDOF_BUTTON_1,
	NDOF_BUTTON_2,
	NDOF_BUTTON_3,
	NDOF_BUTTON_4,
	NDOF_BUTTON_5,
	NDOF_BUTTON_6,
	NDOF_BUTTON_7,
	NDOF_BUTTON_8,
	NDOF_BUTTON_9,
	NDOF_BUTTON_10,
	NDOF_BUTTON_NONE, // esc key
	NDOF_BUTTON_NONE, // alt key
	NDOF_BUTTON_NONE, // shift key
	NDOF_BUTTON_NONE, // ctrl key
	NDOF_BUTTON_ROTATE,
	NDOF_BUTTON_PANZOOM,
	NDOF_BUTTON_DOMINANT,
	NDOF_BUTTON_PLUS,
	NDOF_BUTTON_MINUS
	};

GHOST_NDOFManager::GHOST_NDOFManager(GHOST_System& sys)
	: m_system(sys)
	, m_deviceType(NDOF_UnknownDevice) // each platform needs its own device detection code
	, m_buttons(0)
	, m_motionTime(1000) // one full second (operators should filter out such large time deltas)
	, m_prevMotionTime(0)
	, m_atRest(true)
	{
	// to avoid the rare situation where one triple is updated and
	// the other is not, initialize them both here:
	memset(m_translation, 0, sizeof(m_translation));
	memset(m_rotation, 0, sizeof(m_rotation));
	}

void GHOST_NDOFManager::setDevice(unsigned short vendor_id, unsigned short product_id)
	{
	switch (vendor_id)
		{
		case 0x046D: // Logitech (3Dconnexion)
			switch (product_id)
				{
				// -- current devices --
				case 0xC626: m_deviceType = NDOF_SpaceNavigator; break;
				case 0xC628: m_deviceType = NDOF_SpaceNavigator; /* for Notebooks */ break;
				case 0xC627: m_deviceType = NDOF_SpaceExplorer; break;
				case 0xC629: m_deviceType = NDOF_SpacePilotPro; break;

				// -- older devices --
				case 0xC623: puts("ndof: SpaceTraveler not supported, please file a bug report"); break;
				case 0xC625: puts("ndof: SpacePilot not supported, please file a bug report"); break;

				default: printf("ndof: unknown Logitech product %04hx\n", product_id);
				}
			break;
		default:
			printf("ndof: unknown vendor %04hx\n", vendor_id);
		}
	}

void GHOST_NDOFManager::updateTranslation(short t[3], GHOST_TUns64 time)
	{
	memcpy(m_translation, t, sizeof(m_translation));
	m_motionTime = time;
	m_atRest = false;
	}

void GHOST_NDOFManager::updateRotation(short r[3], GHOST_TUns64 time)
	{
	memcpy(m_rotation, r, sizeof(m_rotation));
	m_motionTime = time;
	m_atRest = false;
	}

void GHOST_NDOFManager::sendButtonEvent(NDOF_ButtonT button, bool press, GHOST_TUns64 time, GHOST_IWindow* window)
	{
	GHOST_EventNDOFButton* event = new GHOST_EventNDOFButton(time, window);
	GHOST_TEventNDOFButtonData* data = (GHOST_TEventNDOFButtonData*) event->getData();

	data->action = press ? GHOST_kPress : GHOST_kRelease;
	data->button = button;

	#ifdef DEBUG_NDOF_BUTTONS
	printf("%s %s\n", ndof_button_names[button], press ? "pressed" : "released");
	#endif

	m_system.pushEvent(event);
	}

void GHOST_NDOFManager::sendKeyEvent(GHOST_TKey key, bool press, GHOST_TUns64 time, GHOST_IWindow* window)
	{
	GHOST_TEventType type = press ? GHOST_kEventKeyDown : GHOST_kEventKeyUp;
	GHOST_EventKey* event = new GHOST_EventKey(time, type, window, key);

	#ifdef DEBUG_NDOF_BUTTONS
	printf("keyboard %s\n", press ? "down" : "up");
	#endif

	m_system.pushEvent(event);
	}

void GHOST_NDOFManager::updateButton(int button_number, bool press, GHOST_TUns64 time)
	{
	GHOST_IWindow* window = m_system.getWindowManager()->getActiveWindow();

	#ifdef DEBUG_NDOF_BUTTONS
	printf("ndof: button %d -> ", button_number);
	#endif

	switch (m_deviceType)
		{
		case NDOF_SpaceNavigator:
			sendButtonEvent(SpaceNavigator_HID_map[button_number], press, time, window);
			break;
		case NDOF_SpaceExplorer:
			switch (button_number)
				{
				case 6: sendKeyEvent(GHOST_kKeyEsc, press, time, window); break;
				case 7: sendKeyEvent(GHOST_kKeyLeftAlt, press, time, window); break;
				case 8: sendKeyEvent(GHOST_kKeyLeftShift, press, time, window); break;
				case 9: sendKeyEvent(GHOST_kKeyLeftControl, press, time, window); break;
				default: sendButtonEvent(SpaceExplorer_HID_map[button_number], press, time, window);
				}
			break;
		case NDOF_SpacePilotPro:
			switch (button_number)
				{
				case 22: sendKeyEvent(GHOST_kKeyEsc, press, time, window); break;
				case 23: sendKeyEvent(GHOST_kKeyLeftAlt, press, time, window); break;
				case 24: sendKeyEvent(GHOST_kKeyLeftShift, press, time, window); break;
				case 25: sendKeyEvent(GHOST_kKeyLeftControl, press, time, window); break;
				default: sendButtonEvent(SpacePilotPro_HID_map[button_number], press, time, window);
				}
			break;
		case NDOF_UnknownDevice:
			printf("ndof: button %d on unknown device (ignoring)\n", button_number);
		}

	int mask = 1 << button_number;
	if (press)
		m_buttons |= mask; // set this button's bit
	else
		m_buttons &= ~mask; // clear this button's bit
	}

void GHOST_NDOFManager::updateButtons(int button_bits, GHOST_TUns64 time)
	{
	int diff = m_buttons ^ button_bits;

	for (int button_number = 0; button_number <= 31; ++button_number)
		{
		int mask = 1 << button_number;

		if (diff & mask)
			{
			bool press = button_bits & mask;
			updateButton(button_number, press, time);
			}
		}
	}

bool GHOST_NDOFManager::sendMotionEvent()
	{
	if (m_atRest)
		return false;

	GHOST_IWindow* window = m_system.getWindowManager()->getActiveWindow();

	GHOST_EventNDOFMotion* event = new GHOST_EventNDOFMotion(m_motionTime, window);
	GHOST_TEventNDOFMotionData* data = (GHOST_TEventNDOFMotionData*) event->getData();

	const float scale = 1.f / 350.f; // SpaceNavigator sends +/- 350 usually
	// 350 according to their developer's guide; others recommend 500 as comfortable

	// possible future enhancement
	// scale *= m_sensitivity;

	data->tx = -scale * m_translation[0];
	data->ty = scale * m_translation[1];
	data->tz = scale * m_translation[2];

	data->rx = scale * m_rotation[0];
	data->ry = scale * m_rotation[1];
	data->rz = scale * m_rotation[2];

	data->dt = 0.001f * (m_motionTime - m_prevMotionTime); // in seconds

	m_prevMotionTime = m_motionTime;

	#ifdef DEBUG_NDOF_MOTION
	printf("ndof: T=(%.2f,%.2f,%.2f) R=(%.2f,%.2f,%.2f) dt=%.3f\n",
		data->tx, data->ty, data->tz, data->rx, data->ry, data->rz, data->dt);
	#endif

	m_system.pushEvent(event);

	// 'at rest' test goes at the end so that the first 'rest' event gets sent
	m_atRest = m_rotation[0] == 0 && m_rotation[1] == 0 && m_rotation[2] == 0 &&
		m_translation[0] == 0 && m_translation[1] == 0 && m_translation[2] == 0;
	// this needs to be aware of calibration -- 0.01 0.01 0.03 might be 'rest'

	return true;
	}
