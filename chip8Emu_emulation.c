#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>

#include "chip8Emu.h"



// Get User Input
void handle_user_input(chip8_t* c8)
{
  SDL_Event main_events;

  // Chip8 Keypad:
  // 123C     1234
  // 456D     QWER
  // 789E     ASDF
  // A0BF     ZXCV

  while (SDL_PollEvent(&main_events))
  {
    if (main_events.type == SDL_QUIT)
    {
      c8->emu_state = QUIT;
      return;
    }

    else if (main_events.type == SDL_KEYDOWN)
    {
      switch (main_events.key.keysym.sym)
      {
        case SDLK_SPACE:    
        {  
          if (c8->emu_state == RUNNING)
          {
            puts("STATE PAUSED");
            c8->emu_state = PAUSE;
          }
          else
          {
            puts("STATE RESUMED");
            c8->emu_state = RUNNING;
          }
          return;
        }

        case SDLK_ESCAPE:   c8->emu_state = QUIT;   return;

        case SDLK_1:  c8->emu_keypad[0x01] = true;  break;
        case SDLK_2:  c8->emu_keypad[0x02] = true;  break;
        case SDLK_3:  c8->emu_keypad[0x03] = true;  break;
        case SDLK_4:  c8->emu_keypad[0x0C] = true;  break;

        case SDLK_q:  c8->emu_keypad[0x04] = true;  break;
        case SDLK_w:  c8->emu_keypad[0x05] = true;  break;
        case SDLK_e:  c8->emu_keypad[0x06] = true;  break;
        case SDLK_r:  c8->emu_keypad[0x0D] = true;  break;

        case SDLK_a:  c8->emu_keypad[0x07] = true;  break;
        case SDLK_s:  c8->emu_keypad[0x08] = true;  break;
        case SDLK_d:  c8->emu_keypad[0x09] = true;  break;
        case SDLK_f:  c8->emu_keypad[0x0E] = true;  break;

        case SDLK_z:  c8->emu_keypad[0x0A] = true;  break;
        case SDLK_x:  c8->emu_keypad[0x00] = true;  break;
        case SDLK_c:  c8->emu_keypad[0x0B] = true;  break;
        case SDLK_v:  c8->emu_keypad[0x0F] = true;  break;
        
        default:  break;
      }
    }

    else if (main_events.type == SDL_KEYUP)
    {
      switch (main_events.key.keysym.sym)
      {
        case SDLK_1:  c8->emu_keypad[0x01] = false; break;
        case SDLK_2:  c8->emu_keypad[0x02] = false;  break;
        case SDLK_3:  c8->emu_keypad[0x03] = false;  break;
        case SDLK_4:  c8->emu_keypad[0x0C] = false;  break;

        case SDLK_q:  c8->emu_keypad[0x04] = false;  break;
        case SDLK_w:  c8->emu_keypad[0x05] = false;  break;
        case SDLK_e:  c8->emu_keypad[0x06] = false;  break;
        case SDLK_r:  c8->emu_keypad[0x0D] = false;  break;

        case SDLK_a:  c8->emu_keypad[0x07] = false;  break;
        case SDLK_s:  c8->emu_keypad[0x08] = false;  break;
        case SDLK_d:  c8->emu_keypad[0x09] = false;  break;
        case SDLK_f:  c8->emu_keypad[0x0E] = false;  break;

        case SDLK_z:  c8->emu_keypad[0x0A] = false;  break;
        case SDLK_x:  c8->emu_keypad[0x00] = false;  break;
        case SDLK_c:  c8->emu_keypad[0x0B] = false;  break;
        case SDLK_v:  c8->emu_keypad[0x0F] = false;  break;

        default:  break;
      }
    }

    else
    {
      // Do Nothing (unsupported event)
    }
  }
}


