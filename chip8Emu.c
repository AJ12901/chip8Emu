#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <time.h>

#include "chip8Emu.h"

/*
 *
 *
 *    INITIALIZATION AND SETUP FUNCTIONS
 *
 * 
 */

// Initialize user configuration settings received from the CLI
bool init_user_configuration(user_config_params_t* cfg_params, int num_args, char** args_array)
{
  // Setup Default User Parameters
  cfg_params->scale_factor = 15;
  cfg_params->window_height = 32;
  cfg_params->window_width = 64;
  cfg_params->pixel_outlines = true;
  cfg_params->instructions_per_second = 500;

  cfg_params->fg_color = 0xFFFFFFFF;
  cfg_params->bg_color = 0x00000000;

  // If arguments are passed, override defaults
  for (int i=0; i<num_args; i++)
  {
    printf("Argument %d: %s\n", i, args_array[i]);
  }

  return true;
}

// Run once to initialize the SDL parameters (return true if initialized)
bool init_sdl(sdl_params_t* sdl_parameters, user_config_params_t config_parameters)
{
  // Try to initialize SDL
  if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
  {
    SDL_Log("Could not init SDL stuff ... exiting! %s\n", SDL_GetError());
    return false;
  }

  sdl_parameters->main_window = SDL_CreateWindow("Chip8Emu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (config_parameters.window_width * config_parameters.scale_factor), (config_parameters.window_height * config_parameters.scale_factor), 0);

  // Try to create SDL Window
  if (sdl_parameters->main_window == NULL)
  {
    SDL_Log("Could not create window ... exiting! %s\n", SDL_GetError());
    return false;
  }

  sdl_parameters->main_renderer = SDL_CreateRenderer(sdl_parameters->main_window, -1, SDL_RENDERER_ACCELERATED);

  // Try to create window renderer
  if (sdl_parameters->main_renderer == NULL)
  { 
    SDL_Log("Could not create renderer ... exiting! %s\n", SDL_GetError());
    return false;
  }

  return true;
}

// Initialize an instance of a chip8
bool init_chip8(chip8_t* c8, const char rom_name[])
{
  // Programs are generally loaded at RAM location 0x200
  const uint16_t program_entry_point = 0x200;
  
  // Each symbol is represented by a list of 5 bytes representing which of the 40 bits are on/off for that symbol
  // For example, for the letter E, we can see that in binary, the 1's represent an "E" below:
  // 1111 0000
  // 1000 0000
  // 1111 0000
  // 1000 0000
  // 1111 0000

  const uint8_t system_font[] = 
  {
      0xF0, 0x90, 0x90, 0x90, 0xF0,   // 0
      0x20, 0x60, 0x20, 0x20, 0x70,   // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0,   // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0,   // 3
      0x90, 0x90, 0xF0, 0x10, 0x10,   // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0,   // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0,   // 6
      0xF0, 0x10, 0x20, 0x40, 0x40,   // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0,   // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0,   // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90,   // A
      0xF0, 0x90, 0xF0, 0x90, 0xF0,   // B
      0xF0, 0x80, 0x80, 0x80, 0xF0,   // C
      0xF0, 0x90, 0x90, 0x90, 0xF0,   // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0,   // E
      0xF0, 0x80, 0xF0, 0x80, 0x80    // F
  };

  // Load font set (digits 0-9 and letters A-F)
  memcpy(&c8->emu_ram[0], &system_font, sizeof(system_font));

  // Read ROM Data
  FILE* rom_data = fopen(rom_name, "rb");
  if (!rom_data)
  {
    SDL_Log("chip8 ROM file %s cannot be read", rom_name);
    return false;
  }

  // Get ROM size
  fseek(rom_data, 0, SEEK_END);             // Move filePtr to end of file
  const size_t rom_size = ftell(rom_data);  // Get current filePtr
  const size_t max_rom_size = sizeof(c8->emu_ram) - program_entry_point;
  rewind(rom_data);

  if (rom_size > max_rom_size)
  {
    SDL_Log("ROM file size %u ... max allowed size %u.", (unsigned int)rom_size, (unsigned int)max_rom_size);
    return false;
  }

  // Load ROM data
  if (fread(&c8->emu_ram[program_entry_point], rom_size, 1, rom_data) != 1)
  {
    SDL_Log("Error when reading ROM file into chip8 RAM");
    return false;
  }

  fclose(rom_data);

  // Set C8 defaults
  c8->emu_state = RUNNING;
  c8->emu_pc = program_entry_point;
  c8->emu_romName = rom_name;
  c8->emu_subrStack_ptr = &c8->emu_subrStack[0];

  return true;
}

// Clear window to the background color
void clear_window(sdl_params_t* sdl_parameters, user_config_params_t* cfg_params)
{
  uint8_t r = (cfg_params->bg_color >> (32- 8)) & 0xFF;
  uint8_t g = (cfg_params->bg_color >> (32-16)) & 0xFF;
  uint8_t b = (cfg_params->bg_color >> (32-24)) & 0xFF;
  uint8_t a = (cfg_params->bg_color >> (32-32)) & 0xFF;

  SDL_SetRenderDrawColor(sdl_parameters->main_renderer, r, g, b, a);
  SDL_RenderClear(sdl_parameters->main_renderer);
}


/*
 *
 *
 *    CLEANUP FUNCTIONS
 *
 * 
 */

// Clean up SDL Windows, Renderers, etc when SDL is about to quit
void cleanup_sdl(sdl_params_t* sdl_parameters)
{
  SDL_DestroyRenderer(sdl_parameters->main_renderer);
  SDL_DestroyWindow(sdl_parameters->main_window);
  SDL_Quit();
}


/*
 *
 *
 *    MAIN LOOP FUNCTIONS
 *
 * 
 */

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
    switch (main_events.type)
    {
      case SDL_QUIT:
      {
        c8->emu_state = QUIT;
        return;
      }

      case SDL_KEYDOWN:
      {  
        switch (main_events.key.keysym.sym)
        {
          case SDLK_ESCAPE:
          {
            c8->emu_state = QUIT;
            return;
          }

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
        break;
      }

      case SDL_KEYUP:
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
        break;
      }
      
      default:
        break;
    }
  }
}

