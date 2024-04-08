#pragma once

#include "resource.h"

void startLoop();
void stopLoop();
DWORD WINAPI workerThreadFunction(LPVOID lpParam);
LPSTR GetHText();
LPSTR GetDelayText();
void makeWinTrransparent();
void sendKeyEvent(WORD vk, WORD scanCode, DWORD flags);
void sendMouseEvent(LONG dx, LONG dy, DWORD mouseData, DWORD flags, DWORD time);
void clearRecording();
void startRecording();
void stopRecording();
void playRecording();