// Emulate Chip8 Instructions
void emulate_instructions(chip8_t* c8, user_config_params_t* cfg)
{
  // Get Current Instruction OPCODE using PC and RAM (Shift it since C8 is BigEndian and we are LittleEndian)
  uint16_t inst_opcode = (c8->emu_ram[c8->emu_pc] << 8) | (c8->emu_ram[c8->emu_pc+1]);
  
  // Break down instruction
  uint16_t inst_nnn = inst_opcode & 0x0FFF;             // 12 Bit Address
  uint8_t  inst_nn  = inst_opcode & 0x00FF;             // 8 Bit constant
  uint8_t  inst_n   = inst_opcode & 0x000F;             // 4 Bit constant
  uint8_t  inst_x   = (inst_opcode & 0x0F00) >> 8;      // 4 Bit register identifier
  uint8_t  inst_y   = (inst_opcode & 0x00F0) >> 4;      // 4 Bit register identifier
  uint8_t  inst_op  = (inst_opcode & 0xF000) >> 12;     // Identify type/category of instruction

  // Increase PC for next instruction
  c8->emu_pc += 2;

  switch (inst_op)
  {
    case 0x00:
    {
      if (inst_nn == 0xE0)  // 00E0: Clear Screen
      { 
        // Clear Screen 
        memset(&(c8->emu_display[0]), false, sizeof(c8->emu_display));
      }
      else if (inst_nn == 0xEE) // 00EE: Return from subroutine
      {
        // Set PC to the PC from the subroutine stack
        // Decrement first since it is currently pointing to the "next" stack location where a PC will be stored
        c8->emu_subrStack_ptr--;
        c8->emu_pc = *(c8->emu_subrStack_ptr);
      }
      else
      {
        // Invalid opcode ... maybe 0xNNN
      }
      break;
    }

    case 0x01:        // 1NNN: Jump to Address NNN
    {
      c8->emu_pc = inst_nnn;
      break;
    }

    case 0x02:        // 2NNN: Call Subroutine at MemoryAddr NNN
    {
      // Derefernce the stack pointer for the subroutine stack and point it to the incremented PC
      // So, after returning from the SubR, it executes the next instruction (kind of like saving state)
      // Then set, PC to NNN to jump to executing that instruction
      // Increment StackPtr to point to next stack location   (in case two subroutines are stacked)
      *(c8->emu_subrStack_ptr) = c8->emu_pc;
      c8->emu_pc = inst_nnn;
      c8->emu_subrStack_ptr++;
      break;
    }

    case 0x03:      // 3XNN: If V[X] == NN, skip next instruction
    {
      if (c8->emu_V[inst_x] == inst_nn) 
        c8->emu_pc += 2;

      break;
    }

    case 0x04:      // 4XNN: If V[X] != NN, skip next instruction
    {
      if (c8->emu_V[inst_x] != inst_nn) 
        c8->emu_pc += 2;
      
      break;
    }

    case 0x05:      // 4XNN: If V[X] == V[Y], skip next instruction
    {
      if (inst_n != 0)      // Wrong opcode
        break;

      if (c8->emu_V[inst_x] == c8->emu_V[inst_y]) 
        c8->emu_pc += 2;

      break;
    }

    case 0x06:      // 6XNN: Set Data Register VX to NN
    {
      c8->emu_V[inst_x] = inst_nn;
      break;
    }

    case 0x07:      // 7XNN: Add NN to VX (carry flag not changed)
    {
      c8->emu_V[inst_x] += inst_nn;
      break;
    }
    
    case 0x08:
    {
      if (inst_n == 0x00)
        c8->emu_V[inst_x] = c8->emu_V[inst_y];       // 8XY0: Set VX equal to VY

      else if (inst_n == 0x01)
        c8->emu_V[inst_x] |= c8->emu_V[inst_y];      // 8XY1: Set VX equal to VX OR VY

      else if (inst_n == 0x02)
        c8->emu_V[inst_x] &= c8->emu_V[inst_y];      // 8XY2: Set VX equal to VX AND VY

      else if (inst_n == 0x03)
        c8->emu_V[inst_x] ^= c8->emu_V[inst_y];      // 8XY3: Set VX equal to VX XOR VY

      else if (inst_n == 0x04)                                 // 8XY4: Set VX += VY and set VF to 1 if carry
      {
        if ((uint16_t)(c8->emu_V[inst_x] + c8->emu_V[inst_y]) > 255)
          c8->emu_V[0x0F] = 1;
          
        c8->emu_V[inst_x] += c8->emu_V[inst_y];
      }

      else if (inst_n == 0x05)                                 // 8XY5: Set VX -= VY and set VF to 1 if no borrow
      {
        c8->emu_V[0x0F] = (c8->emu_V[inst_x] >= c8->emu_V[inst_y]); 
        c8->emu_V[inst_x] -= c8->emu_V[inst_y];
      }

      else if (inst_n == 0x06)                                 // 8XY6: Store LSb of VX in VF amd shift VX right by 1
      {
        c8->emu_V[0x0F] = c8->emu_V[inst_x] & 0x01;
        c8->emu_V[inst_x] >>= 1;
      }

      else if (inst_n == 0x07)                                 // 8XY7: Set VX = VY - vX and set VF to 1 if no borrow
      {
        c8->emu_V[0x0F] = (c8->emu_V[inst_y] >= c8->emu_V[inst_x]);  
        c8->emu_V[inst_x] = c8->emu_V[inst_y] - c8->emu_V[inst_x];
      }

      else if (inst_n == 0x0E)                                 // 8XYE: Store MSb of VX in VF amd shift VX left by 1
      {
        c8->emu_V[0x0F] = (c8->emu_V[inst_x] & 0x80) >> 7;
        c8->emu_V[inst_x] <<= 1;
      }

      else
      {
        // Wrong OPCODE
      }

      break;
    }


    case 0x09:      // 9XY0: Skip next instruction if VX != VY
    {
      if (c8->emu_V[inst_x] != c8->emu_V[inst_y]) 
        c8->emu_pc += 2;

      break;
    }

    case 0x0A:       // ANNN: Set Memory Index Register I to NNN
    {
      c8->emu_I = inst_nnn;
      break;
    }

    case 0x0B:       // BNNN: Jump to V0 + NNN
    {
      c8->emu_pc = c8->emu_V[0] + inst_nnn;
      break;
    }

    case 0x0C:       // CXNN: VX = rand() & NN
    {
      c8->emu_V[inst_x] = (rand() % 256) & (inst_nn);
      break;
    }

    case 0x0D:      // DXYN: Draw N height Sprite at Coordinate XY
    {
      // Read from mem location I
      // Screen pixels are XOR-ed with sprite bits
      // VF (carry flag) is set if any scren pixels are set off (useful for collision detection)

      // Get Coordinates
      uint8_t x_cor = c8->emu_V[inst_x] % cfg->window_width;
      uint8_t y_cor = c8->emu_V[inst_y] % cfg->window_height;
      const uint8_t x_cor_original = x_cor;

      // Set carry flag to 0  
      c8->emu_V[0x0F] = 0;

      // Loop over all N rows of the sprite
      for (uint8_t i=0; i<inst_n; i++)
      {
        // First, get the next byte/row of the sprite data and reset x for the next row
        const uint8_t sprite_data = c8->emu_ram[c8->emu_I + i];
        x_cor = x_cor_original;

        for (int8_t j=7; j>=0; j--)
        {
          bool *pixel = &c8->emu_display[y_cor * cfg->window_width + x_cor];
          bool sprite_bit = (sprite_data & (1 << j));

          if (sprite_bit && *pixel) 
            c8->emu_V[0x0F] = 1;

          *pixel ^= sprite_bit;

          if (++x_cor >= cfg->window_width) {break;}
        }

        if (++y_cor >= cfg->window_height) {break;}
      }

      break;
    }

    case 0x0E:
    {
      if (inst_nn == 0x9E)          // EX9E: Skip Next Instruction if Key in VX is Pressed
      {
        if (c8->emu_keypad[c8->emu_V[inst_x]])
          c8->emu_pc += 2;
      }

      else if (inst_nn == 0xA1)     // EXA1: Skip Next Instruction if Key in VX is not Pressed
      {
        if (!c8->emu_keypad[c8->emu_V[inst_x]])
          c8->emu_pc += 2;
      }

      else
      {
        // Wrong OPCODE
      }

      break;
    }

    case 0x0F:
    {
      if (inst_nn == 0x0A)        // FX0A: Await until key press and store in VX
      {
        bool is_key_pressed = false;

        for (uint8_t i=0; i< sizeof(c8->emu_keypad); i++)
        {
          if (c8->emu_keypad[i])
          {
            c8->emu_V[inst_x] = i;
            is_key_pressed = true;
            break;
          }
        }

        // Will keep executing the current instruction
        if (!is_key_pressed)
        {
          c8->emu_pc -= 2;
        }
      }

      else if (inst_nn == 0x1E)        // FX1E:  Add VX to register I
      {
        c8->emu_I += c8->emu_V[inst_x];
      }

      else if (inst_nn == 0x07)        // FX07:  VX = delay timer
      {
        c8->emu_V[inst_x] = c8->emu_delayTimer;
      }

      else if (inst_nn == 0x15)        // FX15:  delay timer = VX
      {
        c8->emu_delayTimer = c8->emu_V[inst_x];
      }

      else if (inst_nn == 0x18)        // FX18:  sound timer = VX
      {
        c8->emu_soundTimer = c8->emu_V[inst_x];
      }

      else if (inst_nn == 0x29)        // FX29:  Set reg I to sprite location in mem for character VX
      {
        c8->emu_I = c8->emu_V[inst_x] * 5;
      }

      else if (inst_nn == 0x33)        // FX33:  Store Binary code decimal represenation of VX at mem offset of I
      {
        uint8_t binary_code_decimal = c8->emu_V[inst_x];
        c8->emu_ram[c8->emu_I + 2] = binary_code_decimal % 10;
        binary_code_decimal /= 10;
        c8->emu_ram[c8->emu_I + 1] = binary_code_decimal % 10;
        binary_code_decimal /= 10;
        c8->emu_ram[c8->emu_I + 0] = binary_code_decimal % 10;
      }

      else if (inst_nn == 0x55)        // FX55:  Dump V regs from V0 to VX to mem offset from I
      {
        for (uint8_t i=0; i <=inst_x; i++)
        {
          c8->emu_ram[c8->emu_I + i] = c8->emu_V[i];
        }
      }

      else if (inst_nn == 0x65)        // FX65:  Load V regs from V0 to VX from mem offset from I
      {
        for (uint8_t i=0; i <=inst_x; i++)
        {
          c8->emu_V[i] = c8->emu_ram[c8->emu_I + i]; 
        }
      }

      else
      {
        // Wrong OPCODE
      }

      break;
    }
 
    default:
      break;
  }
}


