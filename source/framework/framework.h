// Copyright [2025] [Nicholas Walton]
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "framework_types.h"

#include "c23defs.h"
#include "osinterface.h"
#include "utilities.h"

static inline mouse_button_e ConvertMouseButton (os_mouse_button_e button) {
	switch (button) {
		case os_MOUSE_LEFT: return MOUSE_LEFT;
		case os_MOUSE_RIGHT: return MOUSE_RIGHT;
		case os_MOUSE_MIDDLE: return MOUSE_MIDDLE;
		case os_MOUSE_X1: return MOUSE_X1;
		case os_MOUSE_X2: return MOUSE_X2;
		default: UNREACHABLE ("%s", "Invalid mouse button! Check the os_mouse_button_e enum for valid values!!! >:(");
	}
	unreachable();
}

extern os_public_t os_public;
extern os_private_t os_private;

#include "log.h"
#include "cereal.h"
#include "DEFER.h"
#include "discrete_random.h"
#include "explorer.h"
#include "folders.h"
#include "resources.h"
#include "render.h"
#include "menu.h"
#include "game_exports.h"
#include "samples.h"
#include "sprite.h"
#include "sound.h"
#include "update.h"
#include "zen_timer.h"

#ifndef OSINTERFACE_FRAME_BUFFER_SCALED
#error "You must define OSINTERFACE_FRAME_BUFFER_SCALED"
#endif
#ifndef OSINTERFACE_EVENT_AND_RENDER_THREADS_ARE_SEPARATE
#error "You must define OSINTERFACE_EVENT_AND_RENDER_THREADS_ARE_SEPARATE"
#endif
#ifndef OSINTERFACE_COLOR_INDEX_MODE
#error "You must define OSINTERFACE_COLOR_INDEX_MODE"
#endif