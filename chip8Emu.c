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
  cfg_params->window_height = 32 * 10;
  cfg_params->window_width = 64 * 10;
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

  sdl_parameters->main_window = SDL_CreateWindow("Chip8Emu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, config_parameters.window_width, config_parameters.window_height, 0);

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

    case 0x02:        // 2NNN: Call Subroutine at MemoryAddr NNN
    {
      printf("Call Subroutine \r\n");
      break;
    }

    case 0x06:      // 6XNN: Set Data Register VX to NN
    {
      printf("Set Reg V%X to NN 0x%02X \r\n", c8->instr.inst_x, c8->instr.inst_nn);
      break;
    }

    case 0x0A:       // ANNN: Set Memory Index Register I to NNN
    {
      printf("Set Reg I to NNN 0x%04X \r\n", c8->instr.inst_nnn);
      break;
    }

    case 0x0D:
    {
      // Leave at 1:08:00

    }

    
    default:
      printf("Unimplemented Opcodes \r\n");
      break;
  }

}


// Emulate Chip8 Instructions
void emulate_instructions(chip8_t* c8)
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

    case 0x06:      // 6XNN: Set Data Register VX to NN
    {
      c8->emu_V[c8->instr.inst_x] = c8->instr.inst_nn;
      break;
    }

    case 0x0A:       // ANNN: Set Memory Index Register I to NNN
    {
      c8->emu_I = c8->instr.inst_nnn;
      break;
    }


    
    default:
      break;
  }





}

// Update the window in the main loop
void update_window(sdl_params_t* sdl_params)
{
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

    emulate_instructions(&chip8_instnace);

    // Delay by ((1/60) * 1000) to get delay in ms for 60HZ 
    SDL_Delay(16.66);

    update_window(&sdl_parameters);
  }

  if (chip8_instnace.emu_state == QUIT)
    SDL_Log("\nchip8Emu quiting ... bye :((\n");


  cleanup_sdl(&sdl_parameters);
  return 0; 
}