void print_debug(chip8_t* c8)
{
  printf("Address: 0x%04X, Opcode: 0x%04X, Desc: ", c8->emu_pc-2, c8->instr.inst_opcode);

  switch (c8->instr.inst_op)
  {
    case 0x00:
    {
      if (c8->instr.inst_nn == 0xE0)  // 00E0: Clear Screen
      {
        printf("Clear Screen\r\n");
      }
      else if (c8->instr.inst_nn == 0xEE) // 00EE: Return from subroutine
      {
        printf("Subroutine Return to addr 0x%04X\r\n", *(c8->emu_subrStack_ptr - 1));
      }
      else
      {
        printf("Unimplemented Opcode 0 \r\n");
      }
      break;
    }

    case 0x01:        // 1NNN: Jump to Address NNN
    {
      printf("Jump to Address NNN (0x%04X) \r\n", c8->instr.inst_nnn);
      break;
    }

    case 0x02:        // 2NNN: Call Subroutine at MemoryAddr NNN
    {
      printf("Call Subroutine \r\n");
      break;
    }

    case 0x03:      // 0x3NN: If V[X] == NN, skip next instruction
    {
      printf("Check if V%X (0x%02X) == NN (0x%02X) and skip next inst if true \r\n",c8->instr.inst_x, c8->emu_V[c8->instr.inst_x], c8->instr.inst_nn);
      break;
    }

    case 0x04:      // 0x4XNN: If V[X] != NN, skip next instruction
    {
      printf("Check if V%X (0x%02X) != NN (0x%02X) and skip next inst if true \r\n",c8->instr.inst_x, c8->emu_V[c8->instr.inst_x], c8->instr.inst_nn);
      break;
    }

    case 0x05:      // 0x4XNN: If V[X] == V[Y], skip next instruction
    {
      printf("Check if V%X (0x%02X) == V%X (0x%02X) and skip next inst if true \r\n",c8->instr.inst_x, c8->emu_V[c8->instr.inst_x], c8->instr.inst_y, c8->emu_V[c8->instr.inst_y]);
      break;
    }


    case 0x06:      // 6XNN: Set Data Register VX to NN
    {
      printf("Set Reg V%X = NN 0x%02X \r\n", c8->instr.inst_x, c8->instr.inst_nn);
      break;
    }

    case 0x07:      // 7XNN: Add NN to VX (carry flag not changed)
    {
      printf("Set Reg V%X (0x%02X) += NN 0x%02X. Result 0x%02X\r\n", c8->instr.inst_x, c8->emu_V[c8->instr.inst_x], c8->instr.inst_nn, c8->emu_V[c8->instr.inst_x] + c8->instr.inst_nn);
      break;
    }

    case 0x08:
    {
      if (c8->instr.inst_n == 0x00)
        printf("Register V%X = V%X (0x%02X) \r\n", c8->instr.inst_x, c8->instr.inst_y, c8->emu_V[c8->instr.inst_y]);

      else if (c8->instr.inst_n == 0x01)
        printf("Register V%X (0x%02X) |= V%X (0x%02X)  Result: 0x%02X\r\n", 
          c8->instr.inst_x, c8->emu_V[c8->instr.inst_x],
          c8->instr.inst_y, c8->emu_V[c8->instr.inst_y], 
          c8->emu_V[c8->instr.inst_x] | c8->emu_V[c8->instr.inst_y]);

      else if (c8->instr.inst_n == 0x02)
        printf("Register V%X (0x%02X) &= V%X (0x%02X)  Result: 0x%02X\r\n", 
          c8->instr.inst_x, c8->emu_V[c8->instr.inst_x],
          c8->instr.inst_y, c8->emu_V[c8->instr.inst_y], 
          c8->emu_V[c8->instr.inst_x] & c8->emu_V[c8->instr.inst_y]);

      else if (c8->instr.inst_n == 0x03)
        printf("Register V%X (0x%02X) ^= V%X (0x%02X)  Result: 0x%02X\r\n", 
          c8->instr.inst_x, c8->emu_V[c8->instr.inst_x],
          c8->instr.inst_y, c8->emu_V[c8->instr.inst_y], 
          c8->emu_V[c8->instr.inst_x] ^ c8->emu_V[c8->instr.inst_y]);

      else if (c8->instr.inst_n == 0x04)                                 // 0x8XY4: Set VX += VY and set VF to 1 if carry
      {
        printf("Register V%X (0x%02X) += V%X (0x%02X)  Result: 0x%02X and VF = %02X\r\n", 
          c8->instr.inst_x, c8->emu_V[c8->instr.inst_x],
          c8->instr.inst_y, c8->emu_V[c8->instr.inst_y], 
          c8->emu_V[c8->instr.inst_x] + c8->emu_V[c8->instr.inst_y],
          ((uint16_t)(c8->emu_V[c8->instr.inst_x] +  c8->emu_V[c8->instr.inst_y])) > 255);
      }

      else if (c8->instr.inst_n == 0x05)                                 // 0x8XY5: Set VX -= VY and set VF to 1 if no borrow
      {
        printf("Register V%X (0x%02X) -= V%X (0x%02X)  Result: 0x%02X and VF = %02X\r\n", 
          c8->instr.inst_x, c8->emu_V[c8->instr.inst_x],
          c8->instr.inst_y, c8->emu_V[c8->instr.inst_y], 
          c8->emu_V[c8->instr.inst_x] - c8->emu_V[c8->instr.inst_y],
          (uint16_t)(c8->emu_V[c8->instr.inst_x] >=  c8->emu_V[c8->instr.inst_y]));
      }

      else if (c8->instr.inst_n == 0x06)                                 // 0x8XY6: Store LSb of VX in VF amd shift VX right by 1
      {
        printf("Register V%X (0x%02X) >>= 1 and VF Bit = %X Result: 0x%02X\r\n", 
          c8->instr.inst_x, c8->emu_V[c8->instr.inst_x],
          c8->emu_V[c8->instr.inst_x] & 1,
          c8->emu_V[c8->instr.inst_x] >> 1);
      }

      else if (c8->instr.inst_n == 0x07)                                 // 0x8XY7: Set VX = VY - vX and set VF to 1 if no borrow
      {
        printf("Register V%X = V%X (0x%02X) - V%X (0x%02X)  Result: 0x%02X and VF = %02X\r\n", 
          c8->instr.inst_x, c8->instr.inst_y, c8->emu_V[c8->instr.inst_y], 
          c8->instr.inst_x, c8->emu_V[c8->instr.inst_x], 
          c8->emu_V[c8->instr.inst_y] - c8->emu_V[c8->instr.inst_x],
          (uint16_t)(c8->emu_V[c8->instr.inst_y] >=  c8->emu_V[c8->instr.inst_x]));
      }

      else if (c8->instr.inst_n == 0x0E)                                 // 0x8XYE: Store MSb of VX in VF amd shift VX left by 1
      {
        printf("Register V%X (0x%02X) <<= 1 and VF Bit = %X Result: 0x%02X\r\n", 
          c8->instr.inst_x, c8->emu_V[c8->instr.inst_x],
          (c8->emu_V[c8->instr.inst_x] & 0x80) >> 7,
          c8->emu_V[c8->instr.inst_x] << 1);
      }

      else
      {
        // Wrong OPCODE
      }

      break;
    }

    case 0x09:      // Skip next instruction if VX != VY
    {
      printf("Check if V%X (0x%02X) != V%X (0x%02X) and skip next inst if true \r\n",c8->instr.inst_x, c8->emu_V[c8->instr.inst_x], c8->instr.inst_y, c8->emu_V[c8->instr.inst_y]);
      break;
    }

    case 0x0A:       // ANNN: Set Memory Index Register I to NNN
    {
      printf("Set Reg I to NNN 0x%04X \r\n", c8->instr.inst_nnn);
      break;
    }

    case 0x0B:       // BNNN: Jump to V0 + NNN
    {
      printf("Set V to V0 (%02X) + NNN (%04X) \r\n", c8->emu_V[0], c8->instr.inst_nnn);
      break;
    }

    case 0x0C:       // CXNN: VX = rand() & NN
    {
      printf("Set V%X = (rand() %% 256) & NN (0x%02X)  \r\n", c8->instr.inst_x, c8->instr.inst_nn);
      break;
    }

    case 0x0D:
    { 
      printf("Draw N (%u) height sprite at coords V%X (0x%02X), V%X, (0x%02X) from memory location I (0x%04X)\r\n", c8->instr.inst_n, c8->instr.inst_x, c8->emu_V[c8->instr.inst_x], c8->instr.inst_y, c8->emu_V[c8->instr.inst_y], c8->emu_I);
      break;
      // Leave at 1:08:00

    }

    case 0x0E:
    {
      if (c8->instr.inst_nn == 0x9E)          // EX9E: Skip Next Instruction if Key in VX is Pressed
      {
        printf("Skip next instruction if key in V%X (0x%02X) is pressed: KP Val: %d \r\n", c8->instr.inst_x, c8->emu_V[c8->instr.inst_x], c8->emu_keypad[c8->emu_V[c8->instr.inst_x]]);
      }

      else if (c8->instr.inst_nn == 0xA1)     // EXA1: Skip Next Instruction if Key in VX is not Pressed
      {
        printf("Skip next instruction if key in V%X (0x%02X) is not pressed: KP Val: %d \r\n", c8->instr.inst_x, c8->emu_V[c8->instr.inst_x], c8->emu_keypad[c8->emu_V[c8->instr.inst_x]]);
      }

      else
      {
        // Wrong OPCODE
      }

      break;
    }

    case 0x0F:
    {
      if (c8->instr.inst_nn == 0x0A)        // FX0A: Await until key press and store in VX
      {
        printf("Await till key is pressed and store key in V%X \r\n", c8->instr.inst_x);
      }

      else if (c8->instr.inst_nn == 0x1E)        // FX1E:  Add VX to register I
      {
        printf("I (0x%04X) += V%X (0x%02X) \r\n", c8->emu_I, c8->instr.inst_x, c8->emu_V[c8->instr.inst_x]);
      }

      else if (c8->instr.inst_nn == 0x07)        // FX07:  VX = delay timer
      {
        printf("Set V%X = delay timer (0x%02X) \r\n", c8->instr.inst_x, c8->emu_delayTimer);
      }

      else if (c8->instr.inst_nn == 0x15)        // FX15:  delay timer = VX
      {
        printf("Set delay timer (0x%02X) = V%X (0x%02X) \r\n", c8->emu_delayTimer, c8->instr.inst_x, c8->emu_V[c8->instr.inst_x]);
      }

      else if (c8->instr.inst_nn == 0x18)        // FX18:  sound timer = VX
      {
        printf("Set sound timer (0x%02X) = V%X (0x%02X) \r\n", c8->emu_soundTimer, c8->instr.inst_x, c8->emu_V[c8->instr.inst_x]);
      }

      else if (c8->instr.inst_nn == 0x29)        // FX29:  Set reg I to sprite location in mem for character VX
      {
        printf("Set I to sprite location in mem for character in V%X (0x%02X) .. Result (VX*5) (0x%02X) \r\n", c8->instr.inst_x, c8->emu_V[c8->instr.inst_x], c8->emu_V[c8->instr.inst_x] * 5);
      }

      else if (c8->instr.inst_nn == 0x33)        // FX33:  Store Binary code decimal represenation of VX at mem offset of I
      {
        printf("Store V%X (0x%02X) as binary coded decimal at mem location I (0x%04X) \r\n", c8->instr.inst_x, c8->emu_V[c8->instr.inst_x], c8->emu_I);
      }

      else if (c8->instr.inst_nn == 0x55)        // FX55:  Dump V regs to mem offset from I
      {
        printf("Dump regs from V0 to V%X (0x%02X) at memory starting from I (0x%04X) \r\n", c8->instr.inst_x, c8->emu_V[c8->instr.inst_x], c8->emu_I);
      }

      else if (c8->instr.inst_nn == 0x65)        // FX65:  Load V regs from mem offset I
      {
        printf("Load regs from V0 to V%X (0x%02X) at memory starting from I (0x%04X) \r\n", c8->instr.inst_x, c8->emu_V[c8->instr.inst_x], c8->emu_I);
      }

      else
      {
        printf("Weird Instruction");
      }
      
      break;
    }

    
    default:
      printf("Unimplemented Opcodes \r\n");
      break;
  }

}


