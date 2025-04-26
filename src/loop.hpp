#ifndef LOOP_HPP
#define LOOP_HPP

#include "other.hpp"
#include "ui/deviceMenu.hpp"
#include "ui/blockedDeviceManager.hpp" // Add the include for blockedDeviceManager

void handleButtonPress(bool button1Pressed, bool button2Pressed);
void handleDiscoveryState(bool button1Pressed, bool button2Pressed);
void handleDeviceSelectionState(bool button1Pressed, bool button2Pressed);
void handleFriendActionMenuState(bool button1Pressed, bool button2Pressed);
void handleFriendRequestState(bool button1Pressed, bool button2Pressed);
void handlePendingRequestsState(bool button1Pressed, bool button2Pressed);
void handleBlockedDevicesListState(bool button1Pressed, bool button2Pressed); // Add declaration
void handleBlockedDeviceActionState(bool button1Pressed, bool button2Pressed); // Add declaration
void performPeriodicTasks();
void handleSerialCommands();
void showToast(const String& message, uint16_t bgColor);
void listDiscoveredDevices();

#endif