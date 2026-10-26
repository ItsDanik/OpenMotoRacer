#ifndef INPUT_H
#define INPUT_H

/* The keys held, by the Windows key codes the game reads (GetAsyncKeyState in the race) */
void input_key(int vk, int down);
int input_down(int vk);
int input_pressed(int vk);      /* went down since the last call for this key */

#endif
