#ifndef __STACK_H__
#define __STACK_H__

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

void Start();
void Stop();
void Reload();
void ActivateNext();
void ActivatePrev();
void MoveForward();
void MoveBackward();
void ShowDesktop(int desktop);
void MoveToDesktop(int desktop);
void ToggleDynamic();
void AddMaster(int nb);

#endif
