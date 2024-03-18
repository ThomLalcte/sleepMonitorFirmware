#ifndef BUTTON_H
#define BUTTON_H

#define buttonPin 5

void initButton();
void buttonTask();
void setButtonPressedCallback(void (*callback)());

#endif