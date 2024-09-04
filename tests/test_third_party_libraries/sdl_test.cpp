#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#pragma clang diagnostic pop

#include <stdio.h>

int main( int argc, char** argv ) {
	( (void) argc );
	( (void) argv );

	SDL_Window* window = SDL_CreateWindow( "This window will destroy itself in 3 seconds", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN );

	if ( !window ) {
		printf( "Failed to create Window: %s\n", SDL_GetError() );
		return 1;
	}

	bool running = true;

	Uint32 startTime = SDL_GetTicks();

	while ( running ) {
		Uint32 now = SDL_GetTicks();

		if ( now - startTime > 3000 ) {
			running = false;
		}
	}

	SDL_DestroyWindow( window );

	return 0;
}