// Update the window
void update_window(sdl_params_t* sdl_params, user_config_params_t* cfg, chip8_t* c8)
{
  // Rectangle object representing each pixel
  SDL_Rect sdl_rectangle = {.x=0, .y=0, .w=cfg->scale_factor, .h=cfg->scale_factor};

  // Get foreground and background colors
  uint8_t fg_r = (cfg->fg_color >> (32- 8)) & 0xFF;
  uint8_t fg_g = (cfg->fg_color >> (32-16)) & 0xFF;
  uint8_t fg_b = (cfg->fg_color >> (32-24)) & 0xFF;
  uint8_t fg_a = (cfg->fg_color >> (32-32)) & 0xFF;

  uint8_t bg_r = (cfg->bg_color >> (32- 8)) & 0xFF;
  uint8_t bg_g = (cfg->bg_color >> (32-16)) & 0xFF;
  uint8_t bg_b = (cfg->bg_color >> (32-24)) & 0xFF;
  uint8_t bg_a = (cfg->bg_color >> (32-32)) & 0xFF;

  // Render the text in the top 10% of the screen
  const char* emulator_name = "Chip8Emu"; // Your emulator name
  TTF_Font* font = TTF_OpenFont("Poxast-R9.ttf", 40); // Load a font
  if (font == NULL) 
  {
    // Handle error if the font cannot be loaded
    printf("Error loading font: %s\n", TTF_GetError());
    return;
  }

  // Render text surface
  SDL_Color text_color = {255, 255, 255, 255}; // Use the foreground color for the text
  SDL_Surface* text_surface = TTF_RenderText_Blended(font, emulator_name, text_color);
  if (text_surface == NULL) 
  {
    // Handle error if text rendering fails
    printf("Error rendering text: %s\n", TTF_GetError());
    return;
  }

  // Create a texture from the surface
  SDL_Texture* text_texture = SDL_CreateTextureFromSurface(sdl_params->main_renderer, text_surface);
  SDL_FreeSurface(text_surface); // Free the surface as it's no longer needed

  // Get the width and height of the text texture
  int text_width = 0, text_height = 0;
  SDL_QueryTexture(text_texture, NULL, NULL, &text_width, &text_height);

  // Set the position to display the text in the top 10% of the window
  SDL_Rect text_rect = { 
      .x = (cfg->side_border) * cfg->scale_factor,
      .y = 0, // Place it at the top of the screen
      .w = text_width,
      .h = text_height
  };

  // Render the text texture
  SDL_RenderCopy(sdl_params->main_renderer, text_texture, NULL, &text_rect);
  SDL_DestroyTexture(text_texture); // Free the texture after rendering

  // Render main game display
  for (uint32_t i=0; i<(sizeof(c8->emu_display)); i++)
  {
    // Emu display is a 1D array representing a 2D screen. Get X and Y coordingates
    sdl_rectangle.x = ((i % cfg->window_width) + cfg->side_border) * cfg->scale_factor;
    sdl_rectangle.y = ((i / cfg->window_width) + cfg->top_border) * cfg->scale_factor;

    // If Pixel is on, draw foreground color
    if (c8->emu_display[i])
    {
      SDL_SetRenderDrawColor(sdl_params->main_renderer, fg_r, fg_g, fg_b, fg_a);
      SDL_RenderFillRect(sdl_params->main_renderer, &sdl_rectangle);
    }

    // If Pixel is on, draw background color
    else
    {
      SDL_SetRenderDrawColor(sdl_params->main_renderer, bg_r, bg_g, bg_b, bg_a);
      SDL_RenderFillRect(sdl_params->main_renderer, &sdl_rectangle);
    }
    
    // Pixel Outline Config (optional)
    if (cfg->pixel_outlines)
    {
      SDL_SetRenderDrawColor(sdl_params->main_renderer, bg_r, bg_g, bg_b, bg_a);
      SDL_RenderDrawRect(sdl_params->main_renderer, &sdl_rectangle);
    }
  }

  SDL_RenderPresent(sdl_params->main_renderer);
  TTF_CloseFont(font);
}


// Update chip8 timers
void update_timers(chip8_t* c8)
{
  if (c8->emu_delayTimer > 0)
    c8->emu_delayTimer--;

  if (c8->emu_soundTimer > 0)
    c8->emu_soundTimer--;

  else
  {
    // Stop Playing sound
  }
}