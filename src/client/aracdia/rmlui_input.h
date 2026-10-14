// Luanti / Aracdia
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "config.h"

#if ENABLE_ARACDIA_RMLUI

struct SEvent;

namespace Rml {
class Context;
}

namespace AracdiaRmlInput {

bool feedEvent(Rml::Context *context, const SEvent &event);

} // namespace AracdiaRmlInput

#endif // ENABLE_ARACDIA_RMLUI