// Emulate Chip8 Instructions
void emulate_instructions(chip8_t* c8, user_config_params_t* cfg)
{
  // Get Current Instruction OPCODE using PC and RAM (Shift it since C8 is BigEndian and we are LittleEndian)
  // Increase PC for next instruction
  c8->instr.inst_opcode = (c8->emu_ram[c8->emu_pc] << 8) | (c8->emu_ram[c8->emu_pc+1]);
  c8->emu_pc += 2;

  // Fill instruction in the struct
  c8->instr.inst_nnn = c8->instr.inst_opcode & 0x0FFF;
  c8->instr.inst_nn  = c8->instr.inst_opcode & 0x00FF;
  c8->instr.inst_n   = c8->instr.inst_opcode & 0x000F;
  c8->instr.inst_x   = (c8->instr.inst_opcode & 0x0F00) >> 8;
  c8->instr.inst_y   = (c8->instr.inst_opcode & 0x00F0) >> 4;
  c8->instr.inst_op  = (c8->instr.inst_opcode & 0xF000) >> 12;

  // print_debug(c8);

  switch (c8->instr.inst_op)
  {
    case 0x00:
    {
      if (c8->instr.inst_nn == 0xE0)  // 00E0: Clear Screen
      { 
        memset(&(c8->emu_display[0]), false, sizeof(c8->emu_display));   // Clear Screen 
      }
      else if (c8->instr.inst_nn == 0xEE) // 00EE: Return from subroutine
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
      c8->emu_pc = c8->instr.inst_nnn;
      break;
    }

    case 0x02:        // 2NNN: Call Subroutine at MemoryAddr NNN
    {
      // Derefernce the stack pointer for the subroutine stack and point it to the incremented PC
      // So, after returning from the SubR, it executes the next instruction (kind of like saving state)
      // Then set, PC to NNN to jump to executing that instruction
      // Increment StackPtr to point to next stack location   (in case two subroutines are stacked)
      *(c8->emu_subrStack_ptr) = c8->emu_pc;
      c8->emu_pc = c8->instr.inst_nnn;
      c8->emu_subrStack_ptr++;
      break;
    }

    case 0x03:      // 3XNN: If V[X] == NN, skip next instruction
    {
      if (c8->emu_V[c8->instr.inst_x] == c8->instr.inst_nn) 
        c8->emu_pc += 2;

      break;
    }

    case 0x04:      // 4XNN: If V[X] != NN, skip next instruction
    {
      if (c8->emu_V[c8->instr.inst_x] != c8->instr.inst_nn) 
        c8->emu_pc += 2;
      
      break;
    }

    case 0x05:      // 4XNN: If V[X] == V[Y], skip next instruction
    {
      if (c8->instr.inst_n != 0)      // Wrong opcode
        break;

      if (c8->emu_V[c8->instr.inst_x] == c8->emu_V[c8->instr.inst_y]) 
        c8->emu_pc += 2;

      break;
    }

    case 0x06:      // 6XNN: Set Data Register VX to NN
    {
      c8->emu_V[c8->instr.inst_x] = c8->instr.inst_nn;
      break;
    }

    case 0x07:      // 7XNN: Add NN to VX (carry flag not changed)
    {
      c8->emu_V[c8->instr.inst_x] += c8->instr.inst_nn;
      break;
    }
    
    case 0x08:
    {
      if (c8->instr.inst_n == 0x00)
        c8->emu_V[c8->instr.inst_x] = c8->emu_V[c8->instr.inst_y];       // 8XY0: Set VX equal to VY

      else if (c8->instr.inst_n == 0x01)
        c8->emu_V[c8->instr.inst_x] |= c8->emu_V[c8->instr.inst_y];      // 8XY1: Set VX equal to VX OR VY

      else if (c8->instr.inst_n == 0x02)
        c8->emu_V[c8->instr.inst_x] &= c8->emu_V[c8->instr.inst_y];      // 8XY2: Set VX equal to VX AND VY

      else if (c8->instr.inst_n == 0x03)
        c8->emu_V[c8->instr.inst_x] ^= c8->emu_V[c8->instr.inst_y];      // 8XY3: Set VX equal to VX XOR VY

      else if (c8->instr.inst_n == 0x04)                                 // 8XY4: Set VX += VY and set VF to 1 if carry
      {
        if ((uint16_t)(c8->emu_V[c8->instr.inst_x] + c8->emu_V[c8->instr.inst_y]) > 255)
          c8->emu_V[0x0F] = 1;
          
        c8->emu_V[c8->instr.inst_x] += c8->emu_V[c8->instr.inst_y];
      }

      else if (c8->instr.inst_n == 0x05)                                 // 8XY5: Set VX -= VY and set VF to 1 if no borrow
      {
        // if (c8->emu_V[c8->instr.inst_x] >= c8->emu_V[c8->instr.inst_y]) 
        //   c8->emu_V[0x0F] = 1;

        c8->emu_V[0x0F] = (c8->emu_V[c8->instr.inst_x] >= c8->emu_V[c8->instr.inst_y]); 
        c8->emu_V[c8->instr.inst_x] -= c8->emu_V[c8->instr.inst_y];
      }

      else if (c8->instr.inst_n == 0x06)                                 // 8XY6: Store LSb of VX in VF amd shift VX right by 1
      {
        c8->emu_V[0x0F] = c8->emu_V[c8->instr.inst_x] & 0x01;
        c8->emu_V[c8->instr.inst_x] >>= 1;
      }

      else if (c8->instr.inst_n == 0x07)                                 // 8XY7: Set VX = VY - vX and set VF to 1 if no borrow
      {
        // if (c8->emu_V[c8->instr.inst_y] >= c8->emu_V[c8->instr.inst_x]) 
        //   c8->emu_V[0x0F] = 1;

        c8->emu_V[0x0F] = (c8->emu_V[c8->instr.inst_y] >= c8->emu_V[c8->instr.inst_x]);  
        c8->emu_V[c8->instr.inst_x] = c8->emu_V[c8->instr.inst_y] - c8->emu_V[c8->instr.inst_x];
      }

      else if (c8->instr.inst_n == 0x0E)                                 // 8XYE: Store MSb of VX in VF amd shift VX left by 1
      {
        c8->emu_V[0x0F] = (c8->emu_V[c8->instr.inst_x] & 0x80) >> 7;
        c8->emu_V[c8->instr.inst_x] <<= 1;
      }

      else
      {
        // Wrong OPCODE
      }

      break;
    }


    case 0x09:      // 9XY0: Skip next instruction if VX != VY
    {
      if (c8->emu_V[c8->instr.inst_x] != c8->emu_V[c8->instr.inst_y]) 
        c8->emu_pc += 2;

      break;
    }

    case 0x0A:       // ANNN: Set Memory Index Register I to NNN
    {
      c8->emu_I = c8->instr.inst_nnn;
      break;
    }

    case 0x0B:       // BNNN: Jump to V0 + NNN
    {
      c8->emu_pc = c8->emu_V[0] + c8->instr.inst_nnn;
      break;
    }

    case 0x0C:       // CXNN: VX = rand() & NN
    {
      c8->emu_V[c8->instr.inst_x] = (rand() % 256) & (c8->instr.inst_nn);
      break;
    }

    case 0x0D:      // DXYN: Draw N height Sprite at Coordinate XY
    {
      // Read from mem location I
      // Screen pixels are XOR-ed with sprite bits
      // VF (carry flag) is set if any scren pixels are set off (useful for collision detection)

      // Get Coordinates
      uint8_t x_cor = c8->emu_V[c8->instr.inst_x] % cfg->window_width;
      uint8_t y_cor = c8->emu_V[c8->instr.inst_y] % cfg->window_height;
      const uint8_t x_cor_original = x_cor;

      // Set carry flag to 0  
      c8->emu_V[0x0F] = 0;

      // Loop over all N rows of the sprite
      for (uint8_t i=0; i<c8->instr.inst_n; i++)
      {
        // First, get the next byte/row of the sprite data and reset x for the next row
        const uint8_t sprite_data = c8->emu_ram[c8->emu_I + i];
        x_cor = x_cor_original;

        for (int8_t j=7; j>=0; j--)
        {
          bool *pixel = &c8->emu_display[y_cor * cfg->window_width + x_cor];
          bool sprite_bit = (sprite_data & (1 << j));

          if (sprite_bit && *pixel) 
          {
            c8->emu_V[0x0F] = 1;
          }

          *pixel ^= sprite_bit;

          printf("Sprite Bit is %d \r\n", sprite_bit);

          if (++x_cor >= cfg->window_width) {break;}
        }

        if (++y_cor >= cfg->window_height) {break;}
      }

      break;
    }

    case 0x0E:
    {
      if (c8->instr.inst_nn == 0x9E)          // EX9E: Skip Next Instruction if Key in VX is Pressed
      {
        if (c8->emu_keypad[c8->emu_V[c8->instr.inst_x]])
          c8->emu_pc += 2;
      }

      else if (c8->instr.inst_nn == 0xA1)     // EXA1: Skip Next Instruction if Key in VX is not Pressed
      {
        if (!c8->emu_keypad[c8->emu_V[c8->instr.inst_x]])
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
      if (c8->instr.inst_nn == 0x0A)        // FX0A: Await until key press and store in VX
      {
        bool is_key_pressed = false;

        for (uint8_t i=0; i< sizeof(c8->emu_keypad); i++)
        {
          if (c8->emu_keypad[i])
          {
            c8->emu_V[c8->instr.inst_x] = i;
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

      else if (c8->instr.inst_nn == 0x1E)        // FX1E:  Add VX to register I
      {
        c8->emu_I += c8->emu_V[c8->instr.inst_x];
      }

      else if (c8->instr.inst_nn == 0x07)        // FX07:  VX = delay timer
      {
        c8->emu_V[c8->instr.inst_x] = c8->emu_delayTimer;
      }

      else if (c8->instr.inst_nn == 0x15)        // FX15:  delay timer = VX
      {
        c8->emu_delayTimer = c8->emu_V[c8->instr.inst_x];
      }

      else if (c8->instr.inst_nn == 0x18)        // FX18:  sound timer = VX
      {
        c8->emu_soundTimer = c8->emu_V[c8->instr.inst_x];
      }

      else if (c8->instr.inst_nn == 0x29)        // FX29:  Set reg I to sprite location in mem for character VX
      {
        c8->emu_I = c8->emu_V[c8->instr.inst_x] * 5;
      }

      else if (c8->instr.inst_nn == 0x33)        // FX33:  Store Binary code decimal represenation of VX at mem offset of I
      {
        uint8_t binary_code_decimal = c8->emu_V[c8->instr.inst_x];
        c8->emu_ram[c8->emu_I + 2] = binary_code_decimal % 10;
        binary_code_decimal /= 10;
        c8->emu_ram[c8->emu_I + 1] = binary_code_decimal % 10;
        binary_code_decimal /= 10;
        c8->emu_ram[c8->emu_I + 0] = binary_code_decimal % 10;
      }

      else if (c8->instr.inst_nn == 0x55)        // FX55:  Dump V regs from V0 to VX to mem offset from I
      {
        for (uint8_t i=0; i <=c8->instr.inst_x; i++)
        {
          c8->emu_ram[c8->emu_I + i] = c8->emu_V[i];
        }
      }

      else if (c8->instr.inst_nn == 0x65)        // FX65:  Load V regs from V0 to VX from mem offset from I
      {
        for (uint8_t i=0; i <=c8->instr.inst_x; i++)
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

// Update the window in the main loop
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

  for (uint32_t i=0; i<(sizeof(c8->emu_display)); i++)
  {
    // Emu display is a 1D array representing a 2D screen. Get X and Y coordingates
    sdl_rectangle.x = (i % cfg->window_width) * cfg->scale_factor;
    sdl_rectangle.y = (i / cfg->window_width) * cfg->scale_factor;

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
}


void update_timers(chip8_t* c8)
{
  if (c8->emu_delayTimer > 0)
  {
    c8->emu_delayTimer--;
  }

  if (c8->emu_soundTimer > 0)
  {
    c8->emu_soundTimer--;
    // Play sound
  }

  else
  {
    // Stop Playing sound
  }
}



int main (int argc, char** argv)
{
  const char* rom_name = argv[1];

  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s <rom_name>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Exit if user configuration parameters not initialized
  user_config_params_t config_parameters = {0};
  if (!init_user_configuration(&config_parameters, argc, argv))
    exit(EXIT_FAILURE);

  // Exit if SDL not initialized
  sdl_params_t sdl_parameters = {0};
  if (!init_sdl(&sdl_parameters, config_parameters))
    exit(EXIT_FAILURE);

  // Exit if Chip8 not initialized
  chip8_t chip8_instnace = {0};
  if (!init_chip8(&chip8_instnace, rom_name))
    exit(EXIT_FAILURE);

  clear_window(&sdl_parameters, &config_parameters);

  // Seed Random Number Generation
  srand(time(NULL));

  while (chip8_instnace.emu_state != QUIT)
  {
    // Handles all user input until nothing remains in the input queue
    handle_user_input(&chip8_instnace);

    if (chip8_instnace.emu_state == PAUSE) {continue;}

    uint64_t time_before_instructions = SDL_GetPerformanceCounter();

    // Emulate some instructions for this frame
    for (uint32_t i=0; i<(config_parameters.instructions_per_second)/60; i++)
    {
      emulate_instructions(&chip8_instnace, &config_parameters);
    }

    uint64_t time_after_instructions = SDL_GetPerformanceCounter();
    double time_emulating_instruction = (double)((time_after_instructions - time_before_instructions) / 1000) / SDL_GetPerformanceFrequency();


    // Delay by ((1/60) * 1000) to get delay in ms for 60HZ 
    SDL_Delay(16.66f > time_emulating_instruction ? 16.66f - time_emulating_instruction : 0);

    update_window(&sdl_parameters, &config_parameters, &chip8_instnace);
    update_timers(&chip8_instnace);
  }

  if (chip8_instnace.emu_state == QUIT)
    SDL_Log("\nchip8Emu quiting ... bye :((\n");


  cleanup_sdl(&sdl_parameters);
  return 0; 
}