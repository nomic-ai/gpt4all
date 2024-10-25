#include "macosdock.h"

#include <Cocoa/Cocoa.h>

void MacOSDock::showIcon()
{
    [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyRegular];
}

void MacOSDock::hideIcon()
{
    [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyProhibited];
}
