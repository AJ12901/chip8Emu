#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

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

  while (SDL_PollEvent(&main_events))
  {
    switch (main_events.type)
    {
      case SDL_QUIT:
        c8->emu_state = QUIT;
        return;
      
      case SDL_KEYDOWN:
        switch (main_events.key.keysym.sym)
        {
          case SDLK_ESCAPE:
            c8->emu_state = QUIT;
            return;

          case SDLK_SPACE:
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

          default:
            break;
        }
        break;

      case SDL_KEYUP:
        break;
      
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

    case 0x0D:
    { 
      printf("Draw N (%u) height sprite at coords V%X (0x%02X), V%X, (0x%02X) from memory location I (0x%04X)\r\n", c8->instr.inst_n, c8->instr.inst_x, c8->emu_V[c8->instr.inst_x], c8->instr.inst_y, c8->emu_V[c8->instr.inst_y], c8->emu_I);
      break;
      // Leave at 1:08:00

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

  print_debug(c8);

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

    case 0x03:      // 0x3XNN: If V[X] == NN, skip next instruction
    {
      if (c8->emu_V[c8->instr.inst_x] == c8->instr.inst_nn) 
        c8->emu_pc += 2;

      break;
    }

    case 0x04:      // 0x4XNN: If V[X] != NN, skip next instruction
    {
      if (c8->emu_V[c8->instr.inst_x] != c8->instr.inst_nn) 
        c8->emu_pc += 2;
      
      break;
    }

    case 0x05:      // 0x4XNN: If V[X] == V[Y], skip next instruction
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
        c8->emu_V[c8->instr.inst_x] = c8->emu_V[c8->instr.inst_y];       // 0x8XY0: Set VX equal to VY

      else if (c8->instr.inst_n == 0x01)
        c8->emu_V[c8->instr.inst_x] |= c8->emu_V[c8->instr.inst_y];      // 0x8XY1: Set VX equal to VX OR VY

      else if (c8->instr.inst_n == 0x02)
        c8->emu_V[c8->instr.inst_x] &= c8->emu_V[c8->instr.inst_y];      // 0x8XY2: Set VX equal to VX AND VY

      else if (c8->instr.inst_n == 0x03)
        c8->emu_V[c8->instr.inst_x] ^= c8->emu_V[c8->instr.inst_y];      // 0x8XY3: Set VX equal to VX XOR VY

      else if (c8->instr.inst_n == 0x04)                                 // 0x8XY4: Set VX += VY and set VF to 1 if carry
      {
        if ((uint16_t)(c8->emu_V[c8->instr.inst_x] + c8->emu_V[c8->instr.inst_y]) > 255)
          c8->emu_V[0x0F] = 1;
          
        c8->emu_V[c8->instr.inst_x] += c8->emu_V[c8->instr.inst_y];
      }

      else if (c8->instr.inst_n == 0x05)                                 // 0x8XY5: Set VX -= VY and set VF to 1 if no borrow
      {
        if (c8->emu_V[c8->instr.inst_x] >= c8->emu_V[c8->instr.inst_y]) 
          c8->emu_V[0x0F] = 1;
          
        c8->emu_V[c8->instr.inst_x] -= c8->emu_V[c8->instr.inst_y];
      }

      else if (c8->instr.inst_n == 0x06)                                 // 0x8XY6: Store LSb of VX in VF amd shift VX right by 1
      {
        c8->emu_V[0x0F] = c8->emu_V[c8->instr.inst_x] & 0x01;
        c8->emu_V[c8->instr.inst_x] >>= 1;
      }

      else if (c8->instr.inst_n == 0x07)                                 // 0x8XY7: Set VX = VY - vX and set VF to 1 if no borrow
      {
        if (c8->emu_V[c8->instr.inst_y] >= c8->emu_V[c8->instr.inst_x]) 
          c8->emu_V[0x0F] = 1;
          
        c8->emu_V[c8->instr.inst_x] = c8->emu_V[c8->instr.inst_y] - c8->emu_V[c8->instr.inst_x];
      }

      else if (c8->instr.inst_n == 0x0E)                                 // 0x8XYE: Store MSb of VX in VF amd shift VX left by 1
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
          bool *pixel = &(c8->emu_display[y_cor * cfg->window_width + x_cor]);
          bool sprite_bit = (sprite_data & (1 << j));

          if (sprite_bit && *pixel) {c8->emu_V[0x0F] = 1;}
          *pixel ^= sprite_bit;

          x_cor++;
          if (x_cor >= cfg->window_width) {break;}
        }

        y_cor++;
        if (y_cor >= cfg->window_height) {break;}
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

  for (long unsigned i=0; i<(sizeof(c8->emu_display)); i++)
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

    // Pixel Outline Config (optional)
    if (cfg->pixel_outlines)
    {
      SDL_SetRenderDrawColor(sdl_params->main_renderer, bg_r, bg_g, bg_b, bg_a);
      SDL_RenderDrawRect(sdl_params->main_renderer, &sdl_rectangle);
    }

    // If Pixel is on, draw background color
    else
    {
      SDL_SetRenderDrawColor(sdl_params->main_renderer, bg_r, bg_g, bg_b, bg_a);
      SDL_RenderFillRect(sdl_params->main_renderer, &sdl_rectangle);
    }
  }


  SDL_RenderPresent(sdl_params->main_renderer);
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

  while (chip8_instnace.emu_state != QUIT)
  {
    // Handles all user input until nothing remains in the input queue
    handle_user_input(&chip8_instnace);

    if (chip8_instnace.emu_state == PAUSE) {continue;}

    emulate_instructions(&chip8_instnace, &config_parameters);

    // Delay by ((1/60) * 1000) to get delay in ms for 60HZ 
    SDL_Delay(16.66);

    update_window(&sdl_parameters, &config_parameters, &chip8_instnace);
  }

  if (chip8_instnace.emu_state == QUIT)
    SDL_Log("\nchip8Emu quiting ... bye :((\n");


  cleanup_sdl(&sdl_parameters);
  return 0; 
}