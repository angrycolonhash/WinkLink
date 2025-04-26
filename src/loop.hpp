#ifndef LOOP_HPP
#define LOOP_HPP

#include "other.hpp"
#include "ui/deviceMenu.hpp"

void handleButtonPress(bool button1Pressed, bool button2Pressed);
void handleDiscoveryState(bool button1Pressed, bool button2Pressed);
void handleDeviceSelectionState(bool button1Pressed, bool button2Pressed);
void handleFriendActionMenuState(bool button1Pressed, bool button2Pressed);
void handleFriendRequestState(bool button1Pressed, bool button2Pressed);
void performPeriodicTasks();
void handleSerialCommands();
void showToast(const String& message, uint16_t bgColor);
void listDiscoveredDevices(); // New helper function for serial commands

#endif